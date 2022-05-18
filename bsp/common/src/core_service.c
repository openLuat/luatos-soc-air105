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
#include "zbar.h"
#include "symbol.h"
#include "image.h"
#include "tiny_jpeg.h"
extern void SPI_TestInit(uint8_t SpiID, uint32_t Speed, uint32_t TestLen);
extern void SPI_TestOnce(uint8_t SpiID, uint32_t TestLen);
extern void DHT11_TestOnce(uint8_t Pin, CBFuncEx_t CB);
extern void DHT11_TestResult(void);
extern void SHT30_Init(CBFuncEx_t CB, void *pParam);
extern void SHT30_GetResult(CBFuncEx_t CB, void *pParam);
extern void GC032A_TestInit(void);
extern void OV2640_TestInit(void);



enum
{
	SERVICE_LCD_DRAW = SERVICE_EVENT_ID_START + 1,
	SERVICE_CAMERA_DRAW,
	SERVICE_SDHC_WRITE,
	SERVICE_SPIFLASH_ERASE,
	SERVICE_SPIFLASH_WRITE,
	SERVICE_SPIFLASH_CLOSE,
	SERVICE_FILE_WRITE,
	SERVICE_DECODE_QR,
	SERVICE_SCAN_KEYBOARD,
	SERVICE_ENCODE_JPEG_START,
	SERVICE_ENCODE_JPEG_RUN,
	SERVICE_ENCODE_JPEG_END,
};


typedef struct
{
	tje_write_func *JPEGEncodeWriteFun;
	void *JPEGEncodeWriteParam;
	CoreUpgrade_HeadStruct *pFotaHead;
	SPIFlash_CtrlStruct *pSpiFlash;
	HANDLE LCDHandle;
	HANDLE ServiceHandle;
	HANDLE StorgeHandle;
	HANDLE UserHandle;
	uint64_t LCDDrawRequireByte;
	uint64_t LCDDrawDoneByte;
	uint32_t InitAllocMem;
	uint32_t LastAllocMem;
	uint32_t CRC32_Table[256];
	Buffer_Struct FotaDataBuf;
	uint32_t FotaPos;
	uint32_t FotaDoneLen;
	uint32_t FotaCRC32;
	uint8_t RFix;
	uint8_t GFix;
	uint8_t BFix;
	uint8_t FotaFindHead;
	uint8_t FotaReady;
}Service_CtrlStruct;

static Service_CtrlStruct prvService;

static void prvCore_LCDCmd(LCD_DrawStruct *Draw, uint8_t Cmd)
{
    GPIO_Output(Draw->DCPin, 0);
    if (Draw->DCDelay)
    {
    	SysTickDelay(Draw->DCDelay * CORE_TICK_1US);
    }
    GPIO_Output(Draw->CSPin, 0);
    SPI_BlockTransfer(Draw->SpiID, &Cmd, NULL, 1);
    GPIO_Output(Draw->CSPin, 1);
    GPIO_Output(Draw->DCPin, 1);
//    SysTickDelay(300);
}

static void prvCore_LCDData(LCD_DrawStruct *Draw, uint8_t *Data, uint32_t Len)
{
    GPIO_Output(Draw->CSPin, 0);
    SPI_BlockTransfer(Draw->SpiID, Data, NULL, Len);
    GPIO_Output(Draw->CSPin, 1);
}

static uint16_t RGB888toRGB565(uint8_t red, uint8_t green, uint8_t blue, uint8_t IsBe)
{
	uint16_t B = (blue >> 3) & 0x001F;
	uint16_t G = ((green >> 2) << 5) & 0x07E0;
	uint16_t R = ((red >> 3) << 11) & 0xF800;
	uint16_t C = (R | G | B);
	if (IsBe)
	{
		C = BSP_Swap16(C);
	}
	return C;
}

static void RGB565toRGB888(uint16_t In, uint8_t *Out, uint8_t IsBe)
{
	if (IsBe)
	{
		In = BSP_Swap16(In);
	}
	Out[2] = ((In & 0x001F) << 3) | prvService.BFix;
	Out[1] = ((In & 0x07E0) >> 3) | prvService.GFix;
	Out[0] = ((In & 0xF800) >> 8) | prvService.RFix;
}

static void prv_CoreJpegSave(void* context, void* data, int size)
{

}

