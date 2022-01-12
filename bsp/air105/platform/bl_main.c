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
extern const uint32_t __isr_start_address;
extern const uint32_t __os_heap_start;
uint32_t SystemCoreClock;
#define FW_UPGRADE_START_TIME	300
#define FW_UPGRADE_ALL_TIME	5000
#define FW_UPGRADE_DATA_TIME 50
#define FW_OTA_FLASH_BUF_LEN	4096
enum
{
	FW_UPGRADE_STATE_IDLE,
	FW_UPGRADE_STATE_START,
	FW_UPGRADE_STATE_RUN,
	FW_UPGRADE_STATE_WAIT_END,
};

typedef struct
{
	Buffer_Struct FWDataBuffer;
	uint32_t Data[16 * 1024];
	uint32_t FWStartAddress;
	uint32_t FWTotalLen;
	uint32_t FWCRC32;
	uint32_t FinishLen;
	uint32_t NextAddress;
	uint32_t NextEraseAddress;
	uint8_t State;
	uint8_t ForceOut;
}BL_CtrlStuct;

#define BL_DBG DBG_INFO
//#define BL_DBG(X, Y...)

typedef  void (*pFunction)(void);

static uint32_t CRC32_Table[256];
static BL_CtrlStuct prvBL;

void Jump_AppRun(uint32_t Address)
{
  /* Jump to user application */
	pFunction Jump_To_Application;
	uint32_t JumpAddress;
	__disable_irq();
	DBG_INFO("\r\njump to 0x%x !", Address);
	JumpAddress = *(__IO uint32_t*) (Address + 4);
	Jump_To_Application = (pFunction) JumpAddress;
	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) Address);
	/* Jump to application */
	Jump_To_Application();
}

