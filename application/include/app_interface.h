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

#ifndef __APP_INTERFACE_H__
#define __APP_INTERFACE_H__
#include "global_config.h"
#include "platform_define.h"
#include "resource_map.h"
#include "bsp_common.h"
#include "core_dma.h"
#include "core_flash.h"
#include "core_debug.h"
#include "core_tick.h"
#include "core_otp.h"
#include "core_timer.h"
#include "core_i2c.h"
#include "core_spi.h"
#include "core_wdt.h"
#include "core_rtc.h"
#include "core_pm.h"
#include "core_hwtimer.h"
#include "core_service.h"
#include "core_keyboard.h"
#include "core_dcmi.h"
#include "core_rng.h"
#include "core_task.h"
#include "audio_ll_drv.h"
#include "core_soft_keyboard.h"
#include "lfs.h"
#include "usb_driver.h"
#include "usb_hid.h"
void FileSystem_Init(void);
#endif
