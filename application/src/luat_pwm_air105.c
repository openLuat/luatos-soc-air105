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
#include "luat_pwm.h"

#include "app_interface.h"

#define LUAT_LOG_TAG "luat.pwm"
#include "luat_log.h"
#include "luat_gpio.h"

int luat_pwm_setup(luat_pwm_conf_t* conf) {
    int channel = conf->channel;
	size_t period = conf->period;
	size_t pulse = conf->pulse;
	size_t pnum = conf->pnum;
	size_t precision = conf->precision;

	if (precision != 100 && precision != 1000) {
		LLOGW("only 100 or 1000 PWM precision supported");
		return -1;
	}

	switch (channel)
	{
		case 0:
			GPIO_Iomux(GPIOB_00, 2);
			break;
		case 1:
			GPIO_Iomux(GPIOB_01, 2);
			break;
		case 2:
			GPIO_Iomux(GPIOA_02, 2);
			break;
		case 3:
			GPIO_Iomux(GPIOA_03, 2);
			break;
		case 4:
			GPIO_Iomux(GPIOC_06, 2);
			break;
		case 5:
			GPIO_Iomux(GPIOC_07, 2);
			break;
		case 6:
			GPIO_Iomux(GPIOC_08, 2);
			break;
		case 7:
			GPIO_Iomux(GPIOC_09, 2);
			break;
		default:
			break;
	}

	// 主频48M
	//uint32_t hz = 48000000 / period / precision;

	//HWTimer_StartPWM(channel, hz * pulse, hz * (precision - pulse), pnum);
	if(pulse * (1000 / precision) == 1000)//HWTimer_SetPWM传入duty大于999会不执行
	{
		switch (channel)
		{
			case 0:
				luat_gpio_mode(GPIOB_00, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOB_00, Luat_GPIO_HIGH);
				break;
			case 1:
				luat_gpio_mode(GPIOB_01, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOB_01, Luat_GPIO_HIGH);
				break;
			case 2:
				luat_gpio_mode(GPIOA_02, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOA_02, Luat_GPIO_HIGH);
				break;
			case 3:
				luat_gpio_mode(GPIOA_03, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOA_03, Luat_GPIO_HIGH);
				break;
			case 4:
				luat_gpio_mode(GPIOC_06, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOC_06, Luat_GPIO_HIGH);
				break;
			case 5:
				luat_gpio_mode(GPIOC_07, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOC_07, Luat_GPIO_HIGH);
				break;
			case 6:
				luat_gpio_mode(GPIOC_08, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOC_08, Luat_GPIO_HIGH);
				break;
			case 7:
				luat_gpio_mode(GPIOC_09, Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, 1);
				luat_gpio_set(GPIOC_09, Luat_GPIO_HIGH);
				break;
			default:
				break;
		}
	}
	else
		HWTimer_SetPWM(channel, period, pulse * (1000 / precision), pnum);
    return 0;
}

// @return -1 打开失败。 0 打开成功
int luat_pwm_open(int channel, size_t period, size_t pulse,int pnum) {
	switch (channel)
	{
		case 0:
			GPIO_Iomux(GPIOB_00, 2);
			break;
		case 1:
			GPIO_Iomux(GPIOB_01, 2);
			break;
		case 2:
			GPIO_Iomux(GPIOA_02, 2);
			break;
		case 3:
			GPIO_Iomux(GPIOA_03, 2);
			break;
		case 4:
			GPIO_Iomux(GPIOC_06, 2);
			break;
		case 5:
			GPIO_Iomux(GPIOC_07, 2);
			break;
		case 6:
			GPIO_Iomux(GPIOC_08, 2);
			break;
		case 7:
			GPIO_Iomux(GPIOC_09, 2);
			break;
		default:
			break;
	}
	HWTimer_SetPWM(channel, period, pulse * 10, pnum);
    return 0;
}

int luat_pwm_capture(int channel,int freq) {
	return 0;
}

// @return -1 关闭失败。 0 关闭成功
int luat_pwm_close(int channel) {
    HWTimer_Stop(channel);
    return 0;
}

