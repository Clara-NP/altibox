

/**
 * @file storage.c
 * @author Liu Chuansen (samule@neptune-robotics.com)
 * @brief 使用双区驱动，实现配置保存
 * @version 0.1
 * @date 2023-04-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "storage.h"

#include <storage/single_block.h>
#include <storage/dual_block.h>
#include <common/soc/soc_flash.h>
#include <config/export_ids.h>
#include <config/product_config.h>

#define TRACE_TAG  "storage"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>


/**
 * @brief 数据标签
 * 
 */
enum FACTORY_DATA_TAG 
{
    FACTORY_DATA_TAG_V0 = 0,
    /// 当前数据标签
    FACTORY_DATA_TAG_CURRENT = FACTORY_DATA_TAG_V0
};

/**
 * @brief 数据标签
 * 
 */
enum USER_DATA_TAG 
{
    USER_DATA_TAG_V0 = 0,
    /// 当前数据标签
    USER_DATA_TAG_CURRENT = USER_DATA_TAG_V0
};

/**
 * @brief 存储管理对象
 * 
 */
static singleBlockStorage_t *s_factoryStorage = NULL;
static dualBlockStorage_t *s_userStorage = NULL;

/**
 * @brief 初始化存储空间
 * 
 * @return int 总是使用顶部的两个分区
 */
int storageInit(void)
{
    s_factoryStorage = singleBlockStorageInit(SINGLE_BLOCK_I2C_EEPROM, EEPROM_MAIN, FACTORY_PARAMETER_ADDRESS, FACTORY_PARAMETER_SIZE);
    s_userStorage = dualBlockStorageInit(DUAL_BLOCK_I2C_EEPROM, EEPROM_MAIN, USER_PARAMETER_ADDRESS, USER_PARAMETER_SIZE);

    if (s_factoryStorage == NULL)
    {
        wlog("factory storage init failed");
        return RET_FAILED;
    }

    if (s_userStorage == NULL)
    {
        wlog("user storage init failed");
        return RET_FAILED;
    }

    // 产测参数
    singleStorageStatus_t factoryStatus;
    if (singleBlockStorageGetStatus(s_factoryStorage, &factoryStatus) == RET_SUCCESS)
    {
        ilog("factory storage tag:%d, size:%d, saved count:%d", factoryStatus.dataTag, factoryStatus.dataSize, factoryStatus.savedCount);

        // 如果标签不一样，给出warning
        // 如果标签一样，但大小不一样，给出warning
        if ((int)factoryStatus.dataTag < FACTORY_DATA_TAG_CURRENT)
        {
            wlog("factory storage with %s tag:%d, current tag:%d", "old", factoryStatus.dataTag, FACTORY_DATA_TAG_CURRENT);
        }
        else if ((int)factoryStatus.dataTag > FACTORY_DATA_TAG_CURRENT)
        {
            wlog("factory storage with %s tag:%d, current tag:%d", "future", factoryStatus.dataTag, FACTORY_DATA_TAG_CURRENT);
        }
        else 
        {
            if (factoryStatus.dataSize != sizeof(factoryParameter_t))
            {
                wlog("factory storage data size(%d) is not match for define(%d)", factoryStatus.dataSize, sizeof(factoryParameter_t));
            }
        }

        ilog("factory storage state, block:0x%04x ", factoryStatus.blockState);
    }


    // 用户参数
    storageStatus_t status;
    if (dualBlockStorageGetStatus(s_userStorage, &status) == RET_SUCCESS)
    {
        ilog("user storage tag:%d, size:%d, saved count:%d", status.dataTag, status.dataSize, status.savedCount);

        // 如果标签不一样，给出warning
        // 如果标签一样，但大小不一样，给出warning
        if ((int)status.dataTag < USER_DATA_TAG_CURRENT)
        {
            wlog("user storage with %s tag:%d, current tag:%d", "old", status.dataTag, USER_DATA_TAG_CURRENT);
        }
        else if ((int)status.dataTag > USER_DATA_TAG_CURRENT)
        {
            wlog("user storage with %s tag:%d, current tag:%d", "future", status.dataTag, USER_DATA_TAG_CURRENT);
        }
        else 
        {
            if (status.dataSize != sizeof(userParameter_t))
            {
                wlog("user storage data size(%d) is not match for define(%d)", status.dataSize, sizeof(userParameter_t));
            }
        }

        ilog("user storage state, block1:0x%02x block2:0x%02x", status.blockState & 0xff, (status.blockState >> 8) & 0xff);
    }

    return RET_SUCCESS;
}


