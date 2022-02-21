#include "user.h"

typedef struct
{
	uint32_t HWFlagBit;
	uint32_t DrvFlagBit;
}PM_CtrlStruct;
static PM_CtrlStruct prvPM;

void PM_Init(void)
{
	GPIO->WAKE_TYPE_EN = (1 << 12) | (1 << 11);
//	GPIO->WAKE_P0_EN = 0xffffffff;
//	GPIO->WAKE_P1_EN = 0xffffffff;
//	GPIO->WAKE_P2_EN = 0xffffffff;
	GPIO->WAKE_P0_EN = 0;
	GPIO->WAKE_P1_EN = 0;
	GPIO->WAKE_P2_EN = 0;
	SYSCTRL->ANA_CTRL |= 0x00f0;
}

void PM_SetHardwareRunFlag(uint32_t PmHWSn, uint32_t OnOff)
{
	if (OnOff)
	{
		prvPM.HWFlagBit |= (1 << PmHWSn);
	}
	else
	{
		prvPM.HWFlagBit &= ~(1 << PmHWSn);
	}
}

void PM_SetDriverRunFlag(uint32_t PmDrvSn, uint32_t OnOff)
{
	if (OnOff)
	{
		prvPM.DrvFlagBit |= (1 << PmDrvSn);
	}
	else
	{
		prvPM.DrvFlagBit &= ~(1 << PmDrvSn);
	}
}

void PM_Print(void)
{
	DBG("%x,%x", prvPM.HWFlagBit, prvPM.DrvFlagBit);
}

int32_t __FUNC_IN_RAM__ PM_Sleep(void)
{
	uint64_t StartTick;
	uint32_t Temp;
	if (prvPM.HWFlagBit || prvPM.DrvFlagBit) return -ERROR_DEVICE_BUSY;
	__disable_irq();
	StartTick = RTC_GetUTC();
	SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_KBD|SYSCTRL_APBPeriph_GPIO;
	SYSCTRL->FREQ_SEL = (SYSCTRL->FREQ_SEL & ~(SYSCTRL_FREQ_SEL_POWERMODE_Mask)) | SYSCTRL_FREQ_SEL_POWERMODE_CLOSE_CPU_MEM;
	Temp = TRNG->RNG_ANA;
	TRNG->RNG_ANA = Temp | TRNG_RNG_AMA_PD_ALL_Mask;
	__WFI();
	TRNG->RNG_ANA = Temp;
	SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_ALL;
	WDT_Feed();
	SysTickAddSleepTime((RTC_GetUTC() - StartTick) * CORE_TICK_1S);
	Timer_WakeupRun();
	__enable_irq();
	return ERROR_NONE;

}
