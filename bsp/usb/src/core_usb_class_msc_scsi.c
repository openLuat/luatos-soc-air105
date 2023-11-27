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


typedef struct
{
	int32_t (*CmdFun)(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
	uint8_t Cmd;
}SCSI_CmdFun;

static int32_t prvSCSI_TestUnitReady(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_Inquiry(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_ReadFormatCapacity(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_ReadCapacity10(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_ReadCapacity16(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_RequestSense(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_StartStopUnit(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_AllowPreventRemovable(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_ModeSense6(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_ModeSense10(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_Write(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_Read(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static int32_t prvSCSI_Verify(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static void prvUSB_SCSIHandleToDeviceData(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static void prvUSB_SCSIHandleToHostData(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
static void prvUSB_SCSIHandleCmd(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);

static const SCSI_CmdFun prvSCSI_CmdFunList []=
{
		{prvSCSI_Write, SCSI_WRITE10},
		{prvSCSI_Write, SCSI_WRITE12},
		{prvSCSI_Write, SCSI_WRITE16},
		{prvSCSI_Read, SCSI_READ10},
		{prvSCSI_Read, SCSI_READ12},
		{prvSCSI_Read, SCSI_READ16},
		{prvSCSI_RequestSense, SCSI_REQUEST_SENSE},
		{prvSCSI_TestUnitReady, SCSI_TEST_UNIT_READY},
		{prvSCSI_Inquiry, SCSI_INQUIRY},
		{prvSCSI_ReadFormatCapacity, SCSI_READ_FORMAT_CAPACITIES},
		{prvSCSI_ReadCapacity10, SCSI_READ_CAPACITY10},
		{prvSCSI_ReadCapacity16, SCSI_READ_CAPACITY16},
		{prvSCSI_StartStopUnit, SCSI_START_STOP_UNIT},
		{prvSCSI_AllowPreventRemovable, SCSI_ALLOW_MEDIUM_REMOVAL},
		{prvSCSI_Verify, SCSI_VERIFY10},
		{prvSCSI_ModeSense6, SCSI_MODE_SENSE6},
		{prvSCSI_ModeSense10, SCSI_MODE_SENSE10},

};



static void prvUSB_SendCSW(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	pMSC->CSW.dTag = pMSC->CBW.dTag;
	pMSC->CSW.bStatus = pMSC->CSWStatus;
	if (pMSC->CBW.dDataLength >= pMSC->XferDoneLen)
	{
		pMSC->CSW.dDataResidue = pMSC->CBW.dDataLength - pMSC->XferDoneLen;
	}
	else
	{
		DBG("%u,%u", pMSC->CBW.dDataLength, pMSC->XferDoneLen);
		pMSC->CSW.dDataResidue = 0;
	}
	pMSC->BotState = USB_MSC_BOT_STATE_CSW;
	pMSC->XferTotalLen = 0;
	pMSC->XferDoneLen = 0;
	pMSC->LastXferLen = 0;
	USB_StackTxEpData(pMSC->USB_ID, pMSC->ToHostEpIndex, &pMSC->CSW, USB_MSC_BOT_CSW_LENGTH, USB_MSC_BOT_CSW_LENGTH, 0);
	Timer_Stop(pMSC->ReadTimer);
}

static int32_t prvUSB_MSCTimeout(void *pData, void *pParam)
{
	MSC_SCSICtrlStruct *pMSC = (void *)pParam;
	DBG("bot timeout!, reboot usb");
	Core_USBDefaultDeviceStart(pMSC->USB_ID);

}

static void prvUSB_SendBotData(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	pMSC->XferTotalLen = pMSC->BotDataBuffer.Pos;
	pMSC->LastXferLen = pMSC->BotDataBuffer.Pos;
	pMSC->XferDoneLen = 0;
	USB_StackTxEpData(pEpData->USB_ID, pMSC->ToHostEpIndex, pMSC->BotDataBuffer.Data, pMSC->BotDataBuffer.Pos, pMSC->BotDataBuffer.Pos, 0);
	pMSC->BotState = USB_MSC_BOT_STATE_DATA_IN_TO_HOST;
}

void USB_MSCSetToHostData(MSC_SCSICtrlStruct *pMSC, uint8_t USB_ID, const void *pData, uint32_t Len)
{
	pMSC->XferTotalLen = Len;
	pMSC->LastXferLen = Len;
	pMSC->XferDoneLen = 0;
	USB_StackTxEpData(USB_ID, pMSC->ToHostEpIndex, pData, Len, Len, 0);
	pMSC->BotState = USB_MSC_BOT_STATE_DATA_IN_TO_HOST;
}

void USB_MSCSetToDeviceData(MSC_SCSICtrlStruct *pMSC, uint8_t USB_ID, uint32_t Len)
{
	pMSC->XferTotalLen = Len;
	pMSC->XferDoneLen = 0;
	pMSC->BotState = USB_MSC_BOT_STATE_DATA_OUT_TO_DEVICE;
}

void USB_SCSISetSenseState(MSC_SCSICtrlStruct *pMSC, uint8_t Skey, uint8_t ASC, uint8_t ASCQ, uint8_t *pData)
{
	pMSC->Sense.Skey = Skey;
	pMSC->Sense.ASC = ASC;
	pMSC->Sense.ASCQ = ASCQ;
	if (pData)
	{
		memcpy(pMSC->Sense.uData.u8, pData, 4);
	}
	else
	{
		pMSC->Sense.uData.u32 = 0;
	}
}

void USB_MSCHandle(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	pMSC->USB_ID = pEpData->USB_ID;
	if (pEpData->IsToDevice)
	{
		switch(pMSC->BotState)
		{
		case USB_MSC_BOT_STATE_CSW:
			DBG("!");
			pMSC->BotState = USB_MSC_BOT_STATE_IDLE;
		case USB_MSC_BOT_STATE_IDLE:
			if (pMSC->CSW.bStatus)
			{
				USB_StackSetEpStatus(pEpData->USB_ID, pMSC->ToDeviceEpIndex, 1, USB_EP_STATE_ACK);
			}
			if (pEpData->Len != USB_MSC_BOT_CBW_LENGTH)
			{
				USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
				goto ERROR_OUT;
			}
			else
			{
				memcpy(&pMSC->CBW, pEpData->Data, pEpData->Len);
				if ( (pMSC->CBW.dSignature != USB_MSC_BOT_CBW_SIGNATURE) || (pMSC->CBW.bLUN > pMSC->LogicalUnitNum))
				{
					USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
					goto ERROR_OUT;
				}
				pMSC->XferDoneLen = 0;
//				pMSC->TestTime = GetSysTickUS();
				prvUSB_SCSIHandleCmd(pEpData, pMSC);
				if (!pMSC->CBW.dDataLength)
				{
					if (pMSC->BotState != USB_MSC_BOT_STATE_CSW)
					{
						prvUSB_SendCSW(pEpData, pMSC);
					}
				}
				else if ((pMSC->BotState != USB_MSC_BOT_STATE_DATA_OUT_TO_DEVICE) && (pMSC->BotState != USB_MSC_BOT_STATE_DATA_IN_TO_HOST) )
				{
					DBG("%d,%x,%d",pMSC->BotState, pMSC->CBW.CB[0], pMSC->CSWStatus);
					goto ERROR_OUT;
				}
			}
			break;
		case USB_MSC_BOT_STATE_DATA_OUT_TO_DEVICE:
			prvUSB_SCSIHandleToDeviceData(pEpData, pMSC);
			break;
		default:
			DBG("%d", pMSC->BotState);
			pMSC->CSWStatus = USB_MSC_CSW_PHASE_ERROR;
			USB_StackSetEpStatus(pEpData->USB_ID, pMSC->ToDeviceEpIndex, 1, USB_EP_STATE_STALL);
			prvUSB_SendCSW(pEpData, pMSC);
			return;
			break;
		}
	}
	else
	{
		switch(pMSC->BotState)
		{
		case USB_MSC_BOT_STATE_CSW:
			pMSC->BotState = USB_MSC_BOT_STATE_IDLE;
			if (pMSC->CSW.bStatus)
			{
				USB_StackSetEpStatus(pEpData->USB_ID, pMSC->ToDeviceEpIndex, 1, USB_EP_STATE_ACK);
			}

			break;
		case USB_MSC_BOT_STATE_DATA_IN_TO_HOST:
			prvUSB_SCSIHandleToHostData(pEpData, pMSC);
			break;
		case USB_MSC_BOT_STATE_IDLE:
			DBG("!");
			break;
		default:
			DBG("%d", pMSC->BotState);
			pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
			USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
			prvUSB_SendCSW(pEpData, pMSC);
			break;
		}
	}
	return ;
ERROR_OUT:
	DBG("!");
	pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
	USB_StackSetEpStatus(pEpData->USB_ID, pMSC->ToDeviceEpIndex, 1, USB_EP_STATE_STALL);
	prvUSB_SendCSW(pEpData, pMSC);
}

static void prvUSB_SCSIHandleCmd(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	uint32_t i;
	int32_t Result = -1;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	if (pMSC->CBW.bLUN > pMSC->LogicalUnitNum)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
		goto CMD_PROC_END;
	}
	pMSC->CSWStatus = USB_MSC_CSW_CMD_PASSED;
	for(i = 0; i < sizeof(prvSCSI_CmdFunList)/sizeof(SCSI_CmdFun); i++)
	{
		if (prvSCSI_CmdFunList[i].Cmd == pMSC->CBW.CB[0])
		{
			Result = prvSCSI_CmdFunList[i].CmdFun(pEpData, pMSC);
			goto CMD_PROC_END;
		}
	}
	Result = pUserFun->UserCmd(pEpData, pMSC);
CMD_PROC_END:
	if (Result)
	{
		//DBG("%02x", pMSC->CBW.CB[0]);
		if (!pMSC->Sense.Skey)
		{
			USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_COMMAND_OPERATION_CODE, 0, NULL);
		}
		pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
		if (pMSC->CBW.dDataLength)
		{
			if (!pMSC->CBW.bmFlags)	//To Device
			{
				DBG("!");
				USB_StackSetEpStatus(pEpData->USB_ID, pMSC->ToDeviceEpIndex, 1, USB_EP_STATE_STALL);
			}
		}
		prvUSB_SendCSW(pEpData, pMSC);
	}
	else
	{
		USB_SCSISetSenseState(pMSC, 0, 0, 0, NULL);
	}
	return;
}

static void prvUSB_SCSIHandleToDeviceData(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	int32_t Result = 0;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	if (pEpData->Len)
	{
		Result = pUserFun->Write(pMSC->CBW.bLUN, pEpData->Data, pEpData->Len, pMSC->pUserData);
	}
	if (Result)
	{
		DBG("sense error %x,%x,%x", pMSC->Sense.Skey, pMSC->Sense.ASC, pMSC->Sense.ASCQ);
		pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
		if (!pMSC->Sense.Skey)
		{
			USB_SCSISetSenseState(pMSC, SENSE_KEY_UNIT_ATTENTION, MEDIUM_HAVE_CHANGED, 0, NULL);
		}
	}
	pMSC->XferDoneLen += pEpData->Len;
	if (pMSC->XferDoneLen >= pMSC->XferTotalLen)
	{
		prvUSB_SendCSW(pEpData, pMSC);
	}
}

static void prvUSB_SCSIHandleToHostData(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	int32_t Result;
	uint8_t *TxData;
	uint32_t TxLen;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	pMSC->XferDoneLen += pMSC->LastXferLen;
	if (pMSC->XferDoneLen >= pMSC->XferTotalLen)
	{
//		DBG("%llu", GetSysTickUS() - pMSC->TestTime);
		prvUSB_SendCSW(pEpData, pMSC);
		return;
	}
	Result = pUserFun->Read(pMSC->CBW.bLUN, pMSC->XferTotalLen - pMSC->XferDoneLen, &TxData, &TxLen, pMSC->pUserData);
//	DBG("%u,%u,%u,%u", pMSC->XferTotalLen, pMSC->XferDoneLen, pMSC->LastXferLen, TxLen);
	if (Result)
	{
		DBG("sense error %x,%x,%x", pMSC->Sense.Skey, pMSC->Sense.ASC, pMSC->Sense.ASCQ);
		pMSC->CSWStatus = USB_MSC_CSW_CMD_FAILED;
		if (!pMSC->Sense.Skey)
		{
			USB_SCSISetSenseState(pMSC, SENSE_KEY_UNIT_ATTENTION, MEDIUM_HAVE_CHANGED, 0, NULL);
		}
		prvUSB_SendCSW(pEpData, pMSC);
	}
	else
	{
		pMSC->LastXferLen = TxLen;
		USB_StackTxEpData(pEpData->USB_ID, pMSC->ToHostEpIndex, TxData, TxLen, TxLen, 0);

	}
	pUserFun->ReadNext(pMSC->CBW.bLUN, pMSC->pUserData);
}

void USB_MSCReset(MSC_SCSICtrlStruct *pMSC)
{
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	OS_ReInitBuffer(&pMSC->BotDataBuffer, 512);
	memset(&pMSC->CSW, 0, sizeof(MSC_BOT_CSWTypeDef));
	memset(&pMSC->CBW, 0, sizeof(MSC_BOT_CBWTypeDef));
	pMSC->CSW.dSignature = USB_MSC_BOT_CSW_SIGNATURE;
	pMSC->CSWStatus = USB_MSC_CSW_CMD_PASSED;
	pMSC->XferTotalLen = 0;
	pMSC->XferDoneLen = 0;
	pMSC->LastXferLen = 0;
	pMSC->BotState = USB_MSC_BOT_STATE_IDLE;
	pMSC->MediumState = SCSI_MEDIUM_UNLOCKED;
	pUserFun->Init(0, pMSC->pUserData);
}

static int32_t prvSCSI_TestUnitReady(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	/* case 9 : Hi > D0 */
	if (pMSC->CBW.dDataLength)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	if (pMSC->MediumState == SCSI_MEDIUM_EJECTED)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
		return -1;
	}

	if (pUserFun->IsReady(pMSC->CBW.bLUN, pMSC->pUserData) != 0)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
		return -1;
	}

	return 0;
}


/**
  * @brief  SCSI_Inquiry
  *         Process Inquiry command
  * @param  pMSC->CBW.bLUN: Logical unit number
  * @param  pMSC->CBW.CB: Command parameters
  * @retval status
  */
static int32_t prvSCSI_Inquiry(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	uint8_t *pPage;
	uint16_t TxLen;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;

	if (!pMSC->CBW.dDataLength)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	if ((pMSC->CBW.CB[1] & 0x01U) != 0U) /* Evpd is set */
	{
		if (pMSC->CBW.CB[2] == 0U) /* Request for Supported Vital Product Data Pages*/
		{
			TxLen = MIN(LENGTH_INQUIRY_PAGE00, pMSC->CBW.CB[4]);
			USB_MSCSetToHostData(pMSC, pEpData->USB_ID, pUserFun->pPage00InquiryData, TxLen);
		}
		else if (pMSC->CBW.CB[2] == 0x80U) /* Request for VPD page 0x80 Unit Serial Number */
		{
			TxLen = MIN(LENGTH_INQUIRY_PAGE80, pMSC->CBW.CB[4]);
			USB_MSCSetToHostData(pMSC, pEpData->USB_ID, pUserFun->pPage80InquiryData, TxLen);
		}
		else /* Request Not supported */
		{
			USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
			return -1;
		}
	}
	else
	{
		TxLen = MIN(STANDARD_INQUIRY_DATA_LEN, pMSC->CBW.CB[4]);
		USB_MSCSetToHostData(pMSC, pEpData->USB_ID, pUserFun->pStandardInquiry, TxLen);

	}

	return 0;
}



static int32_t prvSCSI_ReadCapacity10(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	int32_t ret;
	uint32_t BlockNum, BlockSize;
	uint8_t Data[READ_CAPACITY10_DATA_LEN];
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	ret = pUserFun->GetCapacity(pMSC->CBW.bLUN, &BlockNum, &BlockSize, pMSC->pUserData);

	if (ret || (pMSC->MediumState == SCSI_MEDIUM_EJECTED))
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
		return -1;
	}

	if (!pMSC->CBW.dDataLength)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	pMSC->BotDataBuffer.Pos = 0;
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockNum - 1);
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockSize);
	prvUSB_SendBotData(pEpData, pMSC);


	return 0;

}

static int32_t prvSCSI_ReadCapacity16(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	int32_t ret;
	uint32_t BlockNum, BlockSize;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	ret = pUserFun->GetCapacity(pMSC->CBW.bLUN, &BlockNum, &BlockSize, pMSC->pUserData);

	if (ret || (pMSC->MediumState == SCSI_MEDIUM_EJECTED))
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
		return -1;
	}

	if (!pMSC->CBW.dDataLength)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	pMSC->BotDataBuffer.Pos = 0;
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, 0);
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockNum - 1);
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockSize);
	pMSC->BotDataBuffer.Pos = MIN(pMSC->BotDataBuffer.MaxLen, pMSC->CBW.dDataLength);
	prvUSB_SendBotData(pEpData, pMSC);

	return 0;

}

static int32_t prvSCSI_ReadFormatCapacity(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	int32_t ret;
	uint8_t Data[READ_FORMAT_CAPACITY_DATA_LEN];
	uint32_t BlockNum, BlockSize, TargetLen;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	ret = pUserFun->GetCapacity(pMSC->CBW.bLUN, &BlockNum, &BlockSize, pMSC->pUserData);

	if (ret || (pMSC->MediumState == SCSI_MEDIUM_EJECTED))
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
		return -1;
	}

	if (!pMSC->CBW.dDataLength)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}
	pMSC->BotDataBuffer.Pos = 0;
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, 8);
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockNum - 1);
	BytesPutBe32ToBuf(&pMSC->BotDataBuffer, BlockSize);
	pMSC->BotDataBuffer.Data[8] = 2;
	prvUSB_SendBotData(pEpData, pMSC);
	return 0;
}

static int32_t prvSCSI_ModeSense6(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	uint16_t TxLen;
	if (pMSC->CBW.dDataLength)
	{
		TxLen = MIN(MODE_SENSE6_LEN, pMSC->CBW.dDataLength);
		USB_MSCSetToHostData(pMSC, pEpData->USB_ID, pUserFun->pModeSense6Data, TxLen);
	}
	return 0;
}


static int32_t prvSCSI_ModeSense10(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	uint16_t TxLen;
	if (pMSC->CBW.dDataLength)
	{
		TxLen = MIN(MODE_SENSE10_LEN, pMSC->CBW.CB[4]);
		USB_MSCSetToHostData(pMSC, pEpData->USB_ID, pUserFun->pModeSense10Data, TxLen);
	}
	return 0;
}


static int32_t prvSCSI_RequestSense(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;


	if (pMSC->CBW.dDataLength == 0U)
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}
	memset(pMSC->BotDataBuffer.Data, 0, REQUEST_SENSE_DATA_LEN);

	pMSC->BotDataBuffer.Data[0] = 0x70U;
	pMSC->BotDataBuffer.Data[7] = REQUEST_SENSE_DATA_LEN - 6U;

	pMSC->BotDataBuffer.Data[2] = pMSC->Sense.Skey;
	pMSC->BotDataBuffer.Data[12] = (uint8_t)pMSC->Sense.ASC;
	pMSC->BotDataBuffer.Data[13] = (uint8_t)pMSC->Sense.ASCQ;
	pMSC->BotDataBuffer.Pos = MIN(REQUEST_SENSE_DATA_LEN, pMSC->CBW.CB[4]);
	prvUSB_SendBotData(pEpData, pMSC);
	return 0;
}


