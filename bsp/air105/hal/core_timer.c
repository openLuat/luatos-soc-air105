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
//#define TM_DBG DBG_INFO
#define TM_DBG(X, Y...)
extern const uint32_t __os_heap_start;
struct Timer_InfoStruct
{
	llist_head Node;
	uint64_t TargetTick;
	uint64_t AddTick;
	uint32_t ID;
	CBFuncEx_t CallBack;
	void *Param;
	void *Handle;
};

typedef struct
{
	llist_head Head;
	uint64_t NextTick;
	uint32_t IDSn;

}Timer_CtrlStruct;
static Timer_t prvTimer[TIMER_SN_MAX];
static Timer_CtrlStruct prvTimerCtrl;

static void Timer_Update(Timer_t *Timer, uint64_t AddTick, uint8_t IsNew)
{
	if (IsNew)
	{
		Timer->TargetTick = GetSysTick();
	}
	Timer->TargetTick = Timer->TargetTick + AddTick;
}

static void Timer_SetNextIsr(void)
{
	uint32_t Critical = OS_EnterCritical();
	volatile uint32_t clr;
	Timer_t *Timer = (Timer_t *)prvTimerCtrl.Head.next;
	uint64_t NowTick, PassTick;
	if (llist_empty(&prvTimerCtrl.Head))
	{
		TIMM0->TIM[SYS_TIMER_TIM].ControlReg = 0;
		prvTimerCtrl.NextTick = 0;
	}
	else if (prvTimerCtrl.NextTick != Timer->TargetTick)
	{
		TIMM0->TIM[SYS_TIMER_TIM].ControlReg = 0;
		clr = TIMM0->TIM[SYS_TIMER_TIM].EOI;
		NowTick = GetSysTick() + 4;

		if (Timer->TargetTick > NowTick)
		{
			PassTick = Timer->TargetTick - NowTick;
			if (PassTick >= 0xfffffffe)
			{
				PassTick = 0xfffffffe;
			}
		}
		else
		{
			PassTick = 5;
		}
		prvTimerCtrl.NextTick = Timer->TargetTick;
		ISR_Clear(SYS_TIMER_IRQ);
		TIMM0->TIM[SYS_TIMER_TIM].LoadCount = (uint32_t)PassTick - 1;
		TIMM0->TIM[SYS_TIMER_TIM].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
	}
	OS_ExitCritical(Critical);
}

static int32_t Timer_AddNew(void *pData, void *pParam)
{
	Timer_t *OldTimer = (Timer_t *)pData;
	Timer_t *NewTimer = (Timer_t *)pParam;
	if (NewTimer->TargetTick < OldTimer->TargetTick)
	{
		llist_add_tail(&NewTimer->Node, &OldTimer->Node);
		return 1;
	}
	return 0;
}

static int32_t Timer_CheckTo(void *pData, void *pParam)
{
	Timer_t *OldTimer = (Timer_t *)pData;
	if ((OldTimer->TargetTick - SYS_TIMER_1US/4) <= prvTimerCtrl.NextTick)
	{
		return 1;
	}
	return 0;
}

static void SystemTimerIrqHandler( int32_t Line, void *pData)
{
	uint32_t Critical;
	uint64_t NowTick;
	Timer_t *Timer;
	volatile uint32_t clr;
	clr = TIMM0->TIM[SYS_TIMER_TIM].EOI;
	TIMM0->TIM[SYS_TIMER_TIM].ControlReg = 0;
	do {
		Critical = OS_EnterCritical();
		Timer = llist_traversal(&prvTimerCtrl.Head, Timer_CheckTo, NULL);
		if (!Timer)
		{
			OS_ExitCritical(Critical);
			break;
		}
		llist_del(&Timer->Node);
		OS_ExitCritical(Critical);
		if (Timer->CallBack)
		{
			Timer->CallBack(Timer, Timer->Param);
		}
		else if (Timer->Handle)
		{
			Task_SendEvent(Timer->Handle, CORE_TIMER_TIMEOUT, Timer->Param, 0, 0);
		}
		if (Timer->AddTick)
		{
			Critical = OS_EnterCritical();
			Timer_Update(Timer, Timer->AddTick, 0);
			if (llist_empty(&prvTimerCtrl.Head))
			{
				llist_add_tail(&Timer->Node, &prvTimerCtrl.Head);
			}
			else
			{
				if (!llist_traversal(&prvTimerCtrl.Head, Timer_AddNew, Timer))
				{
					llist_add_tail(&Timer->Node, &prvTimerCtrl.Head);
				}
			}
			OS_ExitCritical(Critical);
		}

	}while(Timer);
	Timer_SetNextIsr();


}

