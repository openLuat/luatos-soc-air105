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
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_uart.h"

#include "app_interface.h"


#define LUAT_LOG_TAG "luat.uart"
#include "luat_log.h"

// #define UART_RX_USE_DMA

//串口数量，编号从0开始
#define MAX_DEVICE_COUNT UART_MAX
//存放串口设备句柄
static uint8_t serials_marks[MAX_DEVICE_COUNT+1] ={0};


int luat_uart_exist(int uartid){
    if (uartid < 1 || uartid > MAX_DEVICE_COUNT){
        return 0;
    }
    return 1;
}

static int32_t luat_uart_cb(void *pData, void *pParam){
    rtos_msg_t msg;
    uint32_t uartid = (uint32_t)pData;
    uint32_t State = (uint32_t)pParam;
    uint32_t len;
    // LLOGD("luat_uart_cb pData:%d pParam:%d ",uartid,State);
    switch (State){
        case UART_CB_TX_BUFFER_DONE:
            break;
        case UART_CB_TX_ALL_DONE:
            msg.handler = l_uart_handler;
            msg.arg1 = uartid;
            msg.arg2 = 0;
            msg.ptr = NULL;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_RX_BUFFER_FULL:
            break;
        case UART_CB_RX_TIMEOUT:
            if (serials_marks[uartid]) return 0;
            serials_marks[uartid] = 1;
            len = Uart_RxBufferRead(uartid, NULL, 0);
            msg.handler = l_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = uartid;
            msg.arg2 = len;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_RX_NEW:
            break;
        case DMA_CB_DONE:
            break;
        case UART_CB_ERROR:
            break;
        case DMA_CB_ERROR:
            LLOGD("%x", Uart_GetLastError(uartid));
            break;
	}
}

static int usb_uart_handler(lua_State *L, void* ptr,int arg1) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_pop(L, 1);
    int uart_id = msg->arg1;
    // LLOGD("msg->arg1: %d,msg->arg2: %d",msg->arg1,msg->arg2);
    if (msg->arg2 == 1) {
        lua_getglobal(L, "sys_pub");
        lua_pushstring(L, "USB_UART_INC");
        lua_pushinteger(L, uart_id);
        lua_pushinteger(L, 1);
        lua_call(L, 3, 0);
    }
    else if(msg->arg2 == 2){
        lua_getglobal(L, "sys_pub");
        lua_pushstring(L, "USB_UART_INC");
        lua_pushinteger(L, uart_id);
        lua_pushinteger(L, 0);
        lua_call(L, 3, 0);
    }
    // 给rtos.recv方法返回个空数据
    // lua_pushinteger(L, 0);
    return 0;
}

static int32_t luat_uart_usb_cb(void *pData, void *pParam){
    rtos_msg_t msg = {0};
    uint32_t uartid = (uint32_t)pData;
    uint32_t State = (uint32_t)pParam;
    uint32_t len;
    // LLOGD("luat_uart_cb pData:%d pParam:%d ",uartid,State);
    switch (State){
        case UART_CB_TX_BUFFER_DONE:
            break;
        case UART_CB_TX_ALL_DONE:
            msg.handler = l_uart_handler;
            msg.arg1 = 4;
            msg.arg2 = 0;
            msg.ptr = NULL;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_RX_BUFFER_FULL:
            break;
        case UART_CB_RX_TIMEOUT:
            break;
        case UART_CB_RX_NEW:
            if (serials_marks[4]) return 0;
            serials_marks[4] = 1;
            len = Core_VUartRxBufferRead(VIRTUAL_UART0,NULL,0);
            msg.handler = l_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = 4;
            msg.arg2 = len;
            luat_msgbus_put(&msg, 1);
            break;
        case DMA_CB_DONE:
            break;
        case UART_CB_ERROR:
            // LLOGI("usb disconnect");
            Core_VUartBufferTxStop(VIRTUAL_UART0);
            msg.handler = usb_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = 4;
            msg.arg2 = 2;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_CONNECTED:
            //对方打开串口工具
            msg.handler = usb_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = 4;
            msg.arg2 = 1;
            luat_msgbus_put(&msg, 1);
		    break;
	}
}

int luat_uart_setup(luat_uart_t *uart){
    int parity = 0;
    if (uart->id > MAX_DEVICE_COUNT){
        return -1;
    }
    switch (uart->id){
        case UART_ID0:
            break;
        case UART_ID1:
            GPIO_Iomux(GPIOC_01, 3);
            GPIO_Iomux(GPIOC_00, 3);
            break;
        case UART_ID2:
            GPIO_Iomux(GPIOD_13, 0);
            GPIO_Iomux(GPIOD_12, 0);
            break;
        case UART_ID3:
            GPIO_Iomux(GPIOE_08, 2);
            GPIO_Iomux(GPIOE_09, 2);
            break;
        default:
            break;
    }
    if (uart->parity == 1)parity = UART_PARITY_ODD;
    else if (uart->parity == 2)parity = UART_PARITY_EVEN;
    int stop_bits = (uart->stop_bits)==1?UART_STOP_BIT1:UART_STOP_BIT2;
    if (uart->id >= UART_MAX) {
    	Core_VUartInit(VIRTUAL_UART0, uart->baud_rate, 1, (uart->data_bits), parity, stop_bits, NULL);
    } else {
        Uart_BaseInit(uart->id, uart->baud_rate, 1, (uart->data_bits), parity, stop_bits, NULL);
    }
    return 0;
}


int luat_uart_write(int uartid, void *data, size_t length){
    //printf("uid:%d,data:%s,length = %d\r\n",uartid, (char *)data, length);
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    if (uartid == 4)
        Core_VUartBufferTx(VIRTUAL_UART0, (const uint8_t *)data, length);
    else
        Uart_BufferTx(uartid, (const uint8_t *)data, length);
    return length;
}

int luat_uart_read(int uartid, void *buffer, size_t length){
    int ret = 0;
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    serials_marks[uartid] = 0;
    if (uartid == 4)
        ret = Core_VUartRxBufferRead(VIRTUAL_UART0, (uint8_t *)buffer,length);
    else
        ret = Uart_RxBufferRead(uartid, (uint8_t *)buffer,length);
    // LLOGD("uartid:%d buffer:%s ",uartid,buffer);
    return ret;
}

int luat_uart_close(int uartid){
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    if (uartid == 4)
        Core_VUartDeInit(VIRTUAL_UART0);
    else
        Uart_DeInit(uartid);
    return 0;
}

int luat_setup_cb(int uartid, int received, int sent){
    if (!luat_uart_exist(uartid)){
        return -1;
    }
    if (uartid == 4)
        Core_VUartSetCb(VIRTUAL_UART0, luat_uart_usb_cb);
    else
        Uart_SetCb(uartid, luat_uart_cb);
#ifndef UART_RX_USE_DMA
    Uart_EnableRxIrq(uartid);
#endif
    return 0;
}
