/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __CORE_DEBUG_H__
#define __CORE_DEBUG_H__
#include "stdint.h"
#include "bsp_common.h"

enum
{
	DBG_CMD_REBOOT = 0,
	DBG_CMD_FLASHDOWNLOADSTART,
	DBG_CMD_FLASHDOWNLOAD,
	DBG_CMD_FLASHDOWNLOADEND,
	DBG_CMD_RUNAPP,
	DBG_DEVICE_FW_UPGRADE_READY = 0x80,
	DBG_DEVICE_FW_UPGRADE_RESULT,
	DBG_CMD_INFO = 0xff,
};

#ifdef __BUILD_APP__

#define DBG_INFO(x,y...) DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)
#define DBG_ERR(x,y...) DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)
#define DBG(x,y...)		DBG_Printf("%s %d:"x"\r\n", __FUNCTION__,__LINE__,##y)
#define DBGF	DBG_ERR("!")

#else

#define DBG_INFO(x,y...) DBG_Trace("%s %d:"x, __FUNCTION__,__LINE__,##y)
#define DBG_ERR(x,y...) DBG_Trace("%s %d:"x, __FUNCTION__,__LINE__,##y)
//#define DBG(x,y...) DBG_Trace("%s %d:"x, __FUNCTION__,__LINE__,##y)
#define DBG(x,y...)
#define DBGF	DBG_ERR("!")
#endif

/**
 * @brief log日志输出初始化
 * 
 * @param AppMode 是否是APP模式，1是 0否
 */
void DBG_Init(uint8_t AppMode);

/**
 * @brief 16进制数据转成字符串输出
 * 
 * @param Data 
 * @param len 
 */
void DBG_HexPrintf(void *Data, unsigned int len);

/**
 * @brief 非阻塞输出，app用
 * 
 * @param format 
 * @param ... 
 */
void DBG_Printf(const char* format, ...);

/**
 * @brief 阻塞输出，在bootloader和打印堆栈信息用
 * 
 * @param format 
 * @param ... 
 */
void DBG_Trace(const char* format, ...);

/**
 * @brief 非阻塞直接输出，app用
 * 
 * @param Data 
 * @param Len 
 */
void DBG_DirectOut(void *Data, uint32_t Len);

/**
 * @brief 阻塞输出到串口，app用，一般不需要调用
 * @param IsForce 是否强制发送
 */
void DBG_Send(void);

void DBG_Response(uint8_t Cmd, uint8_t Result, uint8_t *Data, uint8_t Len);

void DBG_SetRxCB(CBFuncEx_t cb);

void DBG_SetTxCB(CBDataFun_t cb);

void add_printf_data(uint8_t *data, uint32_t len);

#endif
