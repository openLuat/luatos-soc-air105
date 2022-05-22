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

#include "luat_base.h"
#include "luat_softkeyboard.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"

#include "app_interface.h"

#define LUAT_LOG_TAG "soft_keyboard"
#include "luat_log.h"

static void l_softkeyboard_irq_cb(void *pData, void *pParam) {
    rtos_msg_t msg = {0};
    msg.handler = l_softkeyboard_handler;
    msg.arg1 = 0;
    msg.arg2 = (uint16_t) pData;
    msg.ptr = (((uint32_t)pData) >> 16) & 0x1;
    luat_msgbus_put(&msg, 0);
}

int luat_softkeyboard_init(luat_softkeyboard_conf_t *conf){
	SoftKB_Setup(6250, 4, 2, 1, l_softkeyboard_irq_cb, NULL);
	SoftKB_IOConfig(conf->inio, conf->inio_num, conf->outio, conf->outio_num, 0);
    SoftKB_Start();
    return 0;
}

int luat_softkeyboard_deinit(luat_softkeyboard_conf_t *conf){
    SoftKB_Stop();
    return 0;
}

