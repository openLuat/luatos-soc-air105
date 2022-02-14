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

	USB_SetupInfoStruct Setup;
	USB_HWCapsStruct HWCaps;
	usb_device_request_t LastRequest;
	Buffer_Struct FullConfigDataBuf;
	Buffer_Struct FullOtherSpeedConfigDataBuf;
	USB_EndpointCtrlStruct *pEpCtrl;
	Timer_t *WaitDataTimer;
	HANDLE pHWCtrl;
	union
	{
		uint16_t DeviceStatus;
		struct
		{
			uint16_t SelfPower:1;
			uint16_t RemoteWakeupEnable:1;
			uint16_t U1Enable:1;
			uint16_t U2Enable:1;
			uint16_t LTMEnable:1;
			uint16_t Zero:11;
		}DEVSTATUS_b;
	};
	union
	{
		uint16_t InterfaceStatus;
		struct
		{
			uint16_t RemoteWakeCapable:1;
			uint16_t RemoteWakeupEnable:1;
			uint16_t Zero:14;
		}INFSTATUS_b;
	};
	uint8_t DeviceState;
	uint8_t Ep0Stage;
	uint8_t IsRequestError;
	uint8_t BusPowered;
	uint8_t SelfID;
	uint8_t ConfigNo;
	uint8_t DefaultConfigNo;
}USB_StackCtrlStruct;

#define USB_DBG(x, y...)
//#define USB_DBG	DBG
#define USB_ERR	DBG

static USB_StackCtrlStruct prvUSBCore[USB_MAX];

static int32_t prvUSB_StackDummyEpCB(void *pData, void *pParam)
{
	USB_EndpointDataStruct *EpData = (USB_EndpointDataStruct *)pData;
	USB_ERR("USB%d EP%d no work!", EpData->USB_ID, EpData->EpIndex);
	return -ERROR_OPERATION_FAILED;
}

static int32_t prvUSB_StackDummyStateCB(void *pData, void *pParam)
{
	return -ERROR_OPERATION_FAILED;
}

static void prvUSB_ResetEpCtrl(uint8_t USB_ID, uint8_t Index)
{
	OS_DeInitBuffer(&prvUSBCore[USB_ID].pEpCtrl[Index].RxBuf);
	memset(&prvUSBCore[USB_ID].pEpCtrl[Index], 0, sizeof(USB_EndpointCtrlStruct));
	prvUSBCore[USB_ID].pEpCtrl[Index].CB = prvUSB_StackDummyEpCB;
	prvUSBCore[USB_ID].pEpCtrl[Index].ForceZeroPacket = 0;
}

static int32_t prvUSB_SetupRxTimeout(void *pData, void *pParam)
{
	USB_SetDeviceEPStatus(pParam, 0, 1, USB_EP_STATE_STALL);
	USB_ERR("!");
}

static void prvUSB_StackEpInit(uint8_t USB_ID)
{
	usb_full_config_t *pConfig = prvUSBCore[USB_ID].Setup.pCurConfigInfo->pFullConfig[prvUSBCore[USB_ID].Setup.CurSpeed];
	uint32_t i, Index;
	usb_full_interface_t *pFullInterface;
	uint8_t EpID, ToHost, EpNum;

	for(Index = 0; Index < pConfig->InterfaceNum; Index++)
	{
		pFullInterface = &pConfig->pInterfaceFullDesc[Index];
		EpNum = pFullInterface->EndpointNum;
		for(i = 0; i < EpNum; i++)
		{
			ToHost = UE_GET_DIR(pFullInterface->pEndpointDesc[i].bEndpointAddress);
			EpID = UE_GET_ADDR(pFullInterface->pEndpointDesc[i].bEndpointAddress);
			prvUSBCore[USB_ID].pEpCtrl[EpID].XferType = UE_GET_XFERTYPE(pFullInterface->pEndpointDesc[i].bmAttributes);
			prvUSBCore[USB_ID].pEpCtrl[EpID].MaxPacketLen = BytesGetLe16(pFullInterface->pEndpointDesc[i].wMaxPacketSize);
			if (ToHost)
			{
				prvUSBCore[USB_ID].pEpCtrl[EpID].ToHostEnable = 1;
			}
			else
			{
				prvUSBCore[USB_ID].pEpCtrl[EpID].ToDeviceEnable = 1;
				OS_ReInitBuffer(&prvUSBCore[USB_ID].pEpCtrl[EpID].RxBuf, prvUSBCore[USB_ID].pEpCtrl[EpID].MaxPacketLen * 2);
			}
			USB_DBG("interface %d, Ep %x address %x fifo %d type %d", Index, i,
					pFullInterface->pEndpointDesc[i].bEndpointAddress,
					prvUSBCore[USB_ID].pEpCtrl[EpID].MaxPacketLen, prvUSBCore[USB_ID].pEpCtrl[EpID].XferType);
		}
	}
}

