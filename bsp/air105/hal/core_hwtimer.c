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
//	const I2C_TypeDef *RegBase;
	const int IrqLine;
	OPQueue_CmdStruct *Cmd;
	CBFuncEx_t CmdDoneCB;
	void *pCmdDoneParam;
	uint32_t TotalCount;
	uint32_t TotalRepeat;
	uint32_t CurCount;
	uint32_t RepeatCnt;
	uint32_t CmdQueuePos;
	uint32_t LastRepeatCnt;
	uint32_t LastCount;
	uint8_t IsQueueRunning;
	uint8_t ContinueDelay;
}HWTimer_CtrlStruct;

static HWTimer_CtrlStruct prvHWTimer[HW_TIMER_MAX] = {
		{
//				TIMM0,
				TIM0_0_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
		{
//				TIMM0,
				TIM0_1_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
		{
//				TIMM0,
				TIM0_2_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
		{
//				TIMM0,
				TIM0_3_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
		{
//				TIMM0,
				TIM0_4_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
		{
//				TIMM0,
				TIM0_5_IRQn,
				NULL,
				{
						NULL, 0, 0,
				}
		},
};

static void prvHWTimer_IrqHandlerOperationQueue( int32_t Line, void *pData);
static void prvHWTimer_StartOperationQueue(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin);

static int32_t __FUNC_IN_RAM__ prvHWTimer_DummyCB(void *pData, void *pParam)
{
	return 0;
}

static void prvHWTimer_IrqHandlerEndOperationQueue( int32_t Line, void *pData)
{
	uint32_t HWTimerID = (uint32_t)pData;
	HWTimer_Stop(HWTimerID);
	prvHWTimer[HWTimerID].CmdDoneCB(0, prvHWTimer[HWTimerID].pCmdDoneParam);
}

static int32_t __FUNC_IN_RAM__ prvHWTimer_OperationQueuExti(void *pData, void *pParam)
{
	uint32_t HWTimerID = (uint32_t)pParam;
	prvHWTimer_StartOperationQueue(HWTimerID, &prvHWTimer[HWTimerID], (uint32_t)pData);
	return 0;
}

typedef int (*queue_fun)(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin);
#define OPERATION_GO_ON		(0)
#define OPERATION_OUT_IRQ	(-1)
#define OPERATION_RESET_TIMER	(1)
static int __FUNC_IN_RAM__ prvHWTimer_OperationEnd(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->CurCount = 0;
	HWTimer->RepeatCnt++;
	if (HWTimer->TotalRepeat && (HWTimer->RepeatCnt >= HWTimer->TotalRepeat))
	{
		prvHWTimer[HWTimerID].LastCount = prvHWTimer[HWTimerID].CurCount;
		prvHWTimer[HWTimerID].LastRepeatCnt = prvHWTimer[HWTimerID].RepeatCnt;
		TIMM0->TIM[HWTimerID].ControlReg = 0;
		TIMM0->TIM[HWTimerID].LoadCount = 24;

		ISR_SetHandler(prvHWTimer[HWTimerID].IrqLine, prvHWTimer_IrqHandlerEndOperationQueue, HWTimerID);
#ifdef __BUILD_OS__
		ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, IRQ_LOWEST_PRIORITY - 1);
#else
		ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, 6);
#endif
		TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
		return OPERATION_OUT_IRQ;
	}
	else
	{
		return OPERATION_GO_ON;
	}
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationDelayOnce(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->ContinueDelay = 0;
	return OPERATION_RESET_TIMER;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationDelay(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->ContinueDelay = 1;
	return OPERATION_RESET_TIMER;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationRepeatDelay(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->CurCount++;
	return OPERATION_OUT_IRQ;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationSetGPIOOut(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level);
	GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode > 1)?0:1);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationSetGPIOIn(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode > 1)?0:1);
	GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 1, 0);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationGPIOOut(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	GPIO_Output(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationGPIOIn(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationGPIOInCB(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
	HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, &HWTimer->Cmd[HWTimer->CurCount]);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationCB(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, HWTimer->Cmd[HWTimer->CurCount].uParam.pParam);
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationCaptureSet(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.PullMode > 1)?0:1);
	GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 1, 0);
	TIMM0->TIM[HWTimerID].ControlReg = 0;
	TIMM0->TIM[HWTimerID].LoadCount = HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt;
	TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
	GPIO_ExtiSetCB(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, prvHWTimer_OperationQueuExti, HWTimerID);
	prvHWTimer[HWTimerID].ContinueDelay = 0;
	switch(HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.ExtiMode)
	{
	case OP_QUEUE_CMD_IO_EXTI_BOTH:
		GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 1, 1);
		break;
	case OP_QUEUE_CMD_IO_EXTI_UP:
		GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 1, 0);
		break;
	case OP_QUEUE_CMD_IO_EXTI_DOWN:
		GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 0, 1);
		break;
	}

	HWTimer->CurCount++;
	return OPERATION_OUT_IRQ;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationCapture(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->Cmd[HWTimer->CurCount].PinOrDelay = Pin;
	if (!TIMM0->TIM[HWTimerID].ControlReg)
	{
		HWTimer->Cmd[HWTimer->CurCount + 1].Operation = OP_QUEUE_CMD_CAPTURE_END;
	}
	else
	{
		HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt = TIMM0->TIM[HWTimerID].LoadCount - TIMM0->TIM[HWTimerID].CurrentValue;
		HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
	}
	HWTimer->CurCount++;
	if (OP_QUEUE_CMD_CAPTURE_END != HWTimer->Cmd[HWTimer->CurCount].Operation)
	{
		return OPERATION_OUT_IRQ;
	}
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationCaptureCB(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	HWTimer->Cmd[HWTimer->CurCount].PinOrDelay = Pin;
	if (!TIMM0->TIM[HWTimerID].ControlReg)
	{
		HWTimer->Cmd[HWTimer->CurCount + 1].Operation = OP_QUEUE_CMD_CAPTURE_END;
	}
	else
	{
		HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt = TIMM0->TIM[HWTimerID].LoadCount - TIMM0->TIM[HWTimerID].CurrentValue;
		HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
	}
	HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, &HWTimer->Cmd[HWTimer->CurCount]);
	HWTimer->CurCount++;
	if (OP_QUEUE_CMD_CAPTURE_END != HWTimer->Cmd[HWTimer->CurCount].Operation)
	{
		return OPERATION_OUT_IRQ;
	}
	return OPERATION_GO_ON;
}

static int __FUNC_IN_RAM__ prvHWTimer_OperationCaptureEnd(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	GPIO_ExtiSetCB(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, NULL, NULL);
	GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 0, 0);
	TIMM0->TIM[HWTimerID].ControlReg = 0;
	HWTimer->CurCount++;
	return OPERATION_GO_ON;
}

