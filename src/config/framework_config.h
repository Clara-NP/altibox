
#ifndef __FRAMEWORK_CONFIG_H__
#define __FRAMEWORK_CONFIG_H__

// CONSOLE参数

/**
 * @brief 配置CONSOLE提示字串
 * 
 */
#define CONFIG_CONSOLE_PROMPT           "NEPT> "

/**
 * @brief 配置CONSOLE是否需要登录
 * 
 */
// #define CONFIG_CONSOLE_REQUIRE_LOGIN    1

/**
 * @brief 配置CONSOLE输入回车时是否执行上一次的命令
 * 
 */
#define CONFIG_CONSOLE_ENTER_EXECUTE_LAST_COMMAND  1

/**
 * @brief 配置CONSOLE登录静态密码
 * 
 */
// #define CONFIG_CONSOLE_LOGIN_PASSWORD   "434d53"

/**
 * @brief UART启用RS485方向控制
 * 
 */
#define CONFIG_UART_SUPPORT_RS485    1

/**
 * @brief 定义流任务的数量 
 * 
 */
#define CONFIG_FLOW_TASK_MAX_NUM  2



// 加载平台的配置文件
#include "platform_stm32.h"


#if defined(BUILD_RELEASED)

#define CONFIG_TRACE_USE_LOCAL_VSPRINTF  1
#define CONFIG_TRACE_PRINT_TAG 0 

#define CONFIG_TRACE_VIA_RTT  0
#define CONFIG_TRACE_VIA_UART 0

#define CONFIG_CLI_GPIO   0
#define CONFIG_CLI_PIN    0
#define CONFIG_CLI_ADC    0
#define CONFIG_CLI_CAN    0
#define CONFIG_CLI_SHT3X  0
#else

#ifdef CONFIG_TRACE_VIA_UART
#undef CONFIG_TRACE_VIA_UART
#endif
#define CONFIG_TRACE_VIA_UART 0
#endif 

//#define CONFIG_DRIVER_I2C_GPIO_SIMULATE 1

#endif // __FRAMEWORK_CONFIG_H__

