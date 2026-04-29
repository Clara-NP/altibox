/**
 * @file altibox.h
 * @author Liu Chuansen (179712066@qq.com)
 * @brief 实现电源控制盒的软件
 * @version 0.1
 * @date 2023-04-29
 *
 * @copyright Copyright (c) 2023
 *
 * @note 电源盒主要任务
 *
 * 1. 从RS485总线上接收指令，并响应
 * 2. 定期采集BMP585上的高度状态
 *
 *
 * 状态逻辑
 *   等待串口命令，如果收到给自己的请求，返回响应
 *   每隔一秒读取一次BMP585
 */

#include <common/generic.h>
#include <can/can.h>
#include <serial/serial.h>
#include <adc/adc.h>
#include <rtc/rtc.h>
#include <sensor/bmp585/bmp585.h>
#include <misc/simple_filter.h>
#include <npcp/lib/npcp_payload_ext.h>
#include <npcp/lib/npcp_frame.h>
#include <npcp/slave/npcp_slave.h>
#include <npcp/slave/npcp_slave_upgrade.h>
#include <flow_task/flow_task.h>

#include <neptune/nept_hwtype.h>

#include "config/export_ids.h"
#include "config/product_config.h"

#include <common/soc/soc_info.h>

#include "storage.h"
// #include "module.h"
#include "sensor.h"

#define TRACE_TAG "main"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>


// 软件版本号
#define DEVICE_SW_VERSION  NPCP_VERSION(CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR)


// 定义一个默认值
const factoryParameter_t g_factoryDefaultParameter =
{
    .address = 1,
    .hwVersion = 0,
    .model = "",
    .sn = "",
};

// 定义一个默认值
const userParameter_t g_userDefaultParameter =
{
    .antiTamperState = 0,
    .reserved = {0},
};

/**
 * @brief npcp升级参数默认配置（stm32f103cb）
 * 
 * 
 *      |---------------------|
 *      |    dual block(4K)   |
 *      |---------------------|
 *      |      备份区(60K)     |
 *      |---------------------|
 *      |      程序区 (60K)    |
 *      |---------------------|
 *      |    BootLoader(4K)   |
 *      |---------------------|                     
 */
#ifdef CONFIG_NPCP_UPGRADE_ENABLE
const npcpSlaveUpgradeConfig_t npcpUpgradeConfig = 
{
    .upgradeAddress = (0x8000000+64*1024),
    .upgradeSize = 60*1024,
    .appAddress = (0x8000000+4*1024),
    .pageSize = 2048,
    .eraseTime = 10,
    .frameSize = 96,
    .singleWriteSize = 128,
    .frameInterval = 0,
    .startDelay = ((64*1024) / 2048 * 10  + 2000),                          //.upgradeSize/.pageSize*.eraseTime + 2000,
    .verifyDelay = ((64*1024) / CONFIG_NPCP_UPGRADE_VERIFY_UNIT*5 + 500), //.upgradeSize/CONFIG_NPCP_UPGRADE_VERIFY_UNIT*5 + 500,
    .infoWrite = NULL,
    .infoRead = NULL,
};
#endif

/**
 * @brief 从机服务的常量信息
 * 
 */
const npcpSlaveInfo_t s_slaveInfo = 
{
    .bus = SERIAL_RS485_1,
    .devType = NPCP_DEVTYPE_ALTIBOX,
    /// 当前协议版本号为0
    .protocol = 0,
    /// 当前设备协议版本号为0
    .deviceProtocol = 0,
    /// 接收超时为50ms
    .rxTimeout = 50,
#ifdef CONFIG_NPCP_UPGRADE_ENABLE
    .upgradeConfig = &npcpUpgradeConfig,
#endif
};





/**
 * @brief 获取工厂参数
 *
 * @param parameter
 */
static inline void getFactoryParameter(factoryParameter_t *parameter)
{
    int ret = loadFactoryParameter(parameter);
    if (ret != RET_SUCCESS)
    {
        wlog("load factory parameter failed, try default");
        osMemcpy(parameter, &g_factoryDefaultParameter, sizeof(g_factoryDefaultParameter));
    }
}

