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
    Task_Create(thread->thread, NULL, thread->stack_size, thread->priority, thread->name);
    return 0;
}

int luat_sem_create(luat_sem_t* semaphore){
    semaphore->userdata = xSemaphoreCreateBinary();
    return 0;
}
int luat_sem_delete(luat_sem_t* semaphore){
    vSemaphoreDelete(semaphore->userdata);
    return 0;
}

int luat_sem_take(luat_sem_t* semaphore,uint32_t timeout){
    return xSemaphoreTake(semaphore->userdata, timeout);
}

int luat_sem_release(luat_sem_t* semaphore){
    OS_MutexRelease(semaphore->userdata);
    return 0;
}
