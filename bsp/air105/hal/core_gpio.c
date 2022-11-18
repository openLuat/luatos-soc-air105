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

#include "user.h"
typedef struct
{
	volatile GPIO_TypeDef *RegBase;
	const int32_t IrqLine;
	uint16_t ODBitMap;

}GPIO_ResourceStruct;

typedef struct
{
	CBFuncEx_t CB;
	void *pParam;
}EXTI_CBStruct;

typedef struct
{
	EXTI_CBStruct ExtiCB[GPIO_MAX];
}GPIO_CtrlStruct;

static GPIO_CtrlStruct prvGPIO;

static GPIO_ResourceStruct prvGPIO_Resource[6] =
{
		{
				GPIOA,
				EXTI0_IRQn,
				0,
		},
		{
				GPIOB,
				EXTI1_IRQn,
				0,
		},
		{
				GPIOC,
				EXTI2_IRQn,
				0,
		},
		{
				GPIOD,
				EXTI3_IRQn,
				0,
		},
		{
				GPIOE,
				EXTI4_IRQn,
				0,
		},
		{
				GPIOF,
				EXTI5_IRQn,
				0,
		},
};

static int32_t GPIO_IrqDummyCB(void *pData, void *pParam)
{
//	DBG("%d", pData);
	return 0;
}
static void __FUNC_IN_RAM__ GPIO_IrqHandle(int32_t IrqLine, void *pData)
{
	volatile uint32_t Port = (uint32_t)pData;
	volatile uint32_t Sn, i, Pin;
	CBFuncEx_t CB;
	if (GPIO->INTP_TYPE_STA[Port].INTP_STA)
	{
		Sn = GPIO->INTP_TYPE_STA[Port].INTP_STA;
		GPIO->INTP_TYPE_STA[Port].INTP_STA = 0xffff;
		Port = (Port << 4);

		for(i = 0; i < 16; i++)
		{
			if (Sn & (1 << i))
			{
				Pin = Port+i;
				//DBG("%d,%x,%x", Pin, prvGPIO.ExtiCB[Pin].CB, prvGPIO.ExtiCB[Pin].pParam);
				prvGPIO.ExtiCB[Pin].CB((void *)Pin, prvGPIO.ExtiCB[Pin].pParam);
			}
		}
	}
	ISR_Clear(IrqLine);
}

void GPIO_GlobalInit(CBFuncEx_t Fun)
{
	uint32_t i;
	if (Fun)
	{
		for(i = 0; i < GPIO_MAX; i++)
		{
			prvGPIO.ExtiCB[i].CB = Fun;
		}
	}
	else
	{
		for(i = 0; i < GPIO_MAX; i++)
		{
			prvGPIO.ExtiCB[i].CB = GPIO_IrqDummyCB;
		}
	}
	for(i = 0; i < 6; i++)
	{
		GPIO->INTP_TYPE_STA[i].INTP_TYPE = 0;
		GPIO->INTP_TYPE_STA[i].INTP_STA = 0xffff;
#ifdef __BUILD_OS__
		ISR_SetPriority(prvGPIO_Resource[i].IrqLine, IRQ_MAX_PRIORITY + 1);
#else
		ISR_SetPriority(prvGPIO_Resource[i].IrqLine, 3);
#endif
		ISR_SetHandler(prvGPIO_Resource[i].IrqLine, GPIO_IrqHandle, (void *)i);
		ISR_OnOff(prvGPIO_Resource[i].IrqLine, 1);
	}

}

void __FUNC_IN_RAM__ GPIO_Config(uint32_t Pin, uint8_t IsInput, uint8_t InitValue)
{
	uint8_t Port = (Pin >> 4);
	uint8_t orgPin = Pin;
	Pin = 1 << (Pin & 0x0000000f);
	GPIO_Iomux(orgPin, 1);
	if (IsInput)
	{
		prvGPIO_Resource[Port].RegBase->OEN |= Pin;
	}
	else
	{
		prvGPIO_Resource[Port].RegBase->BSRR |= InitValue?Pin:(Pin << 16);
		prvGPIO_Resource[Port].RegBase->OEN &= ~Pin;
	}
	prvGPIO_Resource[Port].ODBitMap &= ~Pin;
//	DBG("%d, %x, %x, %x",Port, Pin, prvGPIO_Resource[Port].RegBase->OEN,prvGPIO_Resource[Port].RegBase->IODR);
}