static void prvUSB_StackMakeFullConfigData(uint8_t USB_ID)
{
	usb_full_config_t *pConfig;
	usb_config_descriptor_t TempConfigDesc;
	uint32_t i, Index;
	volatile uint16_t TotalLen;

	pConfig = prvUSBCore[USB_ID].Setup.pCurConfigInfo->pFullConfig[prvUSBCore[USB_ID].Setup.CurSpeed];
	memcpy(&TempConfigDesc, pConfig->pConfigDesc, sizeof(usb_config_descriptor_t));
	if (!prvUSBCore[USB_ID].BusPowered)
	{
		TempConfigDesc.bmAttributes &= ~UC_BUS_POWERED;
		TempConfigDesc.bmAttributes |= UC_SELF_POWERED;
	}
	else
	{
		TempConfigDesc.bmAttributes |= UC_BUS_POWERED;
	}
	TotalLen = sizeof(usb_config_descriptor_t);
	if (pConfig->pInterfaceAssocDesc)
	{
		TotalLen += sizeof(usb_interface_assoc_descriptor_t);
	}
	for(Index = 0; Index < pConfig->InterfaceNum; Index++)
	{
		TotalLen += sizeof(usb_interface_descriptor_t);

		if (pConfig->pInterfaceFullDesc[Index].GetOtherDesc)
		{
			TotalLen += pConfig->pInterfaceFullDesc[Index].GetOtherDesc(NULL, NULL);
		}
		if (USB_DEVICE_SPEED_SUPER_SPEED == prvUSBCore[USB_ID].Setup.CurSpeed)
		{
			for(i = 0; i < pConfig->pInterfaceFullDesc[Index].EndpointNum; i++)
			{
				TotalLen += sizeof(usb_endpoint_descriptor_t);
				TotalLen += sizeof(usb_endpoint_ss_comp_descriptor_t);
			}
		}
		else
		{
			for(i = 0; i < pConfig->pInterfaceFullDesc[Index].EndpointNum; i++)
			{
				TotalLen += sizeof(usb_endpoint_descriptor_t);
			}
		}
	}
	OS_ReInitBuffer(&prvUSBCore[USB_ID].FullConfigDataBuf, TotalLen);
	BytesPutLe16(TempConfigDesc.wTotalLength, TotalLen);
	TempConfigDesc.bConfigurationValue = prvUSBCore[USB_ID].ConfigNo;
	OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, &TempConfigDesc, sizeof(usb_config_descriptor_t));

	if (pConfig->pInterfaceAssocDesc)
	{
		OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, pConfig->pInterfaceAssocDesc, sizeof(usb_interface_assoc_descriptor_t));
	}
	for(Index = 0; Index < pConfig->InterfaceNum; Index++)
	{
		OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, pConfig->pInterfaceFullDesc[Index].pInterfaceDesc, sizeof(usb_interface_descriptor_t));
		if (pConfig->pInterfaceFullDesc[Index].GetOtherDesc)
		{
			prvUSBCore[USB_ID].FullConfigDataBuf.Pos += pConfig->pInterfaceFullDesc[Index].GetOtherDesc(prvUSBCore[USB_ID].FullConfigDataBuf.Data + prvUSBCore[USB_ID].FullConfigDataBuf.Pos, NULL);
		}
		if (USB_DEVICE_SPEED_SUPER_SPEED == prvUSBCore[USB_ID].Setup.CurSpeed)
		{
			for(i = 0; i < pConfig->pInterfaceFullDesc[Index].EndpointNum; i++)
			{
				OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, &pConfig->pInterfaceFullDesc[Index].pEndpointDesc[i], sizeof(usb_endpoint_descriptor_t));
				OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, &pConfig->pInterfaceFullDesc[Index].pEndpointSSCompDesc[i], sizeof(usb_endpoint_ss_comp_descriptor_t));
			}
		}
		else
		{
			for(i = 0; i < pConfig->pInterfaceFullDesc[Index].EndpointNum; i++)
			{
				OS_BufferWrite(&prvUSBCore[USB_ID].FullConfigDataBuf, &pConfig->pInterfaceFullDesc[Index].pEndpointDesc[i], sizeof(usb_endpoint_descriptor_t));
			}
		}

	}

	OS_DeInitBuffer(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf);
	switch(prvUSBCore[USB_ID].Setup.CurSpeed)
	{
	case USB_DEVICE_SPEED_FULL_SPEED:
		pConfig = prvUSBCore[USB_ID].Setup.pCurConfigInfo->pFullConfig[USB_DEVICE_SPEED_HIGH_SPEED];
		if (pConfig)
		{
			goto MAKE_OTHER_SPEED;
		}
		break;
	case USB_DEVICE_SPEED_HIGH_SPEED:
		pConfig = prvUSBCore[USB_ID].Setup.pCurConfigInfo->pFullConfig[USB_DEVICE_SPEED_FULL_SPEED];
		if (pConfig)
		{
			goto MAKE_OTHER_SPEED;
		}
		break;
	default:

		return;
	}
	return;
MAKE_OTHER_SPEED:
	OS_ReInitBuffer(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf, TotalLen);
	memcpy(&TempConfigDesc, pConfig->pConfigDesc, sizeof(usb_config_descriptor_t));
	BytesPutLe16(TempConfigDesc.wTotalLength, TotalLen);
	TempConfigDesc.bDescriptorType = UDESC_OTHER_SPEED_CONFIGURATION;
	TempConfigDesc.bConfigurationValue = prvUSBCore[USB_ID].ConfigNo;
	if (!prvUSBCore[USB_ID].BusPowered)
	{
		TempConfigDesc.bmAttributes &= ~UC_BUS_POWERED;
		TempConfigDesc.bmAttributes |= UC_SELF_POWERED;
	}
	else
	{
		TempConfigDesc.bmAttributes |= UC_BUS_POWERED;
	}
	OS_BufferWrite(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf, &TempConfigDesc, sizeof(usb_config_descriptor_t));
	if (pConfig->pInterfaceAssocDesc)
	{
		OS_BufferWrite(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf, pConfig->pInterfaceAssocDesc, sizeof(usb_interface_assoc_descriptor_t));
	}
	for(Index = 0; Index < pConfig->InterfaceNum; Index++)
	{
		OS_BufferWrite(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf, pConfig->pInterfaceFullDesc[Index].pInterfaceDesc, sizeof(usb_interface_descriptor_t));
		if (pConfig->pInterfaceFullDesc[Index].GetOtherDesc)
		{
			prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf.Pos += pConfig->pInterfaceFullDesc[Index].GetOtherDesc(prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf.Data + prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf.Pos, NULL);
		}
		for(i = 0; i < pConfig->pInterfaceFullDesc[Index].EndpointNum; i++)
		{
			OS_BufferWrite(&prvUSBCore[USB_ID].FullOtherSpeedConfigDataBuf, &pConfig->pInterfaceFullDesc[Index].pEndpointDesc[i], sizeof(usb_endpoint_descriptor_t));
		}
	}
}

