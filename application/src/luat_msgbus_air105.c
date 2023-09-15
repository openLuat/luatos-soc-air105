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


#include "luat_msgbus.h"

#include "app_interface.h"
#if 0
#define QUEUE_LENGTH 0xFF
#define ITEM_SIZE sizeof(rtos_msg_t)

#if configSUPPORT_STATIC_ALLOCATION
static StaticQueue_t xStaticQueue = {0};
#endif
static QueueHandle_t xQueue = {0};

#if configSUPPORT_STATIC_ALLOCATION
static uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ];
#endif

void luat_msgbus_init(void) {
    if (!xQueue) {
        #if configSUPPORT_STATIC_ALLOCATION
        xQueue = xQueueCreateStatic( QUEUE_LENGTH,
                                 ITEM_SIZE,
                                 ucQueueStorageArea,
                                 &xStaticQueue );
        #else
        xQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
        #endif
    }
}

uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout) {
    if (xQueue == NULL)
        return 1;
    return xQueueSendFromISR(xQueue, msg, NULL) == pdTRUE ? 0 : 1;
}

uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout) {
    if (xQueue == NULL)
        return 1;
    return xQueueReceive(xQueue, msg, timeout) == pdTRUE ? 0 : 1; // 要不要除portTICK_RATE_MS呢?
}

uint32_t luat_msgbus_freesize(void) {
    if (xQueue == NULL)
        return 1;
    return 1;
}
#endif
static HANDLE prvTaskHandle;
void luat_msgbus_init(void) {
	prvTaskHandle = Task_GetCurrent();
}

uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout) {
    Task_SendEvent(prvTaskHandle, msg->handler, (uint32_t)msg->ptr, msg->arg1, msg->arg2);
    return 0;
}

uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout) {
    return Task_GetEventByMS(prvTaskHandle, 0, msg, NULL, timeout);
}

uint32_t luat_msgbus_freesize(void) {
    return 1;
}

uint8_t luat_msgbus_is_empty(void)
{
	return !Task_GetEventCnt(prvTaskHandle);
}
