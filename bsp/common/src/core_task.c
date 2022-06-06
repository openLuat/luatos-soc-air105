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
enum
{
	TASK_POINT_LIST_HEAD,
#ifdef __USE_SEMAPHORE__
	TASK_POINT_EVENT_SEM,
	TASK_POINT_DELAY_SEM,
#endif
	TASK_POINT_EVENT_TIMER,
	TASK_POINT_DELAY_TIMER,
	TASK_POINT_DEBUG,
#ifdef __USE_FREERTOS_NOTIFY__
	TASK_NOTIFY_EVENT = tskDEFAULT_INDEX_TO_NOTIFY,
	TASK_NOTIFY_DELAY,
#endif
};

typedef struct
{
	llist_head Node;
	uint32_t ID;
	uint32_t Param1;
	uint32_t Param2;
	uint32_t Param3;
}Core_EventStruct;

extern void * vTaskGetPoint(TaskHandle_t xHandle, uint8_t Sn);
extern void vTaskSetPoint(TaskHandle_t xHandle, uint8_t Sn, void * p);
extern void *vTaskGetCurrent(void);
extern char * vTaskInfo(void *xHandle, uint32_t *StackTopAddress, uint32_t *StackStartAddress, uint32_t *Len);
extern void vTaskModifyPoint(TaskHandle_t xHandle, uint8_t Sn, int Add);

static int32_t prvTaskTimerCallback(void *pData, void *pParam)
{
	Task_SendEvent(pParam, CORE_EVENT_TIMEOUT, 0, 0, 0);
	return 0;
}

static int32_t prvTaskDelayTimerCallback(void *pData, void *pParam)
{
#ifdef __USE_SEMAPHORE__
	SemaphoreHandle_t sem = (SemaphoreHandle_t)vTaskGetPoint(pParam, TASK_POINT_DELAY_SEM);
	OS_MutexRelease(sem);
#endif
#ifdef __USE_FREERTOS_NOTIFY__
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskGenericNotifyGiveFromISR(pParam, TASK_NOTIFY_DELAY, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken)
	{
		portYIELD_WITHIN_API();
	}
#endif
	return 0;
}

HANDLE Task_Create(TaskFun_t EntryFunction, void *Param, uint32_t StackSize, uint32_t Priority, const char *Name)
{
	TaskHandle_t Handle;
	llist_head *Head;
	Timer_t *Timer, *DelayTimer;
#ifdef __USE_SEMAPHORE__
	SemaphoreHandle_t Sem, DelaySem;
#endif
	if (pdPASS != xTaskCreate(EntryFunction, Name, StackSize>>2, Param, Priority, &Handle))
	{
		return NULL;
	}
	if (!Handle) return NULL;
	Head = OS_Zalloc(sizeof(llist_head));
#ifdef __USE_SEMAPHORE__
	Sem = OS_MutexCreate();
	DelaySem = OS_MutexCreate();
#endif
	Timer = Timer_Create(prvTaskTimerCallback, Handle, NULL);
	DelayTimer = Timer_Create(prvTaskDelayTimerCallback, Handle, NULL);
	INIT_LLIST_HEAD(Head);
	vTaskSetPoint(Handle, TASK_POINT_LIST_HEAD, Head);
#ifdef __USE_SEMAPHORE__
	vTaskSetPoint(Handle, TASK_POINT_EVENT_SEM, Sem);
	vTaskSetPoint(Handle, TASK_POINT_DELAY_SEM, DelaySem);
#endif
	vTaskSetPoint(Handle, TASK_POINT_DELAY_TIMER, DelayTimer);
	vTaskSetPoint(Handle, TASK_POINT_EVENT_TIMER, Timer);
	vTaskSetPoint(Handle, TASK_POINT_DEBUG, 0);
//	DBG("%s, %x", Name, Handle);
	return Handle;
}

void Task_Exit(void)
{
	vTaskDelete(NULL);
}