static void prvLCD_Task(void* params)
{
	uint64_t StartUS;
	OS_EVENT Event;
	int32_t Result;
	LCD_DrawStruct *Draw;
	PV_Union uPV;
	uint16_t uColor[256];
	uint32_t i, j, Size;
	for(i = 0; i < 256; i++)
	{
		uColor[i] = RGB888toRGB565(i, i, i, 1);
	}
	while(1)
	{
		Result = Task_GetEvent(prvService.LCDHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case SERVICE_LCD_DRAW:

			Draw = (LCD_DrawStruct *)Event.Param1;
			if (Draw->Speed > 30000000)
			{
				SPI_SetTxOnlyFlag(Draw->SpiID, 1);
			}
			SPI_SetNewConfig(Draw->SpiID, Draw->Speed, Draw->Mode);
			SPI_DMATxInit(Draw->SpiID, LCD_SPI_TX_DMA_STREAM, 0);
			SPI_DMARxInit(Draw->SpiID, LCD_SPI_RX_DMA_STREAM, 0);
			prvCore_LCDCmd(Draw, 0x2a);
			BytesPutBe16(uPV.u8, Draw->x1 + Draw->xoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->x2 + Draw->xoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2b);
			BytesPutBe16(uPV.u8, Draw->y1 + Draw->yoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->y2 + Draw->yoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2c);
			GPIO_Output(Draw->CSPin, 0);
//			StartUS = GetSysTickUS();
//			SPI_Transfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size, 2);

			SPI_BlockTransfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size);
//			if (Draw->Size > 38000)
//			{
//				DBG("%u, %u", Draw->Size, (uint32_t)(GetSysTickUS() - StartUS));
//			}
			prvService.LCDDrawDoneByte += Draw->Size;
			free(Draw->Data);
			GPIO_Output(Draw->CSPin, 1);
			free(Draw);
			SPI_SetTxOnlyFlag(Draw->SpiID, 0);
			break;
		case SERVICE_CAMERA_DRAW:
			Draw = (LCD_DrawStruct *)Event.Param1;

			SPI_SetTxOnlyFlag(Draw->SpiID, 1);
			SPI_SetNewConfig(Draw->SpiID, Draw->Speed, Draw->Mode);
			SPI_DMATxInit(Draw->SpiID, LCD_SPI_TX_DMA_STREAM, 0);
			SPI_DMARxInit(Draw->SpiID, LCD_SPI_RX_DMA_STREAM, 0);
			prvCore_LCDCmd(Draw, 0x2a);
			BytesPutBe16(uPV.u8, Draw->x1 + Draw->xoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->x2 + Draw->xoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2b);
			BytesPutBe16(uPV.u8, Draw->y1 + Draw->yoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->y2 + Draw->yoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2c);
			GPIO_Output(Draw->CSPin, 0);
			switch(Draw->ColorMode)
			{
			case COLOR_MODE_RGB_565:
				SPI_BlockTransfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size);
				break;
			case COLOR_MODE_GRAY:
				Size = Draw->Size/10;
				uPV.pu16 = malloc(Size * 2);
				for(i = 0; i < 10; i++)
				{
					for(j = 0; j < Size; j++)
					{
						uPV.pu16[j] = uColor[Draw->Data[i * Size + j]];
					}
					SPI_BlockTransfer(Draw->SpiID, uPV.pu8, uPV.pu8, Size * 2);
				}
				free(uPV.pu16);
				break;
			}
			free(Draw);
			GPIO_Output(Draw->CSPin, 1);
			SPI_SetTxOnlyFlag(Draw->SpiID, 0);
			break;
		}
	}
}

static void prvStorge_Task(void* params)
{
	OS_EVENT Event;
	uint32_t *Param;
	SDHC_SPICtrlStruct *pSDHC;
	CommonFun_t CB;
	HANDLE fd;
	while(1)
	{
		Task_GetEventByMS(prvService.StorgeHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case SERVICE_SDHC_WRITE:
			pSDHC = (SDHC_SPICtrlStruct *)Event.Param1;
			if (pSDHC->WaitFree)
			{
				DBG("sdhc wait reboot");
				free(Event.Param2);
				free(Event.Param3);
				break;
			}
			Param = (uint32_t *)Event.Param3;
			SDHC_SpiWriteBlocks(pSDHC, Event.Param2, Param[0], Param[1]);
			free(Event.Param2);
			free(Event.Param3);
			break;
		case SERVICE_SPIFLASH_ERASE:
			if (prvService.pSpiFlash)
			{
				SPIFlash_Erase(prvService.pSpiFlash, Event.Param1, Event.Param2);
			}
			if (Event.Param3)
			{
				CB = (CommonFun_t )Event.Param3;
				CB();
			}
			break;
		case SERVICE_SPIFLASH_WRITE:
			if (prvService.pSpiFlash)
			{
				SPIFlash_Write(prvService.pSpiFlash, Event.Param1, Event.Param2, Event.Param3);
				prvService.FotaCRC32 = CRC32_Cal(prvService.CRC32_Table, Event.Param2, Event.Param3, prvService.FotaCRC32);
				prvService.FotaDoneLen += Event.Param3;
			}
			free(Event.Param2);
			break;
		case SERVICE_SPIFLASH_CLOSE:
			free(prvService.pSpiFlash);
			prvService.pSpiFlash = NULL;
			break;
		case SERVICE_FILE_WRITE:
#ifdef __LUATOS__
			fd = luat_fs_fopen((char *)Event.Param1, "w");
			//DBG("%x,%s",fd, (char *)Event.Param1);
			if (fd)
			{
				luat_fs_fwrite(Event.Param2, Event.Param3, 1, fd);
				luat_fs_fclose(fd);
			}
#endif
			prvService.FotaDoneLen += Event.Param3;
			free(Event.Param2);
			break;
		}
	}
}

