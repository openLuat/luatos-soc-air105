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

#ifndef __APP_INC_H__
#define __APP_INC_H__
#include "bl_inc.h"
#if 0
typedef unsigned char       u8;
typedef char                s8;
typedef unsigned short      u16;
typedef short               s16;
typedef unsigned int        u32;
typedef int                 s32;
typedef unsigned long long  u64;
typedef long long           s64;
//typedef unsigned char       bool;
typedef unsigned char       uint8_t;
//typedef char                int8_t;
typedef unsigned short      uint16_t;
typedef short               int16_t;
//typedef unsigned int        uint32_t;
//typedef int                 int32_t;
typedef unsigned long long  uint64_t;
typedef long long           int64_t;
#endif
#include "core_hwtimer.h"
#include "core_spi.h"
#include "core_adc.h"
#include "core_dac.h"
#include "core_wdt.h"
#include "core_usb_ll_driver.h"
#include "core_keyboard.h"
#include "core_dcmi.h"
#include "core_rng.h"
#ifdef __BUILD_OS__
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bget.h"
#include "core_task.h"
#include "core_service.h"
#define mallc OS_Malloc
#define zallc OS_Zalloc
#define reallc OS_Realloc
#define free OS_Free
#endif
#endif