static queue_fun queueFunlist[] =
{
		prvHWTimer_OperationEnd,
		prvHWTimer_OperationDelayOnce,	//只有一次delay
		prvHWTimer_OperationDelay,	//连续delay，配合OP_QUEUE_CMD_REPEAT_DELAY使用
		prvHWTimer_OperationRepeatDelay,	//重复OP_QUEUE_CMD_CONTINUE_DELAY
		prvHWTimer_OperationSetGPIOOut,
		prvHWTimer_OperationSetGPIOIn,
		prvHWTimer_OperationGPIOOut,
		prvHWTimer_OperationGPIOIn,
		prvHWTimer_OperationGPIOInCB,
		prvHWTimer_OperationCB,
		prvHWTimer_OperationCaptureSet,
		prvHWTimer_OperationCapture,
		prvHWTimer_OperationCaptureCB,
		prvHWTimer_OperationCaptureEnd,
};

static void __FUNC_IN_RAM__ prvHWTimer_StartOperationQueue(uint8_t HWTimerID, HWTimer_CtrlStruct *HWTimer, uint8_t Pin)
{
	volatile uint32_t Period;
	int result;
	while(HWTimer->IsQueueRunning)
	{

//		DBG("%u,%u,%u,%u,%u,%u,%u,%u,%u,%x", HWTimer->TotalRepeat, HWTimer->RepeatCnt,
//				HWTimer->TotalCount, HWTimer->CurCount, HWTimer->Cmd[HWTimer->CurCount].Operation,
//				HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level,
//				HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode, HWTimer->Cmd[HWTimer->CurCount].uArg.Time,
//				HWTimer->Cmd[HWTimer->CurCount].CB);
#if 1
		result = queueFunlist[HWTimer->Cmd[HWTimer->CurCount].Operation](HWTimerID, HWTimer, Pin);
		switch(result)
		{
		case OPERATION_OUT_IRQ:
			return;
		case OPERATION_RESET_TIMER:
			goto START_HWTIMER;
			break;
		}
#else
		switch(HWTimer->Cmd[HWTimer->CurCount].Operation)
		{
		case OP_QUEUE_CMD_GPIO_OUT:
			GPIO_Output(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_GPIO_IN_CB:
			HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
			HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, &HWTimer->Cmd[HWTimer->CurCount]);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_GPIO_IN:
			HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_ONE_TIME_DELAY:
			HWTimer->ContinueDelay = 0;
			goto START_HWTIMER;
			break;
		case OP_QUEUE_CMD_REPEAT_DELAY:
			HWTimer->CurCount++;
			return;
			break;
		case OP_QUEUE_CMD_CAPTURE:
			HWTimer->Cmd[HWTimer->CurCount].PinOrDelay = Pin;
			if (!TIMM0->TIM[HWTimerID].ControlReg)
			{
				HWTimer->Cmd[HWTimer->CurCount + 1].Operation = OP_QUEUE_CMD_CAPTURE_END;
			}
			else
			{
				HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt = TIMM0->TIM[HWTimerID].LoadCount - TIMM0->TIM[HWTimerID].CurrentValue;
				HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
			}
			HWTimer->CurCount++;
			if (OP_QUEUE_CMD_CAPTURE_END != HWTimer->Cmd[HWTimer->CurCount].Operation)
			{
				return;
			}
			break;
		case OP_QUEUE_CMD_CAPTURE_CB:
			HWTimer->Cmd[HWTimer->CurCount].PinOrDelay = Pin;
			if (!TIMM0->TIM[HWTimerID].ControlReg)
			{
				HWTimer->Cmd[HWTimer->CurCount + 1].Operation = OP_QUEUE_CMD_CAPTURE_END;
			}
			else
			{
				HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt = TIMM0->TIM[HWTimerID].LoadCount - TIMM0->TIM[HWTimerID].CurrentValue;
				HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level = GPIO_Input(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay);
			}
			HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, &HWTimer->Cmd[HWTimer->CurCount]);
			HWTimer->CurCount++;
			if (OP_QUEUE_CMD_CAPTURE_END != HWTimer->Cmd[HWTimer->CurCount].Operation)
			{
				return;
			}
			break;
		case OP_QUEUE_CMD_CONTINUE_DELAY:
			HWTimer->ContinueDelay = 1;
			goto START_HWTIMER;
			break;

		case OP_QUEUE_CMD_SET_GPIO_DIR_OUT:
			GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.Level);
			GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode > 1)?0:1);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_SET_GPIO_DIR_IN:
			GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.IOArg.PullMode > 1)?0:1);
			GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 1, 0);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_CB:
			HWTimer->Cmd[HWTimer->CurCount].CB(HWTimerID, HWTimer->Cmd[HWTimer->CurCount].uParam.pParam);
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_CAPTURE_SET:
			GPIO_PullConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.PullMode, (HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.PullMode > 1)?0:1);
			GPIO_Config(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 1, 0);
			TIMM0->TIM[HWTimerID].ControlReg = 0;
			TIMM0->TIM[HWTimerID].LoadCount = HWTimer->Cmd[HWTimer->CurCount].uParam.MaxCnt;
			TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
			GPIO_ExtiSetCB(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, prvHWTimer_OperationQueuExti, HWTimerID);
			prvHWTimer[HWTimerID].ContinueDelay = 0;
			switch(HWTimer->Cmd[HWTimer->CurCount].uArg.ExitArg.ExtiMode)
			{
			case OP_QUEUE_CMD_IO_EXTI_BOTH:
				GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 1, 1);
				break;
			case OP_QUEUE_CMD_IO_EXTI_UP:
				GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 1, 0);
				break;
			case OP_QUEUE_CMD_IO_EXTI_DOWN:
				GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 0, 1);
				break;
			}

			HWTimer->CurCount++;
			return;
			break;
		case OP_QUEUE_CMD_CAPTURE_END:
			GPIO_ExtiSetCB(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, NULL, NULL);
			GPIO_ExtiConfig(HWTimer->Cmd[HWTimer->CurCount].PinOrDelay, 0, 0, 0);
			TIMM0->TIM[HWTimerID].ControlReg = 0;
			HWTimer->CurCount++;
			break;
		case OP_QUEUE_CMD_END:
			HWTimer->CurCount = 0;
			HWTimer->RepeatCnt++;
			if (HWTimer->TotalRepeat && (HWTimer->RepeatCnt >= HWTimer->TotalRepeat))
			{
				TIMM0->TIM[HWTimerID].ControlReg = 0;
				TIMM0->TIM[HWTimerID].LoadCount = 24;

				ISR_SetHandler(prvHWTimer[HWTimerID].IrqLine, prvHWTimer_IrqHandlerEndOperationQueue, HWTimerID);
#ifdef __BUILD_OS__
				ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, IRQ_LOWEST_PRIORITY - 1);
