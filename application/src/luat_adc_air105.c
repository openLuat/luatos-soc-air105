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

#include "luat_adc.h"
#include "luat_base.h"

#include "core_adc.h"
#include "platform_define.h"

#define LUAT_LOG_TAG "luat.adc"
#include "luat_log.h"

int luat_adc_open(int ch, void *args)
{
    switch (ch)
    {
    case 0:
        return 0;
    case 1:
        GPIO_Iomux(GPIOC_00, 2);
        break;
    case 2:
        GPIO_Iomux(GPIOC_01, 2);
        break;
    case 3:
        return -1;
    case 4:
        GPIO_Iomux(GPIOC_03, 2);
        break;
    case 5:
        GPIO_Iomux(GPIOC_04, 2);
        break;
    case 6:
        GPIO_Iomux(GPIOC_05, 2);
        break;
    default:
        return -1;
    }
    ADC_ChannelOnOff(ch, 1);
    return 0;
}

int luat_adc_read(int ch, int *val, int *val2)
{
    int voltage = 0;
    voltage = ADC_GetChannelValue(ch);
    *val = voltage;
    *val2 = voltage;
    return 0;
}

int luat_adc_close(int ch){
    if (ch)
        ADC_ChannelOnOff(ch, 0);
    return 0;
}
