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
#include "luat_log.h"
#include "luat_cmux.h"
#include "luat_conf_bsp.h"
#include "app_interface.h"

extern luat_cmux_t cmux_ctx;

void DBG_DirectOut(void *Data, uint32_t Len);

void luat_nprint(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_ctx.state == 1 && cmux_ctx.log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    DBG_DirectOut(s, l);
}

void luat_log_write(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_ctx.state == 1 && cmux_ctx.log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    DBG_DirectOut(s, l);
}

static void luat_cmux_write_cb(uint8_t *data, uint32_t len){
    luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,data, len);
}

void luat_cmux_log_set(uint8_t state) {
    if (state == 1){
        DBG_SetTxCB(luat_cmux_write_cb);
    }else if(state == 0){
        DBG_SetTxCB(add_printf_data);
    }
    
}