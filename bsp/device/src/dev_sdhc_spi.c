#include "user.h"
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define CMD2	(2)
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

#define SD_CMD_GO_IDLE_STATE          0   /* CMD0 = 0x40  */
#define SD_CMD_SEND_OP_COND           1   /* CMD1 = 0x41  */
#define SD_CMD_SEND_IF_COND           8   /* CMD8 = 0x48  */
#define SD_CMD_SEND_CSD               9   /* CMD9 = 0x49  */
#define SD_CMD_SEND_CID               10  /* CMD10 = 0x4A */
#define SD_CMD_STOP_TRANSMISSION      12  /* CMD12 = 0x4C */
#define SD_CMD_SEND_STATUS            13  /* CMD13 = 0x4D */
#define SD_CMD_SET_BLOCKLEN           16  /* CMD16 = 0x50 */
#define SD_CMD_READ_SINGLE_BLOCK      17  /* CMD17 = 0x51 */
#define SD_CMD_READ_MULT_BLOCK        18  /* CMD18 = 0x52 */
#define SD_CMD_SET_BLOCK_COUNT        23  /* CMD23 = 0x57 */
#define SD_CMD_WRITE_SINGLE_BLOCK     24  /* CMD24 = 0x58 */
#define SD_CMD_WRITE_MULT_BLOCK       25  /* CMD25 = 0x59 */
#define SD_CMD_PROG_CSD               27  /* CMD27 = 0x5B */
#define SD_CMD_SET_WRITE_PROT         28  /* CMD28 = 0x5C */
#define SD_CMD_CLR_WRITE_PROT         29  /* CMD29 = 0x5D */
#define SD_CMD_SEND_WRITE_PROT        30  /* CMD30 = 0x5E */
#define SD_CMD_SD_ERASE_GRP_START     32  /* CMD32 = 0x60 */
#define SD_CMD_SD_ERASE_GRP_END       33  /* CMD33 = 0x61 */
#define SD_CMD_UNTAG_SECTOR           34  /* CMD34 = 0x62 */
#define SD_CMD_ERASE_GRP_START        35  /* CMD35 = 0x63 */
#define SD_CMD_ERASE_GRP_END          36  /* CMD36 = 0x64 */
#define SD_CMD_UNTAG_ERASE_GROUP      37  /* CMD37 = 0x65 */
#define SD_CMD_ERASE                  38  /* CMD38 = 0x66 */
#define SD_CMD_SD_APP_OP_COND         41  /* CMD41 = 0x69 */
#define SD_CMD_APP_CMD                55  /* CMD55 = 0x77 */
#define SD_CMD_READ_OCR               58  /* CMD55 = 0x79 */
#define SD_DEFAULT_BLOCK_SIZE (512)

#define SD_DBG	DBG_INFO
enum
{
	DEV_SDCARD_SPI_DONE = SERVICE_EVENT_ID_START + 1,
	SDCARD_STATE_IDLE = 0,
	SDCARD_STATE_RUN,
	SDCARD_STATE_READ,
	SDCARD_STATE_WRITE,
};

static HANDLE prvRWMutex;

static const uint8_t prvSDHC_StandardInquiryData[STANDARD_INQUIRY_DATA_LEN] =
{
		0x00, //磁盘设备
		0x80, //其中最高位D7为RMB。RMB=0，表示不可移除设备。如果RMB=1，则为可移除设备。
		0x02, //各种版本号
		0x02, //数据响应格式 0x02
		0x1F, //附加数据长度，为31字节
		0x00, //保留
		0x00, //保留
		0x00, //保留
		'A','I','R','M','2','M',' ',' ',
		'L','U','A','T','O','S','-','S','O','C',' ','U','S','B',' ',' ',
		'1','.','0','0'
};

extern const uint8_t prvCore_MSCPage00InquiryData[LENGTH_INQUIRY_PAGE00];
/* USB Mass storage VPD Page 0x80 Inquiry Data for Unit Serial Number */
extern const uint8_t prvCore_MSCPage80InquiryData[LENGTH_INQUIRY_PAGE80];
/* USB Mass storage sense 6 Data */
extern const uint8_t prvCore_MSCModeSense6data[MODE_SENSE6_LEN];
/* USB Mass storage sense 10  Data */
extern const uint8_t prvCore_MSCModeSense10data[MODE_SENSE10_LEN];

static int32_t prvSDHC_SCSIInit(uint8_t LUN, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	if (!SDHC_IsReady(pSDHC))
	{
		SDHC_SpiInitCard(pSDHC);
	}
	return SDHC_IsReady(pSDHC)?ERROR_NONE:-ERROR_OPERATION_FAILED;
}

static int32_t prvSDHC_SCSIGetCapacity(uint8_t LUN, uint32_t *BlockNum, uint32_t *BlockSize, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	if (!pSDHC->Info.LogBlockSize || !pSDHC->Info.LogBlockNbr)
	{
		SDHC_SpiReadCardConfig(pSDHC);
	}
	*BlockSize = pSDHC->Info.LogBlockSize;
	*BlockNum = pSDHC->Info.LogBlockNbr;
	return 0;
}

static int32_t prvSDHC_SCSIIsReady(uint8_t LUN, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	return SDHC_IsReady(pSDHC)?ERROR_NONE:-ERROR_OPERATION_FAILED;
}

