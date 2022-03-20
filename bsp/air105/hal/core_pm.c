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
	SENSOR->SEN_EXTS_START = 0x55;
	int i;
	for(i = 0; i < 19; i++)
	{
		SENSOR->SEN_EN[i] = 0x80000055;
	}
	SYSCTRL->MSR_CR1 |= BIT(27);
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
	volatile uint32_t Temp, Temp2, Temp3;
	if (prvPM.HWFlagBit || prvPM.DrvFlagBit) return -ERROR_DEVICE_BUSY;
	__disable_irq();
	SYSCTRL->ANA_CTRL |= BIT(7)|BIT(5)|BIT(4);
	SYSCTRL->LDO25_CR |= BIT(4);
	Temp2 = ADC0->ADC_CR1;
	Temp3 = DAC->DAC_CR1;
	ADC0->ADC_CR1 |= BIT(8);
	ADC0->ADC_CR1 &= ~BIT(6);
	DAC->DAC_CR1 |= BIT(4);
	StartTick = RTC_GetUTC();
	Temp = TRNG->RNG_ANA;
	TRNG->RNG_ANA = Temp | TRNG_RNG_AMA_PD_ALL_Mask;
//	SYSCTRL->PHER_CTRL |= BIT(20);

	__WFI();
//	SYSCTRL->LDO25_CR &= ~BIT(5);
//	SYSCTRL->PHER_CTRL &= ~BIT(20);
	TRNG->RNG_ANA = Temp;
	ADC0->ADC_CR1 = Temp2;
	DAC->DAC_CR1 = Temp3;
	SYSCTRL->LDO25_CR &= ~BIT(4);
    SYSCTRL->ANA_CTRL &= ~(BIT(7)|BIT(5)|BIT(4));
	WDT_Feed();
	SysTickAddSleepTime((RTC_GetUTC() - StartTick) * CORE_TICK_1S);
	Timer_WakeupRun();
	__enable_irq();
	return ERROR_NONE;

}

void PM_Set12MSource(uint8_t XTAL, uint32_t DelayCnt)
{
	volatile uint32_t delay = DelayCnt;
	if (XTAL == ((SYSCTRL->FREQ_SEL & BIT(12)) >> 12))
	{
		__disable_irq();
		if (XTAL)
		{
			SYSCTRL->FREQ_SEL &= ~BIT(12);
		}
		else
		{
			SYSCTRL->FREQ_SEL |= BIT(12);
		}
		while(--delay) {;}
		__enable_irq();
	}
}

void PM_Set32KSource(uint8_t XTAL)
{
	if (XTAL == ((BPU->SEN_ANA0 & BIT(10)) >> 10)) return ;
	if (XTAL)
	{
		BPU->SEN_ANA0 |= BIT(10);
	}
	else
	{
		BPU->SEN_ANA0 &= ~BIT(10);
	}
}
