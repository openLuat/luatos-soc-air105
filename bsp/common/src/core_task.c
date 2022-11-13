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
#ifdef __BUILD_OS__
#define __USE_FREERTOS_NOTIFY__
#define __USE_CORE_TIMER__
enum
{
	TASK_NOTIFY_EVENT = tskDEFAULT_INDEX_TO_NOTIFY,
	TASK_NOTIFY_DELAY,
};

typedef struct
{
	StaticTask_t TCB;
	uint8_t dummy[1];	//不加这个就会死机
	volatile llist_head EventHead;
#ifdef __USE_CORE_TIMER__
	Timer_t *EventTimer;
	Timer_t *DelayTimer;
#endif
	uint16_t EventCnt;
	uint8_t isRun;
}Core_TaskStruct;

typedef struct
{
	llist_head Node;
	uint32_t ID;
	uint32_t Param1;
	uint32_t Param2;
	uint32_t Param3;
}Core_EventStruct;


extern void *vTaskGetCurrent(void);
extern char * vTaskInfo(void *xHandle, uint32_t *StackTopAddress, uint32_t *StackStartAddress, uint32_t *Len);


static int32_t prvTaskTimerCallback(void *pData, void *pParam)
{
	Task_SendEvent(pParam, CORE_EVENT_TIMEOUT, 0, 0, 0);
	return 0;
}

#ifdef __USE_CORE_TIMER__
static int32_t prvTaskDelayTimerCallback(void *pData, void *pParam)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskGenericNotifyGiveFromISR(pParam, TASK_NOTIFY_DELAY, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken)
	{
		portYIELD_WITHIN_API();
	}
	return 0;
}
#endif
HANDLE Task_Create(TaskFun_t EntryFunction, void *Param, uint32_t StackSize, uint32_t Priority, const char *Name)
{
	StackSize = (StackSize + 3) >> 2;
	uint32_t *stack_mem = zalloc(StackSize * 4);
	Core_TaskStruct *Handle = zalloc(sizeof(Core_TaskStruct));
	if (!Handle || !stack_mem)
	{
		free(Handle);
		free(stack_mem);
		return NULL;
	}
#ifdef __USE_CORE_TIMER__
	Handle->EventTimer = Timer_Create(prvTaskTimerCallback, Handle, NULL);
	Handle->DelayTimer = Timer_Create(prvTaskDelayTimerCallback, Handle, NULL);
#endif
	if (!xTaskCreateStatic(EntryFunction, Name, StackSize, Param, Priority, stack_mem, &Handle->TCB))
	{
		Timer_Release(Handle->EventTimer);
		Timer_Release(Handle->DelayTimer);
		free(Handle);
		free(stack_mem);
		return NULL;
	}

	Handle->TCB.uxDummy20 = 0;
	INIT_LLIST_HEAD(&Handle->EventHead);
	Handle->isRun = 1;
	return Handle;
}

static int32_t DeleteTaskEvent(void *data, void *param)
{
	return LIST_DEL;
}

void Task_Exit(HANDLE TaskHandle)
{
	if (!TaskHandle)
	{
		TaskHandle = vTaskGetCurrent();
	}
	Core_TaskStruct *Handle = (Core_TaskStruct *)TaskHandle;
	uint32_t cr;
	cr = OS_EnterCritical();
	llist_traversal(&Handle->EventHead, DeleteTaskEvent, NULL);
	Handle->isRun = 0;
#ifdef __USE_CORE_TIMER__
	Timer_Release(Handle->EventTimer);
	Timer_Release(Handle->DelayTimer);
#endif
	OS_ExitCritical(cr);
	vTaskDelete(&Handle->TCB);

}

void Task_SendEvent(HANDLE TaskHandle, uint32_t ID, uint32_t P1, uint32_t P2, uint32_t P3)
{
	if (!TaskHandle) return;
	uint32_t Critical;

	Core_TaskStruct *Handle = (void *)TaskHandle;
	volatile Core_EventStruct *Event = zalloc(sizeof(Core_EventStruct));
	volatile Core_EventStruct *Event2;
	Event->ID = ID;
	Event->Param1 = P1;
	Event->Param2 = P2;
	Event->Param3 = P3;

	Critical = OS_EnterCritical();
	Handle->EventCnt++;
	Event2 = Handle->EventHead.next;
	llist_add_tail(&Event->Node, &(Handle->EventHead));
	Event2 = Handle->EventHead.next;
	if (Handle->EventCnt >= 1024)
	{
		DBG_Trace("%s wait too much msg! %u", vTaskInfo(TaskHandle, &ID, &P1, &P2), Handle->EventCnt);
		Core_PrintServiceStack();
		__disable_irq();
		while(1) {;}
	}
	OS_ExitCritical(Critical);
#ifdef __USE_FREERTOS_NOTIFY__
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (OS_CheckInIrq())
	{
		vTaskGenericNotifyGiveFromISR(TaskHandle, TASK_NOTIFY_EVENT, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken)
		{
			portYIELD_WITHIN_API();
		}
	}
	else
	{
		xTaskGenericNotify(TaskHandle, TASK_NOTIFY_EVENT, ( 0 ), eIncrement, NULL );
	}

#endif
}

