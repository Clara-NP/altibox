

#ifndef __EXPORT_IDS_H__
#define __EXPORT_IDS_H__


/**
 * @brief GPIO定义
 * 
 */
enum EXPORT_GPIO
{
    LED_SYS = 0,
    TEST_OUT,

    EXPORT_GPIO_MAX_NUM
};


/**
 * @brief 串口逻辑ID定义
 * 
 */
enum EXPORT_SERIAL
{
    SERIAL_RS485_1,
    SERIAL_RS485_2,

    EXPORT_SERIAL_MAX_NUM,
};


/**
 * @brief CAN总线逻辑ID定义
 * 
 */
enum EXPORT_CAN
{
    CAN_MAIN = 0,
    CAN_EXTEND,

    EXPORT_CAN_MAX_NUM,
};


/**
 * @brief ADC定义
 * 
 */
enum EXPORT_ADC 
{
    ADC_MCU_TEMPERATURE = 0,
    ADC_MCU_VREF,
    ADC_CURRENT,
    ADC_VOLTAGE,

    EXPORT_ADC_MAX_NUM,
};


/**
 * @brief I2C总线逻辑ID定义
 * 
 */
enum EXPORT_I2C
{
    I2C_MAIN = 0,
    I2C_EXTEND,

    EXPORT_I2C_MAX_NUM,
};


/**
 * @brief 板上的SHT30温湿度传感器
 * 
 */
enum EXPORT_BMP585
{
    BMP585_BOARD = 1,

    EXPORT_BMP585_MAX_NUM,
};


/**
 * @brief 板上的EEPROM
 * 
 */
enum EXPORT_EEPROM
{
    EEPROM_MAIN = 0,

    EXPORT_EEPROM_MAX_NUM,
};



#endif // __EXPORT_IDS_H__