void USB_StackResetEpBuffer(uint8_t USB_ID, uint8_t Index)
{
	if (prvUSBCore[USB_ID].pEpCtrl[Index].RxBuf.MaxLen > prvUSBCore[USB_ID].HWCaps.EpBufMaxLen)
	{
		OS_DeInitBuffer(&prvUSBCore[USB_ID].pEpCtrl[Index].RxBuf);
		OS_InitBuffer(&prvUSBCore[USB_ID].pEpCtrl[Index].RxBuf, prvUSBCore[USB_ID].pEpCtrl[Index].MaxPacketLen * 2);
	}
	else
	{
		prvUSBCore[USB_ID].pEpCtrl[Index].RxBuf.Pos = 0;
	}
	memset(&prvUSBCore[USB_ID].pEpCtrl[Index].TxBuf, 0, sizeof(Buffer_Struct));
}

void USB_StackSetControl(uint8_t USB_ID, HANDLE pHWCtrl,USB_EndpointCtrlStruct *pEpCtrl, USB_HWCapsStruct *Caps)
{
	uint32_t i;
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USBCore->pHWCtrl = pHWCtrl;
	memcpy(&USBCore->HWCaps, Caps, sizeof(USB_HWCapsStruct));
	for(i = 0; i < USB_EP_MAX; i++)
	{
		USBCore->pEpCtrl[i].CB = prvUSB_StackDummyEpCB;
	}
	USBCore->SelfID = USB_ID;
	USBCore->pEpCtrl = pEpCtrl;
	prvUSBCore[USB_ID].WaitDataTimer = Timer_Create(prvUSB_SetupRxTimeout, pHWCtrl, NULL);
}

void USB_StackClearSetup(uint8_t USB_ID)
{
	USB_SetupInfoStruct *pSetup = &prvUSBCore[USB_ID].Setup;
	uint32_t i, j;
	if (pSetup->pString)
	{
		for(i = 0;i < pSetup->StringNum; i++)
		{
			OS_DeInitBuffer(&pSetup->pString[i]);
		}
	}
	free(pSetup->pString);
	memset(&prvUSBCore[USB_ID].Setup, 0, sizeof(USB_SetupInfoStruct));
	for(i = 0; i < USB_EP_MAX; i++)
	{
		prvUSB_ResetEpCtrl(USB_ID, i);
//		prvUSBCore[USB_ID].pEpCtrl[i].CB = prvUSB_StackDummyEpCB;
	}
	prvUSBCore[USB_ID].Ep0Stage = USB_EP0_STAGE_SETUP;
	prvUSBCore[USB_ID].pEpCtrl[0].ForceZeroPacket = 1;
	prvUSBCore[USB_ID].Setup.CB = prvUSB_StackDummyStateCB;
}

void USB_StackDeviceAfterDisconnect(uint8_t USB_ID)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	PV_Union uPV;
	uint8_t i;
	uPV.u8[0] = USB_ID;
	uPV.u8[1] = USBD_BUS_TYPE_ENABLE_CONNECT;
	USBCore->Setup.CB(uPV.u32, USBCore->Setup.pUserData);
}

void USB_StackSetDeviceConfig(uint8_t USB_ID, usb_device_descriptor_t *pDeviceDesc, USB_FullConfigStruct *pConfigInfo, uint8_t ConfigNum, uint8_t StringNum, CBFuncEx_t CB, void *pUserData)
{
	USB_SetupInfoStruct *pSetup = &prvUSBCore[USB_ID].Setup;
	uint32_t i;
	if (pSetup->pDeviceDesc)
	{
		USB_ERR("clear device info first");
		return;
	}
	pSetup->pDeviceDesc = pDeviceDesc;
	if (CB)
	{
		pSetup->CB = CB;
	}
	else
	{
		pSetup->CB = prvUSB_StackDummyStateCB;
	}
	pSetup->pUserData = pUserData;
	pSetup->pConfigInfo = pConfigInfo;
	pSetup->ConfigNum = ConfigNum;

	pSetup->pString = zalloc(sizeof(Buffer_Struct) * StringNum);
	pSetup->StringNum = StringNum;
	prvUSBCore[USB_ID].pEpCtrl[0].ToDeviceEnable = 1;
	prvUSBCore[USB_ID].pEpCtrl[0].ToHostEnable = 1;
	prvUSBCore[USB_ID].DefaultConfigNo = 0;
	prvUSBCore[USB_ID].ConfigNo = 0;
	OS_ReInitBuffer(&prvUSBCore[USB_ID].pEpCtrl[0].RxBuf, pSetup->pDeviceDesc->bMaxPacketSize * 2);
}

void USB_StackSetDeviceSpeed(uint8_t USB_ID, uint8_t Speed)
{
	prvUSBCore[USB_ID].Setup.CurSpeed = Speed;
}

void USB_StackSetEpCB(uint8_t USB_ID, uint8_t Index, CBFuncEx_t CB, void *pUserData)
{
	USB_SetupInfoStruct *pSetup = &prvUSBCore[USB_ID].Setup;
	if (!CB)
	{
		prvUSBCore[USB_ID].pEpCtrl[Index].CB = prvUSB_StackDummyEpCB;
	}
	else
	{
		prvUSBCore[USB_ID].pEpCtrl[Index].CB = CB;
	}
	prvUSBCore[USB_ID].pEpCtrl[Index].pData = pUserData;
}

