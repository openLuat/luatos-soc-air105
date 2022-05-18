#ifdef __BUILD_OS__
#include "app_inc.h"


#else
#include "bl_inc.h"
#endif

enum
{
	SPIFLASH_STATE_IDLE = 0,
	SPIFLASH_STATE_RUN,
	SPIFLASH_STATE_READ,
	SPIFLASH_STATE_ERASE,
	SPIFLASH_STATE_WRITE,
};

const uint8_t SPI_FLASH_VENDOR_DICT[] = {
	0xA1,//"FM"
	0xC8,//"GD"
	0x9D,//"ISSI"
	0xC2,//"KH"
	0xEF,//"WB"
	0x68,
};

const uint8_t SPI_FLASH_SIZE_DICT[] = {
	0x09,//"256Kbit"
	0x10,//"512Kbit"
	0x11,//"1Mbit"
	0x12,//"2Mbit"
	0x13,//"4Mbit"
	0x14,//"8Mbit"
	0x15,//"16Mbit"
	0x16,//"32Mbit"
	0x17,//"64Mbit"
	0x18,//"128Mbit"
};

#define SF_DBG	DBG_INFO

static void SPIFlash_CS(SPIFlash_CtrlStruct *Ctrl, uint8_t OnOff)
{
	GPIO_Output(Ctrl->CSPin, !OnOff);
	if (Ctrl->CSDelayUS)
	{
		SysTickDelay(Ctrl->CSDelayUS * CORE_TICK_1US);
	}
}

static void SPIFlash_SpiXfer(SPIFlash_CtrlStruct *Ctrl, uint8_t *TxData, uint8_t *RxData, uint32_t Len)
{
	SPIFlash_CS(Ctrl, 1);
	Ctrl->SPIError = 0;
	Ctrl->SPIEndTick = GetSysTick() + Len * CORE_TICK_1US + CORE_TICK_1MS;
	SPI_Transfer(Ctrl->SpiID, TxData, RxData, Len, Ctrl->IsSpiDMAMode);

}

static void SPIFlash_SpiBlockXfer(SPIFlash_CtrlStruct *Ctrl, uint8_t *TxData, uint8_t *RxData, uint32_t Len)
{
	SPIFlash_CS(Ctrl, 1);
	Ctrl->SPIError = 0;
	SPI_BlockTransfer(Ctrl->SpiID, TxData, RxData, Len);
	SPIFlash_CS(Ctrl, 0);

}

static int32_t SPIFlash_SpiIrqCB(void *pData, void *pParam)
{
	SPIFlash_CtrlStruct *Ctrl = (SPIFlash_CtrlStruct *)pParam;
	SPIFlash_CS(Ctrl, 0);
#ifdef __BUILD_OS__
	if (Ctrl->NotifyTask && Ctrl->IsBlockMode)
	{
		Task_SendEvent(Ctrl->NotifyTask, DEV_SPIFLASH_SPI_DONE, 0, 0, 0);
	}
#endif
}

static void SPIFlash_Wait(SPIFlash_CtrlStruct *Ctrl, uint32_t Tick)
{
#ifdef __BUILD_OS__
	OS_EVENT Event;
	if (Ctrl->NotifyTask && Ctrl->TaskCB)
	{
		Task_GetEvent(Ctrl->NotifyTask, 0xffffffff, &Event, Ctrl->TaskCB, Tick);
	}
	else
	{
		Task_DelayTick(Tick);
	}
#else
	SysTickDelay(Tick);
#endif
}

//用外部flash存储文件才需要开启
#if 0
static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    int ret = (SPIFlash_Read((SPIFlash_CtrlStruct *)cfg->context, (block + __APP_BAK_FLASH_SECTOR_NUM__ + __SCRIPT_FLASH_SECTOR_NUM__) * cfg->block_size + off, buffer, size, 0)) ? LFS_ERR_IO: LFS_ERR_OK;
	//DBG_Trace("block_device_read block %d off %d size %d ret %d", block, off, size, ret);
	return ret;
}

static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int ret =  (SPIFlash_Write((SPIFlash_CtrlStruct *)cfg->context, (block + __APP_BAK_FLASH_SECTOR_NUM__ + __SCRIPT_FLASH_SECTOR_NUM__) * cfg->block_size + off, buffer, size)) ? LFS_ERR_IO: LFS_ERR_OK;
	//DBG_Trace("block_device_prog block %d off %d size %d ret", block, off, size, ret);
	return ret;
}

