
#ifndef __STORAGE_H__
#define __STORAGE_H__

/**
 * @file storage.h
 * @author Liu Chuansen (179712066@qq.com)
 * @brief 存储管理
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <common/generic.h>
#include <storage/single_block.h>
#include <storage/dual_block.h>


/**
 * @brief 产测参数版本0的数据结构
 * 
 */
struct factoryParameterV0
{
    /// 设备地址
    uint8_t address;
    /// 保留
    uint8_t alignment;
    /// 硬件版本
    uint16_t hwVersion;
    /// 型号
    char model[32];
    /// 序列号
    char sn[32];
    /// 保留
    uint8_t reserved[32];
}__PACKED;

/// 总是指向最新的数据结构 
typedef struct factoryParameterV0 factoryParameter_t;



/**
 * @brief 用户参数版本0的数据结构
 * 
 */
struct userParameterV0
{
    /// 防拆状态
    uint8_t antiTamperState;
    /// 保留
    uint8_t reserved[31];
}__PACKED;

/// 总是指向最新的数据结构 
typedef struct userParameterV0 userParameter_t;



/**
 * @brief 初始化存储空间
 * 
 * @return int 总是使用顶部的两个分区
 */
int storageInit(void);

/**
 * @brief 返回存储区错误 see::BLOCK_STATE_BITS
 * 
 * @return uint16_t 
 *  如果返回 0xffff 表示存储初始化有误
 *  否则返回按位状态 
 */
uint16_t factoryStorageState(void);

/**
 * @brief 读产测配置参数
 * 
 * @param parameter 
 * @return int 
 */
int loadFactoryParameter(factoryParameter_t *parameter);

/**
 * @brief 写产测配置参数
 * 
 * @param parameter 
 * @return int 
 */
int saveFactoryParameter(const factoryParameter_t *parameter);

/**
 * @brief 返回存储区错误 see::BLOCK_STATE_BITS
 * 
 * @return uint16_t 
 *  如果返回 0xffff 表示存储初始化有误
 *  否则返回按位状态 
 */
uint16_t userStorageState(void);

/**
 * @brief 读用户配置参数
 * 
 * @param parameter 
 * @return int 
 */
int loadUserParameter(userParameter_t *parameter);

/**
 * @brief 写用户配置参数
 * 
 * @param parameter 
 * @return int 
 */
int saveUserParameter(const userParameter_t *parameter);

#endif // __STORAGE_H__