void USB_StackSetString(uint8_t USB_ID, uint8_t Index, const uint8_t *Data, uint16_t Len)
{
	USB_SetupInfoStruct *pSetup = &prvUSBCore[USB_ID].Setup;
	if (Index >= pSetup->StringNum)
	{
		USB_ERR("over index %d,%d", Index, pSetup->StringNum);
		return;
	}
	OS_ReInitBuffer(&pSetup->pString[Index], Len + 2);
	pSetup->pString[Index].Data[0] = Len + 2;
	pSetup->pString[Index].Data[1] = UDESC_STRING;
	memcpy(pSetup->pString[Index].Data, Data, Len);
}

void USB_StackSetCharString(uint8_t USB_ID, uint8_t Index, const char *Data, uint16_t Len)
{
	uint16_t i;
	USB_SetupInfoStruct *pSetup = &prvUSBCore[USB_ID].Setup;
	if (Index >= pSetup->StringNum)
	{
		USB_ERR("over index %d,%d", Index, pSetup->StringNum);
		return;
	}
	OS_ReInitBuffer(&pSetup->pString[Index], (Len * 2) + 2);
	pSetup->pString[Index].Data[0] = (Len * 2) + 2;
	pSetup->pString[Index].Data[1] = UDESC_STRING;
	pSetup->pString[Index].Pos = 2;
	for(i = 0; i < Len; i++)
	{
		pSetup->pString[Index].Data[pSetup->pString[Index].Pos] = Data[i];
		pSetup->pString[Index].Pos+=2;
	}
}

int32_t USB_StackStop(uint8_t USB_ID)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_Stop(USBCore->pHWCtrl);
	USB_ResetStart(USBCore->pHWCtrl);
	USBCore->DeviceState = USB_STATE_DETACHED;
	return ERROR_NONE;
}

void USB_StackPowerOnOff(uint8_t USB_ID, uint8_t OnOff)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_PowerOnOff(USBCore->pHWCtrl, OnOff);

}

void USB_StackPutRxData(uint8_t USB_ID, uint8_t EpIndex, const uint8_t *Data, uint32_t Len)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	OS_BufferWrite(&USBCore->pEpCtrl[EpIndex].RxBuf, Data, Len);
}

int32_t USB_StackStartDevice(uint8_t USB_ID)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_ResetEnd(USBCore->pHWCtrl);
	USB_SetWorkMode(USBCore->pHWCtrl, USB_MODE_DEVICE);

	USBCore->ConfigNo = 0;
	USBCore->DefaultConfigNo = 0;
	USBCore->Setup.pCurConfigInfo = &USBCore->Setup.pConfigInfo[USBCore->ConfigNo];
	prvUSB_StackEpInit(USB_ID);
	USB_InitEpCfg(USBCore->pHWCtrl);
	prvUSB_StackMakeFullConfigData(USBCore->SelfID);
	USB_Start(USBCore->pHWCtrl);
	USBCore->DeviceState = USB_STATE_DETACHED;
	return ERROR_NONE;
}

int32_t USB_StackStartHost(uint8_t USB_ID)
{
	return -1;
}

int32_t USB_StackStartOTG(uint8_t USB_ID)
{
	return -1;
}

int32_t USB_StackTxEpData(uint8_t USB_ID, uint8_t EpIndex, void *pData, uint16_t Len, uint16_t MaxLen, uint8_t ForceZeroPacket)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_SetDeviceEPStatus(USBCore->pHWCtrl, EpIndex, 0, USB_EP_STATE_ACK);
	if (USBCore->pEpCtrl[EpIndex].TxBuf.Data)
	{
		USB_FlushFifo(USBCore->pHWCtrl, EpIndex, 0);
	}
	Buffer_StaticInit(&USBCore->pEpCtrl[EpIndex].TxBuf, pData, Len);
	USBCore->pEpCtrl[EpIndex].XferMaxLen = MaxLen;
	if (!EpIndex)
	{
		USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_HOST;
	}
	else
	{
		USBCore->pEpCtrl[EpIndex].ForceZeroPacket = ForceZeroPacket;
		if (USBCore->pEpCtrl[EpIndex].TxBuf.Data)
		{
			USB_DeviceXfer(USBCore->pHWCtrl, EpIndex);
		}
		else
		{
			DBG("!");
			USB_SendZeroPacket(USBCore->pHWCtrl, EpIndex);
		}
	}

	return ERROR_NONE;
}

void USB_StackStopDeviceTx(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsNeedNotify)
{
	if (EpIndex)
	{
		USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
		USB_EndpointCtrlStruct *pEpCtrl = &USBCore->pEpCtrl[EpIndex];
		USB_EndpointDataStruct EpData;
		EpData.USB_ID = USB_ID;
		EpData.EpIndex = EpIndex;
		EpData.IsToDevice = 0;
		EpData.Len = 0xffffffff;
		USB_DeviceXferStop(USBCore->pHWCtrl, EpIndex);
		memset(&pEpCtrl->TxBuf, 0, sizeof(Buffer_Struct));
		if (IsNeedNotify)
		{
			pEpCtrl->CB(&EpData, pEpCtrl->pData);
		}
	}
}

void USB_StackEpIntOnOff(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsToDevice, uint8_t OnOff)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_EpIntOnOff(USB_ID, EpIndex, IsToDevice, OnOff);
}

void USB_StackSetEp0Stage(uint8_t USB_ID, uint8_t Stage)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	if (USBCore->Ep0Stage != USB_EP0_STAGE_SETUP)
	{
		USBCore->Ep0Stage = Stage;
	}

}

void USB_StackSetRxEpDataLen(uint8_t USB_ID, uint8_t EpIndex, uint32_t Len)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USBCore->pEpCtrl[EpIndex].XferMaxLen = Len;
}

void USB_StackSetEpStatus(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsToDevice, uint8_t Status)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
//	DBG("%d,%d,%d", EpIndex, IsToDevice, Status);
	USB_SetDeviceEPStatus(USBCore->pHWCtrl, EpIndex, IsToDevice, Status);
}

