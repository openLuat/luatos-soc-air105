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
#include "zbar.h"
#include "symbol.h"
#include "image.h"
#if 1
#define CAMERA_WIDTH	320
#define CAMERA_HEIGHT 240
#else
#define CAMERA_WIDTH	160
#define CAMERA_HEIGHT 80
#endif
#define GC032A_I2C_ADDRESS 0x21
#define OV2640_I2C_ADDRESS 0x30
static const I2C_CommonRegDataStruct GC032A_InitRegQueue[] =
{
        {0xf3, 0xff},
        {0xf7, 0x01},
		{0xf8, 0x03},
        {0xf9, 0xce},
        {0xfa, 0x00},
        {0xfc, 0x02},
        {0xfe, 0x02},
        {0x81, 0x03},


		{0xfe,0x00},
		{0x03,0x01},
		{0x04,0xc2},
		{0x05,0x01},
		{0x06,0xc2},
		{0x07,0x00},
		{0x08,0x08},
		{0x0a,0x04},
		{0x0c,0x04},
		{0x0d,0x01},
		{0x0e,0xe8},
		{0x0f,0x02},
		{0x10,0x88},
		//{0x17,0x55},
		{0x17,0x54},
		{0x19,0x04},
		{0x1a,0x0a},
		{0x1f,0x40},
		{0x20,0x30},
		{0x2e,0x80},
		{0x2f,0x2b},
		{0x30,0x1a},
		{0xfe,0x02},
		{0x03,0x02},
		{0x06,0x60},
		{0x05,0xd7},
		{0x12,0x89},
		{0xfe,0x00},
		{0x18,0x02},
		{0xfe,0x02},
		{0x40,0x22},
		{0x45,0x00},
		{0x46,0x00},
		{0x49,0x20},
		{0xfe,0x01},
		{0x0a,0xc5},
		{0x45,0x00},
		{0xfe,0x00},
		{0x40,0xff},
		{0x41,0x25},
		{0x42,0x83},
		{0x43,0x10},
		{0x46,0x26},
		{0x49,0x03},
		{0x4f,0x01},
		{0xde,0x84},
		{0xfe,0x02},
		{0x22,0xf6},
		{0xfe,0x01},
		{0x12,0xa0},
		{0x13,0x3a},
		{0xc1,0x3c},
		{0xc2,0x50},
		{0xc3,0x00},
		{0xc4,0x32},
		{0xc5,0x24},
		{0xc6,0x16},
		{0xc7,0x08},
		{0xc8,0x08},
		{0xc9,0x00},
		{0xca,0x20},
		{0xdc,0x8a},
		{0xdd,0xa0},
		{0xde,0xa6},
		{0xdf,0x75},
		{0xfe,0x01},
		{0x7c,0x09},
		{0x65,0x00},
		{0x7c,0x08},
		{0x56,0xf4},
		{0x66,0x0f},
		{0x67,0x84},
		{0x6b,0x80},
		{0x6d,0x12},
		{0x6e,0xb0},
		{0x86,0x00},
		{0x87,0x00},
		{0x88,0x00},
		{0x89,0x00},
		{0x8a,0x00},
		{0x8b,0x00},
		{0x8c,0x00},
		{0x8d,0x00},
		{0x8e,0x00},
		{0x8f,0x00},
		{0x90,0xef},
		{0x91,0xe1},
		{0x92,0x0c},
		{0x93,0xef},
		{0x94,0x65},
		{0x95,0x1f},
		{0x96,0x0c},
		{0x97,0x2d},
		{0x98,0x20},
		{0x99,0xaa},
		{0x9a,0x3f},
		{0x9b,0x2c},
		{0x9c,0x5f},
		{0x9d,0x3e},
		{0x9e,0xaa},
		{0x9f,0x67},
		{0xa0,0x60},
		{0xa1,0x00},
		{0xa2,0x00},
		{0xa3,0x0a},
		{0xa4,0xb6},
		{0xa5,0xac},
		{0xa6,0xc1},
		{0xa7,0xac},
		{0xa8,0x55},
		{0xa9,0xc3},
		{0xaa,0xa4},
		{0xab,0xba},
		{0xac,0xa8},
		{0xad,0x55},
		{0xae,0xc8},
		{0xaf,0xb9},
		{0xb0,0xd4},
		{0xb1,0xc3},
		{0xb2,0x55},
		{0xb3,0xd8},
		{0xb4,0xce},
		{0xb5,0x00},
		{0xb6,0x00},
		{0xb7,0x05},
		{0xb8,0xd6},
		{0xb9,0x8c},
		{0xfe,0x01},
		{0xd0,0x40},
		{0xd1,0xf8},
		{0xd2,0x00},
		{0xd3,0xfa},
		{0xd4,0x45},
		{0xd5,0x02},
		{0xd6,0x30},
		{0xd7,0xfa},
		{0xd8,0x08},
		{0xd9,0x08},
		{0xda,0x58},
		{0xdb,0x02},
		{0xfe,0x00},
		{0xfe,0x00},
		{0xba,0x00},
		{0xbb,0x04},
		{0xbc,0x0a},
		{0xbd,0x0e},
		{0xbe,0x22},
		{0xbf,0x30},
		{0xc0,0x3d},
		{0xc1,0x4a},
		{0xc2,0x5d},
		{0xc3,0x6b},
		{0xc4,0x7a},
		{0xc5,0x85},
		{0xc6,0x90},
		{0xc7,0xa5},
		{0xc8,0xb5},
		{0xc9,0xc2},
		{0xca,0xcc},
		{0xcb,0xd5},
		{0xcc,0xde},
		{0xcd,0xea},
		{0xce,0xf5},
		{0xcf,0xff},
		{0xfe,0x00},
		{0x5a,0x08},
		{0x5b,0x0f},
		{0x5c,0x15},
		{0x5d,0x1c},
		{0x5e,0x28},
		{0x5f,0x36},
		{0x60,0x45},
		{0x61,0x51},
		{0x62,0x6a},
		{0x63,0x7d},
		{0x64,0x8d},
		{0x65,0x98},
		{0x66,0xa2},
		{0x67,0xb5},
		{0x68,0xc3},
		{0x69,0xcd},
		{0x6a,0xd4},
		{0x6b,0xdc},
		{0x6c,0xe3},
		{0x6d,0xf0},
		{0x6e,0xf9},
		{0x6f,0xff},
		{0xfe,0x00},
		{0x70,0x50},
		{0xfe,0x00},
		{0x4f,0x01},
		{0xfe,0x01},
		{0x44,0x04},
		{0x1f,0x30},
		{0x20,0x40},
		{0x26,0x9a},
		{0x27,0x02},
		{0x28,0x0d},
		{0x29,0x02},
		{0x2a,0x0d},
		{0x2b,0x02},
		{0x2c,0x58},
		{0x2d,0x07},
		{0x2e,0xd2},
		{0x2f,0x0b},
		{0x30,0x6e},
		{0x31,0x0e},
		{0x32,0x70},
		{0x33,0x12},
		{0x34,0x0c},
		{0x3c,0x20},
		{0x3e,0x20},
		{0x3f,0x2d},
		{0x40,0x40},
		{0x41,0x5b},
		{0x42,0x82},
		{0x43,0xb7},
		{0x04,0x0a},
		{0x02,0x79},
		{0x03,0xc0},
		{0xcc,0x08},
		{0xcd,0x08},
		{0xce,0xa4},
		{0xcf,0xec},
		{0xfe,0x00},
		{0x81,0xb8},
		{0x82,0x12},
		{0x83,0x0a},
		{0x84,0x01},
		{0x86,0x25},
		{0x87,0x18},
		{0x88,0x10},
		{0x89,0x70},
		{0x8a,0x20},
		{0x8b,0x10},
		{0x8c,0x08},
		{0x8d,0x0a},
		{0xfe,0x00},
		{0x8f,0xaa},
		{0x90,0x9c},
		{0x91,0x52},
		{0x92,0x03},
		{0x93,0x03},
		{0x94,0x08},
		{0x95,0x44},
		{0x97,0x00},
		{0x98,0x00},
		{0xfe,0x00},
		{0xa1,0x30},
		{0xa2,0x41},
		{0xa4,0x30},
		{0xa5,0x20},
		{0xaa,0x30},
		{0xac,0x32},
		{0xfe,0x00},
		{0xd1,0x3c},
		{0xd2,0x3c},
		{0xd3,0x38},
		{0xd6,0xf4},
		{0xd7,0x1d},
		{0xdd,0x73},
		{0xde,0x84},
		{0xfe,0x01},
		{0x13,0x20},
		{0xfe,0x00},
		{0x4f,0x00},
		{0x03,0x00},
		{0x04,0xa0},
		{0x71,0x60},
		{0x72,0x40},
		{0x42,0x80},
		{0x77,0x64},
		{0x78,0x40},
		{0x79,0x60},

		{0xfe, 0x00},
//		{0x44, 0x06},	//改成RGB565输出
		{0x44,0x90},	//输出灰度
        {0x46, 0x0f},	//决定了VSYNC，HSYNC, pixclk
};
enum
{
	DECODE_DONE = USER_EVENT_ID_START + 100,
};

