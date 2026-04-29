
/**
 * @file sensor.c
 * @author Liu Chuansen (samule@neptune-robotics.com)
 * @brief 传感器采集任务
 * @version 0.1
 * @date 2023-05-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <common/generic.h>

#include <flow_task/flow_task.h>
#include <sensor/bmp585/bmp585.h>
#include <adc/adc.h>

#include "config/export_ids.h"
#include "config/product_config.h"
#include "altibox.h"
#include "sensor.h"

#define TRACE_TAG "sensor"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

/**
 * @brief 电源管理对象
 * 
 */
static sensorManager_t s_sensorManager = { 0 };
static flowTask_t *s_sensorTask = NULL;

// 获取当前实例
#define getInstance() &s_sensorManager
#define BMP585_BOARD 1
static bool s_bmp585ConfigFlag = false;
static const bmp585Config_t s_bmp585Config = {
    .osr = 0x52,
    .odr = 0xAB,
    .iir = 0x1B,
    .dsp = 0xFB,
    .fifoSel = 0x03,
    .fifoConfig = 0x0C,
};

/**
 * @brief 日志是否需要被抑制
 * 
 * @param log 
 * @return true 不打印
 * @return false 打印
 */
static inline bool logSuppressed(logSuppress_t *log)
{
    if (log->printCount < log->maxPrintCount)
    {
        log->printCount ++;
    }

    return (log->printCount < log->maxPrintCount) ? false : true; 
}

/// 复位日志抑制
#define logSuppressReset(_log) do { (_log)->printCount = 0;} while(0) 


/**
 * @brief 设置操作成功
 * 
 * @param name 名称
 * @param ctrl 
 * @return bool  true - 表示 状态改变， false - 表示 状态未改变 
 */
static bool setSuccess(const char *name, stateControl_t *ctrl)
{
    bool change = false;
    if (ctrl->connected == false)
    {
        ilog("%s connected", name);
        change = true;
    }

    ctrl->connected = true;
    ctrl->failedCount = 0;
    return change;
}

/**
 * @brief 设置一次失败
 * 
 * @param name 
 * @param ctrl 
 * @return bool 表示状态是否改变
 */
static bool setFailed(const char *name, stateControl_t *ctrl)
{
    bool changed = false;

    if (ctrl->failedCount < ctrl->maxFailedCount)
    {
        ctrl->failedCount ++;
    }
    
    if (ctrl->failedCount >= ctrl->maxFailedCount)
    {
        if (ctrl->connected)
        {
            wlog("%s disconneted", name);
            changed = true;
        }

        ctrl->connected = false;
    }

    return changed;
}


/**
 * @brief 过程0，初始化和同步
 * 
 * @param userData 
 * @return int 
 */
static int initAndSync(void *userData, uint16_t flag, uint16_t index)
{
    sensorManager_t *pm = (sensorManager_t *)userData;

    // 等待同步时间到达
    if (loopTimerExpired(&pm->syncTimer, upTime()))
    {
        PREPARE_NEXT_PROCESS(pm);
        return TASK_FORWARD;
    }

    return TASK_REMAIN;
}


/**
 * @brief 5KW模块获取电压电流等运行状态
 * 
 * @param userData 
 * @return int 
 */
