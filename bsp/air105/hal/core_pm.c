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
	SYSCTRL->MSR_CR1 |= BIT(27);
	SYSCTRL->LDO25_CR |= BIT(4);
	SYSCTRL->ANA_CTRL |= BIT(7)|BIT(4)|BIT(5);
	SENSOR->SEN_EXTS_START = 0x55;
	int i;
	for(i = 0; i < 19; i++)
	{
		SENSOR->SEN_EN[i] = 0x80000055;
	}
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

int32_t PM_Sleep(void)
{
	uint64_t StartTick;
	uint32_t Temp, Temp2, Temp3, Temp4;
	if (prvPM.HWFlagBit || prvPM.DrvFlagBit) return -ERROR_DEVICE_BUSY;
	__disable_irq();
	Temp2 = ADC0->ADC_CR1;
	Temp3 = DAC->DAC_CR1;
	Temp4 = ADC0->ADC_CR2;
	ADC0->ADC_CR1 |= BIT(8);
	ADC0->ADC_CR1 &= ~BIT(6);
	DAC->DAC_CR1 |= (1 << 4);
	ADC_IntelResistance(0);
	StartTick = RTC_GetUTC();
	Temp = TRNG->RNG_ANA;
	TRNG->RNG_ANA = Temp | TRNG_RNG_AMA_PD_ALL_Mask;
//	SYSCTRL->PHER_CTRL |= BIT(20);
//	SYSCTRL->LDO25_CR |= BIT(5);
	__WFI();
//	SYSCTRL->LDO25_CR &= ~BIT(5);
//	SYSCTRL->PHER_CTRL &= ~BIT(20);
	TRNG->RNG_ANA = Temp;
	ADC0->ADC_CR1 = Temp2;
	DAC->DAC_CR1 = Temp3;
	ADC0->ADC_CR2 = Temp4;
	WDT_Feed();
	SysTickAddSleepTime((RTC_GetUTC() - StartTick) * CORE_TICK_1S);
	Timer_WakeupRun();
	__enable_irq();
	return ERROR_NONE;

}