/**
 * @brief 获取参数
 *
 * @param parameter
 */
static inline void getUserParameter(userParameter_t *parameter)
{
    int ret = loadUserParameter(parameter);
    if (ret != RET_SUCCESS)
    {
        wlog("load user parameter failed, try default");
        osMemcpy(parameter, &g_userDefaultParameter, sizeof(g_userDefaultParameter));
    }
}


/**
 * @brief 本地状态管理
 *
 */
typedef struct
{
    /// 从机服务
    npcpSlaveService_t *slaveService;

    /// 产测参数
    factoryParameter_t factory;
    /// 用户参数
    userParameter_t user;

    /// 设备上报状态汇总
    uint16_t deviceState;

    // 用作调试
    loopTimer_t slowTimer;

    /// 软件版本信息
    uint16_t swVersion;
    /// 表示 总线的状态
    bool busActive;
    /// @brief 表示主机的状态
    bool masterActive;

    /// @brief 表示MCU的读保护状态, true - 读保护启用, false - 读保护未启用
    bool mcuRdpEnabled;

}localManager_t;

/**
 * @brief 本地管理实例
 *
 */
static localManager_t s_manager = {0};
// 获取本地对象实例
#define getInstance() &s_manager

static uint32_t floatToUint32(float v, float mul)
{
    return (uint32_t)(v * mul);
}

/**
 * @brief 刷新自己的状态
 * 
 */
static void deviceStateRefresh(void)
{
    localManager_t *m = getInstance();

    uint16_t state = 0;

    if (!sensorGetBool(BOARD_BMP585))
    {
        state = 0;  // ALTIBOX_STATE_OFFLINE
    } else {
        state = 1;  // ALTIBOX_STATE_ONLINE
    }

    // // 存储不可用
    // if( factoryStorageState() /*|| userStorageState()*/)
    // {
    //     state |= POWERBOX_STATE_STORAGE_INVALID;
    // }
    // TODO: 处理其他更多的数据

    if (m->deviceState != state)
    {
        wlog("altibox state changed: 0x%04x -> 0x%04x", m->deviceState, state);
        m->deviceState = state;
    }
}


/**
 * @brief 从机事件处理
 * 
 * @param service 
 * @param event 
 * @param eventData 
 * @return int 
 */