#else
				ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, 6);
#endif
				TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
				return;
			}
			break;
		}
#endif

	}
	return ;
START_HWTIMER:
	TIMM0->TIM[HWTimerID].ControlReg = 0;
	Period = HWTimer->Cmd[HWTimer->CurCount].uArg.Time;
	Period = Period * SYS_TIMER_1US + HWTimer->Cmd[HWTimer->CurCount].PinOrDelay;
	TIMM0->TIM[HWTimerID].LoadCount = Period - 1;
	TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
	HWTimer->CurCount++;
	return ;
}

static void __FUNC_IN_RAM__ prvHWTimer_IrqHandlerPWM( int32_t Line, void *pData)
{
	uint32_t HWTimerID = (uint32_t)pData;
	volatile uint32_t clr;

	clr = TIMM0->TIM[HWTimerID].EOI;
	if (prvHWTimer[HWTimerID].ContinueDelay)
	{
		if ((++prvHWTimer[HWTimerID].CurCount) >= prvHWTimer[HWTimerID].TotalCount)
		{
			TIMM0->TIM[HWTimerID].ControlReg = 0;
			ISR_OnOff(prvHWTimer[HWTimerID].IrqLine, 0);
			ISR_Clear(prvHWTimer[HWTimerID].IrqLine);
			prvHWTimer[HWTimerID].IsQueueRunning = 0;
			prvHWTimer[HWTimerID].TotalCount = 0;
			PM_SetHardwareRunFlag(PM_HW_TIMER_0 + HWTimerID, 0);
		}
		else
		{
			prvHWTimer[HWTimerID].ContinueDelay = 0;
		}
	}
	else
	{
		prvHWTimer[HWTimerID].ContinueDelay = 1;
	}


}

