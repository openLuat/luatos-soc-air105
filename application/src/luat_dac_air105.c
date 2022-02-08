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
#include "luat_dac.h"
#include "app_interface.h"

int luat_dac_setup(uint32_t ch, uint32_t freq, uint32_t mode) {
    if (ch != 0)
        return -1;
    GPIO_Iomux(GPIOC_01, 2);
    DAC_DMAInit(ch, DAC_TX_DMA_STREAM);
    DAC_Setup(ch, freq, mode);
    return 0;
}

int luat_dac_write(uint32_t ch, uint16_t* buff, size_t len) {
    if (ch != 0)
        return -1;
    DAC_Send(ch, buff, len, NULL, NULL);
    return 0;
}

int luat_dac_close(uint32_t ch) {
	DAC_ForceStop(ch);
    return 0; // 暂不支持关闭
}

