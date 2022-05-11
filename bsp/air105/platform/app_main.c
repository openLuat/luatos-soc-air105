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
uint8_t gMainWDTEnable;
static HANDLE prvWDTTimer;
extern const uint32_t __isr_start_address;
extern const uint32_t __os_heap_start;
extern const uint32_t __ram_end;
extern const uint32_t __preinit_fun_array_start;
extern const uint32_t __preinit_fun_array_end;
extern const uint32_t __init_fun_array_start;
extern const uint32_t __init_fun_array_end;
extern const uint32_t __task_fun_array_start;
extern const uint32_t __task_fun_array_end;
#define SCRIPT_LUADB_START_ADDR			(__FLASH_BASE_ADDR__ + __CORE_FLASH_BLOCK_NUM__ * __FLASH_BLOCK_SIZE__)
const uint32_t __attribute__((section (".app_info")))
    g_CAppInfo[8] =
{
	__APP_START_MAGIC__,
	&__isr_start_address,
	0,//此处后期放入luatos固件版本
	0,
	SCRIPT_LUADB_START_ADDR,
	SCRIPT_LUADB_START_ADDR + __SCRIPT_FLASH_BLOCK_NUM__ * __FLASH_BLOCK_SIZE__,
	0,
	0,
};

static void prvSystemReserCtrl(void)
{
#ifdef __DEBUG__
	WDT_Feed();
#endif
}
void SystemInit(void)
{
	memcpy(__SRAM_BASE_ADDR__, (uint32_t)(&__isr_start_address), 1024);
	SCB->VTOR = __SRAM_BASE_ADDR__;
//#ifdef __USE_XTL__
//	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_EXT | SYSCTRL_FREQ_SEL_XTAL_192Mhz | SYSCTRL_FREQ_SEL_POWERMODE_CLOSE_CPU_MEM;
//#else
//	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_INC | SYSCTRL_FREQ_SEL_XTAL_192Mhz | SYSCTRL_FREQ_SEL_POWERMODE_CLOSE_CPU_MEM;;
//#endif
//	SCB->VTOR = (uint32_t)(&__isr_start_address);
	SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_ALL;
	SYSCTRL->SOFT_RST1 = SYSCTRL_APBPeriph_ALL;
    SYSCTRL->SOFT_RST2 &= ~SYSCTRL_USB_RESET;
    SYSCTRL->LOCK_R |= SYSCTRL_USB_RESET;
    SYSCTRL->LDO25_CR &= ~(BIT(4)|BIT(5));
//    BPU->SEN_ANA0 &= ~(1 << 10);
//#ifdef __USE_32K_XTL__
//	BPU->SEN_ANA0 |= (1 << 10)|(7 << 25) | (2 << 29);	//外部32768+最大充电电流+最大晶振供电
//#else
//	BPU->SEN_ANA0 &= ~(1 << 10);
//	BPU->SEN_ANA0 |= (7 << 25) | (2 << 29);	//内部32K+最大充电电流+最大晶振供电
//#endif
	BPU->SEN_ANA0 |= (7 << 25) | (2 << 29);//最大充电电流+中等晶振供电
	__enable_irq();
}

void SystemReset(void)
{
	NVIC_SystemReset();
}

void SystemCoreClockUpdate (void)            /* Get Core Clock Frequency      */
{
	SystemCoreClock = HSE_VALUE * (((SYSCTRL->FREQ_SEL & SYSCTRL_FREQ_SEL_XTAL_Mask) >> SYSCTRL_FREQ_SEL_XTAL_Pos) + 1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
        char * pcTaskName )
{
	DBG("%s stack overflow", pcTaskName);
}

void vApplicationTickHook( void )
{
	//PowerOnTickCnt++;
}

static int32_t prvMainWDT(void *pData, void *pParam)
{
	WDT_Feed();
}

void vApplicationIdleHook( void )
{
	if (gMainWDTEnable)
	{
		WDT_Feed();
		Timer_StartMS(prvWDTTimer, 12000, 1);
	}
	PM_Sleep();
}


static void prvMain_InitHeap(void)
{
	uint32_t prvHeapLen;
    prvHeapLen = (uint32_t)(&__ram_end) - (uint32_t)(&__os_heap_start);
	bpool((uint32_t)(&__os_heap_start), prvHeapLen);
}

static void prvTask_Init(void)
{
	volatile CommonFun_t *pFun;
	for(pFun = &__task_fun_array_start; pFun < &__task_fun_array_end; pFun++)
	{
		(*pFun)();
	}
}

static void prvDrv_Init(void)
{
	volatile CommonFun_t *pFun;
	for(pFun = &__init_fun_array_start; pFun < &__init_fun_array_end; pFun++)
	{
		(*pFun)();
	}
}

static void prvHW_Init(void)
{
	volatile CommonFun_t *pFun;
	for(pFun = &__preinit_fun_array_start; pFun < &__preinit_fun_array_end; pFun++)
	{
		(*pFun)();
	}
}

int main(void)
{
    cm_backtrace_init_ex("air105", "1.0", "v0001", prvSystemReserCtrl);
	__NVIC_SetPriorityGrouping(7 - __NVIC_PRIO_BITS);//对于freeRTOS必须这样配置
	SystemCoreClockUpdate();
    CoreTick_Init();
    prvMain_InitHeap();
	DMA_GlobalInit();
	Uart_GlobalInit();
	DMA_TakeStream(DMA1_STREAM_1);//for qspi
	DBG_Init(1);
	PM_Init();
	Timer_Init();
	GPIO_GlobalInit(NULL);
	prvHW_Init();
	prvDrv_Init();
	gMainWDTEnable = 1;
	prvWDTTimer = Timer_Create(prvMainWDT, NULL, NULL);
	prvTask_Init();
#ifdef __DEBUG__
	DBG("APP Build debug %s %s!", __DATE__, __TIME__);
#else
	DBG("APP Build release %s %s!", __DATE__, __TIME__);
#endif
	/* 启动调度，开始执行任务 */
	vTaskStartScheduler();
    /* 如果系统正常启动是不会运行到这里的，运行到这里极有可能是空闲任务heap空间不足造成创建失败 */
    while (1)
	{
	}
}


