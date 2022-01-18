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
#include "luat_keyboard.h"

#include "app_interface.h"

#define LUAT_LOG_TAG "keyboard"
#include "luat_log.h"


static int32_t kb_cb(void *pData, void *pParam) {
	luat_keyboard_irq_cb cb = (luat_keyboard_irq_cb)pParam;
    if (cb) {
        luat_keyboard_ctx_t ctx;
        ctx.port = 0;
        ctx.pin_data = (uint16_t) pData;
        ctx.state = (((uint32_t)pData) >> 16) & 0x1;
        cb(&ctx);
    }
}

int luat_keyboard_init(luat_keyboard_conf_t *conf) {
    // 按conf->port_conf 设置io复用
    if (conf->pin_conf & (1 << 0)) 
        GPIO_Iomux(GPIOD_12, 2); // keyboard0
    if (conf->pin_conf & (1 << 1))
        GPIO_Iomux(GPIOD_13, 2); // keyboard1
    if (conf->pin_conf & (1 << 2))
        GPIO_Iomux(GPIOD_14, 2); // keyboard2
    if (conf->pin_conf & (1 << 3))
        GPIO_Iomux(GPIOD_15, 2); // keyboard3
    if (conf->pin_conf & (1 << 4))
        GPIO_Iomux(GPIOE_00, 2); // keyboard4
    if (conf->pin_conf & (1 << 5))
        GPIO_Iomux(GPIOE_01, 2); // keyboard5
    if (conf->pin_conf & (1 << 6))
        GPIO_Iomux(GPIOE_02, 2); // keyboard6
    if (conf->pin_conf & (1 << 7))
        GPIO_Iomux(GPIOD_10, 2); // keyboard7
    if (conf->pin_conf & (1 << 8))
        GPIO_Iomux(GPIOD_11, 2); // keyboard8
    //---------------------------
    KB_Setup(conf->pin_map, 7, kb_cb, conf->cb);
    return 0;
}

int luat_keyboard_deinit(luat_keyboard_conf_t *conf) {
    KB_Stop();
    return 0;
}