static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    int ret =  (SPIFlash_Erase((SPIFlash_CtrlStruct *)cfg->context, (block + __APP_BAK_FLASH_SECTOR_NUM__ + __SCRIPT_FLASH_SECTOR_NUM__) * cfg->block_size, cfg->block_size)) ? LFS_ERR_IO: LFS_ERR_OK;
	//DBG_Trace("block_device_erase block %d ret %d", block, ret);
	//DDBG_TraceBG("block_device_erase addr %d size %d", (block + __APP_BAK_FLASH_SECTOR_NUM__ + __SCRIPT_FLASH_SECTOR_NUM__) * cfg->block_size, cfg->block_size);
	return ret;
}

static int block_device_sync(const struct lfs_config *cfg)
{
	//DBG_Trace("block_device_sync");
    return 0;
}

#define LFS_BLOCK_DEVICE_READ_SIZE      (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (SPIFLASH_PAGE_LEN)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (16)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (256)

//__attribute__ ((aligned (8))) static char lfs_cache_buff[LFS_BLOCK_DEVICE_CACHE_SIZE];
__attribute__ ((aligned (8))) static char lfs_read_buff[LFS_BLOCK_DEVICE_READ_SIZE];
__attribute__ ((aligned (8))) static char lfs_prog_buff[LFS_BLOCK_DEVICE_PROG_SIZE];
__attribute__ ((aligned (8))) static char lfs_lookahead_buff[16];
#endif

void SPIFlash_Init(SPIFlash_CtrlStruct *Ctrl, void *LFSConfig)
{
	Ctrl->IsInit = 0;
	Ctrl->State = SPIFLASH_STATE_IDLE;
	if (SPIFlash_ID(Ctrl) != ERROR_NONE)
	{
		return;
	}
	Ctrl->IsInit = 1;
//用外部flash存储文件才需要开启
#if 0
	struct lfs_config *config = LFSConfig;
	//SF_DBG("ID:%02x,%02x,%02x,Size:%uKB", Ctrl->FlashID[0], Ctrl->FlashID[1], Ctrl->FlashID[2], Ctrl->Size);
	config->context = Ctrl;
	config->cache_size = LFS_BLOCK_DEVICE_READ_SIZE;
	config->prog_size = SPIFLASH_PAGE_LEN;
	config->block_size = SPIFLASH_SECTOR_LEN;
	config->read_size = LFS_BLOCK_DEVICE_READ_SIZE;
	config->block_cycles = 200;
	config->lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD;
	//config->block_count = (Ctrl->Size / 4) - __APP_BAK_FLASH_SECTOR_NUM__ - __SCRIPT_FLASH_SECTOR_NUM__;
	config->block_count = 336 / 4 ; // TODO fixed as Air302, for now, modify later.
	config->name_max = 63;
	config->read = block_device_read;
	config->prog = block_device_prog;
	config->erase = block_device_erase;
	config->sync  = block_device_sync;

	//config->buffer = (void*)lfs_cache_buff;
	config->read_buffer = (void*)lfs_read_buff;
	config->prog_buffer = (void*)lfs_prog_buff;
	config->lookahead_buffer = (void*)lfs_lookahead_buff;

#endif
	SF_DBG("ID:%02x,%02x,%02x,Size:%uKB", Ctrl->FlashID[0], Ctrl->FlashID[1], Ctrl->FlashID[2], Ctrl->Size);
	Ctrl->EraseSectorWaitTime = 50;
	Ctrl->EraseBlockWaitTime = 250;
	Ctrl->ProgramWaitTimeUS = 300;
	Ctrl->ProgramTime = 10;
	Ctrl->EraseSectorTime = 400;
	Ctrl->EraseBlockTime = 2400;
	SPIFlash_ReadSR(Ctrl);
	if (Ctrl->FlashSR != 0xff && Ctrl->FlashSR != 0x0)
	{
		if (!Ctrl->SPIError)
		{
			SPIFlash_WriteEnable(Ctrl);
			SPIFlash_WriteSR(Ctrl, 0);
			SPIFlash_ReadSR(Ctrl);
			while(Ctrl->FlashSR & SPIFLASH_SR_BUSY)
			{
				SPIFlash_ReadSR(Ctrl);
			}

		}
	}


}