typedef struct
{
	HANDLE NotifyTaskHandler;
	uint8_t TestBuf[4096];
	uint8_t *DataCache;
	uint32_t TotalSize;
	uint32_t CurSize;
	uint16_t Width;
	uint16_t Height;
	uint8_t DataBytes;
	uint8_t IsDecode;
	uint8_t BufferFull;
}QR_DecodeCtrlStruct;
static QR_DecodeCtrlStruct prvDecodeQR;
static int32_t prvCamera_DCMICB(void *pData, void *pParam)
{
	Buffer_Struct *RxBuf = (Buffer_Struct *)pData;
	if (!pData)
	{
		if (!prvDecodeQR.DataCache)
		{
			prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
			prvDecodeQR.BufferFull = 0;
		}
		else
		{
			if (prvDecodeQR.TotalSize != prvDecodeQR.CurSize)
			{
//				DBG_ERR("%d, %d", prvDecodeQR.CurSize, prvDecodeQR.TotalSize);
			}
			else if (!prvDecodeQR.IsDecode)
			{
				prvDecodeQR.IsDecode = 1;
				Task_SendEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, prvDecodeQR.DataCache, 0, 0);
				prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
				prvDecodeQR.BufferFull = 0;
			}
			else
			{
				prvDecodeQR.BufferFull = 1;
			}
		}
		prvDecodeQR.CurSize = 0;
		return 0;
	}
	if (prvDecodeQR.DataCache)
	{
		if (prvDecodeQR.BufferFull)
		{
			if (!prvDecodeQR.IsDecode)
			{
				prvDecodeQR.IsDecode = 1;
				Task_SendEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, prvDecodeQR.DataCache, 0, 0);
				prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
				prvDecodeQR.BufferFull = 0;
				prvDecodeQR.CurSize = 0;
			}
			else
			{
				return 0;
			}

		}
	}
	else
	{
		prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
		prvDecodeQR.BufferFull = 0;
		prvDecodeQR.CurSize = 0;
	}
	memcpy(&prvDecodeQR.DataCache[prvDecodeQR.CurSize], RxBuf->Data, RxBuf->MaxLen * 4);
	prvDecodeQR.CurSize += RxBuf->MaxLen * 4;
	if (prvDecodeQR.TotalSize < prvDecodeQR.CurSize)
	{
		DBG_ERR("%d,%d", prvDecodeQR.TotalSize, prvDecodeQR.CurSize);
		prvDecodeQR.CurSize = 0;
	}
	return 0;
}

