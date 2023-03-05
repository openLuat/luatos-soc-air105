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
#include "app_interface.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_uart.h"




#define LUAT_LOG_TAG "luat.uart"
#include "luat_log.h"

// #define UART_RX_USE_DMA

//串口数量，编号从0开始
#define MAX_DEVICE_COUNT UART_MAX
//存放串口设备句柄
typedef struct
{
	timer_t *rs485_timer;
	union
	{
		uint32_t rs485_param;
		struct
		{
			uint32_t wait_time:30;
			uint32_t rx_level:1;
			uint32_t is_485used:1;
		}rs485_param_bit;
	};
	uint16_t unused;
	uint8_t rx_mark;
	uint8_t rs485_pin;
}serials_info;
static serials_info serials[MAX_DEVICE_COUNT+1] ={0};

static int32_t luat_uart_wait_timer_cb(void *pData, void *pParam)
{
    uint32_t uartid = (uint32_t)pParam;
    rtos_msg_t msg;
    msg.handler = l_uart_handler;
    msg.arg1 = uartid;
    msg.arg2 = 0;
    msg.ptr = NULL;
    luat_msgbus_put(&msg, 1);
    if (serials[uartid].rs485_param_bit.is_485used) {
    	GPIO_Output(serials[uartid].rs485_pin, serials[uartid].rs485_param_bit.rx_level);
    }
}

