
/**
 * @file board_v1.h
 * @author Liu Chuansen (179712066@qq.com)
 * @brief NEPTUNE ONE 板级配置文件
 * @version 0.1
 * @date 2023-03-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <common/generic.h>

#include <gpio/gpio.h>
#include <serial/serial.h>
#include <i2c/i2c.h>
#include <adc/adc.h>
#include <can/can.h>
#include <rtc/rtc.h>
#include <sensor/bmp585/bmp585.h>
#include <eeprom/i2c_eeprom.h>

#include <config/export_ids.h>

#ifndef __BOARD_DEFINE__
#error "!!!This header is only for board.c. please check"
#endif 


/**
 * @brief 定义板的名称
 * 
 */

// 显示V3.1及兼容的硬件版本
#define BOARD_NAME "altibox"

/**
 * @brief 定义使用的GPIO
 * 
 */
static const gpioConfig_t s_gpios[] = 
{
    { LED_SYS,                          GPIO_CHIP_SOC, _PB2, GPIO_DIR_OUTPUT, GPIO_FLAG_ACTIVE_LOW},            
};

/**
 * @brief 定义项目使用串口列表 
 * 
 */
static const serialConfig_t s_serials[] = 
{
    /// CORE USART1 PA9(tx) PA10(rx)
    {
        .id = SERIAL_RS485_1, .chip = SERIAL_CHIP_SOC, .port = _UART1, .setting = _UART_SETTING_DEFAULT(115200), 
        .control = _UART_TX_DMA | _UART_RX_DMA | _UART_RS485_GPIO(_PA8), .chipParameter = NULL
    },  
};


/**
 * @brief CAN 总线
 * 
 * @note　BS1 BS2的设置依赖于系统时钟及分频系统
 *  
 * 如果-初始化时给出警告：
 *   W (soc-can) can2 clock unexpected, set:500000, real:535714
 * 表示初始值设置有误差
 * 
 * STM32F407工作在168MHZ 主频，CAN总线时钟为42MHZ 1 + BS1+BS2 =1 + 8 + 5 刚好被整除, 
 * STM32F446 180MHZ， CAN总线时钟为 45MHZ 1 + BS1+BS2 = 1 + 8 + 6 刚好被整除 
 */
static const canConfig_t s_cans[] = 
{
    {.id = CAN_MAIN, .chip = CAN_CHIP_SOC, .port = _CAN1, .parameter = \
        {.canIdFilterValue = 0, .canIdFilterMask = 0, .rate = CAN_RATE_125KBS, \
        .timing = _CAN_BS1(3) | _CAN_BS2(2), .specialParameter = NULL}}
};


/**
 * @brief 定义ADC外设
 *
 */
static const adcConfig_t s_adcs[] =
{
    // parameter -> dma:ffff 指定启用DMA方式

    /*
    电压采集，
     Vadc/3.3v = ADC/4096
     Vadc = (ADC * 3.3) / 4096
     
     电压:
     Vin / Vadc = (1000K + 5.1K) / 5.1K
     Vin = (1005.1K / 5.1K) * Vadc 

     电压(HW>= 3.1):
     Vin * 8 / Vadc = (1000K + 500) / 500
     Vin = ((1000500 / 500) * Vadc) / 8

     电流： 转换为 A
     (30 * 3.3 / (4096 * 60 * 75)) * 1000
    */
    {.id = ADC_CURRENT,         .adc = _ADC1, .channel = _ADC_CH0,  \
        .flags = ADC_FLAG_RANGE(12), .calculation = ADC_LINEAR(0, (66.0f/4096/3)), .parameter = "dma:30003"},

    {.id = ADC_VOLTAGE,         .adc = _ADC1, .channel = _ADC_CH1,  \
        .flags = ADC_FLAG_RANGE(12), .calculation = ADC_LINEAR(0, 0.2015168f)},
    /*
        STM32F1 ADC 温度计算：
        计算温度公式：T（℃）=（Vsense-V25）/Avg_Slope+25
        Vsense是温度通道测得的电压值，V25是25℃时的典型电压值（1.43），Avg_Slope是温度与Vsense曲线的平均斜率（典型值为4.3mv/℃）
        ( [ADC]*3.3f/4096-1.43)/0.0043+25)
        将它转换为线性公式：
          ((ADC * 3.3 * 10000)/(4096 * 43) - (1.43 * 10000) / 43) + 25
           = ADC * (4125 / 512 * 43) - (14300/43) + 25 
           = ADC * (4125 / 22016) - 13225/43
    */
    {.id = ADC_MCU_TEMPERATURE,              .adc = _ADC1, .channel = _ADC_CH16, \
        .flags = ADC_FLAG_RANGE(12), .calculation = ADC_LINEAR(-13225.0f/43, 4125.0f/22016.0f)},

    /*
        这里测试的是1.224V对应的ADC值，通过这个参考点，可用来计算VREF的值，即满量程对应的值。一般是MCU的VDD。

        VDD/4096 = 1.224 / ADC
        VDD = 1.224 * 4096 /ADC 
    */
    {.id = ADC_MCU_VREF,                     .adc = _ADC1, .channel = _ADC_CH17, \
        .flags = ADC_FLAG_RANGE(12), .calculation = ADC_INVERSE_PROPORTION(0, 1.224f * 4096)},    
};

/**
 * @brief CAN 总线
 * 
 * @note　BS1 BS2的设置依赖于系统时钟及分频系统
 *  
 * 如果-初始化时给出警告：
 *   W (soc-can) can2 clock unexpected, set:500000, real:535714
 * 表示初始值设置有误差
 * 
 * STM32F407工作在168MHZ 主频，CAN总线时钟为42MHZ 1 + BS1+BS2 =1 + 8 + 5 刚好被整除, 
 * STM32F446 180MHZ， CAN总线时钟为 45MHZ 1 + BS1+BS2 = 1 + 8 + 6 刚好被整除 
 */
static const i2cConfig_t s_i2cs[] = 
{
    {.id = I2C_MAIN, .chip = I2C_CHIP_SOC, .port = _I2C1, .rate = 100000, .chipParameter = NULL},
    {.id = I2C_MAIN, .chip = I2C_CHIP_SOC, .port = _I2C2, .rate = 100000, .chipParameter = NULL}
    //{.id = I2C_MAIN, .chip = I2C_CHIP_GPIO_SIMULATE, .port = _I2C2, .rate = 100000, .chipParameter = "scl:26,sda:27"}
};


/**
 * @brief 板上BMP585
 * 
 */

static const i2cDeviceConfig_t s_bmp585 = {.id = BMP585_BOARD, .bus = I2C_MAIN, .address = 0x47};

/**
 * @brief 定义RTC时钟芯片
 *
 */
static const rtcConfig_t s_rtc ={.id = 0, .chip = RTC_CHIP_SOC, .chipParameter = NULL};


/**
 * @brief 板上EEPROM i2c配置
 * 
 */
static const i2cDeviceConfig_t s_eepromBus = {.id = EEPROM_MAIN, .bus = I2C_MAIN, .address = 0x50};

/**
 * @brief 板上EEPROM 地址配置
 * 
 */
static const i2cEepromConfig_t s_eeprom = {.pageSize = 32, .pageDelay = 10, .byteDelay = 1, .chipSize = 4096};

