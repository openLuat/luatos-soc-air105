#include "user.h"

enum
{
	AUDIO_STATE_IDLE,
	AUDIO_STATE_NEED_DATA,
	AUDIO_STATE_WAIT_PAUSE,
	AUDIO_STATE_PAUSE,
};

typedef struct
{
	llist_head Node;
	PV_Union uPV;
	uint32_t Len;
}Audio_DataBlockStruct;

typedef struct
{
	uint32_t DACOutRMode;
	uint8_t DACBit;
}Audio_CtrlStruct;

static Audio_CtrlStruct prvAudio;

static int32_t prvAudio_DACCB(void *pData, void *pParam)
{
	Audio_StreamStruct *pStream = (Audio_StreamStruct *)pParam;
	Audio_DataBlockStruct *OldBlock, *NewBlock;
	OldBlock = (Audio_DataBlockStruct *)pStream->DataHead.next;
	llist_del(&OldBlock->Node);


	if (!llist_empty(&pStream->DataHead) && !pStream->IsPause)
	{
		NewBlock = (Audio_DataBlockStruct *)pStream->DataHead.next;
		DAC_Send(pStream->BusID, NewBlock->uPV.pu16, NewBlock->Len, prvAudio_DACCB, pParam);
		pStream->IsHardwareRun = 1;
	}
	free(OldBlock->uPV.pu8);
	free(OldBlock);
	pStream->CB(pStream, NULL);
	if (llist_empty(&pStream->DataHead))
	{
		pStream->IsHardwareRun = 0;
		pStream->CB(pStream, INVALID_HANDLE_VALUE);
	}
	else if (!pStream->IsPause)
	{
		if (!pStream->IsHardwareRun)
		{
			NewBlock = (Audio_DataBlockStruct *)pStream->DataHead.next;
			DAC_Send(pStream->BusID, NewBlock->uPV.pu16, NewBlock->Len, prvAudio_DACCB, pParam);
			pStream->IsHardwareRun = 1;
		}

	}
	else
	{
		pStream->IsHardwareRun = 0;
	}
	return 0;
}

void Audio_GlobalInit(void)
{
	prvAudio.DACOutRMode = 1;
	prvAudio.DACBit = 10;
}

int32_t Audio_StartRaw(Audio_StreamStruct *pStream)
{
	INIT_LLIST_HEAD(&pStream->DataHead);
	if (pStream->BitDepth > 16) return -ERROR_PARAM_INVALID;
	if (pStream->BitDepth < 8) return -ERROR_PARAM_INVALID;
	switch(pStream->Format)
	{
	case AUSTREAM_FORMAT_PCM:
		break;
	default:
		return -ERROR_PARAM_INVALID;
	}
	switch(pStream->BusType)
	{
	case AUSTREAM_BUS_DAC:
		GPIO_Iomux(GPIOC_00, 2);
		DAC_ForceStop(pStream->BusID);
		DAC_DMAInit(0, DAC_TX_DMA_STREAM);
		DAC_Setup(pStream->BusID, pStream->SampleRate, prvAudio.DACOutRMode);
		pStream->IsHardwareRun = 0;
		pStream->IsPause = 0;
		break;
	default:
		return -ERROR_PARAM_INVALID;
	}
	return ERROR_NONE;
}

static int32_t prvAudio_RunDAC(Audio_StreamStruct *pStream)
{
	Audio_DataBlockStruct *Block = (Audio_DataBlockStruct *)pStream->DataHead.next;
	if (llist_empty(&pStream->DataHead))
	{
		pStream->CB(pStream, NULL);
	}
	else
	{
		Block = (Audio_DataBlockStruct *)pStream->DataHead.next;
		DAC_Send(pStream->BusID, Block->uPV.pu16, Block->Len, prvAudio_DACCB, pStream);
		pStream->IsHardwareRun = 1;
	}
	return ERROR_NONE;
}