int32_t SPIFlash_ID(SPIFlash_CtrlStruct *Ctrl)
{
	uint32_t i,j;
	for(i = 0; i < 10; i++)
	{
		Ctrl->Tx[0] = SPIFLASH_CMD_RDID;
		SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 4);

		if (Ctrl->Rx[1] == 0xff || !Ctrl->Rx[1])
		{
			SPIFlash_Wait(Ctrl, 10 * CORE_TICK_1MS);
		}
		else
		{
			break;
		}
	}
	if (!Ctrl->SPIError)
	{
		memcpy(Ctrl->FlashID, Ctrl->Rx+1, 3);
		for (i = 0; i < sizeof(SPI_FLASH_VENDOR_DICT)/sizeof(uint8_t); i++)
		{
			//读取到正确的ID后，开始解除FLASH锁定
			if (Ctrl->FlashID[0] == SPI_FLASH_VENDOR_DICT[i])
			{
				Ctrl->Size = 32;
				for (j = 0; j < sizeof(SPI_FLASH_SIZE_DICT)/sizeof(uint8_t); j++)
				{
					if (Ctrl->FlashID[2] == SPI_FLASH_SIZE_DICT[j])
					{
						return ERROR_NONE;
					}
					Ctrl->Size *= 2;
				}
			}
		}
		return -ERROR_DEVICE_BUSY;


	}
	return ERROR_NONE;
}

int32_t SPIFlash_ReadSR(SPIFlash_CtrlStruct *Ctrl)
{
	Ctrl->FlashSR = 0xff;
	Ctrl->Tx[0] = SPIFLASH_CMD_RDSR;
	Ctrl->Rx[1] = 0xff;
	SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 2);
	if (!Ctrl->SPIError)
	{
		Ctrl->FlashSR = Ctrl->Rx[1];
	}
	else
	{
		DBGF;
		return -ERROR_OPERATION_FAILED;
	}
	//SF_DBG("%02x", Ctrl->FlashSR);
	return ERROR_NONE;
}

int32_t SPIFlash_CheckBusy(SPIFlash_CtrlStruct *Ctrl)
{
	int Result = SPIFlash_ReadSR(Ctrl);
	if (Result < 0)
	{
		return -ERROR_OPERATION_FAILED;
	}
	if (!Ctrl->SPIError && !(Ctrl->FlashSR & SPIFLASH_SR_BUSY))
	{
		//DBGF;
		Ctrl->IsPPErase = 0;
	}
	else
	{
		//DBGF;
	}
	return ERROR_NONE;
}

