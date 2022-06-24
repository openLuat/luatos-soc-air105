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

int luat_thread_start(luat_thread_t* thread){
	thread->handle = Task_Create(thread->task_fun, thread->userdata, thread->stack_size, thread->priority, thread->name);
	return 0;
}

int luat_send_event_to_task(void* task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3)
{
	Task_SendEvent(task_handle, id, param1, param2, param3);
	return 0;
}

int luat_wait_event_from_task(void* task_handle, uint32_t wait_event_id, void *out_event, void *call_back, uint32_t ms)
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