/**
  * @brief  SCSI_StartStopUnit
  *         Process Start Stop Unit command
  * @param  pMSC->CBW.bLUN: Logical unit number
  * @param  pMSC->CBW.CB: Command parameters
  * @retval status
  */
static int32_t prvSCSI_StartStopUnit(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	if ((pMSC->MediumState == SCSI_MEDIUM_LOCKED) && ((pMSC->CBW.CB[4] & 0x3U) == 2U))
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	if ((pMSC->CBW.CB[4] & 0x3U) == 0x1U) /* START=1 */
	{
		pMSC->MediumState = SCSI_MEDIUM_UNLOCKED;
	}
	else if ((pMSC->CBW.CB[4] & 0x3U) == 0x2U) /* START=0 and LOEJ Load Eject=1 */
	{
		pMSC->MediumState = SCSI_MEDIUM_EJECTED;
	}
	else if ((pMSC->CBW.CB[4] & 0x3U) == 0x3U) /* START=1 and LOEJ Load Eject=1 */
	{
		pMSC->MediumState = SCSI_MEDIUM_UNLOCKED;
	}
	else
	{
		USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
		return -1;
	}

	return 0;
}


/**
  * @brief  SCSI_AllowPreventRemovable
  *         Process Allow Prevent Removable medium command
  * @param  pMSC->CBW.bLUN: Logical unit number
  * @param  pMSC->CBW.CB: Command parameters
  * @retval status
  */
