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
#include "LzmaLib.h"
extern const uint32_t __isr_start_address;
extern const uint32_t __os_heap_start;
extern const uint32_t __ram_end;
uint32_t SystemCoreClock;
#define FW_UPGRADE_START_TIME	300
#define FW_UPGRADE_ALL_TIME	2000
#define FW_UPGRADE_DATA_TIME 50
#define FW_OTA_FLASH_BUF_LEN	SPI_FLASH_BLOCK_SIZE
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
	uint32_t FWStartAddress;
	uint32_t FWTotalLen;
	uint32_t FWCRC32;
	uint32_t NextLzmaAddress;
//	uint32_t XferLen;
	uint8_t *NextLzmaData;
	uint32_t NextLzmaDataLen;
	uint8_t NextLzmaHeader[LZMA_PROPS_SIZE + 8];
	uint32_t NextLzmaHeaderSize;
//	uint32_t CurSPIFlashStart;
//	uint32_t NextSPIFlashStart;
//	uint8_t *CurSPIFlashData;
//	uint8_t *NextSPIFlashData;
	uint8_t State;
	uint8_t ForceOut;
	uint8_t SPIFlashReadDone;
}BL_CtrlStuct;

#define BL_DBG DBG_INFO
//#define BL_DBG(X, Y...)

typedef  void (*pFunction)(void);

static uint32_t CRC32_Table[256];
static BL_CtrlStuct prvBL;
static SPIFlash_CtrlStruct prvSPIFlash;