void __FUNC_IN_RAM__ GPIO_ODConfig(uint32_t Pin, uint8_t InitValue)
{
	uint8_t Port = (Pin >> 4);
	uint8_t orgPin = Pin;
	Pin = 1 << (Pin & 0x0000000f);
	GPIO_Iomux(orgPin, 1);

	if (InitValue)
	{
		prvGPIO_Resource[Port].RegBase->OEN |= Pin;
		prvGPIO_Resource[Port].RegBase->PUE |= Pin;
	}
	else
	{
		prvGPIO_Resource[Port].RegBase->PUE &= ~Pin;
		prvGPIO_Resource[Port].RegBase->BSRR |= (Pin << 16);
		prvGPIO_Resource[Port].RegBase->OEN &= ~Pin;
	}
	prvGPIO_Resource[Port].ODBitMap |= Pin;
//	DBG("%d, %x, %x, %x",Port, Pin, prvGPIO_Resource[Port].RegBase->OEN, prvGPIO_Resource[Port].RegBase->IODR);
}

void __FUNC_IN_RAM__ GPIO_PullConfig(uint32_t Pin, uint8_t IsPull, uint8_t IsUp)
{
	uint8_t Port = (Pin >> 4);
	Pin = 1 << (Pin & 0x0000000f);
	if (IsPull && IsUp)
	{
		prvGPIO_Resource[Port].RegBase->PUE |= Pin;
	}
	else
	{
		prvGPIO_Resource[Port].RegBase->PUE &= ~Pin;
	}
}

void GPIO_ExtiConfig(uint32_t Pin, uint8_t IsLevel, uint8_t IsRiseHigh, uint8_t IsFallLow)
{
	uint8_t Port = (Pin >> 4);
	uint32_t Type = 0;
	uint32_t Mask = ~(0x03 << ((Pin & 0x0000000f) * 2));
	if (!IsLevel)
	{
		if (IsRiseHigh && IsFallLow)
		{
			Type = 0x03 << ((Pin & 0x0000000f) * 2);
		}
		else if (IsFallLow)
		{
			Type = 0x02 << ((Pin & 0x0000000f) * 2);
		}
		else if (IsRiseHigh)
		{
			Type = 0x01 << ((Pin & 0x0000000f) * 2);
		}
	}
	GPIO->INTP_TYPE_STA[Port].INTP_TYPE = (GPIO->INTP_TYPE_STA[Port].INTP_TYPE & Mask) | Type;
	uint32_t Sn = Pin / 32;
	uint32_t Pos = 1 << (Pin % 32);
	if (!IsLevel && (IsRiseHigh || IsFallLow))
	{
		switch(Sn)
		{
		case 0:
			GPIO->WAKE_P0_EN |= Pos;
			break;
		case 1:
			GPIO->WAKE_P1_EN |= Pos;
			break;
		case 2:
			GPIO->WAKE_P2_EN |= Pos;
			break;
		}
	}
	else
	{
		switch(Sn)
		{
		case 0:
			GPIO->WAKE_P0_EN &= ~Pos;
			break;
		case 1:
			GPIO->WAKE_P1_EN &= ~Pos;
			break;
		case 2:
			GPIO->WAKE_P2_EN &= ~Pos;
			break;
		}
	}
}

void GPIO_ExtiSetCB(uint32_t Pin, CBFuncEx_t CB, void *pParam)
{
	if (CB)
	{
		prvGPIO.ExtiCB[Pin].CB = CB;
	}
	else
	{
		prvGPIO.ExtiCB[Pin].CB = GPIO_IrqDummyCB;
	}
	prvGPIO.ExtiCB[Pin].pParam = pParam;
}