uint8_t SPIFlash_WaitOpDone(SPIFlash_CtrlStruct *Ctrl)
{
	uint32_t FlashAddress, DummyLen;
	switch(Ctrl->State)
	{
	case SPIFLASH_STATE_READ:
		if (SPI_IsTransferBusy(Ctrl->SpiID))
		{
			if (GetSysTick() > Ctrl->SPIEndTick)
			{
				SF_DBG("spi xfer timeout!");
				Ctrl->SPIError = 1;
				Ctrl->State = SPIFLASH_STATE_IDLE;
				SPI_TransferStop(Ctrl->SpiID);
				SPIFlash_CS(Ctrl, 0);
				return 1;
			}
		}
		else
		{
			Ctrl->State = SPIFLASH_STATE_IDLE;
			SPIFlash_CS(Ctrl, 0);
			return 1;
		}
		break;
	case SPIFLASH_STATE_ERASE:
		SPIFlash_CheckBusy(Ctrl);
		if (!Ctrl->IsPPErase)
		{
			if (Ctrl->FinishLen >= Ctrl->DataLen)
			{
				Ctrl->FlashError = 0;
				Ctrl->State = SPIFLASH_STATE_IDLE;
				return 1;
			}
			SPIFlash_WriteEnable(Ctrl);
			FlashAddress = Ctrl->AddrStart + Ctrl->FinishLen;
			DummyLen = Ctrl->DataLen - Ctrl->FinishLen;
			if (!(FlashAddress & SPI_FLASH_BLOCK_MASK) && (DummyLen >= SPI_FLASH_BLOCK_SIZE))
			{
				SF_DBG("erase block 0x%x", FlashAddress);
				Ctrl->Tx[0] = SPIFLASH_CMD_BE;
				Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->EraseBlockTime * CORE_TICK_1MS;
				Ctrl->FinishLen += SPI_FLASH_BLOCK_SIZE;
			}
			else
			{
				SF_DBG("erase sector 0x%x", FlashAddress);
				Ctrl->Tx[0] = SPIFLASH_CMD_SE;
				Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->EraseSectorTime * CORE_TICK_1MS;
				Ctrl->FinishLen += SPI_FLASH_SECTOR_SIZE;
			}
			Ctrl->Tx[1] = (FlashAddress & 0x00ff0000) >> 16;
			Ctrl->Tx[2] = (FlashAddress & 0x0000ff00) >> 8;
			Ctrl->Tx[3] = (FlashAddress & 0x000000ff);
			SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 4);
		}
		else
		{
			if (GetSysTick() >= Ctrl->FlashOPEndTick)
			{
				SF_DBG("erase timeout!");
				Ctrl->FlashError = 1;
				Ctrl->State = SPIFLASH_STATE_IDLE;
				return 1;
			}
		}
		break;
	case SPIFLASH_STATE_WRITE:
		if (SPI_IsTransferBusy(Ctrl->SpiID))
		{
			if (GetSysTick() > Ctrl->SPIEndTick)
			{
				SF_DBG("spi xfer timeout!");
				Ctrl->SPIError = 1;
				Ctrl->FlashError = 1;
				Ctrl->State = SPIFLASH_STATE_IDLE;
				SPI_TransferStop(Ctrl->SpiID);
				SPIFlash_CS(Ctrl, 0);
				return 1;
			}
		}
		else
		{

			SPIFlash_CheckBusy(Ctrl);
			if (!Ctrl->IsPPErase)
			{
				if (Ctrl->FinishLen >= Ctrl->DataLen)
				{
					Ctrl->FlashError = 0;
					Ctrl->State = SPIFLASH_STATE_IDLE;
					return 1;
				}
				SPIFlash_WriteEnable(Ctrl);
				FlashAddress = Ctrl->AddrStart + Ctrl->FinishLen;
				DummyLen = Ctrl->DataLen - Ctrl->FinishLen;
				SF_DBG("Program 0x%x", FlashAddress);
				Ctrl->Tx[0] = SPIFLASH_CMD_PP;
				Ctrl->Tx[1] = (FlashAddress & 0x00ff0000) >> 16;
				Ctrl->Tx[2] = (FlashAddress & 0x0000ff00) >> 8;
				Ctrl->Tx[3] = (FlashAddress & 0x000000ff);
				if (DummyLen > SPIFLASH_PAGE_LEN)
				{
					DummyLen = SPIFLASH_PAGE_LEN;
				}
				memcpy(Ctrl->Tx + 4, Ctrl->TxBuf + Ctrl->FinishLen, DummyLen);
				SPIFlash_CS(Ctrl, 1);
				Ctrl->SPIError = 0;
				SPI_Transfer(Ctrl->SpiID, Ctrl->Tx, Ctrl->Rx, DummyLen + 4, Ctrl->IsSpiDMAMode);
				Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->ProgramTime * CORE_TICK_1MS;
				Ctrl->SPIEndTick = GetSysTick() + CORE_TICK_1MS;
				Ctrl->FinishLen += DummyLen;
			}
			else
			{
				if (GetSysTick() >= Ctrl->FlashOPEndTick)
				{
					SF_DBG("Program timeout!");
					Ctrl->FlashError = 1;
					Ctrl->State = SPIFLASH_STATE_IDLE;
					return 1;
				}
			}
		}
		break;
	default:
		Ctrl->FlashError = 1;
		Ctrl->State = SPIFLASH_STATE_IDLE;
		return 1;
	}
	return 0;
}

int32_t SPIFlash_WriteEnable(SPIFlash_CtrlStruct *Ctrl)
{
	Ctrl->Tx[0] = SPIFLASH_CMD_WREN;
	SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 1);
	return 0;
}

int32_t SPIFlash_WriteSR(SPIFlash_CtrlStruct *Ctrl, uint8_t SR)
{
	Ctrl->Tx[0] = SPIFLASH_CMD_WRSR;
	Ctrl->Tx[1] = SR;
	SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 2);
	return 0;
}