void USB_StackDeviceBusChange(uint8_t USB_ID, uint8_t Type)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	PV_Union uPV;
	uint8_t i;
	uPV.u8[0] = USB_ID;
	uPV.u8[1] = Type;
	switch (Type)
	{
	case USBD_BUS_TYPE_SUSPEND:
		PM_SetDriverRunFlag(PM_DRV_USB, 0);
		Core_USBAction(USB_ID, SERV_USB_SUSPEND, USBCore->pHWCtrl);
		USBCore->Setup.CB(uPV.u32, USBCore->Setup.pUserData);
		break;
	case USBD_BUS_TYPE_RESUME:
		Core_USBAction(USB_ID, SERV_USB_RESUME, USBCore->pHWCtrl);
		USBCore->Setup.CB(uPV.u32, USBCore->Setup.pUserData);
		break;
	case USBD_BUS_TYPE_RESET:
		PM_SetDriverRunFlag(PM_DRV_USB, 1);
		for(i = 0; i < USB_EP_MAX; i++)
		{
			USB_StackResetEpBuffer(USB_ID, i);
		}
		USBCore->DeviceState = USB_STATE_ATTACHED;
		USBCore->Setup.CB(uPV.u32, USBCore->Setup.pUserData);
		break;
	case USBD_BUS_TYPE_NEW_SOF:
		break;
	case USBD_BUS_TYPE_DISCONNECT:
		PM_SetDriverRunFlag(PM_DRV_USB, 0);
		if (USBCore->DeviceState != USB_STATE_DETACHED)
		{
			USB_Stop(USBCore->pHWCtrl);
			USB_ResetStart(USBCore->pHWCtrl);
			USBCore->DeviceState = USB_STATE_DETACHED;
			//交给用户层决定是继续保持device，还是变成OTG
			Core_USBAction(USB_ID, SERV_USB_RESET_END, USBCore->pHWCtrl);
		}
		break;
	}
}

static int32_t prvUSB_DeviceNoSupport(void *pData, void *pParam)
{
	return -1;
}

static int32_t prvUSB_DeviceGetStatus(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	uint8_t EpID, ToHost;
	switch(Request->bmRequestType & UT_RECIP_MASK)
	{
	case UT_RECIP_DEVICE:
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->DeviceStatus, 2);
		break;
	case UT_RECIP_INTERFACE:
		if (Request->wIndex[0] >= USBCore->Setup.pCurConfigInfo->pFullConfig[USBCore->Setup.CurSpeed]->InterfaceNum)
		{
			return -1;
		}
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->InterfaceStatus, 2);
		break;
	case UT_RECIP_ENDPOINT:
		ToHost = UE_GET_DIR(Request->wIndex[0]);
		EpID = UE_GET_ADDR(Request->wIndex[0]);
		if (EpID >= USB_EP_MAX)
		{
			return -1;
		}
		if (ToHost)
		{
			Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->pEpCtrl[EpID].ToHostStatus, 2);
		}
		else
		{
			Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->pEpCtrl[EpID].ToDeviceStatus, 2);
		}
		break;
	default:
		return -1;
		break;
	}
	USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_HOST;
	return 0;
}

static int32_t prvUSB_DeviceClearFeature(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	uint16_t wValue = BytesGetLe16(Request->wValue);
	uint8_t EpID, ToHost;
	USBCore->Ep0Stage = USB_EP0_STAGE_SETUP;
	switch(Request->bmRequestType & UT_RECIP_MASK)
	{
	case UT_RECIP_DEVICE:
		switch(wValue)
		{
		case UF_DEVICE_REMOTE_WAKEUP:
			if (USBCore->HWCaps.FEATURE_b.RemoteWakeup)
			{
				USBCore->DEVSTATUS_b.RemoteWakeupEnable = 0;
				USBCore->INFSTATUS_b.RemoteWakeupEnable = 0;
			}
			else
			{
				return -1;
			}
			break;
		case UF_U1_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.U1)
			{
				USBCore->DEVSTATUS_b.U1Enable = 0;
			}
			else
			{
				return -1;
			}
			break;
		case UF_U2_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.U2)
			{
				USBCore->DEVSTATUS_b.U2Enable = 0;
			}
			else
			{
				return -1;
			}
			break;
		case UF_LTM_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.LTM)
			{
				USBCore->DEVSTATUS_b.LTMEnable = 0;
			}
			else
			{
				return -1;
			}
			break;
		default:
			return -1;
			break;
		}
		break;
	case UT_RECIP_INTERFACE:
		if (Request->wIndex[0] >= USBCore->Setup.pCurConfigInfo->pFullConfig[USBCore->Setup.CurSpeed]->InterfaceNum)
		{
			return -1;
		}
		switch(wValue)
		{
		case UF_DEVICE_SUSPEND:
			break;
		default:
			return -1;
			break;
		}
		break;
	case UT_RECIP_ENDPOINT:
		Request->wIndex[0] &= 0x00ff;
		ToHost = UE_GET_DIR(Request->wIndex[0]);
		EpID = UE_GET_ADDR(Request->wIndex[0]);
		if (EpID >= USB_EP_MAX)
		{
			return -1;
		}
		if (ToHost)
		{
			switch(wValue)
			{
			case UF_ENDPOINT_HALT:
				USBCore->pEpCtrl[EpID].INSTATUS_b.Halt = 0;
				break;
			default:
				break;
			}
		}
		else
		{
			switch(wValue)
			{
			case UF_ENDPOINT_HALT:
				USBCore->pEpCtrl[EpID].OUTSTATUS_b.Halt = 0;
				break;
			default:
				break;
			}
		}
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

