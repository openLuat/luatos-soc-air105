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
static uint8_t TestBuf[4096];
static int32_t prvTest_USBCB(void *pData, void *pParam)
{
	uint32_t UartID = (uint32_t)pData;
	uint32_t State = (uint32_t)pParam;
	uint16_t ReadLen;
	switch (State)
	{

	case UART_CB_TX_BUFFER_DONE:
	case UART_CB_TX_ALL_DONE:
//		DBG("usb send ok");
//		Core_PrintMemInfo();
		break;
	case UART_CB_RX_BUFFER_FULL:
	case UART_CB_RX_TIMEOUT:
	case UART_CB_RX_NEW:
		ReadLen = Core_VUartRxBufferRead(UartID, TestBuf, sizeof(TestBuf));
		Core_VUartBufferTx(UartID, TestBuf, ReadLen);
		break;
	case UART_CB_ERROR:
		DBG("usb disconnect");
		Core_VUartBufferTxStop(UartID); //也可以选择不清除，USB连上后，会自动重发，但是会有重复发送的情况，需要用户去处理
		break;
	case UART_CB_CONNECTED:
		Core_VUartBufferTx(UartID, "hello\r\n", 7);
		break;
	}
}

int32_t prvTest_HIDCB(void *pData, void *pParam)
{
	Buffer_Struct *Buf;
	switch((uint32_t)pParam)
	{
	case USB_HID_NOT_READY:
		DBG("hid disconnected");
		break;
	case USB_HID_READY:
		DBG("hid connected");
		break;
	case USB_HID_SEND_DONE:
		DBG("hid send ok");
		break;
	default:
		Buf = (Buffer_Struct *)pParam;
		DBG("hid get data:");
		DBG_HexPrintf(Buf->Data, Buf->Pos);
		Core_VHIDSendRawData(0, "just Test Once!\n", 16);
		break;
	}
    return 0;
}

void Test_USBStart(void)
{
	Core_VUartInit(VIRTUAL_UART0, 0, 1, 0, 0, 0, prvTest_USBCB);
	Core_VHIDInit(0, prvTest_HIDCB);
//	Core_VUartSetRxTimeout(VIRTUAL_UART0, 200);
}

void prvUSB_Test(void *p)
{
	SDHC_SPICtrlStruct SDHC;
	memset(&SDHC, 0, sizeof(SDHC));
	SDHC.SpiID = SPI_ID0;
	SDHC.CSPin = GPIOC_13;
	SDHC.IsSpiDMAMode = 0;
//	SDHC.NotifyTask = Task_GetCurrent();
//	SDHC.TaskCB = prvEventCB;
    SDHC.SDHCReadBlockTo = 5 * CORE_TICK_1MS;
    SDHC.SDHCWriteBlockTo = 25 * CORE_TICK_1MS;
    SDHC.IsPrintData = 0;
    GPIO_Iomux(GPIOC_12,2);
    GPIO_Iomux(GPIOC_14,2);
    GPIO_Iomux(GPIOC_15,2);
    GPIO_Config(SDHC.CSPin, 0, 1);
    SPI_MasterInit(SDHC.SpiID, 8, SPI_MODE_0, 400000, NULL, NULL);

    SDHC_SpiInitCard(&SDHC);
    if (SDHC.IsInitDone)
    {
    	SPI_SetNewConfig(SDHC.SpiID, 24000000, SPI_MODE_0);
    	SDHC_SpiReadCardConfig(&SDHC);
    	DBG("卡容量 %ublock", SDHC.Info.LogBlockNbr);
    	Test_USBStart();
    	Core_UDiskAttachSDHC(0, &SDHC);
    	Core_USBDefaultDeviceStart(0);
    }
    else
    {
    	Test_USBStart();
    	Core_USBDefaultDeviceStart(0);
    }
	Task_Exit();
}
void USB_TestInit(void)
{
	Task_Create(prvUSB_Test, NULL, 2 * 1024, SERVICE_TASK_PRO, "usb task");
}

//INIT_TASK_EXPORT(USB_TestInit, "3");