static int32_t prvTest_VUartCB(void *pData, void *pParam)
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
		ReadLen = Core_VUartRxBufferRead(UartID, prvDecodeQR.TestBuf, sizeof(prvDecodeQR.TestBuf));
		Core_VUartBufferTx(UartID, prvDecodeQR.TestBuf, ReadLen);
		break;
	case UART_CB_ERROR:
		DBG("usb disconnect");
		Core_VUartBufferTxStop(UartID); //也可以选择不清除，USB连上后，会自动重发，但是会有重复发送的情况，需要用户去处理
		break;
	case UART_CB_CONNECTED:
		break;
	}
}

static int32_t prvTest_VHIDCB(void *pData, void *pParam)
{
	uint32_t USB_ID = (uint32_t)pData;
	uint32_t State = (uint32_t)pParam;
	uint16_t ReadLen;
	switch (State)
	{
	case USB_HID_NOT_READY:
	case USB_HID_READY:
	case USB_HID_SEND_DONE:
		DBG("%d", State);
		break;
	}
}

static void zbar_task(void *pData)
{
	OS_EVENT Event;
	unsigned int len;
	int ret;
	char *string;
	zbar_t *zbar;
	LCD_DrawStruct *draw;
	Core_VUartInit(VIRTUAL_UART0, 0, 1, 0, 0, 0, prvTest_VUartCB);
	Core_VHIDInit(USB_ID0, prvTest_VHIDCB);
	while (1) {
		Task_GetEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, &Event, NULL, 0);
		draw = malloc(sizeof(LCD_DrawStruct));
		draw->Mode = SPI_MODE_0;
		draw->Speed = 48000000;
		draw->SpiID = HSPI_ID0;
		draw->CSPin = GPIOC_14;
		draw->DCPin = 72;
		draw->x1 = 0;
		draw->x2 = prvDecodeQR.Width - 1;
		draw->y1 = 0;
		draw->y2 = prvDecodeQR.Height - 1;
		draw->xoffset = 0;
		draw->yoffset = 0;
		draw->Size = prvDecodeQR.TotalSize;
		draw->ColorMode = COLOR_MODE_GRAY;
		draw->Data = Event.Param1;
		Core_CameraDraw(draw);
		zbar = zbar_create();

		ret = zbar_run(zbar, prvDecodeQR.Width, prvDecodeQR.Height, Event.Param1);

		if (ret > 0)
		{
			string = zbar_get_data_ptr(zbar, &len);
			memcpy(prvDecodeQR.TestBuf, string, len);
			Core_VUartBufferTx(VIRTUAL_UART0, prvDecodeQR.TestBuf, len);
			DBG("%.*s", len, string);
			Core_PrintMemInfo();
			Core_VHIDUploadData(USB_ID0, string, len);
		}
		zbar_destory(zbar);
		free(Event.Param1);
		prvDecodeQR.IsDecode = 0;
	}
}