static int32_t prvUSB_DeviceSetFeature(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	uint8_t EpID, ToHost;
	USBCore->Ep0Stage = USB_EP0_STAGE_SETUP;
	if (USBCore->DeviceState < USB_STATE_ADDRESSED)
	{
		return -1;
	}
	switch(Request->bmRequestType & UT_RECIP_MASK)
	{
	case UT_RECIP_DEVICE:
		switch(Request->wValue[0])
		{
		case UF_DEVICE_REMOTE_WAKEUP:
			if (USBCore->HWCaps.FEATURE_b.RemoteWakeup)
			{
				USBCore->DEVSTATUS_b.RemoteWakeupEnable = 1;
				USBCore->INFSTATUS_b.RemoteWakeupEnable = 1;
			}
			else
			{
				return -1;
			}
			break;
		case UF_U1_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.U1)
			{
				USBCore->DEVSTATUS_b.U1Enable = 1;
			}
			else
			{
				return -1;
			}
			break;
		case UF_U2_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.U2)
			{
				USBCore->DEVSTATUS_b.U2Enable = 1;
			}
			else
			{
				return -1;
			}
			break;
		case UF_LTM_ENABLE:
			if (USBCore->HWCaps.FEATURE_b.LTM)
			{
				USBCore->DEVSTATUS_b.LTMEnable = 1;
			}
			else
			{
				return -1;
			}
			break;
		default:
			return -1;
			break;
		}
		break;
	case UT_RECIP_INTERFACE:
		if (Request->wIndex[0] >= USBCore->Setup.pCurConfigInfo->pFullConfig[USBCore->Setup.CurSpeed]->InterfaceNum)
		{
			return -1;
		}
		switch(Request->wValue[0])
		{
		case UF_DEVICE_SUSPEND:
			if (Request->wIndex[1] & 0x01)
			{
				//设备准备休眠
				Core_USBAction(USBCore->SelfID, SERV_USB_SUSPEND, USBCore->pHWCtrl);
			}
			else
			{
				//设备唤醒
				USB_ResumeStart(USBCore->pHWCtrl);
				Core_USBAction(USBCore->SelfID, SERV_USB_RESUME, USBCore->pHWCtrl);
			}
			if (Request->wIndex[1] & 0x02)
			{
				USBCore->DEVSTATUS_b.RemoteWakeupEnable = 1;
				USBCore->INFSTATUS_b.RemoteWakeupEnable = 1;
			}
			else
			{
				USBCore->DEVSTATUS_b.RemoteWakeupEnable = 0;
				USBCore->INFSTATUS_b.RemoteWakeupEnable = 0;
			}

			break;
		default:
			break;
		}
		break;
	case UT_RECIP_ENDPOINT:
		Request->wIndex[0] &= 0x00ff;
		ToHost = UE_GET_DIR(Request->wIndex[0]);
		EpID = UE_GET_ADDR(Request->wIndex[0]);
		if (EpID >= USB_EP_MAX)
		{
			return -1;
		}
		if (ToHost)
		{
			switch(Request->wValue[0])
			{
			case UF_ENDPOINT_HALT:
				USBCore->pEpCtrl[EpID].INSTATUS_b.Halt = 1;
				break;
			default:
				break;
			}
		}
		else
		{
			switch(Request->wValue[0])
			{
			case UF_ENDPOINT_HALT:
				USBCore->pEpCtrl[EpID].OUTSTATUS_b.Halt = 1;
				break;
			default:
				break;
			}
		}
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

static int32_t prvUSB_DeviceSetAddress(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	USB_SetDeviceAddress(USBCore->pHWCtrl, Request->wValue[0]);
	if (!USBCore->ConfigNo)
	{
		USBCore->ConfigNo = 0;
		USBCore->DefaultConfigNo = 0;
		USBCore->DeviceState = USB_STATE_ADDRESSED;
		USBCore->Setup.pCurConfigInfo = &USBCore->Setup.pConfigInfo[USBCore->ConfigNo];
		prvUSB_StackEpInit(USBCore->SelfID);
		USB_ReInitEpCfg(USBCore->pHWCtrl);
		prvUSB_StackMakeFullConfigData(USBCore->SelfID);
	}

	return 0;
}

static int32_t prvUSB_DeviceGetDescriptor(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	uint16_t TxLen;
	uint8_t CurSpeed = USBCore->Setup.CurSpeed;
	USBCore->pEpCtrl[0].XferMaxLen = BytesGetLe16(Request->wLength);
	USB_EndpointDataStruct EpData;

	switch (Request->wValue[1])
	{
	case UDESC_DEVICE:
		TxLen = MIN(USBCore->pEpCtrl[0].XferMaxLen, sizeof(usb_device_descriptor_t));
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, USBCore->Setup.pDeviceDesc, TxLen);
		break;
	case UDESC_CONFIG:
		TxLen = MIN(USBCore->pEpCtrl[0].XferMaxLen, USBCore->FullConfigDataBuf.Pos);
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, USBCore->FullConfigDataBuf.Data, TxLen);
		break;
	case UDESC_STRING:
		if (Request->wValue[0] < USBCore->Setup.StringNum)
		{
			TxLen = MIN(USBCore->pEpCtrl[0].XferMaxLen, USBCore->Setup.pString[Request->wValue[0]].MaxLen);
			Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, USBCore->Setup.pString[Request->wValue[0]].Data, TxLen);
			break;
		}
		else
		{
			return -1;
		}
	case UDESC_DEVICE_QUALIFIER:
		if (USBCore->HWCaps.FEATURE_b.HighSpeed && USBCore->HWCaps.FEATURE_b.FullSpeed && !USBCore->HWCaps.FEATURE_b.SuperSpeed)
		{
			if (USBCore->Setup.pCurConfigInfo->pQualifierDesc)
			{
				TxLen = MIN(USBCore->pEpCtrl[0].XferMaxLen, sizeof(usb_device_qualifier_t));
				Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, USBCore->Setup.pCurConfigInfo->pQualifierDesc, TxLen);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
		break;
	case UDESC_OTHER_SPEED_CONFIGURATION:
		if (USBCore->HWCaps.FEATURE_b.HighSpeed && USBCore->HWCaps.FEATURE_b.FullSpeed)
		{
			if (USBCore->FullOtherSpeedConfigDataBuf.Data && USBCore->FullOtherSpeedConfigDataBuf.Pos)
			{
				TxLen = MIN(USBCore->pEpCtrl[0].XferMaxLen, USBCore->FullOtherSpeedConfigDataBuf.Pos);
				Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, USBCore->FullOtherSpeedConfigDataBuf.Data, TxLen);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
		break;
	case UDESC_BOS:
	case UDESC_OTG:
	case UDESC_DEBUG:
		return -1;
		break;
	default:
		memset(&EpData, 0, sizeof(EpData));
		EpData.USB_ID = USBCore->SelfID;
		EpData.IsToDevice = 1;
		EpData.Data = USBCore->pEpCtrl[0].RxBuf.Data;
		EpData.Len = USBCore->pEpCtrl[0].RxBuf.Pos;
		EpData.pLastRequest = Request;
		return USBCore->pEpCtrl[0].CB(&EpData, USBCore->pEpCtrl[0].pData);
		break;
	}
	USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_HOST;
	return 0;
}