static void __FUNC_IN_RAM__ prvHWTimer_IrqHandlerOperationQueue( int32_t Line, void *pData)
{
	uint32_t HWTimerID = (uint32_t)pData;
	volatile uint32_t clr;

	clr = TIMM0->TIM[HWTimerID].EOI;
	if (!prvHWTimer[HWTimerID].ContinueDelay)
	{
		TIMM0->TIM[HWTimerID].ControlReg = 0;
	}
	prvHWTimer_StartOperationQueue(HWTimerID, &prvHWTimer[HWTimerID], GPIO_NONE);
}

void HWTimer_StartPWM(uint8_t HWTimerID, uint32_t HighCnt, uint32_t LowCnt, uint8_t IsOnePulse)
{
	if (!HighCnt) HighCnt = 1;
	if (!LowCnt) LowCnt = 1;
	TIMM0->TIM[HWTimerID].LoadCount = LowCnt - 1;
	TIMM0->TIM_ReloadCount[HWTimerID] = HighCnt - 1;
	if (IsOnePulse)
	{
		TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_PWM_SINGLE_PULSE|TIMER_CONTROL_REG_TIMER_PWM|TIMER_CONTROL_REG_TIMER_MODE|TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_INTERRUPT;;
	}
	else
	{
		if (prvHWTimer[HWTimerID].TotalCount)
		{
//			DBG("start %u,%u,%u",TIMM0->TIM[HWTimerID].LoadCount, TIMM0->TIM_ReloadCount[HWTimerID], prvHWTimer[HWTimerID].TotalCount);
			ISR_SetHandler(prvHWTimer[HWTimerID].IrqLine, prvHWTimer_IrqHandlerPWM, HWTimerID);
			ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, HWTIMER_IRQ_LEVEL);
			ISR_OnOff(prvHWTimer[HWTimerID].IrqLine, 1);
			TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_PWM|TIMER_CONTROL_REG_TIMER_MODE|TIMER_CONTROL_REG_TIMER_ENABLE;
		}
		else
		{
			TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_PWM|TIMER_CONTROL_REG_TIMER_MODE|TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_INTERRUPT;
		}

	}
	PM_SetHardwareRunFlag(PM_HW_TIMER_0 + HWTimerID, 1);
}

