

#ifndef __BOARD_SENSOR_H__
#define __BOARD_SENSOR_H__

/**
 * @file sensor.h
 * @author Liu Chuansen (samule@neptune-robotics.com)
 * @brief 一个传感器采集任务
 * @version 0.1
 * @date 2023-05-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <common/generic.h>
#include <misc/simple_filter.h>

/**
 * @brief 这是一个过程状态
 * @note 不同process的状态必须从0开始定义
 */
enum PROCESS_STATE
{
    /// STATE FOR BMP585
    STATE_BMP585_SEND_COMMAND = 0,
    STATE_BMP585_WAIT_READY,
    STATE_BMP585_READ_RESULT,
    STATE_BMP585_OP_FAILED,

    /// 读取一些模拟量
    STATE_READ_ANALOG_VALUES = 0,
};

typedef struct 
{
    /// 当前打印次数
    uint8_t printCount;
    /// 最多打印次数
    uint8_t maxPrintCount;
}logSuppress_t;

typedef struct 
{
    uint8_t failedCount;
    uint8_t maxFailedCount;
    bool connected;
}stateControl_t;

/// 定义一个简单的过滤器
DEFINE_FILTER(floatFilter, float, 10);

/**
 * @brief ADC采样控制
 *
 */
typedef struct
{
    /// 当前转换后的值
    float value;
    /// 过滤器
    struct floatFilter filter;
}filterValue_t;

/**
 * @brief 传感器任务管理
 * 
 */
typedef struct 
{
    /// 任务同步周期
    loopTimer_t syncTimer;
    /// 等待任务
    sysTick_t waitTick;
    /// 过程状态
    uint8_t processState;

    /// STH3X温湿度状态
    stateControl_t bmp585State;
    /// 错误日志管理
    logSuppress_t bmp585Log;
    /// 板上温度
    float boardTemperature;
    /// 高度
    float boardAltimeter;

}sensorManager_t;


// 进入下一状态
#define PREPARE_NEXT_PROCESS(_pm) do \
{ \
    (_pm)->processState = 0; \
}while(0)


enum SENSOR_ID
{
    BOARD_BMP585 = 0,
    BOARD_TEMPERATURE,
    BOARD_ALTIMETER,
};

/**
 * @brief 初始化传感器采集任务
 * 
 */
void sensorManagerInit(void);


/**
 * @brief 返回一个状态
 * 
 * @param id ::SENSOR_ID
 * @return true 
 * @return false 
 */
bool sensorGetBool(uint8_t id);


/**
 * @brief 返回一个传感器的状态值
 * 
 * @param id ::SENSOR_ID
 * @return float 
 */
float sensorGetFloat(uint8_t id);

#endif // #define __BOARD_SENSOR_H__