static void prvService_Task(void* params)
{
	zbar_image_scanner_t *zbar_scanner;
	zbar_image_t *zbar_image;
	zbar_symbol_t *zbar_symbol;
	CBDataFun_t CBDataFun;
	PV_Union uPV;
	PV_Union uColor;
	HANDLE JPEGEncodeHandle = NULL;
	OS_EVENT Event;
	uint32_t i;
	CBFuncEx_t CB;
	Core_SetRGB565FixValue(0xff, 0xff, 0xff);
//	Audio_Test();
	while(1)
	{
		Task_GetEventByMS(prvService.ServiceHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case SERVICE_SCAN_KEYBOARD:
			SoftKB_ScanOnce();
			break;

		case SERVICE_DECODE_QR:
			if (Event.Param3)
			{
				//此处代码可以更换成第三方解码
				CBDataFun = (CBDataFun_t)(Event.Param3);
				uPV.u32 = Event.Param2;
				zbar_scanner = zbar_image_scanner_create();
				zbar_image_scanner_set_config(zbar_scanner, 0, ZBAR_CFG_ENABLE, 1);
				zbar_image = zbar_image_create();
				zbar_image_set_format(zbar_image, *(int*)"Y800");
				zbar_image_set_size(zbar_image, uPV.u16[0], uPV.u16[1]);
				zbar_image_set_data(zbar_image, Event.Param1, uPV.u16[0] * uPV.u16[1], zbar_image_free_data);
				if (zbar_scan_image(zbar_scanner, zbar_image) > 0)
				{
					zbar_symbol = (zbar_symbol_t *)zbar_image_first_symbol(zbar_image);
					free(Event.Param1);
					//解码完成回调
					CBDataFun(zbar_symbol->data, zbar_symbol->datalen);
				}
				else
				{
					free(Event.Param1);
					//解码失败回调
					CBDataFun(NULL, 0);
				}
				zbar_image_destroy(zbar_image);
				zbar_image_scanner_destroy(zbar_scanner);
			}
			else
			{
				free(Event.Param1);
			}
			break;
		case SERVICE_ENCODE_JPEG_START:
			JPEGEncodeHandle = jpeg_encode_init(prvService.JPEGEncodeWriteFun, prvService.JPEGEncodeWriteParam, Event.Param2, Event.Param1 >> 16, Event.Param1 & 0x0000ffff, 3);
			break;
		case SERVICE_ENCODE_JPEG_RUN:
			if (JPEGEncodeHandle)
			{
				uColor.u32 = Event.Param1;
				switch(Event.Param3)
				{
				case COLOR_MODE_YCBCR_422_CBYCRY:
					uPV.pu8 = malloc((Event.Param2 >> 1) * 3);
					if (!uPV.pu8)
					{
						DBG("jpeg encode no momery");
						break;
					}
					Event.Param2 >>= 2;
					for(i = 0; i < Event.Param2; i++)
					{
						uPV.pu8[i * 6 + 0] = uColor.pu8[i * 4 + 1];
						uPV.pu8[i * 6 + 1] = uColor.pu8[i * 4 + 0];
						uPV.pu8[i * 6 + 2] = uColor.pu8[i * 4 + 2];
						uPV.pu8[i * 6 + 3] = uColor.pu8[i * 4 + 3];
						uPV.pu8[i * 6 + 4] = uColor.pu8[i * 4 + 0];
						uPV.pu8[i * 6 + 5] = uColor.pu8[i * 4 + 2];
					}
					jpeg_encode_run(JPEGEncodeHandle, uPV.pu8, 0);
					free(uPV.pu8);
					break;
				case COLOR_MODE_RGB_565:
					uPV.pu8 = malloc((Event.Param2 >> 1) * 3);
					if (!uPV.pu8)
					{
						DBG("jpeg encode no momery");
						break;
					}
					Event.Param2 >>= 1;
					for(i = 0; i < Event.Param2; i++)
					{
						RGB565toRGB888(uColor.pu16[i], uPV.pu8 + i * 3, 1);
					}
					jpeg_encode_run(JPEGEncodeHandle, uPV.pu8, 1);
					free(uPV.pu8);
					break;
				case COLOR_MODE_GRAY:
					uPV.pu8 = malloc(Event.Param2 * 3);
					if (!uPV.pu8)
					{
						DBG("jpeg encode no momery");
						break;
					}
					for(i = 0; i < Event.Param2; i++)
					{
						uPV.pu8[i * 3 + 0] = uColor.pu8[i];
						uPV.pu8[i * 3 + 1] = uColor.pu8[i];
						uPV.pu8[i * 3 + 2] = uColor.pu8[i];
					}
					jpeg_encode_run(JPEGEncodeHandle, uPV.pu8, 1);
					free(uPV.pu8);
					break;
				default:
					break;
				}
			}
			free(Event.Param1);
			break;
		case SERVICE_ENCODE_JPEG_END:
			if (JPEGEncodeHandle)
			{
				jpeg_encode_end(JPEGEncodeHandle);
				free(JPEGEncodeHandle);
				JPEGEncodeHandle = NULL;
			}
			if (Event.Param1)
			{
				CB = (CBFuncEx_t)Event.Param1;
				CB(NULL, Event.Param2);
			}
			break;
		}
	}
}