void HWTimer_SetPWM(uint8_t HWTimerID, uint32_t Period, uint16_t Duty, uint32_t PulseNum)
{
	uint32_t TotalCnt;
	uint32_t LowCnt, HighCnt;
	if (Duty >= 999) return;
	if (Period > (SystemCoreClock >> 3)) return;
	switch(Period)
	{
	case 24000000:
		LowCnt = 1;
		HighCnt = 1;
		break;
	case 12000000:
		LowCnt = 2;
		HighCnt = 2;
		break;
	case 6000000:
		LowCnt = 3;
		HighCnt = 3;
		break;
	default:
		TotalCnt = (SystemCoreClock >> 2) / Period;
		HighCnt = (TotalCnt *  Duty) / 1000;
		if ((TotalCnt - HighCnt) < 1)
		{
			LowCnt = 1;
			HighCnt = TotalCnt - LowCnt;
		}
		else
		{
			LowCnt = TotalCnt - HighCnt;
		}
		break;
	}
	prvHWTimer[HWTimerID].TotalCount = PulseNum;
	prvHWTimer[HWTimerID].CurCount = 0;
	prvHWTimer[HWTimerID].ContinueDelay = 0;
	HWTimer_StartPWM(HWTimerID, HighCnt, LowCnt, 0);
//	DBG("L %u, H %u", LowCnt, HighCnt);
//	TIMM0->TIM[HWTimerID].LoadCount = LowCnt - 1;
//	TIMM0->TIM_ReloadCount[HWTimerID] = HighCnt - 1;
//	if (IsOnePulse)
//	{
//		TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_PWM_SINGLE_PULSE|TIMER_CONTROL_REG_TIMER_PWM|TIMER_CONTROL_REG_TIMER_MODE|TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_INTERRUPT;;
//	}
//	else
//	{
//		TIMM0->TIM[HWTimerID].ControlReg = TIMER_CONTROL_REG_TIMER_PWM|TIMER_CONTROL_REG_TIMER_MODE|TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_INTERRUPT;
//	}
}