static int32_t prvUSB_DeviceSetDescriptor(void *pData, void *pParam)
{
	return -1;
}

static int32_t prvUSB_DeviceGetConfiguration(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	if (Request->wLength[0] != 1)
	{
		return -1;
	}
	switch(USBCore->DeviceState)
	{
	case USB_STATE_CONFIGURED:
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->ConfigNo, 1);
		USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_HOST;
		break;
	case USB_STATE_ADDRESSED:
		Buffer_StaticInit(&USBCore->pEpCtrl[0].TxBuf, &USBCore->DefaultConfigNo, 1);
		USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_HOST;
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

static int32_t prvUSB_DeviceSetConfiguration(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	if (Request->wValue[0] >= USBCore->Setup.ConfigNum)
	{
		return -1;
	}
	USBCore->ConfigNo = Request->wValue[0];
	USBCore->Setup.pCurConfigInfo = &USBCore->Setup.pConfigInfo[USBCore->ConfigNo];
	if (!USBCore->ConfigNo)
	{
		USBCore->DeviceState = USB_STATE_ADDRESSED;
	}
	else
	{
		USBCore->DeviceState = USB_STATE_CONFIGURED;
	}
	prvUSB_StackEpInit(USBCore->SelfID);
	USB_ReInitEpCfg(USBCore->pHWCtrl);
	prvUSB_StackMakeFullConfigData(USBCore->SelfID);
	return 0;
}

static int32_t prvUSB_DeviceGetInterface(void *pData, void *pParam)
{

	return -1;
}

static int32_t prvUSB_DeviceSetInterface(void *pData, void *pParam)
{
	return -1;
}

static int32_t prvUSB_DeviceSynchFrame(void *pData, void *pParam)
{
	USB_StackCtrlStruct *USBCore = (USB_StackCtrlStruct *)pData;
	usb_device_request_t *Request = (usb_device_request_t *)pParam;
	uint8_t EpID, ToHost;
	USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_DEVICE;
	USBCore->pEpCtrl[0].XferMaxLen = 2;
	return 0;
}



static CBFuncEx_t prvUSB_StandardRequestTable[UR_SYNCH_FRAME + 1] =
{
		prvUSB_DeviceGetStatus,
		prvUSB_DeviceClearFeature,
		prvUSB_DeviceNoSupport,
		prvUSB_DeviceSetFeature,
		prvUSB_DeviceNoSupport,
		prvUSB_DeviceSetAddress,
		prvUSB_DeviceGetDescriptor,
		prvUSB_DeviceSetDescriptor,
		prvUSB_DeviceGetConfiguration,
		prvUSB_DeviceSetConfiguration,
		prvUSB_DeviceGetInterface,
		prvUSB_DeviceSetInterface,
		prvUSB_DeviceSynchFrame,
};

void USB_StackDeviceEp0TxDone(uint8_t USB_ID)
{
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USBCore->Ep0Stage = USB_EP0_STAGE_SETUP;
	USB_SetDeviceNoDataSetup(USBCore->pHWCtrl);
}

void USB_StackAnalyzeDeviceEpRx(uint8_t USB_ID, uint8_t EpIndex)
{
	USB_EndpointDataStruct EpData;
	USB_StackCtrlStruct *USBCore = &prvUSBCore[USB_ID];
	USB_EndpointCtrlStruct *pEpCtrl;
	uint16_t wTemp;
	memset(&EpData, 0, sizeof(EpData));
	EpData.USB_ID = USB_ID;
	EpData.EpIndex = EpIndex;
	EpData.IsToDevice = 1;
	EpData.Data = USBCore->pEpCtrl[EpIndex].RxBuf.Data;
	EpData.Len = USBCore->pEpCtrl[EpIndex].RxBuf.Pos;
	EpData.pLastRequest = &USBCore->LastRequest;
	if (EpIndex)
	{
		USBCore->pEpCtrl[EpIndex].RxBuf.Pos = 0;
		EpData.IsDataStage = (EpData.Len < USBCore->pEpCtrl[EpIndex].MaxPacketLen)?0:1;

		if (USBCore->pEpCtrl[EpIndex].CB(&EpData, USBCore->pEpCtrl[EpIndex].pData))
		{
			USB_SetDeviceEPStatus(USBCore->pHWCtrl, EpIndex, 1, USB_EP_STATE_STALL);
		}
	}
	else
	{
		switch(USBCore->Ep0Stage)
		{
		case USB_EP0_STAGE_SETUP:
			pEpCtrl = &USBCore->pEpCtrl[0];
			if (pEpCtrl->RxBuf.Pos != sizeof(usb_device_request_t))
			{
				USBCore->IsRequestError = 1;
				DBG_ERR("USB%d EP%d Request size %d error", USB_ID, EpIndex, pEpCtrl->RxBuf.Pos);
			}
			memcpy(&USBCore->LastRequest, pEpCtrl->RxBuf.Data, pEpCtrl->RxBuf.Pos);
			if ( !(USBCore->LastRequest.bmRequestType & UT_MASK) )
			{
				if (USBCore->LastRequest.bRequest <= UR_SYNCH_FRAME)
				{
					if (prvUSB_StandardRequestTable[USBCore->LastRequest.bRequest](USBCore, &USBCore->LastRequest))
					{
						USBCore->IsRequestError = 1;
					}
				}
				else
				{
					switch(USBCore->LastRequest.bRequest)
					{
					case UR_SET_SEL:
						if (USBCore->HWCaps.FEATURE_b.U1 && USBCore->HWCaps.FEATURE_b.U2)
						{
							wTemp = BytesGetLe16(USBCore->LastRequest.wValue);
							if (USB_SetISOCHDelay(USBCore->pHWCtrl, wTemp))
							{
								USBCore->IsRequestError = 1;
							}
						}
						else
						{
							USBCore->IsRequestError = 1;
						}

						break;
					case UR_ISOCH_DELAY:
						if (USBCore->HWCaps.FEATURE_b.U1 && USBCore->HWCaps.FEATURE_b.U2)
						{
							pEpCtrl->XferMaxLen = BytesGetLe16(USBCore->LastRequest.wLength);
							USBCore->Ep0Stage = USB_EP0_STAGE_DATA_TO_DEVICE;
						}
						else
						{
							USBCore->IsRequestError = 1;
						}
						break;
					default:
						if (USBCore->pEpCtrl[EpIndex].CB(&EpData, USBCore->pEpCtrl[EpIndex].pData))
						{
							USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
							USBCore->IsRequestError = 1;
						}
						break;
					}


				}

			}
			else
			{
				if (USBCore->pEpCtrl[EpIndex].CB(&EpData, USBCore->pEpCtrl[EpIndex].pData))
				{
					USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
					USBCore->IsRequestError = 1;
				}
			}
			if (USBCore->IsRequestError)
			{
				if (USBCore->LastRequest.bRequest != UR_GET_DESCRIPTOR && USBCore->LastRequest.wValue[1] != UDESC_DEVICE_QUALIFIER)
				{
					DBG_ERR("USB%d EP%d Request %x error", USB_ID, EpIndex, USBCore->LastRequest.bRequest);
					DBG_HexPrintf(USBCore->pEpCtrl[0].RxBuf.Data, USBCore->pEpCtrl[0].RxBuf.Pos);
				}
				USBCore->IsRequestError = 0;
				USBCore->Ep0Stage = USB_EP0_STAGE_SETUP;
				USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
			}
			USBCore->pEpCtrl[0].RxBuf.Pos = 0;
			switch(USBCore->Ep0Stage)
			{
			case USB_EP0_STAGE_SETUP:
				USB_SetDeviceNoDataSetup(USBCore->pHWCtrl);
				break;
			case USB_EP0_STAGE_DATA_TO_HOST:
				USB_DeviceXfer(USBCore->pHWCtrl, 0);
				break;
			case USB_EP0_STAGE_DATA_TO_DEVICE:
				Timer_StartMS(USBCore->WaitDataTimer, 100, 0);
				break;
			}
			pEpCtrl->RxBuf.Pos = 0;
			break;
		case USB_EP0_STAGE_DATA_TO_HOST:
			USB_ERR("!!!");
			USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
			break;
		case USB_EP0_STAGE_DATA_TO_DEVICE:
			if (USBCore->pEpCtrl[0].RxBuf.Pos >= USBCore->pEpCtrl[0].XferMaxLen)
			{
				Timer_Stop(USBCore->WaitDataTimer);
				EpData.IsDataStage = 1;
				USB_DBG("setup data out done!");
				if (USBCore->LastRequest.bRequest & UT_MASK)
				{
					switch(USBCore->LastRequest.bRequest)
					{
					case UR_SYNCH_FRAME:
						USB_DBG("frame sn %u", BytesGetLe16(USBCore->pEpCtrl[0].RxBuf.Data));
						break;
					default:
						if (USBCore->pEpCtrl[0].CB(&EpData, USBCore->pEpCtrl[0].pData))
						{
							USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
						}
						break;
					}
				}
				else
				{
					if (USBCore->pEpCtrl[0].CB(&EpData, USBCore->pEpCtrl[0].pData))
					{
						USB_SetDeviceEPStatus(USBCore->pHWCtrl, 0, 1, USB_EP_STATE_STALL);
					}
				}
				USBCore->pEpCtrl[0].RxBuf.Pos = 0;
			}
			break;
		}
	}

}

void USB_StackSetDeviceSelfPower(uint8_t USB_ID, uint8_t OnOff)
{
	prvUSBCore[USB_ID].DEVSTATUS_b.SelfPower = OnOff;
}

void USB_StackSetDeviceBusPower(uint8_t USB_ID, uint8_t OnOff)
{
	prvUSBCore[USB_ID].BusPowered = OnOff;
}

void USB_StackSetDeviceRemoteWakeupCapable(uint8_t USB_ID, uint8_t OnOff)
{
	if (prvUSBCore[USB_ID].HWCaps.FEATURE_b.RemoteWakeup)
	{
		prvUSBCore[USB_ID].INFSTATUS_b.RemoteWakeCapable = OnOff;
	}
	else
	{
		prvUSBCore[USB_ID].INFSTATUS_b.RemoteWakeCapable = 0;
	}
}