int32_t SPIFlash_Read(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, uint8_t *Buf, uint32_t Length, uint8_t FastRead)
{
#ifdef __BUILD_OS__
	OS_EVENT Event;
#endif
	Ctrl->FlashError = 0;
	Ctrl->Tx[1] = (Address & 0x00ff0000) >> 16;
	Ctrl->Tx[2] = (Address & 0x0000ff00) >> 8;
	Ctrl->Tx[3] = (Address & 0x000000ff);
	SPIFlash_CS(Ctrl, 1);
	Ctrl->SPIError = 0;

	switch (FastRead)
	{
	case 0:
		Ctrl->Tx[0] = SPIFLASH_CMD_READ;
		SPI_BlockTransfer(Ctrl->SpiID, Ctrl->Tx, Ctrl->Rx, 4);

		break;
	default:
		Ctrl->Tx[0] = SPIFLASH_CMD_FTRD;
		SPI_BlockTransfer(Ctrl->SpiID, Ctrl->Tx, Ctrl->Rx, 5);
		break;
	}


	if (Ctrl->IsBlockMode)
	{
#ifdef __BUILD_OS__
		if (Ctrl->NotifyTask && Ctrl->TaskCB)
		{
			Ctrl->SPIError = 0;
			SPI_SetCallbackFun(Ctrl->SpiID, SPIFlash_SpiIrqCB, Ctrl);
			SPI_Transfer(Ctrl->SpiID, Buf, Buf, Length, Ctrl->IsSpiDMAMode);
			if (Task_GetEvent(Ctrl->NotifyTask, DEV_SPIFLASH_SPI_DONE, &Event, Ctrl->TaskCB, Length * CORE_TICK_1US + CORE_TICK_1MS))
			{
				Ctrl->SPIError = 1;
				SPI_TransferStop(Ctrl->SpiID);
			}
			SPI_SetCallbackFun(Ctrl->SpiID, NULL, NULL);
			SPIFlash_CS(Ctrl, 0);
		}
		else
#endif
		{
			SPI_BlockTransfer(Ctrl->SpiID, Buf, Buf, Length);
			SPIFlash_CS(Ctrl, 0);
		}

	}
	else
	{
		Ctrl->State = SPIFLASH_STATE_READ;
		SPIFlash_CS(Ctrl, 1);
		Ctrl->SPIError = 0;
		SPI_Transfer(Ctrl->SpiID, Buf, Buf, Length, Ctrl->IsSpiDMAMode);
		Ctrl->SPIEndTick = GetSysTick() + Length * CORE_TICK_1US + CORE_TICK_1MS;
	}
	return Ctrl->FlashError;
}

int32_t SPIFlash_Write(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, const uint8_t *Buf, uint32_t Length)
{
#ifdef __BUILD_OS__
	OS_EVENT Event;
#endif
	uint32_t FlashAddress, DummyLen;
	Ctrl->FlashError = 0;
	Ctrl->AddrStart = Address;
	Ctrl->TxBuf = Buf;
	Ctrl->DataLen = Length;
	Ctrl->FinishLen = 0;
	Ctrl->SPIError = 0;
//	SF_DBG("%x,%u", Address, Length);

	if (Ctrl->IsBlockMode)
	{
		while((Ctrl->FinishLen < Ctrl->DataLen) && !Ctrl->SPIError)
		{
			FlashAddress = Ctrl->AddrStart + Ctrl->FinishLen;
			DummyLen = Ctrl->DataLen - Ctrl->FinishLen;
			SPIFlash_WriteEnable(Ctrl);
			Ctrl->Tx[0] = SPIFLASH_CMD_PP;
			Ctrl->Tx[1] = (FlashAddress & 0x00ff0000) >> 16;
			Ctrl->Tx[2] = (FlashAddress & 0x0000ff00) >> 8;
			Ctrl->Tx[3] = (FlashAddress & 0x000000ff);
			if (DummyLen > SPIFLASH_PAGE_LEN)
			{
				DummyLen = SPIFLASH_PAGE_LEN;
			}
			memcpy(Ctrl->Tx + 4, Ctrl->TxBuf + Ctrl->FinishLen, DummyLen);
#ifdef __BUILD_OS__
			if (Ctrl->NotifyTask && Ctrl->TaskCB)
			{
				SPIFlash_CS(Ctrl, 1);
				Ctrl->SPIError = 0;
				SPI_SetCallbackFun(Ctrl->SpiID, SPIFlash_SpiIrqCB, Ctrl);
				SPI_Transfer(Ctrl->SpiID, Ctrl->Tx, Ctrl->Rx, DummyLen + 4, Ctrl->IsSpiDMAMode);
				if (Task_GetEvent(Ctrl->NotifyTask, DEV_SPIFLASH_SPI_DONE, &Event, Ctrl->TaskCB, CORE_TICK_1MS))
				{
					Ctrl->SPIError = 1;
					SPI_TransferStop(Ctrl->SpiID);
				}
				SPI_SetCallbackFun(Ctrl->SpiID, NULL, NULL);
				SPIFlash_CS(Ctrl, 0);
			}
			else
#endif
			{
				SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, DummyLen + 4);
			}
			Ctrl->FinishLen += DummyLen;
			if (Ctrl->SPIError)
			{
				break;
			}
			Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->ProgramTime * CORE_TICK_1MS;
			Ctrl->IsPPErase = 1;
			while (Ctrl->IsPPErase && (GetSysTick() < Ctrl->FlashOPEndTick))
			{
				SPIFlash_Wait(Ctrl, Ctrl->ProgramWaitTimeUS * CORE_TICK_1US);
				SPIFlash_CheckBusy(Ctrl);
			}
			if (Ctrl->IsPPErase)
			{
				DBGF;
				Ctrl->FlashError = 1;
				return -ERROR_OPERATION_FAILED;
			}

		}
		//SF_DBG("write ok!")
		if (Ctrl->SPIError)
		{
			DBGF;
			Ctrl->FlashError = 1;
			return -ERROR_OPERATION_FAILED;
		}
	}
	else
	{
		Ctrl->State = SPIFLASH_STATE_WRITE;
	}
	return Ctrl->FlashError;
}

