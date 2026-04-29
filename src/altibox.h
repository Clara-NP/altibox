
#ifndef __ALTIBOX_H__
#define __ALTIBOX_H__

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
 * 我们要定义一些错误状态
 *  BMP585异常
 * 
 */

#include <common/generic.h>


/**
 * @brief 电源盒初始化及自检
 *
 */
void altiBoxInit(void);

/**
 * @brief 主任务
 *
 */
void altiBoxMainTick(void);

/**
 * @brief 是否正在进行 NPCP OTA（与协议升级状态机一致）
 */
bool altiboxOtaBusy(void);

#endif // __POWERBOX_H__