void __FUNC_IN_RAM__ GPIO_Iomux(uint32_t Pin, uint32_t Function)
{
	uint8_t Port = (Pin >> 4);
	uint32_t Mask = ~(0x03 << ((Pin & 0x0000000f) * 2));
	Function = Function << ((Pin & 0x0000000f) * 2);
	GPIO->ALT[Port] = (GPIO->ALT[Port] & Mask) | Function;
}

void __FUNC_IN_RAM__ GPIO_Output(uint32_t Pin, uint8_t Level)
{
	uint8_t Port = (Pin >> 4);
	Pin = 1 << (Pin & 0x0000000f);
	if (prvGPIO_Resource[Port].ODBitMap & Pin)
	{
		if (Level)
		{
			prvGPIO_Resource[Port].RegBase->PUE |= Pin;
			prvGPIO_Resource[Port].RegBase->OEN |= Pin;
		}
		else
		{
			prvGPIO_Resource[Port].RegBase->PUE &= ~Pin;
			prvGPIO_Resource[Port].RegBase->BSRR |= (Pin << 16);
			prvGPIO_Resource[Port].RegBase->OEN &= ~Pin;
		}
	}
	else
	{
		prvGPIO_Resource[Port].RegBase->BSRR |= Level?Pin:(Pin << 16);
	}
//	DBG("%d, %x, %x, %x",Port, Pin, prvGPIO_Resource[Port].RegBase->OEN, prvGPIO_Resource[Port].RegBase->IODR);
//	DBG("%d, %x, %x, %x",Port, Pin, prvGPIO_Resource[Port].RegBase, prvGPIO_Resource[Port].RegBase->IODR);
}

uint8_t __FUNC_IN_RAM__ GPIO_Input(uint32_t Pin)
{
	uint8_t Port = (Pin >> 4);
	Pin = 1 << (Pin & 0x0000000f);
	return (prvGPIO_Resource[Port].RegBase->IODR & (Pin << 16))?1:0;
}

void __FUNC_IN_RAM__ GPIO_Toggle(uint32_t Pin)
{
	uint8_t Port = (Pin >> 4);
	Pin = 1 << (Pin & 0x0000000f);
	prvGPIO_Resource[Port].RegBase->IODR ^= Pin;
}

void __FUNC_IN_RAM__ GPIO_OutputMulti(uint32_t Port, uint32_t Pins, uint32_t Level)
{
	prvGPIO_Resource[Port].RegBase->BSRR |= Level?Pins:(Pins << 16);
}

uint32_t __FUNC_IN_RAM__ GPIO_InputMulti(uint32_t Port)
{
	return (prvGPIO_Resource[Port].RegBase->IODR >> 16);
}

void __FUNC_IN_RAM__ GPIO_ToggleMulti(uint32_t Port, uint32_t Pins)
{
	prvGPIO_Resource[Port].RegBase->IODR ^= (Pins);
}

void __FUNC_IN_RAM__ GPIO_OutPulse(uint32_t Pin, uint8_t *Data, uint16_t BitLen, uint16_t Delay)
{
	volatile int i, j;
	uint8_t table[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	uint8_t Port = (Pin >> 4);
	Pin = 1 << (Pin & 0x0000000f);
	__disable_irq();
	if (Delay)
	{
		for(i = 0;i < BitLen; i++)
		{
			j = Delay;
			prvGPIO_Resource[Port].RegBase->BSRR |= (Data[i >> 3] & (table[i & 0x07]))?Pin:(Pin << 16);
			while(j > 0) {j--;}
		}
	}
	else
	{
		for(i = 0;i < BitLen; i++)
		{
			prvGPIO_Resource[Port].RegBase->BSRR |= (Data[i >> 3] & (table[i & 0x07]))?Pin:(Pin << 16);
		}
	}
	__enable_irq();
}