void HWTimer_Stop(uint8_t HWTimerID)
{
	volatile uint32_t clr;
	ISR_OnOff(prvHWTimer[HWTimerID].IrqLine, 0);
	clr = TIMM0->TIM[HWTimerID].EOI;
	TIMM0->TIM[HWTimerID].ControlReg = 0;
	ISR_Clear(prvHWTimer[HWTimerID].IrqLine);
	prvHWTimer[HWTimerID].IsQueueRunning = 0;
	prvHWTimer[HWTimerID].ContinueDelay = 0;
	PM_SetHardwareRunFlag(PM_HW_TIMER_0 + HWTimerID, 0);
	prvHWTimer[HWTimerID].LastCount = prvHWTimer[HWTimerID].CurCount;
	prvHWTimer[HWTimerID].LastRepeatCnt = prvHWTimer[HWTimerID].RepeatCnt;
}

void HWTimer_GetResultOperationInfo(uint8_t HWTimerID, uint32_t *Repeat, uint32_t *Count)
{
	*Repeat = prvHWTimer[HWTimerID].LastRepeatCnt;
	*Count = prvHWTimer[HWTimerID].LastCount;
}

void HWTimer_InitOperationQueue(uint8_t HWTimerID, uint32_t nCount, uint32_t Repeat, CBFuncEx_t CmdDoneCB, void *pCmdDoneParam)
{
	if (prvHWTimer[HWTimerID].IsQueueRunning)
	{
		HWTimer_Stop(HWTimerID);
		prvHWTimer[HWTimerID].CmdDoneCB(-ERROR_OPERATION_FAILED, prvHWTimer[HWTimerID].pCmdDoneParam);
	}
	prvHWTimer[HWTimerID].TotalCount = nCount;
	prvHWTimer[HWTimerID].TotalRepeat = Repeat;
	if (prvHWTimer[HWTimerID].Cmd)
	{
		free(prvHWTimer[HWTimerID].Cmd);
		prvHWTimer[HWTimerID].Cmd = NULL;
	}
	prvHWTimer[HWTimerID].Cmd = zalloc((nCount + 1) * sizeof(OPQueue_CmdStruct));
	prvHWTimer[HWTimerID].CmdQueuePos = 0;
	if (CmdDoneCB)
	{
		prvHWTimer[HWTimerID].CmdDoneCB = CmdDoneCB;
	}
	else
	{
		prvHWTimer[HWTimerID].CmdDoneCB = prvHWTimer_DummyCB;
	}
	prvHWTimer[HWTimerID].pCmdDoneParam = pCmdDoneParam;
}

void HWTimer_AddOperation(uint8_t HWTimerID, OPQueue_CmdStruct *pCmd)
{

	if (prvHWTimer[HWTimerID].TotalCount > prvHWTimer[HWTimerID].CmdQueuePos)
	{
		memcpy(&prvHWTimer[HWTimerID].Cmd[prvHWTimer[HWTimerID].CmdQueuePos], pCmd, sizeof(OPQueue_CmdStruct));
		prvHWTimer[HWTimerID].CmdQueuePos++;
	}
}


static void HWTimer_ResetOperationQueue(uint8_t HWTimerID)
{
	prvHWTimer[HWTimerID].CurCount = 0;
	prvHWTimer[HWTimerID].RepeatCnt = 0;
}