int32_t Task_GetEvent(HANDLE TaskHandle, uint32_t TargetEventID, OS_EVENT *OutEvent, CBFuncEx_t Callback, uint64_t Tick)
{
	uint64_t ToTick = 0;
	Core_EventStruct *Event;
	uint32_t Critical;
	int32_t Result = ERROR_NONE;
	if (OS_CheckInIrq())
	{
		return -ERROR_PERMISSION_DENIED;
	}
	if (!TaskHandle) return -ERROR_PARAM_INVALID;
	Core_TaskStruct *Handle = (void *)TaskHandle;
	if (vTaskGetCurrent() != TaskHandle)
	{
		return -1;
	}
	if (Tick)
	{
		ToTick = GetSysTick() + Tick;
	}
GET_NEW_EVENT:
	Critical = OS_EnterCritical();
	if (!llist_empty(&Handle->EventHead))
	{

		Event = (Core_EventStruct *)Handle->EventHead.next;
		llist_del(&Event->Node);
		if (Handle->EventCnt > 0)
		{
			Handle->EventCnt--;
		}

	}
	else
	{
		Event = NULL;
	}

	OS_ExitCritical(Critical);

	if (Event)
	{

		OutEvent->ID = Event->ID;
		OutEvent->Param1 = Event->Param1;
		OutEvent->Param2 = Event->Param2;
		OutEvent->Param3 = Event->Param3;
		free(Event);
	}
	else
	{
		if (!ToTick)
		{
			goto WAIT_NEW_EVENT;
		}
		else
		{
			if (ToTick > (5 * SYS_TIMER_1US + GetSysTick()))
			{
				goto WAIT_NEW_EVENT;

			}
			else
			{
				Result = -ERROR_TIMEOUT;
				goto GET_EVENT_DONE;
			}

		}
	}
	switch(OutEvent->ID)
	{
	case CORE_EVENT_TIMEOUT:
		goto GET_NEW_EVENT;
	}
	if ((TargetEventID == CORE_EVENT_ID_ANY) || (OutEvent->ID == TargetEventID))
	{
		goto GET_EVENT_DONE;
	}
	if (Callback)
	{
		Callback(OutEvent, TaskHandle);
	}
	goto GET_NEW_EVENT;
WAIT_NEW_EVENT:
	if (ToTick)
	{
		if (ToTick > (5 * SYS_TIMER_1US + GetSysTick()))
		{
			Timer_Start(Handle->EventTimer, ToTick - GetSysTick(), 0);

		}
		else
		{
			Result = -ERROR_TIMEOUT;
			goto GET_EVENT_DONE;
		}
	}
#ifdef __USE_FREERTOS_NOTIFY__
	xTaskGenericNotifyWait(TASK_NOTIFY_EVENT, 0, 0, NULL, portMAX_DELAY);
#endif
	goto GET_NEW_EVENT;
GET_EVENT_DONE:
	Timer_Stop(Handle->EventTimer);
	return Result;
}

int32_t Task_GetEventByMS(HANDLE TaskHandle, uint32_t TargetEventID, OS_EVENT *OutEvent, CBFuncEx_t Callback, uint32_t ms)
{
	uint64_t ToTick = ms;
	ToTick *= SYS_TIMER_1MS;
	if (ms != 0xffffffff)
	{
		return Task_GetEvent(TaskHandle, TargetEventID, OutEvent, Callback, ToTick);
	}
	else
	{
		return Task_GetEvent(TaskHandle, TargetEventID, OutEvent, Callback, 0);
	}
}

HANDLE Task_GetCurrent(void)
{
	return vTaskGetCurrent();
}

void Task_DelayTick(uint64_t Tick)
{
	if (OS_CheckInIrq() || Tick < (SYS_TIMER_1US*5))
	{
		SysTickDelay((uint32_t)Tick);
		return;
	}
	HANDLE TaskHandle = vTaskGetCurrent();
	Core_TaskStruct *Handle = (void *)TaskHandle;
	Timer_Start(Handle->DelayTimer, Tick, 0);
#ifdef __USE_FREERTOS_NOTIFY__
	xTaskGenericNotifyWait(TASK_NOTIFY_DELAY, 0, 0, NULL, portMAX_DELAY);
#endif
}

void Task_DelayUS(uint32_t US)
{
	uint64_t ToTick = US;
	ToTick *= SYS_TIMER_1US;
	Task_DelayTick(ToTick);
}

void Task_DelayMS(uint32_t MS)
{
	uint64_t ToTick = MS;
	ToTick *= SYS_TIMER_1MS;
	Task_DelayTick(ToTick);
}

void Task_Debug(HANDLE TaskHandle)
{
	if (!TaskHandle)
	{
		TaskHandle = vTaskGetCurrent();
	}
	Core_TaskStruct *Handle = (void *)TaskHandle;
	DBG("%x,%x,%x", &Handle->EventHead, Handle->EventHead.next, Handle->EventHead.prev);
}


static StaticTask_t prvIdleTCB;
static StaticTask_t prvTimerTaskTCB;
StackType_t  prvIdleTaskStack[256];
StackType_t  prvTimerTaskStack[256];
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                           StackType_t ** ppxIdleTaskStackBuffer,
                                           uint32_t * pulIdleTaskStackSize ) /*lint !e526 Symbol not defined as it is an application callback. */
{
	*ppxIdleTaskTCBBuffer = &prvIdleTCB;
	*ppxIdleTaskStackBuffer = prvIdleTaskStack;
	*pulIdleTaskStackSize = 256;
}
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                      StackType_t ** ppxTimerTaskStackBuffer,
                                          uint32_t * pulTimerTaskStackSize )
{
	*ppxTimerTaskTCBBuffer = &prvTimerTaskTCB;
	*ppxTimerTaskStackBuffer = prvTimerTaskStack;
	*pulTimerTaskStackSize = 256;
}


#else
void Task_SendEvent(HANDLE TaskHandle, uint32_t ID, uint32_t P1, uint32_t P2, uint32_t P3)
{

}
#endif