static int32_t prvSCSI_AllowPreventRemovable(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	if (pMSC->CBW.CB[4] == 0U)
	{
		pMSC->MediumState = SCSI_MEDIUM_UNLOCKED;
	}
	else
	{
		pMSC->MediumState = SCSI_MEDIUM_LOCKED;
	}
	return 0;
}


/**
  * @brief  SCSI_Read10
  *         Process Read10 command
  * @param  pMSC->CBW.bLUN: Logical unit number
  * @param  pMSC->CBW.CB: Command parameters
  * @retval status
  */
static int32_t prvSCSI_Read(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	uint32_t BlockNum = 0, BlockSize;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	int ret = pUserFun->GetCapacity(pMSC->CBW.bLUN, &BlockNum, &BlockSize, pMSC->pUserData);
	uint32_t BlockNums = 0, CurBLKAddress;
	CurBLKAddress = BytesGetBe32(&pMSC->CBW.CB[2]);
	if (!pMSC->ReadTimer) pMSC->ReadTimer = Timer_Create(prvUSB_MSCTimeout, pMSC, NULL);
	switch(pMSC->CBW.CB[0])
	{
	case SCSI_READ10:
		BlockNums = BytesGetBe16(&pMSC->CBW.CB[7]);
		break;
	case SCSI_READ12:
		BlockNums = BytesGetBe32(&pMSC->CBW.CB[6]);
		break;
	case SCSI_READ16:
		CurBLKAddress = BytesGetBe32(&pMSC->CBW.CB[6]);
		BlockNums = BytesGetBe32(&pMSC->CBW.CB[10]);
		break;
	}
    /* case 10 : Ho <> Di */
    if ((pMSC->CBW.bmFlags & 0x80U) != 0x80U)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }

    if (ret || pMSC->MediumState == SCSI_MEDIUM_EJECTED)
    {

    	USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
    	return -1;
    }

    if (pUserFun->IsReady(pMSC->CBW.bLUN, pMSC->pUserData) != 0)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
    	return -1;
    }

    /* cases 4,5 : Hi <> Dn */
    if (pMSC->CBW.dDataLength != (BlockNums * BlockSize))
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }

    if (pUserFun->PreRead(pMSC->CBW.bLUN, CurBLKAddress, BlockNums, pMSC->pUserData) != 0)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }
	uint8_t *TxData;
	uint32_t TxLen;
	pMSC->XferTotalLen = pMSC->CBW.dDataLength;
	pMSC->XferDoneLen = 0;
	if (pUserFun->Read(pMSC->CBW.bLUN, pMSC->XferTotalLen - pMSC->XferDoneLen, &TxData, &TxLen, pMSC->pUserData))
	{
		DBG("!");
		return -1;
	}
	pMSC->LastXferLen = TxLen;
	USB_StackTxEpData(pEpData->USB_ID, pMSC->ToHostEpIndex, TxData, TxLen, TxLen, 0);
	pMSC->BotState = USB_MSC_BOT_STATE_DATA_IN_TO_HOST;
	pUserFun->ReadNext(pMSC->CBW.bLUN, pMSC->pUserData);
	if (pMSC->ReadTimeout)
	{
		Timer_StartMS(pMSC->ReadTimer, pMSC->ReadTimeout, 0);
	}

    return 0;
}