void Task_SendEvent(HANDLE TaskHandle, uint32_t ID, uint32_t P1, uint32_t P2, uint32_t P3)
{
	if (!TaskHandle) return;
	uint32_t Critical;
	volatile llist_head *Head = (llist_head *)vTaskGetPoint(TaskHandle, TASK_POINT_LIST_HEAD);
#ifdef __USE_SEMAPHORE__
	SemaphoreHandle_t sem = (SemaphoreHandle_t)vTaskGetPoint(TaskHandle, TASK_POINT_EVENT_SEM);
#endif
	Core_EventStruct *Event = OS_Zalloc(sizeof(Core_EventStruct));
	Event->ID = ID;
	Event->Param1 = P1;
	Event->Param2 = P2;
	Event->Param3 = P3;

	Critical = OS_EnterCritical();
	vTaskModifyPoint(TaskHandle, TASK_POINT_DEBUG, 1);
	llist_add_tail(&Event->Node, Head);
	if (vTaskGetPoint(TaskHandle, TASK_POINT_DEBUG) >= 1024)
	{
		DBG_Trace("%s wait too much msg! %u", vTaskInfo(TaskHandle, &ID, &P1, &P2), vTaskGetPoint(TaskHandle, TASK_POINT_DEBUG));
		Core_PrintServiceStack();
		__disable_irq();
		while(1) {;}
	}
	OS_ExitCritical(Critical);
#ifdef __USE_SEMAPHORE__
	OS_MutexRelease(sem);
#endif
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
	llist_head *Head = (llist_head *)vTaskGetPoint(TaskHandle, TASK_POINT_LIST_HEAD);
#ifdef __USE_SEMAPHORE__
	SemaphoreHandle_t sem = (SemaphoreHandle_t)vTaskGetPoint(TaskHandle, TASK_POINT_EVENT_SEM);
#endif
	Timer_t *Timer = (Timer_t *)vTaskGetPoint(TaskHandle, TASK_POINT_EVENT_TIMER);
	if (Tick)
	{
		ToTick = GetSysTick() + Tick;
	}
GET_NEW_EVENT:
	Critical = OS_EnterCritical();

	if (!llist_empty(Head))
	{
		Event = (Core_EventStruct *)Head->next;
		llist_del(&Event->Node);
		if (vTaskGetPoint(TaskHandle, TASK_POINT_DEBUG) > 0)
		{
			vTaskModifyPoint(TaskHandle, TASK_POINT_DEBUG, -1);
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
		OS_Free(Event);
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
			Timer_Start(Timer, ToTick - GetSysTick(), 0);

		}
		else
		{
			Result = -ERROR_TIMEOUT;
			goto GET_EVENT_DONE;
		}
	}
#ifdef __USE_SEMAPHORE__
	OS_MutexLock(sem);
#endif
#ifdef __USE_FREERTOS_NOTIFY__
	xTaskGenericNotifyWait(TASK_NOTIFY_EVENT, 0, 0, NULL, portMAX_DELAY);
#endif
	goto GET_NEW_EVENT;
GET_EVENT_DONE:
	Timer_Stop(Timer);
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
#ifdef __USE_SEMAPHORE__
	SemaphoreHandle_t sem = (SemaphoreHandle_t)vTaskGetPoint(TaskHandle, TASK_POINT_DELAY_SEM);
#endif
	Timer_t *Timer = (Timer_t *)vTaskGetPoint(TaskHandle, TASK_POINT_DELAY_TIMER);
	Timer_Start(Timer, Tick, 0);
#ifdef __USE_SEMAPHORE__
	OS_MutexLock(sem);
#endif
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
	volatile llist_head *Head = (llist_head *)vTaskGetPoint(TaskHandle, TASK_POINT_LIST_HEAD);
	DBG("%x,%x,%x", Head, Head->next, Head->prev);
}
#else
void Task_SendEvent(HANDLE TaskHandle, uint32_t ID, uint32_t P1, uint32_t P2, uint32_t P3)
{

}
#endif