void DecodeQR_TestInit(void)
{
	uint8_t Reg = 0xf0;
	uint8_t Data[2];
	uint32_t i;
	prvDecodeQR.DataCache = NULL;
	prvDecodeQR.Width = CAMERA_WIDTH;
	prvDecodeQR.Height = CAMERA_HEIGHT;
	prvDecodeQR.TotalSize = prvDecodeQR.Width * prvDecodeQR.Height;
	prvDecodeQR.DataBytes = 1;
	prvDecodeQR.NotifyTaskHandler = Task_Create(zbar_task, NULL, 8 * 1024, 1, "test zbar");
	GPIO_Config(GPIOD_06, 0, 0);
	GPIO_Config(GPIOD_07, 0, 0);
//	GPIO_ExtiSetHWTimerCB(GC032A_IOCB, NULL);
	GPIO_Iomux(GPIOE_01, 3);
	GPIO_Iomux(GPIOE_02, 3);
	GPIO_Iomux(GPIOE_03, 3);

	GPIO_Iomux(GPIOD_01, 3);
	GPIO_Iomux(GPIOD_02, 3);
	GPIO_Iomux(GPIOD_03, 3);
	GPIO_Iomux(GPIOD_08, 3);
	GPIO_Iomux(GPIOD_09, 3);
	GPIO_Iomux(GPIOD_10, 3);
	GPIO_Iomux(GPIOD_11, 3);
	GPIO_Iomux(GPIOE_00, 3);

	GPIO_Iomux(GPIOE_06, 2);
	GPIO_Iomux(GPIOE_07, 2);

	GPIO_Iomux(GPIOA_05, 2);
//	GPIO_Config(GPIOE_03, 1, 1);
//	GPIO_ExtiConfig(GPIOE_03, 0, 1, 0);
	HWTimer_SetPWM(HW_TIMER5, 24000000, 0, 0);
	Task_DelayMS(1);
	GPIO_Output(GPIOD_06, 1);
	GPIO_Output(GPIOD_07, 1);
	Task_DelayUS(10);
	GPIO_Output(GPIOD_07, 0);
	I2C_MasterSetup(I2C_ID0, 400000);
	Data[0] = 0xf4;
	Data[1] = 0x10;

	I2C_BlockWrite(I2C_ID0, GC032A_I2C_ADDRESS, Data, 2, 10, NULL, 0);
	I2C_BlockRead(I2C_ID0, GC032A_I2C_ADDRESS, &Reg, Data, 2, 10, NULL, 0);
	if ((0x23 == Data[0]) && (0x2a == Data[1]))
	{
		DBG("识别到GC032A控制芯片");
	}
	else
	{
		DBG("没有识别到GC032A控制芯片");
		return;
	}
//	Data[0] = 0xfe;
//	Data[1] = 0x00;
//	Reg = 0x44;
//	I2C_BlockWrite(I2C_ID0, GC032A_I2C_ADDRESS, Data, 2, 10, NULL, 0);
//	I2C_BlockRead(I2C_ID0, GC032A_I2C_ADDRESS, &Reg, Data, 2, 10, NULL, 0);
//	DBG("%x,%x", Data[0], Data[1]);
	for(i = 0; i < sizeof(GC032A_InitRegQueue)/sizeof(I2C_CommonRegDataStruct); i++)
	{
		I2C_BlockWrite(I2C_ID0, GC032A_I2C_ADDRESS, GC032A_InitRegQueue[i].Data, 2, 10, NULL, 0);
	}
//	DBG("GC032A Init Done");
//	I2C_Prepare(I2C_ID0, GC032A_I2C_ADDRESS, 1, GC032A_InitDone, 0);
//	I2C_MasterWriteRegQueue(I2C_ID0, GC032A_InitRegQueue, sizeof(GC032A_InitRegQueue)/sizeof(I2C_CommonRegDataStruct), 10, 0);
	DCMI_Setup(0, 0, 0, 8, 0);
	DCMI_SetCallback(prvCamera_DCMICB, 0);
	DCMI_SetCROPConfig(1, (480-prvDecodeQR.Height)/2, ((640-prvDecodeQR.Width)/2)*prvDecodeQR.DataBytes, prvDecodeQR.Height - 1, prvDecodeQR.DataBytes*prvDecodeQR.Width - 1);	//裁剪中间的240*320图像
	DCMI_CaptureSwitch(1, (prvDecodeQR.Width * prvDecodeQR.DataBytes * prvDecodeQR.Height) / 80, 0, 0, 0, NULL);	//240 * 2 / 4 * 12，一次DMA传输有12行数据，一共传输20次
}