extern int luat_fs_init(void);
static void prvLuatOS_Task(void* params)
{
	// 文件系统初始化
	// luat_fs_init();

	// 载入LuatOS主入口
	luat_main();
	// 永不进入的代码, 避免编译警告.
//	for(;;)
//	{
//		vTaskDelayUntil( &xLastWakeTime, 2000 * portTICK_RATE_MS );
//	}
}
extern void luat_base_init(void);

uint32_t Core_LCDDrawCacheLen(void)
{
	if (prvService.LCDDrawRequireByte > prvService.LCDDrawDoneByte)
	{
		return (uint32_t)(prvService.LCDDrawRequireByte - prvService.LCDDrawDoneByte);
	}
	else
	{
		return 0;
	}
}

void Core_LCDDraw(LCD_DrawStruct *Draw)
{
	prvService.LCDDrawRequireByte += Draw->Size;
	Task_SendEvent(prvService.LCDHandle, SERVICE_LCD_DRAW, Draw, 0, 0);
}

void Core_LCDDrawBlock(LCD_DrawStruct *Draw)
{
	PV_Union uPV;
	uint64_t StartUS;
//	if (Draw->Speed > 30000000)
//	{
//		SPI_SetTxOnlyFlag(Draw->SpiID, 1);
//	}
	SPI_SetNewConfig(Draw->SpiID, Draw->Speed, Draw->Mode);
//	SPI_DMATxInit(Draw->SpiID, LCD_SPI_TX_DMA_STREAM, 0);
//	SPI_DMARxInit(Draw->SpiID, LCD_SPI_RX_DMA_STREAM, 0);
	prvCore_LCDCmd(Draw, 0x2a);
	BytesPutBe16(uPV.u8, Draw->x1 + Draw->xoffset);
	BytesPutBe16(uPV.u8 + 2, Draw->x2 + Draw->xoffset);
	prvCore_LCDData(Draw, uPV.u8, 4);
	prvCore_LCDCmd(Draw, 0x2b);
	BytesPutBe16(uPV.u8, Draw->y1 + Draw->yoffset);
	BytesPutBe16(uPV.u8 + 2, Draw->y2 + Draw->yoffset);
	prvCore_LCDData(Draw, uPV.u8, 4);
	prvCore_LCDCmd(Draw, 0x2c);
	GPIO_Output(Draw->CSPin, 0);
//	StartUS = GetSysTickUS();
//			SPI_Transfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size, 2);

	SPI_BlockTransfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size);
//	DBG("%u, %u", Draw->Size, (uint32_t)(GetSysTickUS() - StartUS));
//	if (Draw->Speed > 30000000)
//	{
//		Task_DelayUS(1);
//	}
	GPIO_Output(Draw->CSPin, 1);
	SPI_SetTxOnlyFlag(Draw->SpiID, 0);
}

void Core_CameraDraw(LCD_DrawStruct *Draw)
{
	Task_SendEvent(prvService.LCDHandle, SERVICE_CAMERA_DRAW, Draw, 0, 0);
}

void Core_WriteSDHC(void *pSDHC, void *pData)
{
	uint32_t *Param = malloc(8);
	Param[0] =((SDHC_SPICtrlStruct *)pSDHC)->CurBlock;
	Param[1] = ((SDHC_SPICtrlStruct *)pSDHC)->EndBlock - ((SDHC_SPICtrlStruct *)pSDHC)->CurBlock;
	Task_SendEvent(prvService.StorgeHandle, SERVICE_SDHC_WRITE, pSDHC, pData, Param);
}

void Core_DecodeQR(uint8_t *ImageData, uint16_t ImageW, uint16_t ImageH,  CBDataFun_t CB)
{
	PV_Union uPV;
	uPV.u16[0] = ImageW;
	uPV.u16[1] = ImageH;
	Task_SendEvent(prvService.ServiceHandle, SERVICE_DECODE_QR, ImageData, uPV.u32, CB);
}