int npcpEventHandle(npcpSlaveService_t *service, uint16_t event, const void *eventData)
{
    localManager_t *m = getInstance();

    switch(event)
    {
        case NPCP_EVENT_REBOOT:
        {
            osReboot();
            return RET_SUCCESS;
        }
        case NPCP_EVENT_RESTORE_DEFAULT:
        {
            osMemcpy(&m->user, &g_userDefaultParameter, sizeof(g_userDefaultParameter));
            ilog("reset user farameter to default!!!");
            int ret = saveUserParameter(&m->user);
            if (ret != RET_SUCCESS)
            {
                wlog("user parameter save failed, ret=%d", ret);
                return RET_FAILED;
            } 
            return RET_SUCCESS;
        }

        case NPCP_EVENT_CLEAR_ANTI_TAMPER:
        {
            if (m->user.antiTamperState != 0) 
            {
                ilog("clear anti-tamper state");
                m->user.antiTamperState = 0;
                int ret = saveUserParameter(&m->user);
                if (ret != RET_SUCCESS)
                {
                    wlog("user parameter save failed, ret=%d", ret);
                    m->user.antiTamperState = 1;
                    return RET_FAILED;
                } 
            }
            return RET_SUCCESS;
        }

        case NPCP_EVENT_SET_READ_PROTECT:
        {
            // 设置读保护
            socSetReadProtectState();
            return RET_SUCCESS;
        }

        case NPCP_EVENT_SET_DEVID:
        {
            const npcpEventSetDevId_t *event = (const npcpEventSetDevId_t *)eventData;

            if (m->factory.address != event->newDevId)
            {
                uint8_t oldAddress = m->factory.address;

                m->factory.address = event->newDevId;
                // 保存参数
                int ret = saveFactoryParameter(&m->factory);
                if (ret != RET_SUCCESS)
                {
                    m->factory.address = oldAddress;
                    wlog("parameter save failed, ret=%d", ret);
                    return RET_FAILED;
                }
            }

            return RET_SUCCESS;
        }
        //break;
        case NPCP_EVENT_SET_HARDWARE_VERSION:
        {
            const npcpEventSetHwVersion_t *event = (const npcpEventSetHwVersion_t *)eventData;
            
            if (m->factory.hwVersion != event->version)
            {
                uint16_t oldData = m->factory.hwVersion;

                m->factory.hwVersion = event->version;
                // 保存参数
                int ret = saveFactoryParameter(&m->factory);
                if (ret != RET_SUCCESS)
                {
                    m->factory.hwVersion = oldData;
                    wlog("parameter save failed, ret=%d", ret);
                    return RET_FAILED;
                }
            }

            return RET_SUCCESS;
        }        
        // break;
        case NPCP_EVENT_SET_MODEL:
        {
            const npcpEventSetModel_t *event = (const npcpEventSetModel_t *)eventData;
            
            if (strcmp(event->model, m->factory.model))
            {
                char old[sizeof(m->factory.model)];
                strcpy(old, m->factory.model);
                osSafeStrncpy(m->factory.model, event->model, sizeof(m->factory.model));
                // 保存参数
                int ret = saveFactoryParameter(&m->factory);
                if (ret != RET_SUCCESS)
                {
                    strcpy(m->factory.model, old);
                    wlog("parameter save failed, ret=%d", ret);
                    return RET_FAILED;
                }                
            }

            return RET_SUCCESS;
        }
        //break;

        case NPCP_EVENT_SET_SERIAL_NUMBER:
        {
            const npcpEventSetSn_t *event = (const npcpEventSetSn_t *)eventData;
            
            if (strcmp(event->sn, m->factory.sn))
            {
                char old[sizeof(m->factory.sn)];
                strcpy(old, m->factory.sn);
                osSafeStrncpy(m->factory.sn, event->sn, sizeof(m->factory.sn));
                // 保存参数
                int ret = saveFactoryParameter(&m->factory);
                if (ret != RET_SUCCESS)
                {
                    strcpy(m->factory.sn, old);
                    wlog("parameter save failed, ret=%d", ret);
                    return RET_FAILED;
                }
            }
            return RET_SUCCESS;
        }
        //break;
#ifdef CONFIG_NPCP_UPGRADE_ENABLE        
        case NPCP_EVENT_OTA_START:
        {            
            return npcpSlaveUpgradeGeneralCheck((const npcpStartImageUpgrade_t *)eventData, NPCP_HWTYPE_ALTIBOX_V0, DEVICE_SW_VERSION);
        }
#endif         
        case NPCP_EVENT_CUSTOM_COMMAND:
        {
            const npcpFrameInfo_t *frame = (const npcpFrameInfo_t *)eventData;

            switch(frame->command)
            {
                case CMD_ALTIBOX_GET_STATUS:
                {
                    struct GetAltiBoxStatusResponse ack = { 0 };

                    ack.deviceState = htoles(m->deviceState);
                    ack.altimeter = htoles(floatToUint32(sensorGetFloat(BOARD_ALTIMETER), 10.0f));
                    ack.temperature =  htoles(sensorGetFloat(BOARD_TEMPERATURE)*1.0f);

                    // float temperature = 100.111;
                    // float altimeter = 200.222;
                    // ack.altimeter = htolel(altimeter * 10);
                    // ack.temperature = htoles(temperature * 10);
                    npcpSendResponse(service, frame->sequence, frame->command, (const uint8_t *)&ack, sizeof(ack));
                    return RET_SUCCESS;
                }
                //break;
            }
        }
        break;
    }

    return RET_NO_SUPPORTED;
}