static int32_t prvSDHC_SCSIIsWriteProtected(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvSDHC_SCSIReadNext(uint8_t LUN, void *pUserData);
static int32_t prvSDHC_SCSIPreRead(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	if ((BlockAddress + BlockNums) > pSDHC->Info.LogBlockNbr)
	{
		DBG("%u,%u,%u", BlockAddress, BlockNums, pSDHC->Info.LogBlockNbr);
		return -1;
	}
	pSDHC->CurBlock = BlockAddress;
	pSDHC->EndBlock = BlockAddress + BlockNums;
	if ((pSDHC->PreCurBlock == pSDHC->CurBlock) && pSDHC->PreEndBlock)
	{
//		DBG("%u,%u,%u", pSDHC->PreCurBlock, pSDHC->CurBlock, pSDHC->PreEndBlock);
		pSDHC->CurBlock = pSDHC->PreEndBlock;
		return 0;
	}
	prvSDHC_SCSIReadNext(LUN, pUserData);
	return 0;
}

static int32_t prvSDHC_SCSIRead(uint8_t LUN, uint32_t Len, uint8_t **pOutData, uint32_t *OutLen, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
#if 1
	*pOutData = DBuffer_GetCache(pSDHC->SCSIDataBuf, 1);
	*OutLen = DBuffer_GetDataLen(pSDHC->SCSIDataBuf, 1);
	if (*OutLen > Len)
	{
		*OutLen = Len;
	}
	DBuffer_SetDataLen(pSDHC->SCSIDataBuf, 0, 1);
	DBuffer_SwapCache(pSDHC->SCSIDataBuf);
	if (pSDHC->USBDelayTime)	//不得不降速
	{
		Task_DelayMS(pSDHC->USBDelayTime);
	}
#else
	uint32_t ReadBlocks;
	if (pSDHC->EndBlock <= pSDHC->CurBlock) return -1;

	if ( ((pSDHC->EndBlock - pSDHC->CurBlock) << 9) > pSDHC->SCSIDataBuf->MaxLen)
	{
		ReadBlocks = pSDHC->SCSIDataBuf->MaxLen >> 9;
	}
	else
	{
		ReadBlocks = pSDHC->EndBlock - pSDHC->CurBlock;
	}
	SDHC_SpiReadBlocks(pSDHC, DBuffer_GetCache(pSDHC->SCSIDataBuf, 1), pSDHC->CurBlock, ReadBlocks);
	pSDHC->CurBlock += ReadBlocks;
	*pOutData = DBuffer_GetCache(pSDHC->SCSIDataBuf, 1);
	*OutLen = ReadBlocks << 9;
#endif
	return SDHC_IsReady(pSDHC)?ERROR_NONE:-ERROR_OPERATION_FAILED;
}

static int32_t prvSDHC_SCSIReadNext(uint8_t LUN, void *pUserData)
{
#if 1
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	uint32_t ReadBlocks;
	if (pSDHC->EndBlock <= pSDHC->CurBlock)
	{
		ReadBlocks = pSDHC->SCSIDataBuf->MaxLen >> 9;
		pSDHC->PreCurBlock = pSDHC->EndBlock;
		SDHC_SpiReadBlocks(pSDHC, DBuffer_GetCache(pSDHC->SCSIDataBuf, 1), pSDHC->PreCurBlock, ReadBlocks);
		pSDHC->PreEndBlock = pSDHC->PreCurBlock + ReadBlocks;
		DBuffer_SetDataLen(pSDHC->SCSIDataBuf, pSDHC->SCSIDataBuf->MaxLen, 1);
		return ERROR_NONE;
	}

	if ( ((pSDHC->EndBlock - pSDHC->CurBlock) << 9) > pSDHC->SCSIDataBuf->MaxLen)
	{
		ReadBlocks = pSDHC->SCSIDataBuf->MaxLen >> 9;
	}
	else
	{
		ReadBlocks = pSDHC->EndBlock - pSDHC->CurBlock;
	}
	SDHC_SpiReadBlocks(pSDHC, DBuffer_GetCache(pSDHC->SCSIDataBuf, 1), pSDHC->CurBlock, ReadBlocks);
	pSDHC->CurBlock += ReadBlocks;
	DBuffer_SetDataLen(pSDHC->SCSIDataBuf, ReadBlocks << 9, 1);
#endif
	return ERROR_NONE;
}


static int32_t prvSDHC_SCSIPreWrite(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	if (!SDHC_IsReady(pSDHC)) return -1;
	if ((BlockAddress + BlockNums) > pSDHC->Info.LogBlockNbr)
	{
		return -1;
	}
	pSDHC->CurBlock = BlockAddress;
	pSDHC->EndBlock = BlockAddress + BlockNums;
	OS_ReInitBuffer(&pSDHC->CacheBuf, pSDHC->Info.LogBlockSize * BlockNums);

	return 0;
}

static int32_t prvSDHC_SCSIWrite(uint8_t LUN, uint8_t *Data, uint32_t Len, void *pUserData)
{
	SDHC_SPICtrlStruct *pSDHC = pUserData;
	void *pData;
	if (!SDHC_IsReady(pSDHC)) return -1;
	if (pSDHC->EndBlock <= pSDHC->CurBlock) return -1;
	OS_BufferWrite(&pSDHC->CacheBuf, Data, Len);
	if (pSDHC->CacheBuf.Pos >= pSDHC->CacheBuf.MaxLen)
	{
		pData = pSDHC->CacheBuf.Data;
		memset(&pSDHC->CacheBuf, 0, sizeof(pSDHC->CacheBuf));
		Core_WriteSDHC(pSDHC, pData);
	}
	return ERROR_NONE;
}

static int32_t prvSDHC_SCSIUserCmd(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	return -1;
}

static int32_t prvSDHC_SCSIGetMaxLUN(uint8_t *LUN, void *pUserData)
{
	*LUN = 0;
}

const USB_StorageSCSITypeDef prvSDHC_SCSIFun =
{
		prvSDHC_SCSIGetMaxLUN,
		prvSDHC_SCSIInit,
		prvSDHC_SCSIGetCapacity,
		prvSDHC_SCSIIsReady,
		prvSDHC_SCSIIsWriteProtected,
		prvSDHC_SCSIPreRead,
		prvSDHC_SCSIRead,
		prvSDHC_SCSIReadNext,
		prvSDHC_SCSIPreWrite,
		prvSDHC_SCSIWrite,
		prvSDHC_SCSIUserCmd,
		&prvSDHC_StandardInquiryData,
		&prvCore_MSCPage00InquiryData,
		&prvCore_MSCPage80InquiryData,
		&prvCore_MSCModeSense6data,
		&prvCore_MSCModeSense10data,
};

static void SDHC_SpiCS(SDHC_SPICtrlStruct *Ctrl, uint8_t OnOff)
{
	uint8_t Temp[1] = {0xff};
	GPIO_Output(Ctrl->CSPin, !OnOff);
	if (!OnOff)
	{
		SPI_BlockTransfer(Ctrl->SpiID, Temp, Temp, 1);
	}

}

static int32_t SPIFlash_SpiIrqCB(void *pData, void *pParam)
{
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pParam;
#ifdef __BUILD_OS__
	if (Ctrl->NotifyTask)
	{
		Task_SendEvent(Ctrl->NotifyTask, DEV_SDHC_SPI_DONE, 0, 0, 0);
	}
#endif
	return 0;
}

static uint8_t CRC7(uint8_t * chr, int cnt)
{
	int i,a;
	uint8_t crc,Data;
	crc=0;
	for (a=0;a<cnt;a++)
	{
	   Data=chr[a];
	   for (i=0;i<8;i++)
	   {
		crc <<= 1;

		if ((Data & 0x80)^(crc & 0x80))
		crc ^=0x09;
		Data <<= 1;
	   }
	}
	crc=(crc<<1)|1;
	return(crc);
}

static void SDHC_SpiXfer(SDHC_SPICtrlStruct *Ctrl, uint8_t *Buf, uint16_t TxLen)
{
#ifdef __BUILD_OS__
	OS_EVENT Event;
#endif
#ifdef __BUILD_OS__
	if (Ctrl->NotifyTask && Ctrl->TaskCB)
	{
		Ctrl->SPIError = 0;
		SPI_SetCallbackFun(Ctrl->SpiID, SPIFlash_SpiIrqCB, Ctrl);
		SPI_Transfer(Ctrl->SpiID, Buf, Buf, TxLen, Ctrl->IsSpiDMAMode);
		if (Task_GetEvent(Ctrl->NotifyTask, DEV_SDHC_SPI_DONE, &Event, Ctrl->TaskCB, TxLen * CORE_TICK_1US + CORE_TICK_1MS))
		{
			Ctrl->SPIError = 1;
			SPI_TransferStop(Ctrl->SpiID);
		}
		SPI_SetCallbackFun(Ctrl->SpiID, NULL, NULL);
	}
	else
#endif
	{
		SPI_BlockTransfer(Ctrl->SpiID, Buf, Buf, TxLen);
	}
}

static int32_t SDHC_SpiCmd(SDHC_SPICtrlStruct *Ctrl, uint8_t Cmd, uint32_t Arg, uint8_t NeedStop)
{
	uint64_t OpEndTick;
	uint8_t i, TxLen, DummyLen;
	int32_t Result = -ERROR_OPERATION_FAILED;
	SDHC_SpiCS(Ctrl, 1);
	Ctrl->TempData[0] = 0x40|Cmd;
	BytesPutBe32(Ctrl->TempData + 1, Arg);
	Ctrl->TempData[5] = CRC7(Ctrl->TempData, 5);

	memset(Ctrl->TempData + 6, 0xff, 8);
	TxLen = 14;

	if (Ctrl->IsPrintData)
	{
		DBG_HexPrintf(Ctrl->TempData, TxLen);
	}
	Ctrl->SPIError = 0;
	Ctrl->SDHCError = 0;
	SDHC_SpiXfer(Ctrl, Ctrl->TempData, TxLen);
	if (Ctrl->IsPrintData)
	{
		DBG_HexPrintf(Ctrl->TempData, TxLen);
	}
	for(i = 7; i < TxLen; i++)
	{
		if (Ctrl->TempData[i] != 0xff)
		{
			Ctrl->SDHCState = Ctrl->TempData[i];
			if ((Ctrl->SDHCState == !Ctrl->IsInitDone) || !Ctrl->SDHCState)
			{
				Result = ERROR_NONE;
			}
			DummyLen = TxLen - i - 1;
			memcpy(Ctrl->ExternResult, &Ctrl->TempData[i + 1], DummyLen);
			Ctrl->ExternLen = DummyLen;
			break;
		}
	}
	if (NeedStop)
	{
		SDHC_SpiCS(Ctrl, 0);
	}
	return Result;
}

static int32_t SDHC_SpiReadRegData(SDHC_SPICtrlStruct *Ctrl, uint8_t *RegDataBuf, uint8_t DataLen)
{
	uint64_t OpEndTick;
	int Result = ERROR_NONE;
	uint16_t DummyLen;
	uint16_t i,findResult;
	Ctrl->SPIError = 0;
	Ctrl->SDHCError = 0;
	OpEndTick = GetSysTick() + Ctrl->SDHCReadBlockTo;
	findResult = 0;
	for(i = 0; i < Ctrl->ExternLen; i++)
	{
		if (0xfe == Ctrl->ExternResult[i])
		{
			DummyLen = Ctrl->ExternLen - i - 1;
			if (Ctrl->IsPrintData)
			{
				DBG_HexPrintf(&Ctrl->ExternResult[i], DummyLen + 1);
			}
			memcpy(RegDataBuf, &Ctrl->ExternResult[i + 1], DummyLen);
			memset(RegDataBuf + DummyLen, 0xff, DataLen - DummyLen);
			SDHC_SpiXfer(Ctrl, RegDataBuf + DummyLen, DataLen - DummyLen);
			if (Ctrl->IsPrintData)
			{
				DBG_HexPrintf(RegDataBuf + DummyLen, DataLen - DummyLen);
			}
			goto SDHC_SPIREADREGDATA_DONE;
		}
	}
	while((GetSysTick() < OpEndTick) && !Ctrl->SDHCError)
	{
		memset(Ctrl->TempData, 0xff, 40);
		SDHC_SpiXfer(Ctrl, Ctrl->TempData, 40);
		if (Ctrl->IsPrintData)
		{
			DBG_HexPrintf(Ctrl->TempData, 40);
		}
		for(i = 0; i < 40; i++)
		{
			if (0xfe == Ctrl->TempData[i])
			{
				DummyLen = 40 - i - 1;
				if (DummyLen >= DataLen)
				{
					memcpy(RegDataBuf, &Ctrl->TempData[i + 1], DataLen);
					goto SDHC_SPIREADREGDATA_DONE;
				}
				else
				{
					memcpy(RegDataBuf, &Ctrl->TempData[i + 1], DummyLen);
					memset(RegDataBuf + DummyLen, 0xff, DataLen - DummyLen);
					SDHC_SpiXfer(Ctrl, RegDataBuf + DummyLen, DataLen - DummyLen);
					goto SDHC_SPIREADREGDATA_DONE;
				}
			}
		}
	}
	Result = -ERROR_OPERATION_FAILED;
SDHC_SPIREADREGDATA_DONE:
	SDHC_SpiCS(Ctrl, 0);
	return Result;
}


static int32_t SDHC_SpiWriteBlockData(SDHC_SPICtrlStruct *Ctrl)
{
	uint64_t OpEndTick;
	int Result = -ERROR_OPERATION_FAILED;
	uint16_t TxLen, DoneFlag, waitCnt;
	uint16_t i, crc16;
	uint8_t *pBuf;
	Ctrl->SPIError = 0;
	Ctrl->SDHCError = 0;
	OpEndTick = GetSysTick() + Ctrl->SDHCWriteBlockTo;
	while( (Ctrl->DataBuf.Pos < Ctrl->DataBuf.MaxLen) && (GetSysTick() < OpEndTick) )
	{
		Ctrl->TempData[0] = 0xff;
		Ctrl->TempData[1] = 0xff;
		//SD_DBG("%u,%u", Ctrl->DataBuf.Pos, Ctrl->DataBuf.MaxLen);
		Ctrl->TempData[2] = 0xfc;
		memcpy(Ctrl->TempData + 3, Ctrl->DataBuf.Data + Ctrl->DataBuf.Pos * __SDHC_BLOCK_LEN__, __SDHC_BLOCK_LEN__);
		crc16 = CRC16Cal(Ctrl->DataBuf.Data + Ctrl->DataBuf.Pos * __SDHC_BLOCK_LEN__, __SDHC_BLOCK_LEN__, 0, CRC16_CCITT_GEN, 0);
		BytesPutBe16(Ctrl->TempData + 3 + __SDHC_BLOCK_LEN__, crc16);
		Ctrl->TempData[5 + __SDHC_BLOCK_LEN__] = 0xff;
		TxLen = 6 + __SDHC_BLOCK_LEN__;
		SDHC_SpiXfer(Ctrl, Ctrl->TempData, TxLen);
		if ((Ctrl->TempData[5 + __SDHC_BLOCK_LEN__] & 0x1f) != 0x05)
		{
			DBG("write data error ! x%02x", Ctrl->TempData[5 + __SDHC_BLOCK_LEN__]);
			Ctrl->SDHCError = 1;
			goto SDHC_SPIWRITEBLOCKDATA_DONE;
		}
		DoneFlag = 0;
		waitCnt = 0;
		while( (GetSysTick() < OpEndTick) && !DoneFlag )
		{
			TxLen = Ctrl->WriteWaitCnt?Ctrl->WriteWaitCnt:40;
			memset(Ctrl->TempData, 0xff, TxLen);
			SDHC_SpiXfer(Ctrl, Ctrl->TempData, TxLen);
			for(i = 0; i < TxLen; i++)
			{
				if (Ctrl->TempData[i] == 0xff)
				{
					DoneFlag = 1;
					if ((i + waitCnt) < sizeof(Ctrl->TempData))
					{
						if ((i + waitCnt) != Ctrl->WriteWaitCnt)
						{
//							DBG("%u", Ctrl->WriteWaitCnt);
							Ctrl->WriteWaitCnt = i + waitCnt;
						}
					}
					break;
				}
			}
			waitCnt += TxLen;
		}
		if (!DoneFlag)
		{
			DBG("write data timeout!");
			Ctrl->SDHCError = 1;
			goto SDHC_SPIWRITEBLOCKDATA_DONE;
		}
		Ctrl->DataBuf.Pos++;
		OpEndTick = GetSysTick() + Ctrl->SDHCWriteBlockTo;
	}
	Result = ERROR_NONE;
SDHC_SPIWRITEBLOCKDATA_DONE:
	Ctrl->TempData[0] = 0xfd;
	SPI_BlockTransfer(Ctrl->SpiID, Ctrl->TempData, Ctrl->TempData, 1);

	OpEndTick = GetSysTick() + Ctrl->SDHCWriteBlockTo * Ctrl->DataBuf.MaxLen;
	DoneFlag = 0;
	while( (GetSysTick() < OpEndTick) && !DoneFlag )
	{
		TxLen = sizeof(Ctrl->TempData);
		memset(Ctrl->TempData, 0xff, TxLen);
		SDHC_SpiXfer(Ctrl, Ctrl->TempData, TxLen);
		for(i = 0; i < TxLen; i++)
		{
			if (Ctrl->TempData[i] == 0xff)
			{
				DoneFlag = 1;
				break;
			}
		}
	}
	SDHC_SpiCS(Ctrl, 0);

	return Result;
}

static int32_t SDHC_SpiReadBlockData(SDHC_SPICtrlStruct *Ctrl)
{
	uint64_t OpEndTick;
	int Result = -ERROR_OPERATION_FAILED;
	uint16_t ReadLen, DummyLen, RemainingLen;
	uint16_t i, crc16, crc16_check;
	uint8_t *pBuf;
	Ctrl->SPIError = 0;
	Ctrl->SDHCError = 0;
	OpEndTick = GetSysTick() + Ctrl->SDHCReadBlockTo;
	while( (Ctrl->DataBuf.Pos < Ctrl->DataBuf.MaxLen) && (GetSysTick() < OpEndTick) )
	{

		DummyLen = (__SDHC_BLOCK_LEN__ >> 1);
		memset(Ctrl->TempData, 0xff, DummyLen);
//		SD_DBG("%u,%u", Ctrl->DataBuf.Pos, Ctrl->DataBuf.MaxLen);
		SDHC_SpiXfer(Ctrl, Ctrl->TempData, DummyLen);
		RemainingLen = 0;
		for(i = 0; i < DummyLen; i++)
		{
			if (Ctrl->TempData[i] == 0xfe)
			{
				ReadLen = (DummyLen - i - 1);
				RemainingLen = __SDHC_BLOCK_LEN__ - ReadLen;
				if (ReadLen)
				{
					memcpy(Ctrl->DataBuf.Data + Ctrl->DataBuf.Pos * __SDHC_BLOCK_LEN__, Ctrl->TempData + i + 1, ReadLen);
				}
//				SD_DBG("%u,%u", ReadLen, RemainingLen);
				goto READ_REST_DATA;
			}
		}
		continue;
READ_REST_DATA:
		pBuf = Ctrl->DataBuf.Data + Ctrl->DataBuf.Pos * __SDHC_BLOCK_LEN__ + ReadLen;
		memset(pBuf, 0xff, RemainingLen);
		SDHC_SpiXfer(Ctrl, pBuf, RemainingLen);
		memset(Ctrl->TempData, 0xff, 2);
		SPI_BlockTransfer(Ctrl->SpiID, Ctrl->TempData, Ctrl->TempData, 2);
//		if (Ctrl->IsCRCCheck)
		{
			crc16 = CRC16Cal(Ctrl->DataBuf.Data + Ctrl->DataBuf.Pos * __SDHC_BLOCK_LEN__, __SDHC_BLOCK_LEN__, 0, CRC16_CCITT_GEN, 0);
			crc16_check = BytesGetBe16(Ctrl->TempData);
			if (crc16 != crc16_check)
			{
				DBG("crc16 error %04x %04x", crc16, crc16_check);
				Result = ERROR_NONE;
				goto SDHC_SPIREADBLOCKDATA_DONE;
			}
		}
		Ctrl->DataBuf.Pos++;
		OpEndTick = GetSysTick() + Ctrl->SDHCReadBlockTo;
	}
	Result = ERROR_NONE;

SDHC_SPIREADBLOCKDATA_DONE:
	return Result;
}

static void SDHC_Recovery(SDHC_SPICtrlStruct *Ctrl)
{
	memset(Ctrl->TempData, 0xfd, 512);
	SPI_SetNewConfig(Ctrl->SpiID, 12000000, 0xff);
	GPIO_Output(Ctrl->CSPin, 0);
	SDHC_SpiXfer(Ctrl, Ctrl->TempData, 512);
	GPIO_Output(Ctrl->CSPin, 1);

	memset(Ctrl->TempData, 0xff, 512);
	GPIO_Output(Ctrl->CSPin, 0);
	SDHC_SpiXfer(Ctrl, Ctrl->TempData, 512);
	GPIO_Output(Ctrl->CSPin, 1);
	SDHC_SpiCmd(Ctrl, CMD12, 0, 1);
}

void SDHC_SpiInitCard(void *pSDHC)
{
	uint8_t i;
	uint64_t OpEndTick;
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
	Ctrl->IsInitDone = 0;
	Ctrl->SDHCState = 0xff;
	Ctrl->Info.CardCapacity = 0;
	Ctrl->WriteWaitCnt = 40;
	SPI_SetNewConfig(Ctrl->SpiID, 400000, 0xff);
	GPIO_Output(Ctrl->CSPin, 0);
	SDHC_SpiXfer(Ctrl, Ctrl->TempData, 20);
	GPIO_Output(Ctrl->CSPin, 1);
	memset(Ctrl->TempData, 0xff, 20);
	SDHC_SpiXfer(Ctrl, Ctrl->TempData, 20);
	Ctrl->IsMMC = 0;
	if (SDHC_SpiCmd(Ctrl, CMD0, 0, 1))
	{
		goto INIT_DONE;
	}
	OpEndTick = GetSysTick() + 1 * CORE_TICK_1S;
	if (SDHC_SpiCmd(Ctrl, CMD8, 0x1aa, 1))	//只支持2G以上的SDHC卡
	{
		goto INIT_DONE;
	}
WAIT_INIT_DONE:
	if (GetSysTick() >= OpEndTick)
	{
		goto INIT_DONE;
	}
	if (SDHC_SpiCmd(Ctrl, SD_CMD_APP_CMD, 0, 1))
	{
		goto INIT_DONE;
	}
	if (SDHC_SpiCmd(Ctrl, SD_CMD_SD_APP_OP_COND, 0x40000000, 1))
	{
		goto INIT_DONE;
	}
	Ctrl->IsInitDone = !Ctrl->SDHCState;
	if (!Ctrl->IsInitDone)
	{
		goto WAIT_INIT_DONE;
	}
	if (SDHC_SpiCmd(Ctrl, CMD58, 0, 1))
	{
		goto INIT_DONE;
	}
	Ctrl->OCR = BytesGetBe32(Ctrl->ExternResult);
	SPI_SetNewConfig(Ctrl->SpiID, Ctrl->SpiSpeed, 0xff);
	SD_DBG("sdcard init OK OCR:0x%08x!", Ctrl->OCR);
	return;
INIT_DONE:
	if (!Ctrl->IsInitDone)
	{
		SD_DBG("sdcard init fail!");
	}
	return;
}

void SDHC_SpiReadCardConfig(void *pSDHC)
{
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
	uint8_t CSD_Tab[18];
	uint8_t CID_Tab[18];
	SD_CSD* Csd = &Ctrl->Info.Csd;
	SD_CID* Cid = &Ctrl->Info.Cid;
	SD_CardInfo *pCardInfo = &Ctrl->Info;
	uint64_t Temp;
	uint8_t flag_SDHC = (Ctrl->OCR & 0x40000000) >> 30;
	if (Ctrl->Info.CardCapacity) return;

	if (SDHC_SpiCmd(Ctrl, CMD9, 0, 0))
	{
		goto READ_CONFIG_ERROR;
	}
	if (Ctrl->SDHCState)
	{
		goto READ_CONFIG_ERROR;
	}
	if (SDHC_SpiReadRegData(Ctrl, CSD_Tab, 18))
	{
		goto READ_CONFIG_ERROR;
	}
    /*************************************************************************
      CSD header decoding
    *************************************************************************/

    /* Byte 0 */
    Csd->CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
    Csd->Reserved1 =  CSD_Tab[0] & 0x3F;

    /* Byte 1 */
    Csd->TAAC = CSD_Tab[1];

    /* Byte 2 */
    Csd->NSAC = CSD_Tab[2];

    /* Byte 3 */
    Csd->MaxBusClkFrec = CSD_Tab[3];

    /* Byte 4/5 */
    Csd->CardComdClasses = (CSD_Tab[4] << 4) | ((CSD_Tab[5] & 0xF0) >> 4);
    Csd->RdBlockLen = CSD_Tab[5] & 0x0F;

    /* Byte 6 */
    Csd->PartBlockRead   = (CSD_Tab[6] & 0x80) >> 7;
    Csd->WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
    Csd->RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
    Csd->DSRImpl         = (CSD_Tab[6] & 0x10) >> 4;

    /*************************************************************************
      CSD v1/v2 decoding
    *************************************************************************/

    if(!flag_SDHC)
    {
		Csd->version.v1.Reserved1 = ((CSD_Tab[6] & 0x0C) >> 2);

		Csd->version.v1.DeviceSize =  ((CSD_Tab[6] & 0x03) << 10)
								  |  (CSD_Tab[7] << 2)
								  | ((CSD_Tab[8] & 0xC0) >> 6);
		Csd->version.v1.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
		Csd->version.v1.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
		Csd->version.v1.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
		Csd->version.v1.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
		Csd->version.v1.DeviceSizeMul = ((CSD_Tab[9] & 0x03) << 1)
									 |((CSD_Tab[10] & 0x80) >> 7);
    }
    else
    {
		Csd->version.v2.Reserved1 = ((CSD_Tab[6] & 0x0F) << 2) | ((CSD_Tab[7] & 0xC0) >> 6);
		Csd->version.v2.DeviceSize= ((CSD_Tab[7] & 0x3F) << 16) | (CSD_Tab[8] << 8) | CSD_Tab[9];
		Csd->version.v2.Reserved2 = ((CSD_Tab[10] & 0x80) >> 8);
    }

    Csd->EraseSingleBlockEnable = (CSD_Tab[10] & 0x40) >> 6;
    Csd->EraseSectorSize   = ((CSD_Tab[10] & 0x3F) << 1)
                            |((CSD_Tab[11] & 0x80) >> 7);
    Csd->WrProtectGrSize   = (CSD_Tab[11] & 0x7F);
    Csd->WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
    Csd->Reserved2         = (CSD_Tab[12] & 0x60) >> 5;
    Csd->WrSpeedFact       = (CSD_Tab[12] & 0x1C) >> 2;
    Csd->MaxWrBlockLen     = ((CSD_Tab[12] & 0x03) << 2)
                            |((CSD_Tab[13] & 0xC0) >> 6);
    Csd->WriteBlockPartial = (CSD_Tab[13] & 0x20) >> 5;
    Csd->Reserved3         = (CSD_Tab[13] & 0x1F);
    Csd->FileFormatGrouop  = (CSD_Tab[14] & 0x80) >> 7;
    Csd->CopyFlag          = (CSD_Tab[14] & 0x40) >> 6;
    Csd->PermWrProtect     = (CSD_Tab[14] & 0x20) >> 5;
    Csd->TempWrProtect     = (CSD_Tab[14] & 0x10) >> 4;
    Csd->FileFormat        = (CSD_Tab[14] & 0x0C) >> 2;
    Csd->Reserved4         = (CSD_Tab[14] & 0x03);
    Csd->crc               = (CSD_Tab[15] & 0xFE) >> 1;
    Csd->Reserved5         = (CSD_Tab[15] & 0x01);
#if 0
	if (SDHC_SpiCmd(Ctrl, CMD10, 0, 0))
	{
		goto READ_CONFIG_ERROR;
	}
	if (Ctrl->SDHCState)
	{
		goto READ_CONFIG_ERROR;
	}
	if (SDHC_SpiReadRegData(Ctrl, CID_Tab, 18))
	{
		goto READ_CONFIG_ERROR;
	}
    /* Byte 0 */
    Cid->ManufacturerID = CID_Tab[0];

    /* Byte 1 */
    Cid->OEM_AppliID = CID_Tab[1] << 8;

    /* Byte 2 */
    Cid->OEM_AppliID |= CID_Tab[2];

    /* Byte 3 */
    Cid->ProdName1 = CID_Tab[3] << 24;

    /* Byte 4 */
    Cid->ProdName1 |= CID_Tab[4] << 16;

    /* Byte 5 */
    Cid->ProdName1 |= CID_Tab[5] << 8;

    /* Byte 6 */
    Cid->ProdName1 |= CID_Tab[6];

    /* Byte 7 */
    Cid->ProdName2 = CID_Tab[7];

    /* Byte 8 */
    Cid->ProdRev = CID_Tab[8];

    /* Byte 9 */
    Cid->ProdSN = CID_Tab[9] << 24;

    /* Byte 10 */
    Cid->ProdSN |= CID_Tab[10] << 16;

    /* Byte 11 */
    Cid->ProdSN |= CID_Tab[11] << 8;

    /* Byte 12 */
    Cid->ProdSN |= CID_Tab[12];

    /* Byte 13 */
    Cid->Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
    Cid->ManufactDate = (CID_Tab[13] & 0x0F) << 8;

    /* Byte 14 */
    Cid->ManufactDate |= CID_Tab[14];

    /* Byte 15 */
    Cid->CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
    Cid->Reserved2 = 1;
#endif
    if(flag_SDHC)
    {
		pCardInfo->LogBlockSize = 512;
		pCardInfo->CardBlockSize = 512;
		Temp = 1024 * pCardInfo->LogBlockSize;
		pCardInfo->CardCapacity = (pCardInfo->Csd.version.v2.DeviceSize + 1) * Temp;
		pCardInfo->LogBlockNbr = (pCardInfo->Csd.version.v2.DeviceSize + 1) * 1024;
    }
    else
    {
		pCardInfo->CardCapacity = (pCardInfo->Csd.version.v1.DeviceSize + 1) ;
		pCardInfo->CardCapacity *= (1 << (pCardInfo->Csd.version.v1.DeviceSizeMul + 2));
		pCardInfo->LogBlockSize = 512;
		pCardInfo->CardBlockSize = 1 << (pCardInfo->Csd.RdBlockLen);
		pCardInfo->CardCapacity *= pCardInfo->CardBlockSize;
		pCardInfo->LogBlockNbr = (pCardInfo->CardCapacity) / (pCardInfo->LogBlockSize);
    }
//    DBG("卡容量 %lluKB", pCardInfo->CardCapacity/1024);
	return;
READ_CONFIG_ERROR:
	Ctrl->IsInitDone = 0;
	Ctrl->SDHCError = 1;
	return;
}

void SDHC_SpiReadBlocks(void *pSDHC, uint8_t *Buf, uint32_t StartLBA, uint32_t BlockNums)
{
	uint8_t Retry = 0;
	uint8_t error = 1;
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
#ifdef __BUILD_OS__
	if (OS_MutexLockWtihTime(prvRWMutex, 1000))
	{
		DBG("mutex wait timeout!");
		return;
	}
#endif
	Buffer_StaticInit(&Ctrl->DataBuf, Buf, BlockNums);
SDHC_SPIREADBLOCKS_START:
	if (SDHC_SpiCmd(Ctrl, CMD18, StartLBA + Ctrl->DataBuf.Pos, 0))
	{
		goto SDHC_SPIREADBLOCKS_CHECK;
	}
	if (SDHC_SpiReadBlockData(Ctrl))
	{
		goto SDHC_SPIREADBLOCKS_CHECK;
	}
	for (int i = 0; i < 3; i++)
	{
		if (!SDHC_SpiCmd(Ctrl, CMD12, 0, 1))
		{
			error = 0;
			break;
		}
		else
		{
			Ctrl->SDHCError = 0;
			Ctrl->IsInitDone = 1;
			Ctrl->SDHCState = 0;
		}
	}
SDHC_SPIREADBLOCKS_CHECK:
	if (error)
	{
		DBG("%x,%u,%u",Ctrl->SDHCState, Ctrl->DataBuf.Pos, Ctrl->DataBuf.MaxLen);
	}
	if (Ctrl->DataBuf.Pos != Ctrl->DataBuf.MaxLen)
	{
		Retry++;
		DBG("%d,%u,%u", Retry, Ctrl->DataBuf.Pos, Ctrl->DataBuf.MaxLen);
		if (Retry > 3)
		{

			Ctrl->SDHCError = 1;
			goto SDHC_SPIREADBLOCKS_ERROR;
		}
		else
		{
			Ctrl->SDHCError = 0;
			Ctrl->IsInitDone = 1;
			Ctrl->SDHCState = 0;
		}
		goto SDHC_SPIREADBLOCKS_START;
	}
#ifdef __BUILD_OS__
	OS_MutexRelease(prvRWMutex);
#endif
	return;
SDHC_SPIREADBLOCKS_ERROR:
	DBG("read error!");
	Ctrl->IsInitDone = 0;
	Ctrl->SDHCError = 1;
#ifdef __BUILD_OS__
	OS_MutexRelease(prvRWMutex);
#endif
	return;
}

void SDHC_SpiWriteBlocks(void *pSDHC, const uint8_t *Buf, uint32_t StartLBA, uint32_t BlockNums)
{
	uint8_t Retry = 0;
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
#ifdef __BUILD_OS__
	if (OS_MutexLockWtihTime(prvRWMutex, 1000))
	{
		DBG("mutex wait timeout!");
		return;
	}
#endif
	Buffer_StaticInit(&Ctrl->DataBuf, Buf, BlockNums);
SDHC_SPIREADBLOCKS_START:
	if (SDHC_SpiCmd(Ctrl, CMD25, StartLBA + Ctrl->DataBuf.Pos, 0))
	{
		goto SDHC_SPIREADBLOCKS_ERROR;
	}
	if (SDHC_SpiWriteBlockData(Ctrl))
	{
		goto SDHC_SPIREADBLOCKS_ERROR;
	}
	if (Ctrl->DataBuf.Pos != Ctrl->DataBuf.MaxLen)
	{
		Retry++;
		DBG("%d", Retry);
		if (Retry > 3)
		{
			Ctrl->SDHCError = 1;
			goto SDHC_SPIREADBLOCKS_ERROR;
		}
		goto SDHC_SPIREADBLOCKS_START;
	}
#ifdef __BUILD_OS__
	OS_MutexRelease(prvRWMutex);
#endif
	return;
SDHC_SPIREADBLOCKS_ERROR:
	DBG("write error!");
	Ctrl->IsInitDone = 0;
	Ctrl->SDHCError = 1;
#ifdef __BUILD_OS__
	OS_MutexRelease(prvRWMutex);
#endif
	return;
}

void *SDHC_SpiCreate(uint8_t SpiID, uint8_t CSPin)
{
	SDHC_SPICtrlStruct *Ctrl = zalloc(sizeof(SDHC_SPICtrlStruct));
	Ctrl->CSPin = CSPin;
	Ctrl->SpiID = SpiID;
	Ctrl->SDHCReadBlockTo = 50 * CORE_TICK_1MS;
	Ctrl->SDHCWriteBlockTo = 50 * CORE_TICK_1MS;
#ifdef __BUILD_OS__
	if (!prvRWMutex)
	{
		prvRWMutex = OS_MutexCreateUnlock();
	}
#endif
//	Ctrl->IsPrintData = 1;
	Ctrl->SpiSpeed = 24000000;
	return Ctrl;
}

void SDHC_SpiRelease(void *pSDHC)
{
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;

	SDHC_Recovery(Ctrl);
	OS_DeInitBuffer(&Ctrl->CacheBuf);
#ifdef __BUILD_OS__
	OS_MutexRelease(prvRWMutex);
	Ctrl->WaitFree = 1;
	Task_DelayMS(50);
#endif
	DBG("free %x", pSDHC);
}

uint32_t SDHC_GetLogBlockNbr(void *pSDHC)
{
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
	return Ctrl->Info.LogBlockNbr;
}

uint8_t SDHC_IsReady(void *pSDHC)
{
	SDHC_SPICtrlStruct *Ctrl = (SDHC_SPICtrlStruct *)pSDHC;
	if (!Ctrl->SDHCState && Ctrl->IsInitDone)
	{
		return 1;
	}
	else
	{
		DBG("SDHC error, please reboot tf card");
		return 0;
	}
}


