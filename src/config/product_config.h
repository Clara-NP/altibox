
#ifndef __PRODUCT_CONFIG_H__
#define __PRODUCT_CONFIG_H__


/// 传感器采集周期 - 单位为 ms
#define CONFIG_SENSOR_SAMPLE_PERIOD         100

/// 电源模块状态扫描周期 - 单位为ms
#define CONFIG_POWER_MODULE_SCAN_PERIOD     100


/// 配置版本号
#define CONFIG_VERSION_MAJOR   4
#define CONFIG_VERSION_MINOR   3


//---------------------存储管理-------------------------
#define FACTORY_PARAMETER_ADDRESS      0x0000
#define FACTORY_PARAMETER_SIZE         1024

#define USER_PARAMETER_ADDRESS         0x0400
#define USER_PARAMETER_SIZE            2048



#endif // __PRODUCT_CONFIG_H__