void Core_ScanKeyBoard(void)
{
	Task_SendEvent(prvService.ServiceHandle, SERVICE_SCAN_KEYBOARD, 0, 0, 0);
}

void Core_SetRGB565FixValue(uint8_t R, uint8_t G, uint8_t B)
{
	prvService.RFix = R & 0x07;
	prvService.GFix = G & 0x03;
	prvService.BFix = B & 0x07;
}

void Core_EncodeJPEGSetup(HANDLE Fun, void *pParam)
{
	if (Fun)
	{
		prvService.JPEGEncodeWriteFun = Fun;
	}
	else
	{
		prvService.JPEGEncodeWriteFun = prv_CoreJpegSave;
	}
	prvService.JPEGEncodeWriteParam = pParam;
}

void Core_EncodeJPEGStart(uint32_t Width, uint32_t Height, uint8_t Quality)
{
	Task_SendEvent(prvService.ServiceHandle, SERVICE_ENCODE_JPEG_START, (Width << 16 | Height), Quality, 0);
}

void Core_EncodeJPEGRun(uint8_t *Data, uint32_t Len, uint8_t ColorMode)
{
	Task_SendEvent(prvService.ServiceHandle, SERVICE_ENCODE_JPEG_RUN, Data, Len, ColorMode);
}

void Core_EncodeJPEGEnd(CBFuncEx_t CB, void *pParam)
{
	Task_SendEvent(prvService.ServiceHandle, SERVICE_ENCODE_JPEG_END, CB, pParam, 0);
}

void Core_PrintMemInfo(void)
{
	uint32_t curalloc, totfree, maxfree;
	OS_MemInfo(&curalloc, &totfree, &maxfree);
	DBG("curalloc %uB, totfree %uB, maxfree %uB", curalloc, totfree, maxfree);
}

static uint8_t disassembly_ins_is_bl_blx(uint32_t addr) {
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK         0xF800
#define BL_INS_HIGH         0xF800
#define BL_INS_LOW          0xF000
#define BLX_INX_MASK        0xFF00
#define BLX_INX             0x4700

    if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW) {
        return 1;
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return 1;
    } else {
        return 0;
    }
}

static void prvCore_PrintTaskStack(HANDLE TaskHandle)
{
	uint32_t SP, StackAddress, Len, i, PC;
	uint32_t Buffer[16];
	if (!TaskHandle) return;
	char *Name = vTaskInfo(TaskHandle, &SP, &StackAddress, &Len);
	Len *= 4;
	DBG_Trace("%s call stack info:", Name);
	for(i = 0; i < 16, SP < StackAddress + Len; SP += 4)
	{
		PC = *((uint32_t *) SP) - 4;
		if ((PC > __FLASH_APP_START_ADDR__) && (PC < (__FLASH_BASE_ADDR__ + __CORE_FLASH_BLOCK_NUM__ * __FLASH_BLOCK_SIZE__)))
		{

	        if (PC % 2 == 0) {
	            continue;
	        }
	        PC = *((uint32_t *) SP) - 1;
	        if (disassembly_ins_is_bl_blx(PC - 4))
	        {
				DBG_Trace("%x", PC);
				i++;
	        }
		}
	}
}

void Core_PrintServiceStack(void)
{
	prvCore_PrintTaskStack(prvService.LCDHandle);
	prvCore_PrintTaskStack(prvService.UserHandle);
	prvCore_PrintTaskStack(prvService.ServiceHandle);
}

void Core_DebugMem(uint8_t HeapID, const char *FuncName, uint32_t Line)
{
	int32_t curalloc, totfree, maxfree;
	OS_MemInfo(&curalloc, &totfree, &maxfree);
	if (!prvService.InitAllocMem)
	{
		prvService.InitAllocMem = curalloc;
		prvService.LastAllocMem = curalloc;
		DBG_Printf("base %d\r\n", prvService.InitAllocMem);
	}
	else
	{
		DBG_Printf("%s %u:total %d last %d\r\n", FuncName, Line, curalloc - prvService.InitAllocMem, curalloc - prvService.LastAllocMem);
		prvService.LastAllocMem = curalloc;
	}
}

static void Core_OTAReady(void)
{
	prvService.FotaReady = 1;
}

