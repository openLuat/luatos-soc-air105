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

#ifndef __USB_MSC_H__
#define __USB_MSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_common.h"
#include "usb_scsi.h"
/* MSC Class Config */
#ifndef MSC_MEDIA_PACKET
#define MSC_MEDIA_PACKET             512U
#endif /* MSC_MEDIA_PACKET */

#define MSC_MAX_FS_PACKET            0x40U
#define MSC_MAX_HS_PACKET            0x200U

#define MSC_GET_MAX_LUN              0xFE
#define MSC_RESET                    0xFF
#define USB_MSC_CONFIG_DESC_SIZ      32

enum
{
	USB_MSC_BOT_STATE_IDLE,						/* Idle state */
	USB_MSC_BOT_STATE_DATA_OUT_TO_DEVICE,		/* Data Out state */
	USB_MSC_BOT_STATE_DATA_IN_TO_HOST,			/* Data In state */
	USB_MSC_BOT_STATE_CSW,						/* send csw state */
	USB_MSC_BOT_STATE_ERROR,
};


#define USB_MSC_BOT_CBW_SIGNATURE             0x43425355U
#define USB_MSC_BOT_CSW_SIGNATURE             0x53425355U
#define USB_MSC_BOT_CBW_LENGTH                31U
#define USB_MSC_BOT_CSW_LENGTH                13U
#define USB_MSC_BOT_MAX_DATA                  256U

/* CSW Status Definitions */
#define USB_MSC_CSW_CMD_PASSED                0x00U
#define USB_MSC_CSW_CMD_FAILED                0x01U
#define USB_MSC_CSW_PHASE_ERROR               0x02U

/* BOT Status */
#define USB_MSC_BOT_STATUS_NORMAL             0U
#define USB_MSC_BOT_STATUS_RECOVERY           1U
#define USB_MSC_BOT_STATUS_ERROR              2U


#define USB_MSC_DIR_IN                        0U
#define USB_MSC_DIR_OUT                       1U
#define USB_MSC_BOTH_DIR                      2U

/**
  * @}
  */

/** @defgroup MSC_CORE_Private_TypesDefinitions
  * @{
  */

typedef struct
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
  uint8_t  ReservedForAlign;
}MSC_BOT_CBWTypeDef;


typedef struct
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
  uint8_t  ReservedForAlign[3];
}MSC_BOT_CSWTypeDef;

typedef struct
{
	MSC_BOT_CBWTypeDef  CBW;
	MSC_BOT_CSWTypeDef  CSW;
	Buffer_Struct BotDataBuffer;
	SCSI_SenseTypeDef Sense;
	HANDLE pSCSIUserFunList;
	Timer_t *ReadTimer;
	void *pUserData;
	uint32_t XferDoneLen;
	uint32_t LastXferLen;
	uint32_t XferTotalLen;//实际上需要发送的总数据量，有可能和CBW要求的不一样
	uint8_t BotState;
	uint8_t CSWStatus;
	uint8_t MediumState;
	uint8_t ToHostEpIndex;
	uint8_t ToDeviceEpIndex;
	uint8_t LogicalUnitNum;
	uint8_t USB_ID;
}MSC_SCSICtrlStruct;

typedef struct
{
	int32_t (* GetMaxLUN)(uint8_t *LUN, void *pUserData);
	int32_t (* Init)(uint8_t LUN, void *pUserData);
	int32_t (* GetCapacity)(uint8_t LUN, uint32_t *BlockNum, uint32_t *BlockSize, void *pUserData);
	int32_t (* IsReady)(uint8_t LUN, void *pUserData);
	int32_t (* IsWriteProtected)(uint8_t LUN, void *pUserData);
	int32_t (* PreRead)(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData);
	int32_t (* Read)(uint8_t LUN, uint32_t Len, void **pOutData, uint32_t *OutLen, void *pUserData);
	int32_t (* ReadNext)(uint8_t LUN, void *pUserData);
	int32_t (* PreWrite)(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData);
	int32_t (* Write)(uint8_t LUN, uint8_t *Data, uint32_t Len, void *pUserData);
	int32_t (* DoWrite)(uint8_t LUN, void *pUserData);
	int32_t (* UserCmd)(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
	void *pStandardInquiry;
	void *pPage00InquiryData;
	void *pPage80InquiryData;
	void *pModeSense6Data;
	void *pModeSense10Data;
}USB_StorageSCSITypeDef;


void USB_MSCHandle(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC);
void USB_MSCReset(MSC_SCSICtrlStruct *pMSC);
void USB_MSCSetToDeviceData(MSC_SCSICtrlStruct *pMSC, uint8_t USB_ID, uint32_t Len);
void USB_MSCSetToHostData(MSC_SCSICtrlStruct *pMSC, uint8_t USB_ID, const void *pData, uint32_t Len);
void USB_SCSISetSenseState(MSC_SCSICtrlStruct *pMSC, uint8_t Skey, uint8_t ASC, uint8_t ASCQ, uint8_t *pData);
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USBD_MSC_H */
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
