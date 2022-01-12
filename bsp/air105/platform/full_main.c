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

#include "app_inc.h"
uint32_t SystemCoreClock;
extern const uint32_t __isr_start_address;
extern const uint32_t __os_heap_start;
extern const uint32_t __ram_end;
//struct lfs_config lfs_cfg;
//struct lfs LFS;
void SystemInit(void)
{
	SCB->VTOR = (uint32_t)(&__isr_start_address);
	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_INC | SYSCTRL_FREQ_SEL_XTAL_192Mhz;
#ifndef __DEBUG__
	WDT_SetReload(SYSCTRL->PCLK_1MS_VAL * __WDT_TO_MS__);
	WDT_ModeConfig(WDT_Mode_CPUReset);
	WDT_Enable();
#endif
//	QSPI->DEVICE_PARA = (QSPI->DEVICE_PARA & 0xFFFF) | (68 << 16);
#if (__FPU_PRESENT) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11*2));
#endif
    SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_ALL;
    SYSCTRL->CG_CTRL2 = SYSCTRL_AHBPeriph_DMA|SYSCTRL_AHBPeriph_LCD;
    QSPI_Init(NULL);
    QSPI_SetLatency(0);
	__enable_irq();
}

void SystemCoreClockUpdate (void)            /* Get Core Clock Frequency      */
{
	SystemCoreClock = HSE_VALUE * (((SYSCTRL->FREQ_SEL & SYSCTRL_FREQ_SEL_XTAL_Mask) >> SYSCTRL_FREQ_SEL_XTAL_Pos) + 1);
}

void vApplicationTickHook( void )
{
	//PowerOnTickCnt++;
}

void vApplicationIdleHook( void )
{

}

static int32_t prvUartIrqCB(void *pData, void *pParam)
{
	DBG("%x", pData);
}

void Heart_Task(void *param)
{
	TickType_t xLastWakeTime;
	uint8_t UartID = UART_ID1;
	uint8_t Stream = DMA1_STREAM_2;
	uint32_t channel = SYSCTRL_PHER_CTRL_DMA_CHx_IF_UART1_TX;
	uint8_t Buf[128];
	uint8_t start = 0, i;
//	uint32_t block = 0;
//	uint32_t i;
	xLastWakeTime = xTaskGetTickCount();
	//Uart_GPIOInit(DBG_UART_TX_PORT, (DBG_UART_TX_AF << 24) | DBG_UART_TX_PIN, DBG_UART_RX_PORT, (DBG_UART_RX_AF << 24) | DBG_UART_RX_PIN);
//	Uart_BaseInit(UartID, DBG_UART_BR, 1, prvUartIrqCB);
//	DMA_TakeStream(Stream);
//	Uart_DMATxInit(UartID, Stream, channel);
//	FileSystem_Init();
//	do
//	{
//		DBG_INFO("block%d, test %d", block, start);
//		//memset(Buf, start, 64 * 1024);
//		for(i = 0; i < 16; i++)
//		{
//			lfs_cfg.erase(&lfs_cfg, block + i);
//		}
//		lfs_cfg.read(&lfs_cfg, block, 0, Buf, SPI_FLASH_BLOCK_SIZE);
//		for(i = 0; i < SPI_FLASH_BLOCK_SIZE; i++)
//		{
//			if (Buf[i] != 0xff)
//			{
//				DBG_INFO("erase fail! %u, %02x", i, Buf[i]);
//				break;
//			}
//		}
//		DBG_INFO("erase check ok");
//		memset(Buf, start, SPI_FLASH_BLOCK_SIZE);
//		lfs_cfg.prog(&lfs_cfg, block, 0, Buf, SPI_FLASH_BLOCK_SIZE);
//		memset(Buf, 0xff, SPI_FLASH_BLOCK_SIZE);
//		lfs_cfg.read(&lfs_cfg, block, 0, Buf, SPI_FLASH_BLOCK_SIZE);
//		for(i = 0; i < SPI_FLASH_BLOCK_SIZE; i++)
//		{
//			if (Buf[i] != start)
//			{
//				DBG_INFO("write fail! %u, %02x", i, Buf[i]);
//				break;
//			}
//		}
//		DBG_INFO("write check ok");
//		block += 16;
//		start++;
//		DBG_INFO("test done");
//	}while(block < lfs_cfg.block_count);
	DBG("APP Build %s %s!", __DATE__, __TIME__);
	DBG("app heap info start 0x%x, len %u!", (uint32_t)(&__os_heap_start), (uint32_t)(&__ram_end) - (uint32_t)(&__os_heap_start));

	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, 2000);
		DBG("DSFDSGFDGSFHSF %llu", GetSysTickUS());
//		start++;
//		memset(Buf, start, sizeof(Buf));
//		Uart_DMATx(UartID, Stream, Buf, sizeof(Buf));

		switch(start)
		{
		case 0:
			start++;
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				GPIO_Config(i, 0, 1);
			}
			break;
		case 1:
			start++;
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				GPIO_Config(i, 0, 0);
			}
			break;
		case 2:
			start++;
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				GPIO_PullConfig(i, 0, 0);
				GPIO_Config(i, 1, 0);
			}
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				Buf[i] = GPIO_Input(i);
			}
			for(i = 0; i < 6; i++)
			{
				DBG("Port %d", i);
				DBG_HexPrintf(&Buf[i * 16], 16);
			}

			break;
		default:
			start = 0;
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				GPIO_PullConfig(i, 1, 1);
			}
			for(i = HAL_GPIO_2; i < HAL_GPIO_MAX; i++)
			{
				Buf[i] = GPIO_Input(i);
			}
			for(i = 0; i < 6; i++)
			{
				DBG("Port %d", i);
				DBG_HexPrintf(&Buf[i * 16], 16);
			}
			break;
		}
	}
}

int main(void)
{
	__NVIC_SetPriorityGrouping(7 - __NVIC_PRIO_BITS);//对于freeRTOS必须这样配置
	SystemCoreClockUpdate();
    CoreTick_Init();
	cm_backtrace_init(NULL, NULL, NULL);
	bpool((uint32_t)(&__os_heap_start), (uint32_t)(&__ram_end) - (uint32_t)(&__os_heap_start));
	DMA_GlobalInit();
	DBG_Init(1);
//	DBG("APP Build %s %s!", __DATE__, __TIME__);
//	DBG("app heap info start 0x%x, len %u!", (uint32_t)(&__os_heap_start), (uint32_t)(&__ram_end) - (uint32_t)(&__os_heap_start));
//
	//	DBG_Trace("%x,%x,%x", (&__isr_start_address), (&__os_heap_start), (&__ram_end));
//	__enable_irq();
	xTaskCreate(Heart_Task, "test task", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
	/* 启动调度，开始执行任务 */
	vTaskStartScheduler();
    /* 如果系统正常启动是不会运行到这里的，运行到这里极有可能是空闲任务heap空间不足造成创建失败 */
    while (1)
	{
	}
}