void HWTimer_StartOperationQueue(uint8_t HWTimerID)
{
	if (prvHWTimer[HWTimerID].IsQueueRunning)
	{
		HWTimer_Stop(HWTimerID);
		prvHWTimer[HWTimerID].CmdDoneCB(-ERROR_OPERATION_FAILED, prvHWTimer[HWTimerID].pCmdDoneParam);
	}
	else
	{
		HWTimer_Stop(HWTimerID);
	}
	ISR_SetHandler(prvHWTimer[HWTimerID].IrqLine, prvHWTimer_IrqHandlerOperationQueue, HWTimerID);
	ISR_SetPriority(prvHWTimer[HWTimerID].IrqLine, HWTIMER_IRQ_LEVEL);

	prvHWTimer[HWTimerID].Cmd[prvHWTimer[HWTimerID].CmdQueuePos].Operation = OP_QUEUE_CMD_END;
	HWTimer_ResetOperationQueue(HWTimerID);
	prvHWTimer[HWTimerID].IsQueueRunning = 1;
	prvHWTimer_StartOperationQueue(HWTimerID, &prvHWTimer[HWTimerID], GPIO_NONE);
	ISR_OnOff(prvHWTimer[HWTimerID].IrqLine, 1);
	PM_SetHardwareRunFlag(PM_HW_TIMER_0 + HWTimerID, 1);
}

void HWTimer_ClearOperationQueue(uint8_t HWTimerID)
{
//	HWTimer_ResetOperationQueue(HWTimerID);
	prvHWTimer[HWTimerID].CmdQueuePos = 0;
}


void HWTimer_FreeOperationQueue(uint8_t HWTimerID)
{
	free(prvHWTimer[HWTimerID].Cmd);
	prvHWTimer[HWTimerID].Cmd = NULL;
	prvHWTimer[HWTimerID].CmdDoneCB = prvHWTimer_DummyCB;
}

void HWTimer_AddEndCmdInOperationQueue(uint8_t HWTimerID)
{
	HWTimer_CtrlStruct *HWTimer = &prvHWTimer[HWTimerID];
	if (HWTimer->Cmd[HWTimer->CurCount].Operation != OP_QUEUE_CMD_END)
	{
		HWTimer->Cmd[HWTimer->CurCount + 1].Operation = OP_QUEUE_CMD_END;
	}
}

uint8_t HWTimer_CheckOperationQueueDone(uint8_t HWTimerID)
{
	return !prvHWTimer[HWTimerID].IsQueueRunning;
}

uint32_t HWTimer_GetOperationQueueCaptureResult(uint8_t HWTimerID, CBFuncEx_t CB, void *pParam)
{
	uint32_t i = 0;
	uint32_t Cnt = 0;
	if (!prvHWTimer[HWTimerID].Cmd) return 0;
	for(i = 0; i < prvHWTimer[HWTimerID].CmdQueuePos; i++)
	{
		if (OP_QUEUE_CMD_CAPTURE == prvHWTimer[HWTimerID].Cmd[i].Operation)
		{
			CB(&prvHWTimer[HWTimerID].Cmd[i], pParam);
			Cnt++;
		}
	}
	return Cnt;
}

uint32_t HWTimer_GetOperationQueueLen(uint8_t HWTimerID)
{
	if (!prvHWTimer[HWTimerID].Cmd) return 0;
	return prvHWTimer[HWTimerID].CmdQueuePos;
}

OPQueue_CmdStruct *HWTimer_GetOperationQueue(uint8_t HWTimerID)
{
	return 	prvHWTimer[HWTimerID].Cmd;
}

int __FUNC_IN_RAM__ HWTimer_GetIrqLine(uint8_t HWTimerID)
{
	return prvHWTimer[HWTimerID].IrqLine;
}

uint8_t __FUNC_IN_RAM__ HWTimer_GetIDFromIrqline(int Irqline)
{
	if (Irqline > TIM0_3_IRQn)
	{
		return (Irqline - TIM0_4_IRQn) + 4;
	}
	else
	{
		return (Irqline - TIM0_0_IRQn);
	}
}
