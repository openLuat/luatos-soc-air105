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

#ifndef __USB_DRIVER_H__
#define __USB_DRIVER_H__
#include "usb_define.h"
#include "usb_host.h"
#include "usb_device.h"
#include "bsp_common.h"
enum
{
	USB_EP_STATE_DISABLE,
	USB_EP_STATE_ENABLE,
	USB_EP_STATE_STALL,
	USB_EP_STATE_NAK,
	USB_EP_STATE_ACK,
	USB_EP_STATE_NYET,

	USB_EP0_STAGE_SETUP = 0,
	USB_EP0_STAGE_DATA_TO_DEVICE,
	USB_EP0_STAGE_DATA_TO_HOST,

	SERV_USB_RESET_END = 0,
	SERV_USB_RESUME_END,
	SERV_USB_SUSPEND,
	SERV_USB_RESUME,

};


typedef struct
{
	volatile Buffer_Struct RxBuf;
	volatile Buffer_Struct TxBuf;
	CBFuncEx_t CB;
	void *pData;
	uint32_t XferMaxLen;
	uint16_t MaxPacketLen;
	uint16_t TimeoutMS;
	union
	{
		uint16_t ToHostStatus;
		struct
		{
			uint16_t Halt:1;
			uint16_t Zero:15;
		}INSTATUS_b;
	};
	union
	{
		uint16_t ToDeviceStatus;
		struct
		{
			uint16_t Halt:1;
			uint16_t Zero:15;
		}OUTSTATUS_b;
	};
	uint8_t XferType;
	uint8_t ToHostEnable;
	uint8_t ToDeviceEnable;
	uint8_t ForceZeroPacket;
	uint8_t NeedZeroPacket;
	uint8_t TxZeroPacket;

}USB_EndpointCtrlStruct;

typedef struct
{
	uint8_t *Data;
	uint32_t Len;
	usb_device_request_t *pLastRequest;
	uint8_t USB_ID;
	uint8_t EpIndex;
	uint8_t IsToDevice;
	uint8_t IsDataStage;
}USB_EndpointDataStruct;

typedef struct
{
	uint32_t EpBufMaxLen;
	union
	{
		uint32_t Feature;
		struct
		{
			uint32_t Suspend:1;
			uint32_t RemoteWakeup:1;
			uint32_t U1:1;
			uint32_t U2:1;
			uint32_t LTM:1;
			uint32_t FullSpeed:1;
			uint32_t HighSpeed:1;
			uint32_t SuperSpeed:1;
		}FEATURE_b;
	};
	uint8_t CtrlMode;	//see usb_hc_mode

}USB_HWCapsStruct;

/*************usb common api****************/
void USB_StackSetControl(uint8_t USB_ID, HANDLE pHWCtrl,USB_EndpointCtrlStruct *pEpCtrl, USB_HWCapsStruct *Caps);
int32_t USB_StackStop(uint8_t USB_ID);
void USB_StackPutRxData(uint8_t USB_ID, uint8_t EpIndex, const uint8_t *Data, uint32_t Len);
void USB_StackResetEpBuffer(uint8_t USB_ID, uint8_t Index);
void USB_StackSetEpStatus(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsToDevice, uint8_t Status);
int32_t USB_StackTxEpData(uint8_t USB_ID, uint8_t EpIndex, void *pData, uint32_t Len, uint32_t MaxLen, uint8_t ForceZeroPacket);
void USB_StackSetRxEpDataLen(uint8_t USB_ID, uint8_t EpIndex, uint32_t Len);
void USB_StackEpIntOnOff(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsToDevice, uint8_t OnOff);
/*************usb class api*******************/
/*************usb device api******************/
void USB_StackClearSetup(uint8_t ID);
void USB_StackSetDeviceSelfPower(uint8_t USB_ID, uint8_t OnOff);
void USB_StackSetDeviceRemoteWakeupCapable(uint8_t USB_ID, uint8_t OnOff);
void USB_StackSetDeviceSpeed(uint8_t USB_ID, uint8_t Speed);
void USB_StackSetDeviceConfig(uint8_t USB_ID, usb_device_descriptor_t *pDeviceDesc, USB_FullConfigStruct *pConfigInfo, uint8_t ConfigNum, uint8_t StringNum, CBFuncEx_t CB, void *pUserData);
void USB_StackSetString(uint8_t USB_ID, uint8_t Index, const uint8_t *Data, uint16_t Len);
void USB_StackSetCharString(uint8_t USB_ID, uint8_t Index, const char *Data, uint16_t Len);
void USB_StackSetEpCB(uint8_t USB_ID, uint8_t Index, CBFuncEx_t CB, void *pUserData);
int32_t USB_StackStartDevice(uint8_t USB_ID);
void USB_StackDeviceBusChange(uint8_t USB_ID, uint8_t Type);
void USB_StackDeviceEp0TxDone(uint8_t USB_ID);
void USB_StackSetEp0Stage(uint8_t USB_ID, uint8_t Stage);
void USB_StackAnalyzeDeviceEpRx(uint8_t USB_ID, uint8_t EpIndex);
void USB_StackDeviceAfterDisconnect(uint8_t USB_ID);
void USB_StackStopDeviceTx(uint8_t USB_ID, uint8_t EpIndex, uint8_t IsNeedNotify);
//void USB_StackAnalyzeDeviceEpTx(uint8_t USB_ID, uint8_t EpIndex);
/*************usb host api******************/
void USB_StackHostBusChange(uint8_t USB_ID, uint8_t Type);
#endif