/**
  * @brief  SCSI_Write10
  *         Process Write10 command
  * @param  pMSC->CBW.bLUN: Logical unit number
  * @param  pMSC->CBW.CB: Command parameters
  * @retval status
  */
static int32_t prvSCSI_Write(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	uint32_t BlockNum = 0, BlockSize;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)pMSC->pSCSIUserFunList;
	int ret = pUserFun->GetCapacity(pMSC->CBW.bLUN, &BlockNum, &BlockSize, pMSC->pUserData);
	uint32_t BlockNums = 0, CurBLKAddress;
	CurBLKAddress = BytesGetBe32(&pMSC->CBW.CB[2]);
	switch(pMSC->CBW.CB[0])
	{
	case SCSI_WRITE10:
		BlockNums = BytesGetBe16(&pMSC->CBW.CB[7]);
		break;
	case SCSI_WRITE12:
		BlockNums = BytesGetBe32(&pMSC->CBW.CB[6]);
		break;
	case SCSI_WRITE16:
		CurBLKAddress = BytesGetBe32(&pMSC->CBW.CB[2]);
		BlockNums = BytesGetBe32(&pMSC->CBW.CB[10]);
		break;
	}

    /* case 10 : Ho <> Di */
    if ((pMSC->CBW.bmFlags & 0x80U) == 0x80U)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }

    if (ret || pMSC->MediumState == SCSI_MEDIUM_EJECTED)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);

    	return -1;
    }

    if (pUserFun->IsReady(pMSC->CBW.bLUN, pMSC->pUserData) != 0)
    {
    	DBG("!");
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_NOT_READY, MEDIUM_NOT_PRESENT, 0, NULL);
    	return -1;
    }

    /* cases 4,5 : Hi <> Dn */
    if (pMSC->CBW.dDataLength != (BlockNums * BlockSize))
    {
    	DBG("!");
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }

    if (pUserFun->PreWrite(pMSC->CBW.bLUN, CurBLKAddress, BlockNums, pMSC->pUserData) != 0)
    {
    	USB_SCSISetSenseState(pMSC, SENSE_KEY_ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND, 0, NULL);
    	return -1;
    }

    USB_MSCSetToDeviceData(pMSC, pEpData->USB_ID, pMSC->CBW.dDataLength);
    return 0;
}


static int32_t prvSCSI_Verify(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	return 0;
}



