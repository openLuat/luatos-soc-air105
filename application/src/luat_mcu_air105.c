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
#include "luat_mcu.h"
#include "luat_spi.h"
#include "app_interface.h"

#include "FreeRTOS.h"
extern uint32_t SystemCoreClock;
int luat_mcu_set_clk(size_t mhz) {
    return 0;
}

int luat_mcu_get_clk(void) {
    return SystemCoreClock/1000000;
}

static uint8_t unique_id[16] = {0};
const char* luat_mcu_unique_id(size_t* t) {
    OTP_GetSn(unique_id);
    *t = sizeof(unique_id);
    return unique_id;
}

long luat_mcu_ticks(void) {
    return GetSysTickMS();
}

uint32_t luat_mcu_hz(void) {
    return 1000;
}

uint64_t luat_mcu_tick64(void) {
	return GetSysTick();
}

int luat_mcu_us_period(void) {
	return SYS_TIMER_1US;
}

void luat_mcu_delay_us(int delay)
{
	Task_DelayUS(delay);
}

void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay)
{
	switch(source_main)
	{
	case 0:
		PM_Set12MSource(0, delay);
		break;
	case 1:
		PM_Set12MSource(1, delay);
		break;
	}
	switch(source_32k)
	{
	case 0:
		PM_Set32KSource(0);
		break;
	case 1:
		PM_Set32KSource(1);
		break;
	}
}

int luat_mcu_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen)
{
	PV_Union uPV;
	CoreUpgrade_HeadStruct *Head = luat_heap_malloc(sizeof(CoreUpgrade_HeadStruct));
	memset(Head, 0, sizeof(CoreUpgrade_HeadStruct));
	Head->MaigcNum = __APP_START_MAGIC__;
	uPV.u8[0] = CORE_OTA_MODE_FULL;
	Head->DataStartAddress = start_address;
	if (path && pathlen)
	{
		uPV.u8[1] = CORE_OTA_IN_FILE;
		memcpy(Head->FilePath, path, pathlen);
		Head->Param1 = uPV.u32;
	}
	else
	{
		if (start_address > __FLASH_BASE_ADDR__)
		{
			uPV.u8[1] = CORE_OTA_IN_FLASH;
		}
		else
		{
			uPV.u8[1] = CORE_OTA_OUT_SPI_FLASH;
			uPV.u8[2] = luat_spi_get_hw_bus(spi_device->bus_id);
			Head->Param1 = uPV.u32;

			switch(luat_spi_get_hw_bus(spi_device->bus_id))
			{
			case HSPI_ID0:
				uPV.u8[0] = GPIOC_13;
				uPV.u8[1] = GPIOC_12;
				uPV.u8[2] = GPIOC_15;
				break;
			case SPI_ID0:
				uPV.u8[0] = GPIOB_14;
				uPV.u8[1] = GPIOB_15;
				uPV.u8[2] = GPIOB_12;
				break;
			case SPI_ID1:
				uPV.u8[0] = GPIOA_08;
				uPV.u8[1] = GPIOA_09;
				uPV.u8[2] = GPIOA_06;
				break;
			case SPI_ID2:
				uPV.u8[0] = GPIOB_04;
				uPV.u8[1] = GPIOB_05;
				uPV.u8[2] = GPIOB_02;
				break;

			}
			uPV.u8[3] = spi_device->spi_config.cs;
			Head->Param2 = uPV.u32;
		}
	}
	return Core_OTAInit(Head, len);
}

int luat_mcu_fota_write(uint8_t *data, uint32_t len)
{
	return Core_OTAWrite(data, len);
}

int luat_mcu_fota_done(void)
{
	return Core_OTACheckDone();
}

int luat_mcu_fota_end(uint8_t is_ok)
{
	Core_OTAEnd(is_ok);
	return 0;
}

uint8_t luat_mcu_fota_wait_ready(void)
{
	return Core_OTACheckReadyStart();
}