int Core_OTAInit(CoreUpgrade_HeadStruct *Head, uint32_t size)
{
	prvService.FotaReady = 0;
	PV_Union uPV;
	Flash_Erase(__FLASH_OTA_INFO_ADDR__, __FLASH_SECTOR_SIZE__);
	if (prvService.pFotaHead)
	{
		free(prvService.pFotaHead);
	}
	prvService.pFotaHead = Head;
	OS_ReInitBuffer(&prvService.FotaDataBuf, __FLASH_BLOCK_SIZE__);
	prvService.FotaPos = 0;
	uPV.u32 = prvService.pFotaHead->Param1;
	if (uPV.u8[1] == CORE_OTA_OUT_SPI_FLASH)
	{
		if (prvService.pSpiFlash)
		{
			free(prvService.pSpiFlash);
		}
	}
	prvService.FotaCRC32 = 0xffffffff;
	prvService.FotaPos = 0;
	prvService.FotaDoneLen = 0;
	prvService.FotaFindHead = 0;
	if (uPV.u8[1] == CORE_OTA_OUT_SPI_FLASH)
	{
		prvService.pSpiFlash = zalloc(sizeof(SPIFlash_CtrlStruct));
		prvService.pSpiFlash->SpiID = uPV.u8[2];
		uPV.u32 = prvService.pFotaHead->Param2;
		prvService.pSpiFlash->CSPin = uPV.u8[3];
		prvService.pSpiFlash->IsBlockMode = 1;
		SPIFlash_Init(prvService.pSpiFlash, NULL);
		if (!prvService.pSpiFlash->IsInit)
		{
			return -1;
		}
		Task_SendEvent(prvService.StorgeHandle, SERVICE_SPIFLASH_ERASE, prvService.pFotaHead->DataStartAddress, size, Core_OTAReady);
	}
	else
	{
		prvService.FotaReady = 1;
	}
	return 0;
}

int Core_OTAWrite(uint8_t *Data, uint32_t Len)
{
	if (!prvService.FotaFindHead)
	{
		if (Len < sizeof(CoreUpgrade_FileHeadStruct))
		{
			return -1;
		}
		CoreUpgrade_FileHeadStruct Head;
		memcpy(&Head, Data, sizeof(CoreUpgrade_FileHeadStruct));

		if (Head.MaigcNum != __APP_START_MAGIC__)
		{
			DBG("%x", Head.MaigcNum);
			return -1;
		}
		uint32_t crc32 = CRC32_Cal(prvService.CRC32_Table, &Head.MainVersion, sizeof(CoreUpgrade_FileHeadStruct) - 8, 0xffffffff);
		if (crc32 != Head.CRC32)
		{
			DBG("crc32 %x,%x", crc32, Head.CRC32);
			return -1;
		}
		//由于是整包升级，就不考虑版本的事情了
		prvService.FotaFindHead = 1;
		prvService.pFotaHead->DataLen = Head.DataLen;
		prvService.pFotaHead->DataCRC32 = Head.DataCRC32;
		prvService.pFotaHead->CRC32 = CRC32_Cal(prvService.CRC32_Table, &prvService.pFotaHead->Param1, sizeof(CoreUpgrade_HeadStruct) - 8, 0xffffffff);
		Data = Data + sizeof(CoreUpgrade_FileHeadStruct);
		Len -= sizeof(CoreUpgrade_FileHeadStruct);
		if (!Len) return 0;
	}
	uint32_t DummyLen;
	PV_Union uPV;
	uPV.u32 = prvService.pFotaHead->Param1;
	if (!prvService.pFotaHead )
	{
		return -1;
	}
	switch(uPV.u8[1])
	{
	case CORE_OTA_IN_FLASH:
		break;
	case CORE_OTA_OUT_SPI_FLASH:
		if (!prvService.pSpiFlash)
		{
			return -1;
		}
		break;
	case CORE_OTA_IN_FILE:
		break;
	}

	DummyLen = ((prvService.FotaDataBuf.MaxLen - prvService.FotaDataBuf.Pos) >= Len)?Len:(prvService.FotaDataBuf.MaxLen - prvService.FotaDataBuf.Pos);
	OS_BufferWrite(&prvService.FotaDataBuf, Data, DummyLen);
	if (prvService.FotaDataBuf.Pos == __FLASH_BLOCK_SIZE__)
	{
		switch(uPV.u8[1])
		{
		case CORE_OTA_IN_FLASH:
			prvService.FotaDataBuf.Pos = 0;
			prvService.FotaDoneLen += __FLASH_BLOCK_SIZE__;
			break;
		case CORE_OTA_OUT_SPI_FLASH:
			Task_SendEvent(prvService.StorgeHandle, SERVICE_SPIFLASH_WRITE, prvService.pFotaHead->DataStartAddress + prvService.FotaPos, prvService.FotaDataBuf.Data, __FLASH_BLOCK_SIZE__);
			OS_InitBuffer(&prvService.FotaDataBuf, __FLASH_BLOCK_SIZE__);
			break;
		case CORE_OTA_IN_FILE:
			Task_SendEvent(prvService.StorgeHandle, SERVICE_FILE_WRITE, prvService.pFotaHead->FilePath, prvService.FotaDataBuf.Data, prvService.FotaDataBuf.Pos);
			OS_InitBuffer(&prvService.FotaDataBuf, __FLASH_BLOCK_SIZE__);
			break;
		}
		prvService.FotaPos += __FLASH_BLOCK_SIZE__;
	}
	else if ((prvService.FotaPos + prvService.FotaDataBuf.Pos) >= prvService.pFotaHead->DataLen)
	{
//		DBG("all data rx");
		switch(uPV.u8[1])
		{
		case CORE_OTA_IN_FLASH:
			prvService.FotaDoneLen += prvService.FotaDataBuf.Pos;
			OS_DeInitBuffer(&prvService.FotaDataBuf);
			break;
		case CORE_OTA_OUT_SPI_FLASH:
			Task_SendEvent(prvService.StorgeHandle, SERVICE_SPIFLASH_WRITE, prvService.pFotaHead->DataStartAddress + prvService.FotaPos, prvService.FotaDataBuf.Data, prvService.FotaDataBuf.Pos);
			memset(&prvService.FotaDataBuf, 0, sizeof(prvService.FotaDataBuf));
			break;
		case CORE_OTA_IN_FILE:
			Task_SendEvent(prvService.StorgeHandle, SERVICE_FILE_WRITE, prvService.pFotaHead->FilePath, prvService.FotaDataBuf.Data, prvService.FotaDataBuf.Pos);
			memset(&prvService.FotaDataBuf, 0, sizeof(prvService.FotaDataBuf));
			break;
		}
		return 0;
	}
	if (DummyLen < Len)
	{
		OS_BufferWrite(&prvService.FotaDataBuf, Data + DummyLen, Len - DummyLen);
	}

	if (prvService.FotaDoneLen < prvService.FotaPos)
	{
		return (prvService.FotaPos - prvService.FotaDoneLen);
	}

	return 1;
}