void Jump_AppRun(uint32_t Address)
{
  /* Jump to user application */
	pFunction Jump_To_Application;
	uint32_t JumpAddress;
	__disable_irq();
	DBG_INFO("jump to 0x%x !", Address);
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

static void BL_OTAErase(uint32_t Address, uint32_t Len)
{
	Flash_Erase(Address, Len);
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
	prvBL.NextLzmaAddress = 0;
//	prvBL.XferLen = 0;
	return 0;
}

int BL_DownloadAddData(uint32_t PacketSn, uint8_t *Data, uint32_t Len, uint32_t *NextPacketSn)
{
	uint32_t RestLen = prvBL.FWDataBuffer.MaxLen - prvBL.FWDataBuffer.Pos;
	if (prvBL.NextLzmaAddress != PacketSn)
	{
		*NextPacketSn = prvBL.NextLzmaAddress;
		return -1;
	}
	else
	{
//		prvBL.XferLen += Len;
		prvBL.NextLzmaAddress += Len;
		*NextPacketSn = prvBL.NextLzmaAddress;
		Buffer_StaticWrite(&prvBL.FWDataBuffer, Data, Len);
		if (prvBL.NextLzmaData)
		{
			return 60000;
		}
		else
		{
			return 1;
		}
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

uint8_t BL_RunLzmaBlock(uint8_t *Head, uint8_t HeadLen)
{
	if (!prvBL.FWDataBuffer.Pos) return ERROR_NONE;
	if (!prvBL.NextLzmaData)
	{
		memcpy(prvBL.NextLzmaHeader, Head, HeadLen);
		prvBL.NextLzmaHeaderSize = HeadLen;
		prvBL.NextLzmaData = prvBL.FWDataBuffer.Data;
		prvBL.NextLzmaDataLen = prvBL.FWDataBuffer.Pos;
		OS_InitBuffer(&prvBL.FWDataBuffer, __FLASH_BLOCK_SIZE__);
		prvBL.NextLzmaAddress = 0;
		return ERROR_NONE;
	}
	else
	{
		return ERROR_BUFFER_FULL;
	}
}

void Local_Upgrade(void)
{
	DBuffer_Struct DBuf;
	uint32_t FinishLen = 0;
	uint32_t ProgramLen, CRC32, i;
	uint64_t AllToTick = GetSysTick() + FW_UPGRADE_START_TIME * CORE_TICK_1MS;
	uint64_t DataToTick = GetSysTick() + FW_UPGRADE_DATA_TIME * CORE_TICK_1MS;
	uint8_t ConnectCnt = 0;
	uint8_t ConnectOK = 0;
	uint8_t *CurLzmaData = NULL;
	uint32_t CurLzmaDataLen;
	uint8_t CurLzmaHead[LZMA_PROPS_SIZE + 8];
	uint8_t CurLzmaHeadLen;
	DBuffer_Init(&DBuf, __FLASH_BLOCK_SIZE__);
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
			BL_OTAErase(__FLASH_OTA_INFO_ADDR__, __FLASH_SECTOR_SIZE__);
			AllToTick = GetSysTick() + FW_UPGRADE_ALL_TIME * CORE_TICK_1MS;
			prvBL.State = FW_UPGRADE_STATE_RUN;
			uint8_t *CurLzmaData = NULL;
			prvBL.NextLzmaData = NULL;
//			prvBL.XferLen = 0;
			FinishLen = 0;
			ConnectOK = 1;
			break;
		case FW_UPGRADE_STATE_RUN:
		case FW_UPGRADE_STATE_WAIT_END:
			if (DBuffer_GetDataLen(&DBuf, 0))
			{
				DBuffer_SwapCache(&DBuf);
				DBuffer_SetDataLen(&DBuf, 0, 0);
				goto PROGRAM_BLOCK;
			}
			if (prvBL.NextLzmaData)
			{
				CurLzmaData = prvBL.NextLzmaData;
				CurLzmaDataLen = prvBL.NextLzmaDataLen;
				memcpy(CurLzmaHead, prvBL.NextLzmaHeader, prvBL.NextLzmaHeaderSize);
				CurLzmaHeadLen = prvBL.NextLzmaHeaderSize;
				prvBL.NextLzmaData = NULL;
				if (!CurLzmaHead[0])
				{
					memcpy(DBuffer_GetCache(&DBuf, 1), CurLzmaData, CurLzmaDataLen);
					DBuffer_SetDataLen(&DBuf, CurLzmaDataLen, 1);
				}
				else
				{
					ProgramLen = __FLASH_BLOCK_SIZE__;
					LzmaUncompress(DBuffer_GetCache(&DBuf, 1), &ProgramLen, CurLzmaData, &CurLzmaDataLen, CurLzmaHead, CurLzmaHeadLen);
					DBuffer_SetDataLen(&DBuf, ProgramLen, 1);
				}
				free(CurLzmaData);
				CurLzmaData = NULL;

				goto PROGRAM_BLOCK;
			}
			goto PROGRAM_CHECK;
PROGRAM_BLOCK:
			WDT_Feed();
			if (memcmp(prvBL.FWStartAddress + FinishLen, DBuffer_GetCache(&DBuf, 1), DBuffer_GetDataLen(&DBuf, 1)))
			{
				Flash_EraseStart(prvBL.FWStartAddress + FinishLen, 1);
				while(Flash_CheckBusy())
				{
					if (prvBL.NextLzmaData && !DBuffer_GetDataLen(&DBuf, 0))
					{
						CurLzmaData = prvBL.NextLzmaData;
						CurLzmaDataLen = prvBL.NextLzmaDataLen;
						memcpy(CurLzmaHead, prvBL.NextLzmaHeader, prvBL.NextLzmaHeaderSize);
						CurLzmaHeadLen = prvBL.NextLzmaHeaderSize;
						prvBL.NextLzmaData = NULL;
						if (!CurLzmaHead[0])
						{
							memcpy(DBuffer_GetCache(&DBuf, 0), CurLzmaData, CurLzmaDataLen);
							DBuffer_SetDataLen(&DBuf, CurLzmaDataLen, 0);
						}
						else
						{
							ProgramLen = __FLASH_BLOCK_SIZE__;
							LzmaUncompress(DBuffer_GetCache(&DBuf, 0), &ProgramLen, CurLzmaData, &CurLzmaDataLen, CurLzmaHead, CurLzmaHeadLen);
							DBuffer_SetDataLen(&DBuf, ProgramLen, 0);
						}
						free(CurLzmaData);
						CurLzmaData = NULL;
					}
				}
				CACHE_CleanAll(CACHE);
				Flash_Program(prvBL.FWStartAddress + FinishLen, DBuffer_GetCache(&DBuf, 1), DBuffer_GetDataLen(&DBuf, 1));
			}
			FinishLen += DBuffer_GetDataLen(&DBuf, 1);
			DataToTick = GetSysTick() + FW_UPGRADE_DATA_TIME * CORE_TICK_1MS;
			AllToTick = GetSysTick() + FW_UPGRADE_ALL_TIME * CORE_TICK_1MS;
PROGRAM_CHECK:
			if (FinishLen >= prvBL.FWTotalLen)
			{
				CACHE_CleanAll(CACHE);
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
	if (ConnectOK)
	{
		BL_OTAErase(__FLASH_OTA_INFO_ADDR__, __FLASH_SECTOR_SIZE__);
	}
	DBuffer_DeInit(&DBuf);
}

static int32_t BL_OTAReadDataInFlash(void *pData, void *pParam)
{
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
	memcpy(pBuffer->Data, pData, pBuffer->Pos);
	return 0;
}

static int32_t BL_OTAReadDataInSpiFlash(void *pData, void *pParam)
{
	uint32_t StartAddress = (uint32_t)pData;
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
	while(!SPIFlash_WaitOpDone(&prvSPIFlash))
	{
		;
	}
	SPIFlash_Read(&prvSPIFlash, StartAddress, pBuffer->Data, pBuffer->Pos, 1);
	while(!SPIFlash_WaitOpDone(&prvSPIFlash))
	{
		;
	}
	return 0;
}

static int32_t BL_OTAReadDataInFS(void *pData, void *pParam)
{
	Buffer_Struct *pBuffer = (Buffer_Struct *)pParam;
}


static void BL_SpiInit(uint8_t SpiID, uint8_t *Pin)
{
	switch(SpiID)
	{
	case HSPI_ID0:
        GPIO_Iomux(GPIOC_12,3);
    	GPIO_Iomux(GPIOC_13,3);
    	GPIO_Iomux(GPIOC_15,3);
    	SYSCTRL->CG_CTRL1 |= SYSCTRL_APBPeriph_HSPI;
		break;
	case SPI_ID0:
		switch(Pin[0])
		{
		case GPIOB_14:
			GPIO_Iomux(GPIOB_14,0);
			break;
		case GPIOC_14:
			GPIO_Iomux(GPIOC_14,2);
			break;
		}
		switch(Pin[1])
		{
		case GPIOB_15:
			GPIO_Iomux(GPIOB_15,0);
			break;
		case GPIOC_15:
			GPIO_Iomux(GPIOC_15,2);
			break;
		}
		switch(Pin[2])
		{
		case GPIOB_12:
			GPIO_Iomux(GPIOB_12,0);
			break;
		case GPIOC_12:
			GPIO_Iomux(GPIOC_12,2);
			break;
		}
		SYSCTRL->CG_CTRL1 |= SYSCTRL_APBPeriph_SPI0;
		break;
	case SPI_ID1:
	    GPIO_Iomux(GPIOA_06,3);
	    GPIO_Iomux(GPIOA_08,3);
	    GPIO_Iomux(GPIOA_09,3);
	    SYSCTRL->CG_CTRL1 |= SYSCTRL_APBPeriph_SPI1;
		break;
	case SPI_ID2:
	    GPIO_Iomux(GPIOB_02,0);
	    GPIO_Iomux(GPIOB_04,0);
	    GPIO_Iomux(GPIOB_05,0);
	    SYSCTRL->CG_CTRL1 |= SYSCTRL_APBPeriph_SPI2;
		break;
	}
}

void Remote_Upgrade(void)
{
	CoreUpgrade_SectorStruct Sector;
	CoreUpgrade_HeadStruct Head;
	Buffer_Struct ReadBuffer;
	CBFuncEx_t pReadFunc;
	PV_Union uPV, uPin;
	uint32_t Check;
	uint32_t DoneLen, ReadLen;
	volatile uint32_t ProgramPos, FlashPos;
	int32_t Result;
	uint8_t Reboot = 0;
	uint32_t LzmaDataLen;
	uint8_t LzmaHead[LZMA_PROPS_SIZE + 8];
	uint8_t LzmaHeadLen;
	OS_InitBuffer(&ReadBuffer, FW_OTA_FLASH_BUF_LEN);

	memcpy(&Head, __FLASH_OTA_INFO_ADDR__, sizeof(CoreUpgrade_HeadStruct));
	Check = CRC32_Cal(CRC32_Table, &Head.Param1, sizeof(Head) - 8, 0xffffffff);
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
	uPV.u32 = Head.Param1;
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
	case CORE_OTA_OUT_SPI_FLASH:
		memset(&prvSPIFlash, 0, sizeof(prvSPIFlash));
		prvSPIFlash.SpiID = uPV.u8[2];
		uPin.u32 = Head.Param2;
		BL_SpiInit(prvSPIFlash.SpiID, uPin.u8);
		prvSPIFlash.CSPin = uPin.u8[3];
	    GPIO_Config(prvSPIFlash.CSPin, 0, 1);
	    SPI_MasterInit(prvSPIFlash.SpiID, 8, SPI_MODE_0, 24000000, NULL, NULL);
	    prvSPIFlash.IsBlockMode = 1;
	    prvSPIFlash.IsSpiDMAMode = 1;
		SPI_DMATxInit(prvSPIFlash.SpiID, FLASH_SPI_TX_DMA_STREAM, 0);
		SPI_DMARxInit(prvSPIFlash.SpiID, FLASH_SPI_RX_DMA_STREAM, 0);
		SPIFlash_Init(&prvSPIFlash, NULL);
		pReadFunc = BL_OTAReadDataInSpiFlash;
		DoneLen = 0;
		Check = 0xffffffff;
		while(DoneLen < Head.DataLen)
		{
			if ((Head.DataLen - DoneLen) < SPI_FLASH_BLOCK_SIZE)
			{
				SPIFlash_Read(&prvSPIFlash, Head.DataStartAddress + DoneLen, prvBL.FWDataBuffer.Data, (Head.DataLen - DoneLen), 0);
				Check = CRC32_Cal(CRC32_Table, prvBL.FWDataBuffer.Data, (Head.DataLen - DoneLen), Check);
				DoneLen = Head.DataLen;
			}
			else
			{
				SPIFlash_Read(&prvSPIFlash, Head.DataStartAddress + DoneLen, prvBL.FWDataBuffer.Data, SPI_FLASH_BLOCK_SIZE, 0);
				Check = CRC32_Cal(CRC32_Table, prvBL.FWDataBuffer.Data, SPI_FLASH_BLOCK_SIZE, Check);
				DoneLen += SPI_FLASH_BLOCK_SIZE;
			}
		}
		if (Check != Head.DataCRC32)
		{
			DBG_INFO("ota file CRC32: %x,%x", Check, Head.DataCRC32);
			goto OTA_END;
		}
		break;
	default:
		DBG_INFO("core ota storage mode %u not support", uPV.u8[1]);
		return;
		break;
	}

	DoneLen = 0;

	while(DoneLen < Head.DataLen)
	{
		ReadBuffer.Pos = sizeof(CoreUpgrade_SectorStruct);

		Result = pReadFunc(Head.DataStartAddress + DoneLen, &ReadBuffer);
		DoneLen += sizeof(CoreUpgrade_SectorStruct);
		memcpy(&Sector, ReadBuffer.Data, sizeof(CoreUpgrade_SectorStruct));
		if (Sector.MaigcNum != __APP_START_MAGIC__)
		{
			DBG_INFO("ota sector info error %x", Sector.MaigcNum);
			goto OTA_END;
		}
		ProgramPos = Sector.StartAddress;
		FlashPos = DoneLen + Head.DataStartAddress;
		DoneLen += Sector.TotalLen;
//		CRC32_CreateTable(CRC32_Table, CRC32_GEN);
		if (Sector.DataLen && (Sector.DataLen != Sector.TotalLen))
		{
			ReadLen = 0;
			while(ReadLen < Sector.TotalLen)
			{
				DBG_INFO("ota %x,%x,%u,%u", ProgramPos, FlashPos, ReadLen, Sector.TotalLen);
				Flash_EraseStart(ProgramPos, 1);
				ReadBuffer.Pos = 1;
				pReadFunc(FlashPos, &ReadBuffer);
				FlashPos += ReadBuffer.Pos;
				ReadLen += ReadBuffer.Pos;
				LzmaHeadLen = ReadBuffer.Data[0];
				if (LzmaHeadLen > sizeof(LzmaHead))
				{
					Reboot = 1;
					DBG_INFO("ota zip head error");
					goto OTA_END;
				}
				ReadBuffer.Pos = LzmaHeadLen + 4;
				pReadFunc(FlashPos, &ReadBuffer);
				FlashPos += ReadBuffer.Pos;
				ReadLen += ReadBuffer.Pos;
				memcpy(LzmaHead, ReadBuffer.Data, LzmaHeadLen);
				memcpy(&LzmaDataLen, ReadBuffer.Data + LzmaHeadLen, 4);
				ReadBuffer.Pos = LzmaDataLen;
				pReadFunc(FlashPos, &ReadBuffer);
				FlashPos += ReadBuffer.Pos;
				ReadLen += ReadBuffer.Pos;

				prvBL.FWDataBuffer.Pos = __FLASH_BLOCK_SIZE__;
				Result = LzmaUncompress(prvBL.FWDataBuffer.Data, &prvBL.FWDataBuffer.Pos, ReadBuffer.Data, &LzmaDataLen, LzmaHead, LzmaHeadLen);
				while(Flash_CheckBusy()){;}
				Flash_Program(ProgramPos, prvBL.FWDataBuffer.Data, prvBL.FWDataBuffer.Pos);
				WDT_Feed();
				ProgramPos += __FLASH_BLOCK_SIZE__;

			}
			CACHE_CleanAll(CACHE);
			Check = CRC32_Cal(CRC32_Table, Sector.StartAddress, Sector.DataLen, 0xffffffff);
			DBG_INFO("%08x,%08x", Check, Sector.DataCRC32);
			if (Sector.DataCRC32 != Check)
			{
//				Reboot = 1;
				DBG_INFO("ota final check crc32 fail %x %x", Check, Sector.DataCRC32);
				goto OTA_END;
			}
		}
		else
		{
			DBG_INFO("fota file must zip file!!!");
			goto OTA_END;
		}






	}


OTA_END:
	if (Reboot)
	{
		NVIC_SystemReset();
		while(1){;}
	}
	else
	{
		BL_OTAErase(__FLASH_OTA_INFO_ADDR__, __FLASH_SECTOR_SIZE__);
	}

}

void SystemInit(void)
{
	SYSCTRL->LDO25_CR = (1 << 4);
#ifdef __RUN_IN_RAM__
	memcpy(__SRAM_BASE_ADDR__, &__isr_start_address, 1024);
	SCB->VTOR = __SRAM_BASE_ADDR__;
#else
	SCB->VTOR = (uint32_t)(&__isr_start_address);
#endif
//#ifdef __USE_XTL__
//	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_EXT | SYSCTRL_FREQ_SEL_XTAL_192Mhz;
//#else
//	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_INC | SYSCTRL_FREQ_SEL_XTAL_192Mhz;
//#endif
	SYSCTRL->FREQ_SEL = 0x20000000 | SYSCTRL_FREQ_SEL_HCLK_DIV_1_2 | (1 << 13) | SYSCTRL_FREQ_SEL_CLOCK_SOURCE_INC | SYSCTRL_FREQ_SEL_XTAL_192Mhz | SYSCTRL_FREQ_SEL_POWERMODE_CLOSE_CPU_MEM;
	WDT_SetTimeout(__WDT_TO_MS__);
	WDT_ModeConfig(WDT_Mode_Interrupt);
	WDT_Enable();
	SYSCTRL->PHER_CTRL &= ~BIT(20);	//打开PA8-9-10电源
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
	uint32_t prvHeapLen;
    prvHeapLen = (uint32_t)(&__ram_end) - (uint32_t)(&__os_heap_start);
	bpool((uint32_t)(&__os_heap_start), prvHeapLen);
	CRC32_CreateTable(CRC32_Table, CRC32_GEN);
	__NVIC_SetPriorityGrouping(7 - __NVIC_PRIO_BITS);//对于freeRTOS必须这样配置
	SystemCoreClockUpdate();
    CoreTick_Init();
	cm_backtrace_init(NULL, NULL, NULL);
	Uart_GlobalInit();
	DMA_GlobalInit();
	DBG_Init(0);
	FileSystem_Init();
	OS_InitBuffer(&prvBL.FWDataBuffer, SPI_FLASH_BLOCK_SIZE);
LOCAL_UPGRADE_START:
	Local_Upgrade();
	Uart_BaseInit(DBG_UART_ID, DBG_UART_BR, 0, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, NULL);
#ifndef __RAMRUN__
	Remote_Upgrade();
//	__disable_irq();
	BL_LockFlash();
	BL_EncryptCheck();
	memcpy(AppInfo, __FLASH_APP_START_ADDR__, sizeof(AppInfo));
	if (__APP_START_MAGIC__ == AppInfo[0])
	{
#ifdef __DEBUG__
		DBG_INFO("bootloader build debug %s %s %x!", __DATE__, __TIME__, QSPI->DEVICE_PARA);
#else
		DBG_INFO("bootloader build release %s %s!", __DATE__, __TIME__, QSPI->DEVICE_PARA);
#endif
		Jump_AppRun(AppInfo[1]);
	}
	else
	{
		goto LOCAL_UPGRADE_START;
	}

//	i = i / j; //只是测试一下backtrace功能

#endif
	NVIC_SystemReset();
}