static int getBoardTemperatureAndHumitidy(void *userData, uint16_t flag, uint16_t index)
{
    sensorManager_t *pm = (sensorManager_t *)userData;

    /// 20260429 - 升级过程中执行该任务可能会造成无法响应升级指令
    if (altiboxOtaBusy())
    {
        PREPARE_NEXT_PROCESS(pm);
        return TASK_FORWARD;
    }

    const char *name = "bmp585";

    switch(pm->processState)
    {
        case STATE_BMP585_SEND_COMMAND:
        {
            if (!s_bmp585ConfigFlag)
            {
                int ret = bmp585Config(BMP585_BOARD, &s_bmp585Config);
                if (ret == RET_SUCCESS)
                {
                    logSuppressReset(&pm->bmp585Log);

                    pm->processState = STATE_BMP585_WAIT_READY;
                    // 一般时间5ms 足够了
                    pm->waitTick = upTime() + 10;
                    s_bmp585ConfigFlag = true;
                }
                else
                {
                    // 命令发送失败
                    if (!logSuppressed(&pm->bmp585Log))
                    {
                        elog("bmp585 send command failed, ret=%d", ret);
                    }

                    // 进入错误状态
                    pm->processState = STATE_BMP585_OP_FAILED;            
                }
            } else {
                pm->processState = STATE_BMP585_WAIT_READY;
                // 一般时间5ms 足够了
                pm->waitTick = upTime() + 10;
            }
        }
        break;
        case STATE_BMP585_WAIT_READY:
        // 状态2： 只是等待
        if (upTimeAfter(upTime(), pm->waitTick))
        {
            // UCT to read result
            pm->processState = STATE_BMP585_READ_RESULT;
            // 一般时间5ms 足够了
            pm->waitTick = upTime() + 10;
        }
        break;

        case STATE_BMP585_READ_RESULT:
        {
            // 读结果，如果超时，表示失败
            int ret = bmp585ReadData(BMP585_BOARD, &pm->boardAltimeter, &pm->boardTemperature);
            dlog("bmp585 read result, ret=%d, altitude=%.3f, temperature=%.3f", ret, pm->boardAltimeter, pm->boardTemperature);
            if (ret == RET_SUCCESS)
            {
                if (setSuccess(name, &pm->bmp585State))
                {
                    ilog("bmp585 T=%.3fC H=%.3f%%", pm->boardTemperature, pm->boardAltimeter);
                }
                else 
                {
                    dlog("bmp585 T=%.3fC H=%.3f%%", pm->boardTemperature, pm->boardAltimeter);
                }

                logSuppressReset(&pm->bmp585Log);

                // 操作成功， 执行下一个过程
                PREPARE_NEXT_PROCESS(pm);
                return TASK_FORWARD;
            }
            else 
            {
                /// 未接收到数据，是否超时到了
                if (upTimeAfter(upTime(), pm->waitTick))
                {
                    // 打印日志
                    if (!logSuppressed(&pm->bmp585Log))
                    {
                        elog("bmp585 read result timeout, ret=%d", ret);
                    }

                    // 进行错误状态
                    pm->processState = STATE_BMP585_OP_FAILED;
                }
            }
        }
        break;

        case STATE_BMP585_OP_FAILED:
            // 提交失败
            if (setFailed(name, &pm->bmp585State))
            {
                // 失败后，清空值
                pm->boardTemperature = 0.0f; // 应该使用表示温度无效的标志
                pm->boardAltimeter = 0.0f;
            }

        default:        
            PREPARE_NEXT_PROCESS(pm);
            return TASK_FORWARD;
    }

    return TASK_REMAIN;
}


/**
 * @brief 获取模拟量读数
 * 
 * @param userData 
 * @return int 
 */
static int getAnalogValues(void *userData, uint16_t flag, uint16_t index)
{
    sensorManager_t *pm = (sensorManager_t *)userData;
        
    // // 更新总电压值
    // pm->totalVoltage.value = floatFilterPush(&pm->totalVoltage.filter, adcGet(ADC_VOLTAGE));
    // // 更新总电流值
    // pm->totalCurrent.value = floatFilterPush(&pm->totalCurrent.filter, adcGet(ADC_CURRENT));

    // float mcuTemp, mcuVref;
    // mcuTemp = adcGet(ADC_MCU_TEMPERATURE);
    // mcuVref = adcGet(ADC_MCU_VREF);
    // pm->mcuTemperature.value = floatFilterPush(&pm->mcuTemperature.filter, mcuTemp);

    // dlog("mcu Vref=%.3f T=%.3f C(raw:%.3f C)", mcuVref, pm->mcuTemperature.value, mcuTemp);
    // dlog("power V=%.3f I=%.3f", pm->totalVoltage.value, pm->totalCurrent.value);

    PREPARE_NEXT_PROCESS(pm);
    return TASK_FORWARD;
}



/// 任务的过程函数关联
static const taskProcess_t s_processes[] = 
{
    initAndSync,
    getBoardTemperatureAndHumitidy,
    getAnalogValues,
};



/**
 * @brief 初始化传感器采集任务
 * 
 */
void sensorManagerInit(void)
{
    sensorManager_t *pm = getInstance();

    // reset all datas
    osMemset(pm, 0, sizeof(*pm));

    // 一些控制常量
    pm->bmp585Log.maxPrintCount = 3;
    pm->bmp585State.maxFailedCount = 3;

    // 同步周期为100ms, 10HZ
    loopTimerEnable(&pm->syncTimer, CONFIG_SENSOR_SAMPLE_PERIOD, upTime());

    /// 初始化流任务
    s_sensorTask = flowTaskAdd("sensors", s_processes, ARRAY_SIZE(s_processes), (void *)pm);

    ASSERT(s_sensorTask, "add sensor task failed");
}



/**
 * @brief 返回一个状态
 * 
 * @param id ::SENSOR_ID
 * @return true 
 * @return false 
 */
bool sensorGetBool(uint8_t id)
{
    switch(id)
    {
        case BOARD_BMP585:
        return s_sensorManager.bmp585State.connected;
    }

    return false;
}


/**
 * @brief 返回一个传感器的状态值
 * 
 * @param id ::SENSOR_ID
 * @return float 
 */
float sensorGetFloat(uint8_t id)
{
    switch(id)
    {
        case BOARD_TEMPERATURE:
            return s_sensorManager.boardTemperature;
        case BOARD_ALTIMETER:
            return s_sensorManager.boardAltimeter;
        // case TOTAL_POWER_VOLTAGE:
        // return s_sensorManager.totalVoltage.value;
        // case TOTAL_POWER_CURRENT:
        // return s_sensorManager.totalCurrent.value;
    }

    return 0.0f;
}