/**
 * @brief 返回存储区错误 see::BLOCK_STATE_BITS
 * 
 * @return uint16_t 
 *  如果返回 0xffff 表示存储初始化有误
 *  否则返回按位状态 
 */
uint16_t factoryStorageState(void)
{
    if (s_factoryStorage == NULL)
    {
        return 0xffff;
    }

    singleStorageStatus_t status;
    if (singleBlockStorageGetStatus(s_factoryStorage, &status) == RET_SUCCESS)
    {
        return status.blockState & 0xffff;
    }

    // 有可能是初始化失败了
    return 0xffff;
}


/**
 * @brief 读产测配置参数
 * 
 * @param parameter 
 * @return int 
 */
int loadFactoryParameter(factoryParameter_t *parameter)
{
    if (s_factoryStorage == NULL)
    {
        return RET_NO_OBJECT;
    }

    singleStorageStatus_t status;
    status.dataSize = 0;

    // 查看存储状态
    int ret = singleBlockStorageGetStatus(s_factoryStorage, &status);

    // 如果ret返回失败，dataSize保持为0
    if (status.dataSize > 0)
    {
        dlog("factory find parameter with tag:%d, size:%d", status.dataTag, status.dataSize);
        // 有数据
        if (status.dataTag == FACTORY_DATA_TAG_CURRENT)
        {
            ret = singleBlockStorageRead(s_factoryStorage, (uint8_t *)parameter, sizeof(*parameter));  
            if (ret != RET_SUCCESS)
            {
                return ret;
            }    

            return RET_SUCCESS;        
        }
        else
        {
            // TODO
            elog("factory not matching the current tag(%d %d), invalid data", status.dataTag, FACTORY_DATA_TAG_CURRENT);
            return RET_INVALID_PARAMETER;
        }
    }

    return RET_INVALID_DATA;
}



/**
 * @brief 写产测配置参数
 * 
 * @param parameter 
 * @return int 
 */
int saveFactoryParameter(const factoryParameter_t *parameter)
{
    if (s_factoryStorage == NULL)
    {
        return RET_NO_OBJECT;
    }

    return singleBlockStorageWrite(s_factoryStorage, FACTORY_DATA_TAG_CURRENT, (uint8_t *)parameter, sizeof(*parameter));
}



/**
 * @brief 返回存储区错误 see::BLOCK_STATE_BITS
 * 
 * @return uint16_t 
 *  如果返回 0xffff 表示存储初始化有误
 *  否则返回按位状态 
 */
uint16_t userStorageState(void)
{
    if (s_userStorage == NULL)
    {
        return 0xffff;
    }

    storageStatus_t status;
    if (dualBlockStorageGetStatus(s_userStorage, &status) == RET_SUCCESS)
    {
        return status.blockState & 0xffff;
    }

    // 有可能是初始化失败了
    return 0xffff;
}


/**
 * @brief 读用户配置参数
 * 
 * @param parameter 
 * @return int 
 */
int loadUserParameter(userParameter_t *parameter)
{
    if (s_userStorage == NULL)
    {
        return RET_NO_OBJECT;
    }

    storageStatus_t status;
    status.dataSize = 0;

    // 查看存储状态
    int ret = dualBlockStorageGetStatus(s_userStorage, &status);

    // 如果ret返回失败，dataSize保持为0
    if (status.dataSize > 0)
    {
        dlog("user find parameter with tag:%d, size:%d", status.dataTag, status.dataSize);
        // 有数据
        if (status.dataTag == USER_DATA_TAG_CURRENT)
        {
            ret = dualBlockStorageRead(s_userStorage, (uint8_t *)parameter, sizeof(*parameter));  
            if (ret != RET_SUCCESS)
            {
                return ret;
            }    
            return RET_SUCCESS;        
        }
        else
        {
            // TODO
            elog("user not matching the current tag(%d %d), invalid data", status.dataTag, USER_DATA_TAG_CURRENT);
            return RET_INVALID_PARAMETER;
        }
    }

    return RET_INVALID_DATA;
}



/**
 * @brief 写配置参数
 * 
 * @param parameter 
 * @return int 
 */
int saveUserParameter(const userParameter_t *parameter)
{
    if (s_userStorage == NULL)
    {
        return RET_NO_OBJECT;
    }

    return dualBlockStorageWrite(s_userStorage, USER_DATA_TAG_CURRENT, (uint8_t *)parameter, sizeof(*parameter));
}