int luat_uart_exist(int uartid){
	if (uartid >= LUAT_VUART_ID_0) uartid = MAX_DEVICE_COUNT;
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
//    LLOGD("luat_uart_cb pData:%d pParam:%d ",uartid,State);
    switch (State){
        case UART_CB_TX_BUFFER_DONE:
            break;
        case UART_CB_TX_ALL_DONE:
        	if (serials[uartid].rs485_param_bit.is_485used && serials[uartid].rs485_param_bit.wait_time)
        	{
        		Timer_StartUS(serials[uartid].rs485_timer, serials[uartid].rs485_param_bit.wait_time, 0);
        	}
        	else
        	{
                msg.handler = l_uart_handler;
                msg.arg1 = uartid;
                msg.arg2 = 0;
                msg.ptr = NULL;
                luat_msgbus_put(&msg, 1);
                if (serials[uartid].rs485_param_bit.is_485used && Uart_IsTSREmpty(uartid)) {
                	GPIO_Output(serials[uartid].rs485_pin, serials[uartid].rs485_param_bit.rx_level);
                }
        	}
            break;
        case UART_CB_RX_BUFFER_FULL:
        case UART_CB_RX_TIMEOUT:
            if (serials[uartid].rx_mark) return 0;
            serials[uartid].rx_mark = 1;
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
            msg.arg1 = LUAT_VUART_ID_0;
            msg.arg2 = 0;
            msg.ptr = NULL;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_RX_BUFFER_FULL:
            break;
        case UART_CB_RX_TIMEOUT:
            break;
        case UART_CB_RX_NEW:
            if (serials[MAX_DEVICE_COUNT].rx_mark) return 0;
            serials[MAX_DEVICE_COUNT].rx_mark = 1;
            len = Core_VUartRxBufferRead(VIRTUAL_UART0,NULL,0);
            msg.handler = l_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = LUAT_VUART_ID_0;
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
            msg.arg1 = LUAT_VUART_ID_0;
            msg.arg2 = 2;
            luat_msgbus_put(&msg, 1);
            break;
        case UART_CB_CONNECTED:
            //对方打开串口工具
            msg.handler = usb_uart_handler;
            msg.ptr = NULL;
            msg.arg1 = LUAT_VUART_ID_0;
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
    if (uart->id >= MAX_DEVICE_COUNT) {
    	serials[MAX_DEVICE_COUNT].rx_mark = 0;
    	Core_VUartInit(VIRTUAL_UART0, uart->baud_rate, 1, (uart->data_bits), parity, stop_bits, NULL);
    } else {
    	serials[uart->id].rx_mark = 0;
        Uart_BaseInit(uart->id, uart->baud_rate, 1, (uart->data_bits), parity, stop_bits, NULL);
        Uart_SetRxBufferSize(uart->id, uart->bufsz);
        serials[uart->id].rs485_param_bit.is_485used = (uart->pin485 < GPIO_NONE)?1:0;
        serials[uart->id].rs485_pin = uart->pin485;
        serials[uart->id].rs485_param_bit.rx_level = uart->rx_level;
        serials[uart->id].rs485_param_bit.wait_time = uart->delay;
        if (!serials[uart->id].rs485_timer) {
        	serials[uart->id].rs485_timer = Timer_Create(luat_uart_wait_timer_cb, uart->id, NULL);
        }
        GPIO_Config(serials[uart->id].rs485_pin, 0, serials[uart->id].rs485_param_bit.rx_level);
#ifndef UART_RX_USE_DMA
        Uart_EnableRxIrq(uart->id);
#endif
    }

    return 0;
}


int luat_uart_write(int uartid, void *data, size_t length){
    //printf("uid:%d,data:%s,length = %d\r\n",uartid, (char *)data, length);
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    if (uartid >= MAX_DEVICE_COUNT) {
        Core_VUartBufferTx(VIRTUAL_UART0, (const uint8_t *)data, length);
    }
    else {
    	if (serials[uartid].rs485_param_bit.is_485used) GPIO_Output(serials[uartid].rs485_pin, !serials[uartid].rs485_param_bit.rx_level);
        Uart_BufferTx(uartid, (const uint8_t *)data, length);
    }
    return length;
}

int luat_uart_read(int uartid, void *buffer, size_t length){
    int ret = 0;
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    if (uartid >= LUAT_VUART_ID_0) uartid = MAX_DEVICE_COUNT;
    serials[uartid].rx_mark = 0;
    if (uartid >= MAX_DEVICE_COUNT)
        ret = Core_VUartRxBufferRead(VIRTUAL_UART0, (uint8_t *)buffer,length);
    else
        ret = Uart_RxBufferRead(uartid, (uint8_t *)buffer,length);
//    LLOGD("uartid:%d buffer:%s ret:%d ",uartid,buffer, ret);
    return ret;
}

int luat_uart_close(int uartid){
    if (!luat_uart_exist(uartid)){
        return 0;
    }
    if (uartid >= LUAT_VUART_ID_0) uartid = MAX_DEVICE_COUNT;
    serials[uartid].rs485_param_bit.is_485used = 0;
    if (uartid >= MAX_DEVICE_COUNT)
        Core_VUartDeInit(VIRTUAL_UART0);
    else
        Uart_DeInit(uartid);

    return 0;
}

int luat_setup_cb(int uartid, int received, int sent){
    if (!luat_uart_exist(uartid)){
        return -1;
    }
    if (uartid >= MAX_DEVICE_COUNT)
        Core_VUartSetCb(VIRTUAL_UART0, luat_uart_usb_cb);
    else {
        Uart_SetCb(uartid, luat_uart_cb);
//#ifndef UART_RX_USE_DMA
//        Uart_EnableRxIrq(uartid);
//#endif
    }
    return 0;
}

int luat_uart_wait_485_tx_done(int uartid)
{
	int cnt = 0;
    if (luat_uart_exist(uartid)){
        if (serials[uartid].rs485_param_bit.is_485used){
        	while(!Uart_IsTSREmpty(uartid)) {cnt++;}
        	GPIO_Output(serials[uartid].rs485_pin, serials[uartid].rs485_param_bit.rx_level);
        }
    }
    return cnt;
}

#ifdef LUAT_USE_SOFT_UART
#ifdef __AIR105_BSP__
#include "air105.h"
#include "air105_conf.h"
#endif
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif

static void __FUNC_IN_RAM__ prvHWTimer_IrqHandler(int32_t Line, void *pData)
{
	CommonFun_t callback = (CommonFun_t )pData;
	uint8_t hwtimer_id = HWTimer_GetIDFromIrqline(Line);
	volatile uint32_t clr;
	clr = TIMM0->TIM[hwtimer_id].EOI;
	callback();
}

int luat_uart_soft_setup_hwtimer_callback(int hwtimer_id, CommonFun_t callback)
{
	int irq_line = HWTimer_GetIrqLine(hwtimer_id);

	TIMM0->TIM[hwtimer_id].ControlReg = 0;
	ISR_OnOff(irq_line, 0);
	ISR_Clear(irq_line);
	if (callback)
	{
		ISR_SetHandler(irq_line, prvHWTimer_IrqHandler, callback);
		ISR_SetPriority(irq_line, 3);
		ISR_OnOff(irq_line, 1);
	}
	return 0;
}
void __FUNC_IN_RAM__ luat_uart_soft_gpio_fast_output(int pin, uint8_t value)
{
	GPIO_Output(pin, value);
}
uint8_t __FUNC_IN_RAM__ luat_uart_soft_gpio_fast_input(int pin)
{
	return GPIO_Input(pin);
}
void luat_uart_soft_gpio_fast_irq_set(int pin, uint8_t on_off)
{
	if (on_off)
	{
		GPIO_ExtiConfig(pin, 0, 0, 1);
	}
	else
	{
		GPIO_ExtiConfig(pin, 0, 0, 0);
	}
}
uint32_t luat_uart_soft_cal_baudrate(uint32_t baudrate)
{
	return (48000000/baudrate);
}
void __FUNC_IN_RAM__ luat_uart_soft_hwtimer_onoff(int hwtimer_id, uint32_t period)
{
	TIMM0->TIM[hwtimer_id].ControlReg = 0;

    // Enable TIMER IRQ
    if (period)
    {
    	TIMM0->TIM[hwtimer_id].LoadCount = period - 1;
    	TIMM0->TIM[hwtimer_id].ControlReg = TIMER_CONTROL_REG_TIMER_ENABLE|TIMER_CONTROL_REG_TIMER_MODE;
    }
}

void __FUNC_IN_RAM__ luat_uart_soft_sleep_enable(uint8_t is_enable)
{
	PM_SetDriverRunFlag(PM_DRV_SOFT_UART, !is_enable);
}
#endif