uint8_t BL_CheckFlashPage(uint32_t Page, uint32_t Address)
{
	uint32_t EndAddress = Page * __FLASH_SECTOR_SIZE__ ;

	if (Address >= EndAddress)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint32_t BL_GetFlashPage(uint32_t Address)
{
	return Address/__FLASH_SECTOR_SIZE__;
}

void BL_UnlockFlash(void)
{

}

void BL_LockFlash(void)
{

}

void BL_EraseSector(uint32_t address)
{
	Flash_EraseSector(address, 1);
//	BL_DBG("%x", address);
//	FLASH_EraseSector(address);
//	CACHE_CleanAll(CACHE);
}

void BL_ProgramData(uint32_t address, uint32_t *Data, uint32_t Len)
{
	Flash_ProgramData(address, Data, Len, 1);
//	BL_DBG("%x", address);
//	FLASH_ProgramPage(address, Len, Data);
//	CACHE_CleanAll(CACHE);
}

void FileSystem_Init(void)
{

}

int32_t BL_StartNewDownload(uint32_t Address, uint32_t TotalLen, uint32_t CRC32)
{
	if (prvBL.State != FW_UPGRADE_STATE_IDLE)
	{
		return -1;
	}
	prvBL.FWStartAddress = Address;
	prvBL.FWTotalLen = TotalLen;
	prvBL.FWCRC32 = CRC32;
	prvBL.State = FW_UPGRADE_STATE_START;
	prvBL.NextAddress = Address;
	return 0;
}

int BL_DownloadAddData(uint32_t PacketSn, uint8_t *Data, uint32_t Len, uint32_t *NextPacketSn)
{
	if (prvBL.NextAddress != PacketSn)
	{
		*NextPacketSn = prvBL.NextAddress;
		return -1;
	}
	else
	{
		Buffer_StaticWrite(&prvBL.FWDataBuffer, Data, Len);
		prvBL.NextAddress += Len;
		*NextPacketSn = prvBL.NextAddress;
		return prvBL.FWDataBuffer.Pos;
	}
}

uint8_t BL_DownloadEnd(void)
{
	if (prvBL.State != FW_UPGRADE_STATE_RUN)
	{
		return ERROR_OPERATION_FAILED;
	}
	prvBL.State = FW_UPGRADE_STATE_WAIT_END;
	return ERROR_NONE;
}

uint8_t BL_RunAPP(void)
{
	prvBL.ForceOut = 1;
}

void Local_Upgrade(void)
{
	uint8_t PageData[256];
	uint32_t ProgramLen, CRC32;
	uint64_t AllToTick = GetSysTick() + FW_UPGRADE_START_TIME * CORE_TICK_1MS;
	uint64_t DataToTick = GetSysTick() + FW_UPGRADE_DATA_TIME * CORE_TICK_1MS;
	uint8_t ConnectCnt = 0;
	DBG_Response(DBG_DEVICE_FW_UPGRADE_READY, ERROR_NONE, &ConnectCnt, 1);
	while(!prvBL.ForceOut)
	{
		WDT_Feed();
		switch(prvBL.State)
		{
		case FW_UPGRADE_STATE_IDLE:
			if (DataToTick <= GetSysTick())
			{
				ConnectCnt++;
				DataToTick = GetSysTick() + FW_UPGRADE_DATA_TIME * CORE_TICK_1MS;
				DBG_Response(DBG_DEVICE_FW_UPGRADE_READY, ERROR_NONE, &ConnectCnt, 1);
			}
			break;
		case FW_UPGRADE_STATE_START:
			AllToTick = GetSysTick() + FW_UPGRADE_ALL_TIME * CORE_TICK_1MS;
			BL_EraseSector(prvBL.FWStartAddress);
			prvBL.NextEraseAddress = prvBL.FWStartAddress + __FLASH_SECTOR_SIZE__;
			prvBL.FinishLen = 0;
			prvBL.State = FW_UPGRADE_STATE_RUN;
			break;
		case FW_UPGRADE_STATE_RUN:
		case FW_UPGRADE_STATE_WAIT_END:
			if (prvBL.FinishLen < prvBL.FWTotalLen)
			{
				if ((prvBL.FWDataBuffer.Pos + prvBL.FinishLen) >= prvBL.FWTotalLen)
				{
					AllToTick = GetSysTick() + FW_UPGRADE_ALL_TIME * CORE_TICK_1MS;

					if ((prvBL.FWStartAddress + prvBL.FinishLen) >= prvBL.NextEraseAddress)
					{
						BL_EraseSector(prvBL.NextEraseAddress);
						prvBL.NextEraseAddress += __FLASH_SECTOR_SIZE__;
					}
					ProgramLen = (prvBL.FWDataBuffer.Pos >= __FLASH_PAGE_SIZE__)?__FLASH_PAGE_SIZE__:prvBL.FWDataBuffer.Pos;
					__disable_irq();
					memcpy(PageData, prvBL.FWDataBuffer.Data, ProgramLen);
					OS_BufferRemove(&prvBL.FWDataBuffer, ProgramLen);
					__enable_irq();
					BL_ProgramData(prvBL.FWStartAddress + prvBL.FinishLen, PageData, ProgramLen);
					prvBL.FinishLen += ProgramLen;
				}
				else if (prvBL.FWDataBuffer.Pos >= __FLASH_PAGE_SIZE__)
				{
					AllToTick = GetSysTick() + FW_UPGRADE_ALL_TIME * CORE_TICK_1MS;

					if ((prvBL.FWStartAddress + prvBL.FinishLen) >= prvBL.NextEraseAddress)
					{
						BL_EraseSector(prvBL.NextEraseAddress);
						prvBL.NextEraseAddress += __FLASH_SECTOR_SIZE__;
					}
					ProgramLen = __FLASH_PAGE_SIZE__;
					__disable_irq();
					memcpy(PageData, prvBL.FWDataBuffer.Data, ProgramLen);
					OS_BufferRemove(&prvBL.FWDataBuffer, ProgramLen);
					__enable_irq();
					BL_ProgramData(prvBL.FWStartAddress + prvBL.FinishLen, PageData, ProgramLen);
					prvBL.FinishLen += ProgramLen;
				}
			}
			else
			{
				CRC32 = CRC32_Cal(CRC32_Table, prvBL.FWStartAddress, prvBL.FWTotalLen, CRC32_START);
				if (CRC32 != prvBL.FWCRC32)
				{
					DBG_Response(DBG_DEVICE_FW_UPGRADE_RESULT, ERROR_PROTOCL, &CRC32, 4);
				}
				else
				{
					DBG_Response(DBG_DEVICE_FW_UPGRADE_RESULT, ERROR_NONE, &CRC32, 4);
				}
				AllToTick = GetSysTick() + FW_UPGRADE_START_TIME * CORE_TICK_1MS;
				prvBL.State = FW_UPGRADE_STATE_IDLE;
				DataToTick = GetSysTick() + FW_UPGRADE_DATA_TIME * CORE_TICK_1MS;
			}
			break;
		default:
			break;
		}
		if (AllToTick <= GetSysTick())
		{
			prvBL.ForceOut = 1;
			DBG_Response(DBG_DEVICE_FW_UPGRADE_RESULT, ERROR_TIMEOUT, &ConnectCnt, 1);
		}
	}
}

typedef struct
{
	uint32_t MaigcNum; //升级包标识，标识不对直接抛弃
	uint32_t CRC32;		//后续字节的CRC32校验，所有CRC32规则与ZIP压缩一致
	uint32_t Param;		//升级参数，其中byte0升级类型，byte1升级包存放位置
	uint32_t DataStartAddress;//升级包在flash中的起始地址
	uint32_t DataLen;
	uint32_t DataCRC32;//整包的CRC32
	char FilePath[128];//升级包在文件系统中的绝对路径
}CoreUpgrade_HeadStruct;

static int32_t BL_OTAReadDataInFlash(void *pData, void *pParam)
{
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
	memcpy(pBuffer->Data, pData, pBuffer->Pos);
	return 0;
}

static int32_t BL_OTAReadDataInSpiFlash(void *pData, void *pParam)
{
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
}

static int32_t BL_OTAReadDataInFS(void *pData, void *pParam)
{
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
}

static void BL_OTAErase(uint32_t Address, uint32_t Len)
{
	uint32_t Pos = 0;
	while(Pos < Len)
	{
		Flash_EraseSector(Address + Pos, 0);
		Pos += __FLASH_SECTOR_SIZE__;
	}
}

static void BL_OTAWrite(uint32_t Address, uint8_t *Data, uint32_t Len)
{
	uint32_t Pos = 0;
	while(Pos < Len)
	{
		if ((Len - Pos) > __FLASH_PAGE_SIZE__)
		{
			Flash_ProgramData(Address + Pos, Data + Pos, __FLASH_PAGE_SIZE__, 0);
			Pos += __FLASH_PAGE_SIZE__;
		}
		else
		{
			Flash_ProgramData(Address + Pos, Data + Pos, Len - Pos, 0);
			Pos += Len - Pos;
		}
	}
}

void Remote_Upgrade(void)
{
	CoreUpgrade_HeadStruct Head;
	Buffer_Struct ReadBuffer;
	CBFuncEx_t pReadFunc;
	PV_Union uPV;
	uint32_t Check;
	uint32_t DoneLen;
	int32_t Result;
	uint8_t Reboot = 0;
	uint8_t FlashData[FW_OTA_FLASH_BUF_LEN];
	Buffer_StaticInit(&ReadBuffer, FlashData, FW_OTA_FLASH_BUF_LEN);
	memcpy(&Head, __FLASH_OTA_INFO_ADDR__, sizeof(CoreUpgrade_HeadStruct));
	Check = CRC32_Cal(CRC32_Table, &Head.Param, sizeof(Head) - 8, 0xffffffff);
	if (Head.MaigcNum != __APP_START_MAGIC__)
	{
		DBG_INFO("no ota info");
		return;
	}
	if (Check != Head.CRC32)
	{
		DBG_INFO("ota info error");
		return;
	}
	uPV.u32 = Head.Param;

	switch(uPV.u8[1])
	{
	case CORE_OTA_IN_FLASH:
		Check = CRC32_Cal(CRC32_Table, Head.DataStartAddress, Head.DataLen, 0xffffffff);
		if (Check != Head.DataCRC32)
		{
			DBG_INFO("ota file CRC32: %x,%x", Check, Head.DataCRC32);
			goto OTA_END;
		}
		pReadFunc = BL_OTAReadDataInFlash;
		break;
	default:
		DBG_INFO("core ota storage mode %u not support", uPV.u8[1]);
		return;
		break;
	}

	switch(uPV.u8[0])
	{
	case CORE_OTA_MODE_FULL:
		goto OTA_FULL;
		break;
	default:
		DBG_INFO("core ota mode %u not support", uPV.u8[0]);
		return;
		break;
	}
	goto OTA_END;
OTA_FULL:
	DoneLen = 0;
	while(DoneLen < Head.DataLen)
	{
		ReadBuffer.Pos = ((Head.DataLen - DoneLen) > ReadBuffer.MaxLen)?ReadBuffer.MaxLen:(Head.DataLen - DoneLen);
		Result = pReadFunc(Head.DataStartAddress + DoneLen, &ReadBuffer);
		if (Result < 0)
		{
			Reboot = 1;
			DBG_INFO("core ota read data fail");
			goto OTA_END;
		}
		if (memcmp(__FLASH_APP_START_ADDR__ + DoneLen, ReadBuffer.Data, ReadBuffer.Pos))
		{
			BL_OTAErase(__FLASH_APP_START_ADDR__ + DoneLen, ReadBuffer.Pos);
			BL_OTAWrite(__FLASH_APP_START_ADDR__ + DoneLen, ReadBuffer.Data, ReadBuffer.Pos);
		}
		DoneLen += ReadBuffer.Pos;
	}
	Check = CRC32_Cal(CRC32_Table, __FLASH_APP_START_ADDR__, Head.DataLen, 0xffffffff);
	if (Head.DataCRC32 != Check)
	{
		Reboot = 1;
		DBG_INFO("core ota final check crc32 fail %x %x", Check, Head.DataCRC32);
		goto OTA_END;
	}
	goto OTA_END;
OTA_DIFF:
	goto OTA_END;
OTA_END:
	if (Reboot)
	{
		NVIC_SystemReset();
		while(1){;}
	}
	else
	{
		BL_EraseSector(__FLASH_OTA_INFO_ADDR__);
	}

}

void SystemInit(void)
{
	SYSCTRL->LDO25_CR = (1 << 5);
	SCB->VTOR = (uint32_t)(&__isr_start_address);
#ifdef __USE_XTL__
	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_EXT | SYSCTRL_FREQ_SEL_XTAL_192Mhz;
#else
	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_INC | SYSCTRL_FREQ_SEL_XTAL_192Mhz;
#endif
	WDT_SetTimeout(__WDT_TO_MS__);
	WDT_ModeConfig(WDT_Mode_Interrupt);
	WDT_Enable();

//	QSPI->DEVICE_PARA = (QSPI->DEVICE_PARA & 0xFFFF) | (68 << 16);
#if (__FPU_PRESENT) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11*2));
#endif
    SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_UART0|SYSCTRL_APBPeriph_GPIO|SYSCTRL_APBPeriph_TIMM0;
    SYSCTRL->CG_CTRL2 = SYSCTRL_AHBPeriph_DMA|SYSCTRL_AHBPeriph_USB;
	SYSCTRL->LOCK_R &= ~SYSCTRL_USB_RESET;
    SYSCTRL->SOFT_RST2 |= SYSCTRL_USB_RESET;
    QSPI->DEVICE_PARA = (QSPI->DEVICE_PARA & 0x0000fff0) | (16 << 16) |(0x0a);
//    QSPI_Init(NULL);
//    QSPI_SetLatency(0);

}

void SystemCoreClockUpdate (void)            /* Get Core Clock Frequency      */
{
	SystemCoreClock = HSE_VALUE * (((SYSCTRL->FREQ_SEL & SYSCTRL_FREQ_SEL_XTAL_Mask) >> SYSCTRL_FREQ_SEL_XTAL_Pos) + 1);
}

int main(void)
{
	uint32_t AppInfo[4];
	CRC32_CreateTable(CRC32_Table, CRC32_GEN);
	__NVIC_SetPriorityGrouping(7 - __NVIC_PRIO_BITS);//对于freeRTOS必须这样配置
	SystemCoreClockUpdate();
    CoreTick_Init();
	cm_backtrace_init(NULL, NULL, NULL);
	Uart_GlobalInit();
	DMA_GlobalInit();
	DBG_Init(0);
	FileSystem_Init();
	Buffer_StaticInit(&prvBL.FWDataBuffer, prvBL.Data, sizeof(prvBL.Data));
LOCAL_UPGRADE_START:
	Local_Upgrade();
	Uart_ChangeBR(DBG_UART_ID, DBG_UART_BR);
#ifndef __RAMRUN__
	DBG_INFO("check core ota!");
	Remote_Upgrade();

//	__disable_irq();
	BL_LockFlash();

	memcpy(AppInfo, __FLASH_APP_START_ADDR__, sizeof(AppInfo));
	if (__APP_START_MAGIC__ == AppInfo[0])
	{
#ifdef __DEBUG__
		DBG_INFO("\r\nbootloader build debug %s %s %x!", __DATE__, __TIME__, QSPI->DEVICE_PARA);
#else
		DBG_INFO("\r\nbootloader build release %s %s!", __DATE__, __TIME__, QSPI->DEVICE_PARA);
#endif
		Jump_AppRun(__FLASH_APP_START_ADDR__ + AppInfo[1]);
	}
	else
	{
		goto LOCAL_UPGRADE_START;
	}

//	i = i / j; //只是测试一下backtrace功能

#endif
	NVIC_SystemReset();
}


