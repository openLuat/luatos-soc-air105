
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
