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


#include "luat_base.h"
#include "luat_rtos.h"
#include "app_interface.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define LUAT_LOG_TAG "luat.rtos"
#include "luat_log.h"


int luat_rtos_task_create(luat_rtos_task_handle *task_handle, uint32_t stack_size, uint8_t priority, const char *task_name, luat_rtos_task_entry task_fun, void* user_data, uint16_t event_cout)
{
	priority = configMAX_PRIORITIES * 100 / priority;
	if (!priority) priority = 2;
	if (priority >= configMAX_PRIORITIES) priority -= 1;
	*task_handle = Task_Create(task_fun, user_data, stack_size, priority, task_name);
	return (*task_handle)?0:-1;
}
int luat_send_event_to_task(void* task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3)
{
	Task_SendEvent(task_handle, id, param1, param2, param3);
	return 0;
}

int luat_wait_event_from_task(void* task_handle, uint32_t wait_event_id, luat_event_t *out_event, void *call_back, uint32_t ms)
{
	return Task_GetEventByMS(task_handle, wait_event_id, out_event, call_back, ms);
}


void *luat_create_rtos_timer(void *cb, void *param, void *task_handle)
{
	return Timer_Create(cb, param, task_handle);
}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat)
{
	return Timer_StartMS(timer, ms, is_repeat);
}

int luat_rtos_timer_start(void *timer, uint32_t timeout, uint8_t repeat, luat_rtos_timer_callback_t callback_fun, void *user_param)
{
	Timer_Stop(timer);
	Timer_SetCallback(timer, callback_fun, user_param);
	return Timer_StartMS(timer, timeout, repeat);
}

void luat_stop_rtos_timer(void *timer)
{
	Timer_Stop(timer);
}
void luat_release_rtos_timer(void *timer)
{
	Timer_Release(timer);
}

void luat_task_suspend_all(void)
{
	OS_SuspendTask(NULL);
}

void luat_task_resume_all(void)
{
	OS_ResumeTask(NULL);
}


void *luat_mutex_create(void)
{
	return OS_MutexCreateUnlock();
}
LUAT_RET luat_mutex_lock(void *mutex)
{
	OS_MutexLock(mutex);
	return 0;
}
LUAT_RET luat_mutex_unlock(void *mutex)
{
	OS_MutexRelease(mutex);
	return 0;
}
void luat_mutex_release(void *mutex)
{
	OS_MutexDelete(mutex);
}