static void Timer_Setting(Timer_t *Timer, CBFuncEx_t CB, void *Param, void *NotifyTask)
{
	memset(Timer, 0, sizeof(Timer_t));
	Timer->CallBack = CB;
	Timer->Param = Param;
	Timer->Handle = NotifyTask;
	Timer->Node.next = NULL;
	Timer->Node.prev = NULL;
	Timer->ID = prvTimerCtrl.IDSn;
	prvTimerCtrl.IDSn++;
	TM_DBG("timer %d set", Timer->ID);
}


void Timer_Init(void)
{
	INIT_LLIST_HEAD(&prvTimerCtrl.Head);
	TIMM0->TIM[SYS_TIMER_TIM].ControlReg = 0;
	ISR_OnOff(SYS_TIMER_IRQ, 0);
	ISR_SetHandler(SYS_TIMER_IRQ, SystemTimerIrqHandler);
#ifdef __BUILD_OS__
	ISR_SetPriority(SYS_TIMER_IRQ, IRQ_LOWEST_PRIORITY - 1);
#else
	ISR_SetPriority(SYS_TIMER_IRQ, SYS_TIMER_IRQ_LEVEL);
#endif
	ISR_OnOff(SYS_TIMER_IRQ, 1);
}


void Timer_Task(void *Param)
{

}

void Timer_Run(void)
{
	Timer_Task(NULL);
}


Timer_t * Timer_Create(CBFuncEx_t CB, void *Param, void *NotifyTask)
{
#ifdef __BUILD_OS__
	Timer_t *Timer = OS_Malloc(sizeof(Timer_t));
	if (Timer)
	{
		Timer_Setting(Timer, CB, Param, NotifyTask);
	}
	return Timer;
#else
	return NULL;
#endif
}

Timer_t * Timer_GetStatic(uint32_t Sn, CBFuncEx_t CB, void *Param, void *NotifyTask)
{
	if (Sn < TIMER_SN_MAX)
	{
		Timer_t *Timer = &prvTimer[Sn];
		Timer_Setting(Timer, CB, Param, NotifyTask);
		return Timer;
	}
	else
	{
		return NULL;
	}
}

int Timer_Start(Timer_t *Timer, uint64_t Tick, uint8_t IsRepeat)
{
	uint32_t Critical = OS_EnterCritical();
	llist_del(&Timer->Node);
	OS_ExitCritical(Critical);
	if (IsRepeat)
	{
		Timer->AddTick = Tick;
	}
	else
	{
		Timer->AddTick = 0;
	}
	Timer_Update(Timer, Tick, 1);
	Critical = OS_EnterCritical();
	if (llist_empty(&prvTimerCtrl.Head))
	{
		llist_add_tail(&Timer->Node, &prvTimerCtrl.Head);
	}
	else
	{
		if (!llist_traversal(&prvTimerCtrl.Head, Timer_AddNew, Timer))
		{
			llist_add_tail(&Timer->Node, &prvTimerCtrl.Head);
		}
	}
	OS_ExitCritical(Critical);
	Timer_SetNextIsr();
	return 0;
}

int Timer_StartMS(Timer_t *Timer, uint32_t MS, uint8_t IsRepeat)
{
	uint64_t Tick = MS;
	Tick *= SYS_TIMER_1MS;
	return Timer_Start(Timer, Tick, IsRepeat);
}

int Timer_StartUS(Timer_t *Timer, uint32_t US, uint8_t IsRepeat)
{
	uint64_t Tick = US;
	Tick *= SYS_TIMER_1US;
	return Timer_Start(Timer, Tick, IsRepeat);
}

void Timer_Stop(Timer_t *Timer)
{
	uint32_t Critical = OS_EnterCritical();
	if (prvTimerCtrl.Head.next == &Timer->Node)
	{
		llist_del(&Timer->Node);
		OS_ExitCritical(Critical);
		Timer_SetNextIsr();
		return;
	}
	else
	{
		llist_del(&Timer->Node);
	}
	OS_ExitCritical(Critical);
}

void Timer_Release(Timer_t *Timer)
{
	Timer_Stop(Timer);
	if ((uint32_t)Timer >= (uint32_t)(&__os_heap_start))
	{
		OS_Free(Timer);
	}
}

uint32_t Timer_NextToRest(void)
{
	return TIMM0->TIM[SYS_TIMER_TIM].CurrentValue;
}

uint8_t Timer_IsRunning(Timer_t *Timer)
{
	if (Timer->Node.next && Timer->Node.prev)
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

static int32_t Timer_PrintInfo(void *pData, void *pParam)
{
	Timer_t *NewTimer = (Timer_t *)pData;
	TM_DBG("mem 0x%x, id %u, rest tick %llu", NewTimer, NewTimer->ID, NewTimer->TargetTick - prvTimerCtrl.NextTick);
	return 0;
}

void Timer_PrintAll(void)
{
	uint32_t Critical = OS_EnterCritical();
	llist_traversal(&prvTimerCtrl.Head, Timer_PrintInfo, NULL);
	OS_ExitCritical(Critical);

}
