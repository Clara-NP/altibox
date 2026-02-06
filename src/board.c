

/**
 * @file board.c
 * @author Liu Chuansen (179712066@qq.com)
 * @brief 板级外设初始化
 * @version 0.1
 * @date 2023-03-17
 * 
 * @copyright Copyright (c) 2023
 * 
 * @
 *  需要MDK工程中选择指定的硬件及配置
 *  BOARD_XXX
 *  
 */
/// 启用硬件定义
#define __BOARD_DEFINE__ 

/// 加载板级外设定义
#include "config/board_altibox.h"
//#else 
//#error "Please choose board configurations file"
//#endif 


// 是否启动日志
#define TRACE_ENABLE
#define TRACE_TAG "board"
#define TRACE_LEVEL T_INFO

#include <common/trace.h>


/**
 * @brief 初始化所有外设
 * 
 */
void boardInit(void)
{
    ilog("Board: %s", BOARD_NAME);

    // 初始化GPIO
    gpioInit(s_gpios, ARRAY_SIZE(s_gpios));

    // 初始化串口
    serialInit(s_serials, ARRAY_SIZE(s_serials));

    // 初始化I2C
    i2cInit(s_i2cs, ARRAY_SIZE(s_i2cs));

    // 初始化ADC 
    adcInit(s_adcs, ARRAY_SIZE(s_adcs));

    // 初始化BMP585
    bmp585Init(&s_bmp585, 1);

    // 初始化EEPROM
    i2cEepromInit(&s_eepromBus, &s_eeprom, 1);
}