bool altiboxOtaBusy(void)
{
#ifdef CONFIG_NPCP_UPGRADE_ENABLE
    localManager_t *m = getInstance();
    if (m->slaveService == NULL)
    {
        return false;
    }
    return npcpServiceUpgradeBusy(m->slaveService);
#else
    return false;
#endif
}


/**
 * @brief 电源盒初始化及自检
 *
 */
void altiBoxInit(void)
{
    // reset all
    osMemset(&s_manager, 0, sizeof(s_manager));

    localManager_t *m = getInstance();
    sysTick_t now = upTime();

    m->swVersion = DEVICE_SW_VERSION;

    ilog("software version:V%d.%d", (m->swVersion >> 8) & 0xff, m->swVersion & 0xff);

    // 获取MCU的读保护状态
    m->mcuRdpEnabled = socGetReadProtectState();

    // 100ms
    //loopTimerAtOnce(&m->slowTimer, 1000, now);

    // 加载工厂参数
    getFactoryParameter(&m->factory);
    // 加载用户参数
    getUserParameter(&m->user);

    ilog("address:%d, model:%s serial-number:%s", m->factory.address, m->factory.model, m->factory.sn);
    ilog("hardware version:V%d.%d, anti-tamper:%d", (m->factory.hwVersion >> 8) & 0xff, m->factory.hwVersion & 0xff, m->user.antiTamperState);

    // 初始化传感器采集任务
    sensorManagerInit();

    // 创建一个npcp从机服务实例
    m->slaveService = npcpSlaveServiceInit(&s_slaveInfo, npcpEventHandle);
    ASSERT(m->slaveService, "npcp slave service create failed");

    // 同步参数到NPCP服务
    npcpServiceSetDeviceId(m->slaveService, m->factory.address);
    npcpServiceSetVersionId(m->slaveService, m->factory.hwVersion, m->swVersion);
    npcpServiceSetSerialNumber(m->slaveService, m->factory.sn);
    npcpServiceSetModel(m->slaveService, m->factory.model);
    
}



/**
 * @brief 主任务
 *
 */
void altiBoxMainTick(void)
{
    localManager_t *m = getInstance();
    //sysTick_t now = upTime();
    //int ret = 0;

    // 运行NPCP从机服务实例
    npcpServiceMainTick(m->slaveService);

    /// 更新主机和总线的状态
    bool bus = npcpServiceBusActive(m->slaveService);
    bool master = npcpServiceMasterActive(m->slaveService);

    if ((master != m->masterActive) || (bus != m->busActive))
    {
        m->masterActive = master;        
        m->busActive = bus;

        // 有三个状态：
        // 都不活动  ->  慢闪  0x0003
        // 总线活动 ->   闪  0x0303
        // 主机活动 ->   0x3333
        if (m->masterActive)
        {
            ledFlash(LED_SYS, 0x3333, 0xffff);
        }
        else if (m->busActive)
        {
            ledFlash(LED_SYS, 0x0303, 0xffff);
        }
        else 
        {
            ledFlash(LED_SYS, 0x0003, 0xffff);
        } 
    }
    
    // 每隔100ms 读取温度，湿度等
    // if (loopTimerExpired(&m->slowTimer, now))
    // {

    // }

    /// 更新状态
    deviceStateRefresh();
}

#ifdef CONFIG_CONSOLE 

/**
 * @brief 显示模块信息
 * 
 */
void dumpModuleInfo(void)
{
    localManager_t *m = getInstance();
    rawPrint("--module status--\r\n");
    rawPrint(" state         : 0x%04x\r\n", m->deviceState);
    rawPrint(" temperature   : %.3f C\r\n", sensorGetFloat(BOARD_TEMPERATURE));
    rawPrint(" altimeter     : %.3f m\r\n", sensorGetFloat(BOARD_ALTIMETER));
}

#endif // 