uint8_t Core_OTACheckReadyStart(void)
{
	return prvService.FotaReady;
}

int Core_OTACheckDone(void)
{
	if (!prvService.pFotaHead )
	{
		return -1;
	}
	if (prvService.FotaDoneLen >= prvService.pFotaHead->DataLen)
	{
		return (prvService.FotaCRC32 == prvService.pFotaHead->DataCRC32)?0:-1;
	}
	return 1;
}

//static void Core_OTACheckSpiFlash(void)
//{
//	CoreUpgrade_SectorStruct Sector;
//	Buffer_Struct ReadBuffer;
//	Buffer_Struct DataBuffer;
//	int result;
//	uint32_t Check;
//	uint32_t DoneLen, ReadLen;
//	volatile uint32_t ProgramPos, FlashPos;
//	uint32_t LzmaDataLen;
//	uint8_t LzmaHead[LZMA_PROPS_SIZE + 8];
//	uint8_t LzmaHeadLen;
//
//	DoneLen = 0;
//	OS_InitBuffer(&DataBuffer, SPI_FLASH_BLOCK_SIZE);
//	OS_InitBuffer(&ReadBuffer, SPI_FLASH_BLOCK_SIZE);
//	while(DoneLen < prvService.pFotaHead->DataLen)
//	{
//		ReadBuffer.Pos = sizeof(CoreUpgrade_SectorStruct);
//		SPIFlash_Read(prvService.pSpiFlash, prvService.pFotaHead->DataStartAddress + DoneLen, ReadBuffer.Data, ReadBuffer.Pos, 1);
//		DoneLen += sizeof(CoreUpgrade_SectorStruct);
//		memcpy(&Sector, ReadBuffer.Data, sizeof(CoreUpgrade_SectorStruct));
//		DBG("%x,%x,%x,%x,%x", Sector.MaigcNum, Sector.StartAddress, Sector.TotalLen, Sector.DataLen, Sector.DataCRC32);
//		if (Sector.MaigcNum != __APP_START_MAGIC__)
//		{
//			DBG("ota sector info error %x", Sector.MaigcNum);
//			return;
//		}
//		ProgramPos = Sector.StartAddress;
//		FlashPos = DoneLen + prvService.pFotaHead->DataStartAddress;
//		DoneLen += Sector.TotalLen;
//
//		Check = 0xffffffff;
//		ReadLen = 0;
//		while(ReadLen < Sector.TotalLen)
//		{
//			DBG("ota %x,%x,%u,%u", ProgramPos, FlashPos, ReadLen, Sector.TotalLen);
//			ReadBuffer.Pos = 1;
//			SPIFlash_Read(prvService.pSpiFlash, FlashPos, ReadBuffer.Data, ReadBuffer.Pos, 1);
//			FlashPos += ReadBuffer.Pos;
//			ReadLen += ReadBuffer.Pos;
//			LzmaHeadLen = ReadBuffer.Data[0];
//			if (LzmaHeadLen > sizeof(LzmaHead))
//			{
//				DBG("ota zip head error");
//				return;
//			}
//			ReadBuffer.Pos = LzmaHeadLen + 4;
//			SPIFlash_Read(prvService.pSpiFlash, FlashPos, ReadBuffer.Data, ReadBuffer.Pos, 1);
//			FlashPos += ReadBuffer.Pos;
//			ReadLen += ReadBuffer.Pos;
//			memcpy(LzmaHead, ReadBuffer.Data, LzmaHeadLen);
//			memcpy(&LzmaDataLen, ReadBuffer.Data + LzmaHeadLen, 4);
//			ReadBuffer.Pos = LzmaDataLen;
//			SPIFlash_Read(prvService.pSpiFlash, FlashPos, ReadBuffer.Data, ReadBuffer.Pos, 1);
//			FlashPos += ReadBuffer.Pos;
//			ReadLen += ReadBuffer.Pos;
//
//			DataBuffer.Pos = __FLASH_BLOCK_SIZE__;
//			result = LzmaUncompress(DataBuffer.Data, &DataBuffer.Pos, ReadBuffer.Data, &LzmaDataLen, LzmaHead, LzmaHeadLen);
//			if (DataBuffer.Pos != __FLASH_BLOCK_SIZE__)
//			{
//				DataBuffer.Pos = Sector.DataLen - (ProgramPos - Sector.StartAddress);
//			}
//			Check = CRC32_Cal(prvService.CRC32_Table, DataBuffer.Data, DataBuffer.Pos, Check);
//			ProgramPos += DataBuffer.Pos;
//
//		}
//		DBG("%u, %u", ProgramPos - Sector.StartAddress, Sector.DataLen);
//		if (Sector.DataCRC32 != Check)
//		{
////				Reboot = 1;
//			DBG("ota %x check crc32 fail %x %x", Sector.StartAddress, Check, Sector.DataCRC32);
//			return;
//		}
//		else
//		{
//			DBG("ota %x check ok", Sector.StartAddress);
//		}
//	}
//	OS_DeInitBuffer(&DataBuffer);
//	OS_DeInitBuffer(&ReadBuffer);
//}