int32_t SPIFlash_Erase(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, uint32_t Length)
{
	uint32_t FlashAddress, DummyLen, Tick;
	Ctrl->FlashError = 0;
	Ctrl->AddrStart = Address;
	Ctrl->DataLen = Length;
	Ctrl->FinishLen = 0;

	if (Ctrl->IsBlockMode)
	{
		while(Ctrl->FinishLen < Ctrl->DataLen)
	    {
			SPIFlash_WriteEnable(Ctrl);
			FlashAddress = Ctrl->AddrStart + Ctrl->FinishLen;
			DummyLen = Ctrl->DataLen - Ctrl->FinishLen;
			if (!(FlashAddress & SPI_FLASH_BLOCK_MASK) && (DummyLen >= SPI_FLASH_BLOCK_SIZE))
			{
//				SF_DBG("erase block 0x%x", FlashAddress);
				Ctrl->Tx[0] = SPIFLASH_CMD_BE;
				Tick = Ctrl->EraseBlockWaitTime * CORE_TICK_1MS;
				Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->EraseBlockTime * CORE_TICK_1MS;
				Ctrl->FinishLen += SPI_FLASH_BLOCK_SIZE;
			}
			else
			{
//				SF_DBG("erase sector 0x%x", FlashAddress);
				Ctrl->Tx[0] = SPIFLASH_CMD_SE;
				Tick = Ctrl->EraseSectorWaitTime * CORE_TICK_1MS;
				Ctrl->FlashOPEndTick = GetSysTick() + Ctrl->EraseSectorTime * CORE_TICK_1MS;
				Ctrl->FinishLen += SPI_FLASH_SECTOR_SIZE;
			}
			Ctrl->Tx[1] = (FlashAddress & 0x00ff0000) >> 16;
			Ctrl->Tx[2] = (FlashAddress & 0x0000ff00) >> 8;
			Ctrl->Tx[3] = (FlashAddress & 0x000000ff);
			SPIFlash_SpiBlockXfer(Ctrl, Ctrl->Tx, Ctrl->Rx, 4);
			Ctrl->IsPPErase = 1;

			while (Ctrl->IsPPErase && (GetSysTick() < Ctrl->FlashOPEndTick))
			{
				SPIFlash_Wait(Ctrl, Tick);
				SPIFlash_CheckBusy(Ctrl);
			}
			if (Ctrl->IsPPErase)
			{
				Ctrl->FlashError = 1;
				return -ERROR_OPERATION_FAILED;
			}
	    }
	}
	else
	{
		Ctrl->State = SPIFLASH_STATE_ERASE;
	}
	return Ctrl->FlashError;
}
