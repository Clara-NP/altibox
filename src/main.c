

/**
 * @file main.c
 * @author Liu Chuansen (179712066@qq.com)
 * @brief 纳百外设--电源箱主函数 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <common/generic.h>

#include <console/command.h>
#include <console/console.h>

#include <gpio/gpio.h>
#include <adc/cli_adc.h>
#include <gpio/cli_gpio.h>
#include <can/cli_can.h>
#include <rtc/cli_rtc.h>
#include <sensor/bmp585/bmp585.h>
#include <sensor/sht3x/cli_sht3x.h>
#include <flow_task/flow_task.h>

#include "board.h"
#include "storage.h"
#include "altibox.h"

#include <common/no_trace_exception.h>

#define TRACE_TAG "main"
#define TRACE_LEVEL T_INFO
#define TRACE_ENABLE

#include <common/trace.h>

#ifdef CONFIG_CONSOLE 


extern void dumpModuleInfo(void);


#include <can/can.h>
#include <common/soc/soc_can.h>

/**
 * @brief 显示CAN的状态
 * 
 */
static void dumpCanState(void)
{
    canStatistics_t stats;

    canGetStatistics(CAN_MAIN, &stats);

    rawPrint("--can statistics--\r\n");
    rawPrint(" tx total: %llu\r\n", stats.txFrames);
    rawPrint(" tx error: %llu\r\n", stats.txErrors);
    rawPrint(" rx total: %llu\r\n", stats.rxFrames);
 
    canErrorStatus_t can;
    socCanGetErrorStatus(_CAN1, &can);

    rawPrint("--can error--\r\n");
    rawPrint(" bus-off errors: %lu\r\n", can.counters.busOffErrors);
    rawPrint(" passive errors: %lu\r\n", can.counters.passiveErrors);
    rawPrint(" warning errors: %lu\r\n", can.counters.warningErrors);
    rawPrint(" current tx errors: %u\r\n", can.txErrorCounter);
    rawPrint(" current rx errors: %u\r\n", can.rxErrorCounter);
    rawPrint(" current erros    : bus-off:%d passive:%d warning:%d\r\n", (can.statusBits & _CAN_BUS_OFF_ERROR) ? 1 : 0, 
        (can.statusBits & _CAN_PASSIVE_ERROR) ? 1 : 0, (can.statusBits & _CAN_WARNING_ERROR) ? 1 : 0);

}



/*
local set model <>
local set sn <>
local set address <>
local show module
local show can
local show storage

*/
CMD_DEFINE(local, "local test",     
    "$ set (model|sn|address) <value>\r\n" 
    " $ dump (module|can|storage)\r\n" 
    )
{  
    int ret;

    if (argc < 1)
    {
        return 1;
    }

    const char *cmd = argv[0];

    if (cmd[0] == 's')
    {
        if (argc < 3)
        {
            return 2;
        }
        const char *type = argv[1];

        factoryParameter_t p  = { 0 };
        loadFactoryParameter(&p);

        if (type[0] == 'm')
        {
            osSafeStrncpy(p.model, argv[2], sizeof(p.model));
        }
        else if (type[0] == 's')
        {
            osSafeStrncpy(p.sn, argv[2], sizeof(p.sn));
        }
        else if (type[0] == 'a')
        {
            int addr = strtoul(argv[2], NULL, 10);
            if ((addr < 1) || (addr > 127))
            {
                rawPrint("invalid address:%d\r\n", addr);
                return 2;
            }

            p.address = addr;
        }
        else 
        {
            return 3;
        }

        // 保存参数
        saveFactoryParameter(&p);

        return 0;
    }
    else if (cmd[0] == 'd') 
    {

        if (argc < 2)
        {
            return 2;
        }
        const char *type = argv[1];

        if (type[0] == 'm')
        {
            dumpModuleInfo();            
        }
        else if (type[0] == 'c')
        {
            dumpCanState();
        }
        else if (type[0] == 's')
        {
            factoryParameter_t v;
            userParameter_t u;
            ret = loadFactoryParameter(&v);
            ret = loadUserParameter(&u);
            if (ret == RET_SUCCESS)
            {
                rawPrint("--storage parameters--\r\n");
                rawPrint(" address=%d\r\n", v.address);
                rawPrint(" hw-version=%d\r\n", v.hwVersion);
                rawPrint(" model=%s\r\n", v.model);
                rawPrint(" sn=%s\r\n", v.sn);
                rawPrint(" anti-tamper=%d\r\n", u.antiTamperState);
            }
            else 
            {
                rawPrint("***load parameter failed, ret=%d\r\n", ret);
                return 1;
            }
        }
        else 
        {
            return 3;
        }

        return 0;
    }

    return 2;
}



#endif 


/**
 * @brief 主函数
 * 
 * @return int 
 */
int main(void)
{
    // 初始化BSP platform
    platformInit();

    {
        char timeString[16];
        __buildTime(timeString);
        ilog("FW Version:%s-%s", FRAMEWORK_VERSION_STRING, timeString);
    }

    #ifdef CONFIG_CONSOLE
    // 初始化控制台
    consoleInit();

    addCommand(CMD(adc));
    addCommand(CMD(pin));
    addCommand(CMD(sht));
    addCommand(CMD(can));
    addCommand(CMD(rtc));
    addCommand(CMD(local));
    #endif 

    // 板级初始化
    boardInit();

    // 先亮灯
    ledSet(LED_SYS, 1);

    // 初始化存储
    storageInit();

    // 初始化电源盒管理
    altiBoxInit();

    // 进入后闪灯 0x0003为慢闪
    ledFlash(LED_SYS, 0x0003, 0xffff);

    while(1) 
    {    
        /// 公共模块 - GPIO 调度
        gpioMainLoop();

        // 公共模块 - 流任务调度
        flowTaskSchedule();

        #ifdef CONFIG_CONSOLE
        // 公共模块 调用控制台循环
        consoleLoop(0);
        #endif 

        // 电源盒主任务
        altiBoxMainTick();
    }

    //return 0;
}