static int32_t prvAudio_WriteDACRaw(Audio_StreamStruct *pStream, uint8_t *pByteData, uint32_t ByteLen)
{
	uint32_t i, VaildLen;
	uint32_t DiffBit;
	uint16_t *wTemp;
	Audio_DataBlockStruct *Block = zalloc(sizeof(Audio_DataBlockStruct));
	VaildLen = ByteLen >> (pStream->ChannelCount >> 1);
	//DBG("%u,%u", ByteLen, VaildLen);
	if (pStream->BitDepth > 8)
	{
		Block->uPV.pu8 = malloc(VaildLen);
		if (!Block->uPV.pu8)
		{
			free(Block);
			DBG("no mem!, %u,%u", ByteLen, VaildLen);
			return -ERROR_NO_MEMORY;
		}
		Block->Len = VaildLen >> 1;
		if (2 == pStream->ChannelCount)
		{
			wTemp = pByteData;
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] = wTemp[i * 2];
			}
		}
		else
		{
			memcpy(Block->uPV.pu8, pByteData, VaildLen);
		}

		if (pStream->IsDataSigned)
		{
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] += 0x8000;
			}
		}
		if (pStream->BitDepth >= prvAudio.DACBit)
		{
			DiffBit = pStream->BitDepth - prvAudio.DACBit;
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] = Block->uPV.pu16[i] >> DiffBit;
			}
		}
		else
		{
			DiffBit = prvAudio.DACBit - pStream->BitDepth;
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] = Block->uPV.pu16[i] << DiffBit;
			}
		}

	}
	else
	{
		Block->uPV.pu8 = malloc(VaildLen * 2);
		if (!Block->uPV.pu8)
		{
			free(Block);
			DBG("no mem!, %u,%u", ByteLen, VaildLen);
			return -ERROR_NO_MEMORY;
		}
		Block->Len = VaildLen;
		DiffBit = prvAudio.DACBit - 8;
		if (pStream->IsDataSigned)
		{
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] = ((pByteData[i * pStream->ChannelCount] + 0x80) & 0x00ff);
				Block->uPV.pu16[i] <<= DiffBit;
			}
		}
		else
		{
			for(i = 0; i < Block->Len; i++)
			{
				Block->uPV.pu16[i] = pByteData[i * pStream->ChannelCount];
				Block->uPV.pu16[i] <<= DiffBit;
			}
		}
	}
	uint32_t Critical;
	Critical = OS_EnterCritical();
	llist_add_tail(&Block->Node, &pStream->DataHead);
	OS_ExitCritical(Critical);
	if (!pStream->IsHardwareRun && !pStream->IsPause)
	{
		return prvAudio_RunDAC(pStream);
	}
	return ERROR_NONE;
}

int32_t Audio_WriteRaw(Audio_StreamStruct *pStream, uint8_t *pByteData, uint32_t ByteLen)
{
	if (!ByteLen) {return -ERROR_PARAM_INVALID;}
	switch(pStream->BusType)
	{
	case AUSTREAM_BUS_DAC:
		return prvAudio_WriteDACRaw(pStream, pByteData, ByteLen);
	default:
		return -ERROR_PARAM_INVALID;
	}

}

static int32_t prvAudio_DeleteData(void *pData, void *pParam)
{
	Buffer_Struct *UriBuf = (Buffer_Struct *)pParam;
	Audio_DataBlockStruct *Block = (Audio_DataBlockStruct *)pData;
	free(Block->uPV.p);
	return LIST_DEL;
}

void Audio_Stop(Audio_StreamStruct *pStream)
{
	switch(pStream->BusType)
	{
	case AUSTREAM_BUS_DAC:
		DAC_Stop(pStream->BusID);
		llist_traversal(&pStream->DataHead, prvAudio_DeleteData, NULL);
		break;
	default:
		return;
	}
}

void Audio_Pause(Audio_StreamStruct *pStream)
{
	pStream->IsPause = 1;
}

void Audio_Resume(Audio_StreamStruct *pStream)
{
	pStream->IsPause = 0;
	if (!pStream->IsHardwareRun)
	{
		switch(pStream->BusType)
		{
		case AUSTREAM_BUS_DAC:
			prvAudio_RunDAC(pStream);
			break;
		default:
			return;
		}
	}
}

#ifdef __BUILD_APP__
INIT_DRV_EXPORT(Audio_GlobalInit, "0");
#endif
