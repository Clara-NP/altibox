#ifndef __CONFIG_STM32_H__
#define __CONFIG_STM32_H__

/*******************************************/
/*              PLATFORM-STM32             */
/*******************************************/

/**
 * @brief 平台选择 
 * 
 */
#define CONFIG_PLATFORM_HEADER  "stm32/platform.h"

/**
 * @brief 定义TRACE使用的串口(逻辑索引)
 * 
 */     
#define CONFIG_TRACE_UART           _UART1

/**
 * @brief 启用soc_uart.c中实现的UART1中断处理函数
 * 
 */
#define CONFIG_STM32_UART1_IRQHANDLER 1

/**
 * @brief 启用soc_uart.c中实现的UART2中断处理函数
 * 
 */
#define CONFIG_STM32_UART2_IRQHANDLER 1

/**
 * @brief 启用soc_uart.c中实现的UART3中断处理函数
 * 
 */
//#define CONFIG_STM32_UART3_IRQHANDLER 1


/**
 * @brief 启用UART1的DMA接收
 * 
 */
#define CONFIG_STM32_UART1_RX_DMA_ENABLE 1

/**
 * @brief 启用UART2的DMA接收
 * 
 */
#define CONFIG_STM32_UART2_RX_DMA_ENABLE 1

/**
 * @brief 启用UART3的DMA接收
 * 
 */
//#define CONFIG_STM32_UART3_RX_DMA_ENABLE 1

/**
 * @brief 启用UART4的DMA接收
 * 
 */
//#define CONFIG_STM32_UART4_RX_DMA_ENABLE 1

/**
 * @brief 启用UART5的DMA接收
 * 
 */
//#define CONFIG_STM32_UART5_RX_DMA_ENABLE 1

/**
 * @brief 启用UART6的DMA接收
 * 
 */
//#define CONFIG_STM32_UART6_RX_DMA_ENABLE 1

/**
 * @brief 启用UART1的DMA发送
 * 
 */
#define CONFIG_STM32_UART1_TX_DMA_ENABLE 1

/**
 * @brief 启用UART2的DMA发送
 * 
 */
#define CONFIG_STM32_UART2_TX_DMA_ENABLE 1

/**
 * @brief 启用UART3的DMA发送
 * 
 */
//#define CONFIG_STM32_UART3_TX_DMA_ENABLE 1

/**
 * @brief 启用UART4的DMA发送
 * 
 */
//#define CONFIG_STM32_UART4_TX_DMA_ENABLE 1

/**
 * @brief 启用UART5的DMA发送
 * 
 */
//#define CONFIG_STM32_UART5_TX_DMA_ENABLE 1

/**
 * @brief 启用UART6的DMA发送
 * 
 */
//#define CONFIG_STM32_UART6_TX_DMA_ENABLE 1


/**
 * @brief 启用ADC1的DMA功能
 * 
 */
#define CONFIG_STM32_ADC1_WITH_DMA  1

/**
 * @brief 启用CAN1中断处理函数
 * 
 */
#define CONFIG_STM32_CAN1_IRQHANDLER 1

/**
 * @brief 启用CAN2中断处理函数
 * 
 */
//#define CONFIG_STM32_CAN2_IRQHANDLER 1

/**
 * @brief 启用CAN错误计数
 * 
 */
#define CONFIG_STM32_CAN_ERROR_COUNTERS  1

/**
 * @brief I2C EEPROM 实例数量
 * 
 */
#define CONFIG_I2C_EEPROM_MAX_NUM   1

/**
 * @brief I2C传输最大大小
 * 
 */
#define CONFIG_I2C_TRANSFER_DATA_MAX_SIZE   (32 + 2)

#endif // __CONFIG_STM32_H__