void Core_OTAEnd(uint8_t isOK)
{
	PV_Union uPV;
	uPV.u32 = prvService.pFotaHead->Param1;
	if (isOK && prvService.pFotaHead)
	{
		Flash_Erase(__FLASH_OTA_INFO_ADDR__, __FLASH_SECTOR_SIZE__);
		Flash_Program(__FLASH_OTA_INFO_ADDR__, prvService.pFotaHead, sizeof(CoreUpgrade_HeadStruct));
		switch(uPV.u8[1])
		{
		case CORE_OTA_IN_FLASH:
			break;
		case CORE_OTA_OUT_SPI_FLASH:
//			Core_OTACheckSpiFlash();

			free(prvService.pSpiFlash);
			prvService.pSpiFlash = NULL;
			break;
		case CORE_OTA_IN_FILE:
			break;
		}

	}
	else
	{
		switch(uPV.u8[1])
		{
		case CORE_OTA_IN_FLASH:
			break;
		case CORE_OTA_OUT_SPI_FLASH:
			Task_SendEvent(prvService.StorgeHandle, SERVICE_SPIFLASH_CLOSE, 0, 0, 0);
			break;
		case CORE_OTA_IN_FILE:
			break;
		}
	}
	free(prvService.pFotaHead);
	prvService.pFotaHead = NULL;
	OS_DeInitBuffer(&prvService.FotaDataBuf);
}




void Core_LCDTaskInit(void)
{
	prvService.LCDHandle = Task_Create(prvLCD_Task, NULL, 2 * 1024, HW_TASK_PRO, "lcd task");
}

void Core_ServiceInit(void)
{
	CRC32_CreateTable(prvService.CRC32_Table, CRC32_GEN);
	prvService.ServiceHandle = Task_Create(prvService_Task, NULL, 4 * 1024, SERVICE_TASK_PRO, "service task");
}

void Core_UserTaskInit(void)
{
#ifdef __LUATOS__
	prvService.UserHandle = Task_Create(prvLuatOS_Task, NULL, 8*1024, LUATOS_TASK_PRO, "luatos task");
	luat_base_init();
#endif
}

void Core_StorgeTaskInit(void)
{
	prvService.StorgeHandle = Task_Create(prvStorge_Task, NULL, 1024, SERVICE_TASK_PRO, "storge task");
}


INIT_TASK_EXPORT(Core_LCDTaskInit, "0");
INIT_TASK_EXPORT(Core_ServiceInit, "1");
INIT_TASK_EXPORT(Core_StorgeTaskInit, "2");
INIT_TASK_EXPORT(Core_UserTaskInit, "5");
#endif
