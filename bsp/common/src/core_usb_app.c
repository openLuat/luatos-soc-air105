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
enum
{
	DEVICE_INTERFACE_HID_SN,
	DEVICE_INTERFACE_MSC_SN,
	DEVICE_INTERFACE_SERIAL_SN,
	DEVICE_INTERFACE_MAX,
	USB_HW_ACTION = USB_APP_EVENT_ID_START + 1,
	USBD_MSC_CB,
};

#define DEVICE_HID_KEYBOARD_EP_IN		(0x01)
#define DEVICE_HID_KEYBOARD_EP_OUT		(0x01)
#define DEVICE_MASS_STORAGE_EP_IN		(0x02)
#define DEVICE_MASS_STORAGE_EP_OUT		(0x02)
#define DEVICE_GENERIC_SERIAL_EP_IN		(0x03)
#define DEVICE_GENERIC_SERIAL_EP_OUT	(0x03)
#define VIRTUAL_SERIAL_BUFFER_LEN		(8192)
#define VIRTUAL_VHID_BUFFER_LEN			(2048)
#define VIRTUAL_UDISK_BLOCKS_LEN			(512)
#define VIRTUAL_UDISK_BLOCKS_NUMS			(32)
#define VIRTUAL_UDISK_LEN				(VIRTUAL_UDISK_BLOCKS_NUMS * VIRTUAL_UDISK_BLOCKS_LEN)
//#define TEST_SPEED

typedef struct
{
	uint32_t CurAddress;
	uint32_t BlockNums;
	uint32_t BlockSize;
	uint32_t BlockTotalNums;
	uint8_t *VirtualData;
}Virtual_UDiskCtrlStruct;

typedef struct
{
	Buffer_Struct TxBuf;
	Buffer_Struct TxCacheBuf;	// 缓存下次发送的数据
	Buffer_Struct RxBuf;
	CBFuncEx_t CB;
	uint8_t USB_ID;
	uint8_t IsReady;
	uint8_t ToHostEpIndex;
	uint8_t ToDeviceEpIndex;
}Virtual_HIDCtrlStruct;


typedef struct
{
	Buffer_Struct TxBuf;
	Buffer_Struct TxCacheBuf;	// 缓存下次发送的数据
	Buffer_Struct RxBuf;
	CBFuncEx_t CB;
	Timer_t *RxTimer;
	uint32_t RxTimeoutUS;
	uint8_t *TxData;
	uint8_t USBOnOff;
	uint8_t UartOnOff;
	uint8_t USB_ID;
	uint8_t ToHostEpIndex;
	uint8_t ToDeviceEpIndex;
}Virtual_UartCtrlStruct;

typedef struct
{
	MSC_SCSICtrlStruct tSCSI;
	Virtual_UDiskCtrlStruct tUDisk;
	DBuffer_Struct DataBuf;
	SDHC_SPICtrlStruct *pSDHC;
	HANDLE hTaskHandle;
	uint8_t *pHIDReport;
	uint16_t HIDReportLen;
	uint8_t IsUSBAttachSDHC;
	uint8_t IsStart;
}USB_AppCtrlStruct;

static const uint8_t prvCore_StandardInquiryData[STANDARD_INQUIRY_DATA_LEN] =
{
		0x00, //磁盘设备
		0x00, //其中最高位D7为RMB。RMB=0，表示不可移除设备。如果RMB=1，则为可移除设备。
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

const uint8_t prvCore_MSCPage00InquiryData[LENGTH_INQUIRY_PAGE00] =
{
	0x00,
	0x00,
	0x00,
	(LENGTH_INQUIRY_PAGE00 - 4U),
	0x00,
	0x80
};

/* USB Mass storage VPD Page 0x80 Inquiry Data for Unit Serial Number */
const uint8_t prvCore_MSCPage80InquiryData[LENGTH_INQUIRY_PAGE80] =
{
	0x00,
	0x80,
	0x00,
	LENGTH_INQUIRY_PAGE80,
	0x20,     /* Put Product Serial number */
	0x20,
	0x20,
	0x20
};

/* USB Mass storage sense 6 Data */
const uint8_t prvCore_MSCModeSense6data[MODE_SENSE6_LEN] =
{
	0x22,
	0x00,
	0x00,
	0x00,
	0x08,
	0x12,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};


/* USB Mass storage sense 10  Data */
const uint8_t prvCore_MSCModeSense10data[MODE_SENSE10_LEN] =
{
	0x00,
	0x26,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x08,
	0x12,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};

static USB_AppCtrlStruct prvUSBApp;
static Virtual_HIDCtrlStruct prvLuatOS_VirtualHID;
static Virtual_UartCtrlStruct prvLuatOS_VirtualUart[VIRTUAL_UART_MAX];

/* LangID = 0x0409: U.S. English */
static const char prvCore_StringManufacturer[] = "airm2m";
static const char prvCore_StringProduct[] = "air105 usb device";
static const char prvCore_StringSerial[] = "00001";
static const char prvCore_StringConfig[] = "luatos";
static const char *prvCore_StringInterface[3] = {
		"virtual Mass Storage Device",
		"virtual HID Keyboard",
		"virtual Serial"
};



static usb_device_descriptor_t prvCore_DeviceDesc =
{
		.bLength = USB_LEN_DEV_DESC,
		.bDescriptorType = UDESC_DEVICE,
		.bcdUSB = {0x00, 0x02},
		.bDeviceClass = UDCLASS_IN_INTERFACE,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize = 0x40,
		.idVendor = {0x82, 0x17},
		.idProduct = {0x00, 0x4e},
//		.idVendor = {0x83, 0x04},
//		.idProduct = {0x20, 0x57},
		.bcdDevice = {0x00, 0x02},
		.iManufacturer = USBD_IDX_MANUFACTURER_STR,
		.iProduct = USBD_IDX_PRODUCT_STR,
		.iSerialNumber = 0,
		.bNumConfigurations = 1,
};

static const usb_device_qualifier_t prvCore_QualifierDesc =
{
	.bLength = USB_LEN_DEV_QUALIFIER_DESC,   /* bLength  */
	.bDescriptorType = UDESC_DEVICE_QUALIFIER,   /* bDescriptorType */
	.bcdUSB = {0x00, 0x02},
	.bDeviceClass = UDCLASS_IN_INTERFACE,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 0x40,
	.bNumConfigurations = 1,
	.bReserved = 0
};

static const usb_config_descriptor_t prvCore_ConfigDesc =
{
		.bLength = USB_LEN_CFG_DESC,
		.bDescriptorType = UDESC_CONFIG,
		.wTotalLength = {0x00, 0x00},
		.bNumInterface = DEVICE_INTERFACE_MAX,
		.bConfigurationValue = 1,
		.iConfiguration = USBD_IDX_CONFIG_STR,
		.bmAttributes = UC_SELF_POWERED|UC_BUS_POWERED,
		.bMaxPower = (USB_MIN_POWER >> 1),
};

static const usb_interface_assoc_descriptor_t prvCore_AssocInterfaceDesc =
{
		.bLength = USB_LEN_ASSOC_IF_DESC,
		.bDescriptorType = UDESC_IFACE_ASSOC,
		.bFirstInterface = 0,
		.bInterfaceCount = 1,
//		.bFunctionClass = UICLASS_MASS,
//		.bFunctionSubClass = UISUBCLASS_SCSI,
//		.bFunctionProtocol = UIPROTO_MASS_BBB,
//		.bFunctionClass = 0,
//		.bFunctionSubClass = 0,
//		.bFunctionProtocol = 0,
		.bFunctionClass = UICLASS_HID,
		.bFunctionSubClass = 0,
		.bFunctionProtocol = UIPROTO_BOOT_KEYBOARD,
		.iFunction = USBD_IDX_INTERFACE0_STR,
};

static const usb_interface_descriptor_t prvCore_MassStorgeInterfaceDesc =
{
		.bLength = USB_LEN_IF_DESC,
		.bDescriptorType = UDESC_INTERFACE,
		.bInterfaceNumber = DEVICE_INTERFACE_MSC_SN,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = UICLASS_MASS,
		.bInterfaceSubClass = UISUBCLASS_SCSI,
		.bInterfaceProtocol = UIPROTO_MASS_BBB,
		.iInterface = USBD_IDX_INTERFACE0_STR,
};

static const usb_endpoint_descriptor_t prvCore_MassStorgeEndpointDesc[2] =
{
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_IN|DEVICE_MASS_STORAGE_EP_IN,
				.bmAttributes = UE_BULK,
				.wMaxPacketSize = {0x40,0x00},
				.bInterval = 10,
		},
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_OUT|DEVICE_MASS_STORAGE_EP_OUT,
				.bmAttributes = UE_BULK,
				.wMaxPacketSize = {0x40,0x00},
				.bInterval = 10,
		}
};

static const usb_interface_descriptor_t prvCore_HIDInterfaceDesc =
{
		.bLength = USB_LEN_IF_DESC,
		.bDescriptorType = UDESC_INTERFACE,
		.bInterfaceNumber = DEVICE_INTERFACE_HID_SN,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = UICLASS_HID,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = USBD_IDX_INTERFACE0_STR + 1,
};

static usb_endpoint_descriptor_t prvCore_HIDEndpointDesc[2] =
{
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_IN|DEVICE_HID_KEYBOARD_EP_IN,
				.bmAttributes = UE_INTERRUPT,
				.wMaxPacketSize = {0x08,0x00},
				.bInterval = 10,
		},
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_OUT|DEVICE_HID_KEYBOARD_EP_OUT,
				.bmAttributes = UE_INTERRUPT,
				.wMaxPacketSize = {0x08,0x00},
				.bInterval = 10,
		},
};

static const usb_interface_descriptor_t prvCore_SerialInterfaceDesc =
{
		.bLength = USB_LEN_IF_DESC,
		.bDescriptorType = UDESC_INTERFACE,
		.bInterfaceNumber = DEVICE_INTERFACE_SERIAL_SN,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = UICLASS_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = USBD_IDX_INTERFACE0_STR + 2,
};

static const usb_endpoint_descriptor_t prvCore_SerialEndpointDesc[2] =
{
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_IN|DEVICE_GENERIC_SERIAL_EP_IN,
				.bmAttributes = UE_BULK,
				.wMaxPacketSize = {0x40,0x00},
				.bInterval = 10,
		},
		{
				.bLength = USB_LEN_EP_DESC,
				.bDescriptorType = UDESC_ENDPOINT,
				.bEndpointAddress = UE_DIR_OUT|DEVICE_GENERIC_SERIAL_EP_OUT,
				.bmAttributes = UE_BULK,
				.wMaxPacketSize = {0x40,0x00},
				.bInterval = 10,
		}
};

static const usb_hid_descriptor_t  prvCore_HIDDesc =
{
		.bLength = 9,
		.bDescriptorType = USB_HID_DESCRIPTOR_TYPE,
		.bcdHID = {0x11, 0x01},
		.bCountryCode = 0x21,
		.bNumDescriptors = 1,
		.bReportDescriptorType = USB_HID_REPORT_DESC,
		.wDescriptorLength = {63,0},
};

static char prvCore_HIDCustomReportDescriptor[34] = {
	    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
	    0x09, 0x00,                    // USAGE (Undefined)
	    0xa1, 0x01,                    // COLLECTION (Application)
	    0x09, 0x00,                    //   USAGE (Undefined)
	    0x95, USB_HID_KB_DATA_CACHE,      //   REPORT_COUNT (8)
	    0x75, 0x08,      			   //   REPORT_SIZE (8)
	    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	    0x81, 0x02,                    // INPUT (Data,Var,Abs)
	    0x09, 0x00,                    //   USAGE (Undefined)
	    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	    0x95, USB_HID_KB_DATA_CACHE,      //   REPORT_COUNT (8)
	    0x75, 0x08,                    //   REPORT_SIZE (8)
	    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
	    0xc0                           // END_COLLECTION
};

static const char prvCore_HIDKeyboardReportDesc[64] = {
	    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	    0x09, 0x06,                    // USAGE (Keyboard)
	    0xa1, 0x01,                    // COLLECTION (Application)
	    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
	    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
	    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	    0x75, 0x01,                    //   REPORT_SIZE (1)
	    0x95, 0x08,                    //   REPORT_COUNT (8)
	    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	    0x95, 0x01,                    //   REPORT_COUNT (1)
	    0x75, 0x08,                    //   REPORT_SIZE (8)
	    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
	    0x95, 0x05,                    //   REPORT_COUNT (5)
	    0x75, 0x01,                    //   REPORT_SIZE (1)
	    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
	    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
	    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
	    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
	    0x95, 0x01,                    //   REPORT_COUNT (1)
	    0x75, 0x03,                    //   REPORT_SIZE (3)
	    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
	    0x95, USB_HID_KEY_CACHE,       //   REPORT_COUNT (30)
	    0x75, 0x08,                    //   REPORT_SIZE (8)
	    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	    0x26, 0xff, 0x00,             	  //   LOGICAL_MAXIMUM (255)
	    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
	    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
	    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
	    0xc0                           // END_COLLECTION
};

static int32_t prvCore_GetHIDDesc(void *pData, void *pParam)
{
	usb_hid_descriptor_t HIDDesc;
	if (pData)
	{
		HIDDesc = prvCore_HIDDesc;
		BytesPutLe16(HIDDesc.wDescriptorLength, prvUSBApp.HIDReportLen);
		memcpy(pData, &HIDDesc, sizeof(usb_hid_descriptor_t));
	}
	return sizeof(usb_hid_descriptor_t);
}

static usb_full_interface_t prvCore_InterfaceList[3] =
{
		{
				&prvCore_HIDInterfaceDesc,
				prvCore_HIDEndpointDesc,
				NULL,
				prvCore_GetHIDDesc,
				2,
		},
		{
				&prvCore_MassStorgeInterfaceDesc,
				prvCore_MassStorgeEndpointDesc,
				NULL,
				NULL,
				2,
		},
		{
				&prvCore_SerialInterfaceDesc,
				prvCore_SerialEndpointDesc,
				NULL,
				NULL,
				2,
		},
};
static const usb_full_config_t prvCore_Config =
{
		.pConfigDesc = &prvCore_ConfigDesc,
		.pInterfaceAssocDesc = &prvCore_AssocInterfaceDesc,
		.pInterfaceFullDesc = prvCore_InterfaceList,
		.InterfaceNum = 3,
};
static const USB_FullConfigStruct prvCore_FullConfig =
{
		.pQualifierDesc = NULL,
		.pFullConfig = {&prvCore_Config, NULL, NULL},
};



static int32_t prvCore_SCSIInit(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvCore_SCSIGetCapacity(uint8_t LUN, uint32_t *BlockNum, uint32_t *BlockSize, void *pUserData)
{
	*BlockNum = prvUSBApp.tUDisk.BlockTotalNums;
	*BlockSize = prvUSBApp.tUDisk.BlockSize;
	return 0;
}

static int32_t prvCore_SCSIIsReady(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvCore_SCSIIsWriteProtected(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvCore_SCSIPreRead(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData)
{
	if ((BlockAddress + BlockNums) * prvUSBApp.tUDisk.BlockSize > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	prvUSBApp.tUDisk.CurAddress = BlockAddress * prvUSBApp.tUDisk.BlockSize;
	return 0;
}

static int32_t prvCore_SCSIRead(uint8_t LUN, uint32_t Len, uint8_t **pOutData, uint32_t *OutLen, void *pUserData)
{
	if (Len + prvUSBApp.tUDisk.CurAddress > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	*pOutData = &prvUSBApp.tUDisk.VirtualData[prvUSBApp.tUDisk.CurAddress];
	*OutLen = Len;
	prvUSBApp.tUDisk.CurAddress += Len;
	return 0;
}

static int32_t prvCore_SCSIReadNext(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvCore_SCSIPreWrite(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData)
{
	if ((BlockAddress + BlockNums) * prvUSBApp.tUDisk.BlockSize > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	prvUSBApp.tUDisk.CurAddress = BlockAddress * prvUSBApp.tUDisk.BlockSize;
	return 0;
}

static int32_t prvCore_SCSIWrite(uint8_t LUN, uint8_t *Data, uint32_t Len, void *pUserData)
{
	if (Len + prvUSBApp.tUDisk.CurAddress > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	memcpy(&prvUSBApp.tUDisk.VirtualData[prvUSBApp.tUDisk.CurAddress], Data, Len);
	prvUSBApp.tUDisk.CurAddress += Len;
	return 0;
}

static int32_t prvCore_SCSIUserCmd(USB_EndpointDataStruct *pEpData, MSC_SCSICtrlStruct *pMSC)
{
	return -1;
}

static int32_t prvCore_SCSIGetMaxLUN(uint8_t *LUN, void *pUserData)
{
	*LUN = 0;
}

static const USB_StorageSCSITypeDef prvCore_SCSIFun =
{
		prvCore_SCSIGetMaxLUN,
		prvCore_SCSIInit,
		prvCore_SCSIGetCapacity,
		prvCore_SCSIIsReady,
		prvCore_SCSIIsWriteProtected,
		prvCore_SCSIPreRead,
		prvCore_SCSIRead,
		prvCore_SCSIReadNext,
		prvCore_SCSIPreWrite,
		prvCore_SCSIWrite,
		prvCore_SCSIUserCmd,
		&prvCore_StandardInquiryData,
		&prvCore_MSCPage00InquiryData,
		&prvCore_MSCPage80InquiryData,
		&prvCore_MSCModeSense6data,
		&prvCore_MSCModeSense10data,
};

static void prvCore_VUartOpen(Virtual_UartCtrlStruct *pVUart, uint8_t UartID)
{
	pVUart->USBOnOff = 1;
	OS_DeInitBuffer(&pVUart->TxBuf);
	if (pVUart->TxBuf.Data)
	{
		DBG("send last data %ubyte", pVUart->TxBuf.MaxLen);
		USB_StackTxEpData(pVUart->USB_ID, pVUart->ToHostEpIndex, pVUart->TxBuf.Data, pVUart->TxBuf.MaxLen, pVUart->TxBuf.MaxLen, 0);
	}
	else if (pVUart->TxCacheBuf.Pos)
	{
		DBG("send cache data %ubyte", pVUart->TxCacheBuf.Pos);
		Buffer_StaticInit(&pVUart->TxBuf, pVUart->TxCacheBuf.Data, pVUart->TxCacheBuf.Pos);
		OS_InitBuffer(&pVUart->TxCacheBuf, VIRTUAL_SERIAL_BUFFER_LEN);
		USB_StackTxEpData(pVUart->USB_ID, pVUart->ToHostEpIndex, pVUart->TxBuf.Data, pVUart->TxBuf.MaxLen, pVUart->TxBuf.MaxLen, 0);
	}
	pVUart->CB(UartID, UART_CB_CONNECTED);
}

static int32_t prvCore_VUartRxTimeoutCB(void *pData, void *pParam)
{
	uint32_t UartID = (uint32_t)pParam;
	prvLuatOS_VirtualUart[UartID].CB(UartID, UART_CB_RX_TIMEOUT);
}

static int32_t prvCore_VHIDDummyCB(void *pData, void *pParam)
{
	return 0;
}

static int32_t prvCore_VUartDummyCB(void *pData, void *pParam)
{
	return 0;
}

static int32_t prvCore_VMSCDummyCB(void *pData, void *pParam)
{
	return 0;
}

static void prvCore_VHIDSetReady(uint8_t OnOff)
{
	if (prvLuatOS_VirtualHID.IsReady != OnOff)
	{
		prvLuatOS_VirtualHID.IsReady = OnOff;
		prvLuatOS_VirtualHID.CB(prvLuatOS_VirtualHID.USB_ID, prvLuatOS_VirtualHID.IsReady);
	}
}

static int32_t prvCore_USBEp0CB(void *pData, void *pParam)
{
	int32_t Result = -ERROR_CMD_NOT_SUPPORT;
	USB_EndpointDataStruct *pEpData = (USB_EndpointDataStruct *)pData;
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)prvUSBApp.tSCSI.pSCSIUserFunList;
	uint16_t wLength, wValue;
	Virtual_UartCtrlStruct *pVUart;
//	DBG("%d,%d,%d,%d", pEpData->USB_ID, pEpData->EpIndex, pEpData->IsToDevice, pEpData->Len);
	if (!pEpData->IsDataStage)
	{
		wLength = BytesGetLe16(pEpData->pLastRequest->wLength);
		if (pEpData->IsToDevice)
		{
			switch (pEpData->pLastRequest->bRequest)
			{
			case UCDC_SET_CONTROL_LINE_STATE:
				pVUart = &prvLuatOS_VirtualUart[pEpData->pLastRequest->wIndex[0] - DEVICE_INTERFACE_SERIAL_SN];
				switch (pEpData->pLastRequest->wValue[0])
				{
				case 0:
					//close serial
					pVUart->USBOnOff = 0;
					OS_DeInitBuffer(&pVUart->TxCacheBuf);
					pVUart->CB(pEpData->pLastRequest->wIndex[0] - DEVICE_INTERFACE_SERIAL_SN, UART_CB_ERROR);
					break;
				case 1:
				case 3:
					//open serial
					prvCore_VUartOpen(pVUart, pEpData->pLastRequest->wIndex[0] - DEVICE_INTERFACE_SERIAL_SN);
					prvLuatOS_VirtualUart[pEpData->pLastRequest->wIndex[0] - DEVICE_INTERFACE_SERIAL_SN].USBOnOff = 1;
					break;
				}
				Result = ERROR_NONE;
				break;
			case USB_HID_REQ_SET_IDLE:
				prvCore_VHIDSetReady(1);
				Result = ERROR_NONE;
				break;
			case UR_GET_DESCRIPTOR:
				switch(pEpData->pLastRequest->wValue[1])
				{
				case USB_HID_REPORT_DESC:
					USB_StackTxEpData(pEpData->USB_ID, pEpData->EpIndex, prvUSBApp.pHIDReport, prvUSBApp.HIDReportLen, wLength, 1);
					Result = ERROR_NONE;
					break;
				}
				break;
			case MSC_GET_MAX_LUN:
				pUserFun->GetMaxLUN(&prvUSBApp.tSCSI.LogicalUnitNum, prvUSBApp.tSCSI.pUserData);
				USB_StackTxEpData(pEpData->USB_ID, pEpData->EpIndex, &prvUSBApp.tSCSI.LogicalUnitNum, 1, wLength, 1);

				Result = ERROR_NONE;
				break;
			case MSC_RESET:
				Result = ERROR_NONE;
				break;
			}
		}
	}
	return Result;
}

static int32_t prvCore_MSCCB(void *pData, void *pParam)
{
	USB_EndpointDataStruct *pEpData = (USB_EndpointDataStruct *)pData;
	USB_EndpointDataStruct *pEpDataSave;
	pEpDataSave = malloc(sizeof(USB_EndpointDataStruct));
	memcpy(pEpDataSave, pEpData, sizeof(USB_EndpointDataStruct));
	if (pEpData->Data && pEpData->Len)
	{
		pEpDataSave->Data = malloc(pEpData->Len);
		memcpy(pEpDataSave->Data, pEpData->Data, pEpData->Len);
	}
	else
	{
		pEpDataSave->Data = NULL;
		pEpDataSave->Len = 0;
	}
	Task_SendEvent(prvUSBApp.hTaskHandle, USBD_MSC_CB, pEpDataSave, &prvUSBApp.tSCSI, pParam);
	return ERROR_NONE;
}

static int32_t prvCore_HIDCB(void *pData, void *pParam)
{
	USB_EndpointDataStruct *pEpData = (USB_EndpointDataStruct *)pData;
	uint32_t USB_ID = (uint32_t)pParam;
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	Buffer_Struct Buf;
	if (pEpData->IsToDevice)
	{
		prvCore_VHIDSetReady(1);
		OS_BufferWrite(&pVHID->RxBuf, pEpData->Data, pEpData->Len);
		pVHID->CB(USB_ID, USB_HID_NEW_DATA);
	}
	else
	{
		OS_DeInitBuffer(&pVHID->TxBuf);
		if (pVHID->TxCacheBuf.Pos)
		{
			Buffer_StaticInit(&pVHID->TxBuf, pVHID->TxCacheBuf.Data, pVHID->TxCacheBuf.Pos);
			OS_InitBuffer(&pVHID->TxCacheBuf, VIRTUAL_VHID_BUFFER_LEN);
			USB_StackTxEpData(pVHID->USB_ID, pVHID->ToHostEpIndex, pVHID->TxBuf.Data, pVHID->TxBuf.MaxLen, pVHID->TxBuf.MaxLen, 1);
		}
		else
		{
			pVHID->CB(USB_ID, USB_HID_SEND_DONE);
		}
	}
	return ERROR_NONE;
}

static int32_t prvCore_SerialCB(void *pData, void *pParam)
{
	USB_EndpointDataStruct *pEpData = (USB_EndpointDataStruct *)pData;
	uint32_t UartID = (uint32_t)pParam;
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	if (pEpData->IsToDevice)
	{
		//open serial
		if (!pVUart->USBOnOff)
		{
			prvCore_VUartOpen(pVUart, UartID);
		}
		if (pEpData->Data && pEpData->Len)
		{
			if ((pVUart->RxBuf.Pos + pEpData->Len) >= pVUart->RxBuf.MaxLen)
			{
				OS_ReSizeBuffer(&pVUart->RxBuf, pVUart->RxBuf.MaxLen * 2);
			}
			OS_BufferWrite(&pVUart->RxBuf, pEpData->Data, pEpData->Len);
			if (pEpData->IsDataStage && pVUart->RxTimeoutUS)
			{
				Timer_StartUS(pVUart->RxTimer, pVUart->RxTimeoutUS, 0);
			}
			else
			{
				Timer_Stop(pVUart->RxTimer);
				pVUart->CB(UartID, UART_CB_RX_NEW);
			}
		}
	}
	else
	{
		OS_DeInitBuffer(&pVUart->TxBuf);
		if (pVUart->TxCacheBuf.Pos)
		{
			Buffer_StaticInit(&pVUart->TxBuf, pVUart->TxCacheBuf.Data, pVUart->TxCacheBuf.Pos);
			OS_InitBuffer(&pVUart->TxCacheBuf, VIRTUAL_SERIAL_BUFFER_LEN);
			USB_StackTxEpData(pVUart->USB_ID, pVUart->ToHostEpIndex, pVUart->TxBuf.Data, pVUart->TxBuf.MaxLen, pVUart->TxBuf.MaxLen, 0);
		}
		else
		{
			pVUart->CB(UartID, UART_CB_TX_ALL_DONE);
		}
	}
	return ERROR_NONE;
}

static int32_t prvCore_USBStateCB(void *pData, void *pParam)
{
	PV_Union uPV;
	uint8_t i;
	uPV.p = pData;
	switch(uPV.u8[1])
	{
	case USBD_BUS_TYPE_DISCONNECT:
		prvCore_VHIDSetReady(0);
		USB_MSCReset(&prvUSBApp.tSCSI);
		for(i = 0; i < VIRTUAL_UART_MAX; i++)
		{
			prvLuatOS_VirtualUart[i].USBOnOff = 0;
			OS_DeInitBuffer(&prvLuatOS_VirtualUart[i].TxBuf);
			OS_DeInitBuffer(&prvLuatOS_VirtualUart[i].TxCacheBuf);
			prvLuatOS_VirtualUart[i].CB(i, UART_CB_ERROR);
		}
		break;
	case USBD_BUS_TYPE_ENABLE_CONNECT:
		USB_StackStartDevice(uPV.u8[0]);
		break;
	}
}

void Core_USBSetID(uint8_t USB_ID, uint16_t VID, uint16_t PID)
{
	BytesPutLe16(prvCore_DeviceDesc.idVendor, VID);
	BytesPutLe16(prvCore_DeviceDesc.idProduct, PID);
}

void Core_USBSetHIDMode(uint8_t USB_ID, uint8_t HIDMode, uint8_t BuffSize)
{
	switch(BuffSize)
	{
	case 8:
	case 16:
	case 32:
	case 64:
		break;
	default:
		BuffSize = 8;
		break;
	}
	if (HIDMode)
	{
		prvUSBApp.pHIDReport = &prvCore_HIDCustomReportDescriptor;
		prvUSBApp.HIDReportLen = sizeof(prvCore_HIDCustomReportDescriptor);
		prvCore_HIDCustomReportDescriptor[10] = BuffSize;
		prvCore_HIDCustomReportDescriptor[28] = BuffSize;
		prvCore_HIDEndpointDesc[0].wMaxPacketSize[0] = BuffSize;
		prvCore_HIDEndpointDesc[1].wMaxPacketSize[0] = BuffSize;
	}
	else
	{
		prvUSBApp.pHIDReport = &prvCore_HIDKeyboardReportDesc;
		prvUSBApp.HIDReportLen = sizeof(prvCore_HIDKeyboardReportDesc);
		prvCore_HIDEndpointDesc[0].wMaxPacketSize[0] = 8;
		prvCore_HIDEndpointDesc[1].wMaxPacketSize[0] = 8;
	}
}

void Core_USBDefaultDeviceStart(uint8_t USB_ID)
{
	uint8_t langid[2] = {0x09, 0x04};
	int i;
	USB_StackPowerOnOff(USB_ID, 1);
	USB_StackStop(USB_ID);
	prvLuatOS_VirtualHID.USB_ID = USB_ID;
	prvLuatOS_VirtualHID.IsReady = 0;
	prvLuatOS_VirtualHID.ToHostEpIndex = DEVICE_HID_KEYBOARD_EP_IN;
	prvLuatOS_VirtualHID.ToDeviceEpIndex = DEVICE_HID_KEYBOARD_EP_OUT;
	prvUSBApp.tUDisk.BlockTotalNums = VIRTUAL_UDISK_BLOCKS_NUMS;
	prvUSBApp.tUDisk.BlockSize = VIRTUAL_UDISK_BLOCKS_LEN;
	prvUSBApp.tSCSI.LogicalUnitNum = 0;
	prvUSBApp.tSCSI.ToHostEpIndex = DEVICE_MASS_STORAGE_EP_IN;
	prvUSBApp.tSCSI.ToDeviceEpIndex = DEVICE_MASS_STORAGE_EP_OUT;
	if (!prvUSBApp.pHIDReport)
	{
		prvUSBApp.pHIDReport = &prvCore_HIDKeyboardReportDesc;
		prvUSBApp.HIDReportLen = sizeof(prvCore_HIDKeyboardReportDesc);
	}
	if (!prvUSBApp.tSCSI.pSCSIUserFunList)
	{
		prvUSBApp.tSCSI.pSCSIUserFunList = &prvCore_SCSIFun;
	}
	if (!prvLuatOS_VirtualHID.CB)
	{
		prvLuatOS_VirtualHID.CB = prvCore_VHIDDummyCB;
	}
	OS_ReInitBuffer(&prvLuatOS_VirtualHID.TxCacheBuf, VIRTUAL_VHID_BUFFER_LEN);

	USB_StackClearSetup(USB_ID);
	USB_StackSetDeviceBusPower(USB_ID, 1);
	USB_StackSetEpCB(USB_ID, 0, prvCore_USBEp0CB, NULL);
	USB_StackSetEpCB(USB_ID, DEVICE_HID_KEYBOARD_EP_IN, prvCore_HIDCB, USB_ID);
	USB_StackSetEpCB(USB_ID, DEVICE_HID_KEYBOARD_EP_OUT, prvCore_HIDCB, USB_ID);
	USB_StackSetEpCB(USB_ID, DEVICE_MASS_STORAGE_EP_IN, prvCore_MSCCB, USB_ID);
	USB_StackSetEpCB(USB_ID, DEVICE_MASS_STORAGE_EP_OUT, prvCore_MSCCB, USB_ID);
	USB_StackSetEpCB(USB_ID, DEVICE_GENERIC_SERIAL_EP_IN, prvCore_SerialCB, VIRTUAL_UART0);
	USB_StackSetEpCB(USB_ID, DEVICE_GENERIC_SERIAL_EP_OUT, prvCore_SerialCB, VIRTUAL_UART0);
	USB_StackSetDeviceConfig(USB_ID, &prvCore_DeviceDesc, &prvCore_FullConfig, 1, USBD_IDX_INTERFACE0_STR + 3, prvCore_USBStateCB, NULL);
	USB_StackSetCharString(USB_ID, USBD_IDX_LANGID_STR, langid, 2);
	USB_StackSetCharString(USB_ID, USBD_IDX_MANUFACTURER_STR, prvCore_StringManufacturer, sizeof(prvCore_StringManufacturer));
	USB_StackSetCharString(USB_ID, USBD_IDX_PRODUCT_STR, prvCore_StringProduct, sizeof(prvCore_StringProduct));
	USB_StackSetCharString(USB_ID, USBD_IDX_SERIAL_STR, prvCore_StringSerial, sizeof(prvCore_StringSerial));
	USB_StackSetCharString(USB_ID, USBD_IDX_CONFIG_STR, prvCore_StringConfig, sizeof(prvCore_StringConfig));
	for(i = 0; i < 3; i++)
	{
		USB_StackSetCharString(USB_ID, USBD_IDX_INTERFACE0_STR + i, prvCore_StringInterface[i], strlen(prvCore_StringInterface[i]));
	}
	for(i = 0; i < VIRTUAL_UART_MAX; i++)
	{
		prvLuatOS_VirtualUart[i].USBOnOff = 0;
		prvLuatOS_VirtualUart[i].USB_ID = USB_ID;
		if (!prvLuatOS_VirtualUart[i].RxTimer)
		{
			prvLuatOS_VirtualUart[i].RxTimer = Timer_Create(prvCore_VUartRxTimeoutCB, i, NULL);
		}
		if (!prvLuatOS_VirtualUart[i].CB)
		{
			prvLuatOS_VirtualUart[i].CB = prvCore_VUartDummyCB;
		}
		prvLuatOS_VirtualUart[i].RxTimeoutUS = 150;
		OS_ReInitBuffer(&prvLuatOS_VirtualUart[i].TxCacheBuf, VIRTUAL_SERIAL_BUFFER_LEN);
		OS_ReInitBuffer(&prvLuatOS_VirtualUart[i].RxBuf, VIRTUAL_SERIAL_BUFFER_LEN);
	}
	prvLuatOS_VirtualUart[VIRTUAL_UART0].ToDeviceEpIndex = DEVICE_GENERIC_SERIAL_EP_IN;
	prvLuatOS_VirtualUart[VIRTUAL_UART0].ToHostEpIndex = DEVICE_GENERIC_SERIAL_EP_OUT;
	prvUSBApp.tUDisk.VirtualData = prvLuatOS_VirtualUart[0].TxCacheBuf.Data;
	USB_MSCReset(&prvUSBApp.tSCSI);

	USB_StackStartDevice(USB_ID);
	prvUSBApp.IsStart = 1;
}


void Core_VHIDInit(uint8_t USB_ID, CBFuncEx_t CB)
{
	if (CB)
	{
		prvLuatOS_VirtualHID.CB = CB;
	}
	else
	{
		prvLuatOS_VirtualHID.CB = prvCore_VHIDDummyCB;
	}

}

void Core_VUartInit(uint8_t UartID, uint32_t BaudRate, uint8_t IsRxCacheEnable, uint8_t DataBits, uint8_t Parity, uint8_t StopBits, CBFuncEx_t CB)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
//	switch(UartID)
//	{
//	case VIRTUAL_UART0:
//		if (!prvService.IsUSBOpen)
//		{
//			Core_USBDefaultDeviceStart(USB_ID0);
//			prvService.IsUSBOpen = 1;
//		}
//		break;
//	}
	if (CB)
	{
		pVUart->CB = CB;
	}
	else
	{
		pVUart->CB = prvCore_VUartDummyCB;
	}
	pVUart->UartOnOff = 1;
}

void Core_VUartDeInit(uint8_t UartID)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	pVUart->UartOnOff = 0;
}

void Core_VUartSetRxTimeout(uint8_t UartID, uint32_t TimeoutUS)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	pVUart->RxTimeoutUS = TimeoutUS;
}

void Core_VUartSetCb(uint8_t UartID, CBFuncEx_t CB)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	if (CB)
	{
		pVUart->CB = CB;
	}
	else
	{
		pVUart->CB = prvCore_VUartDummyCB;
	}
}

uint32_t Core_VUartRxBufferRead(uint8_t UartID, uint8_t *Data, uint32_t Len)
{
	uint32_t ReadLen;
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	switch(UartID)
	{
	case VIRTUAL_UART0:
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToDeviceEpIndex, 1, 0);
		break;
	}
	if (!Data || !Len)
	{
		ReadLen = pVUart->RxBuf.Pos;
		goto READ_END;

	}
	ReadLen = (pVUart->RxBuf.Pos < Len)?pVUart->RxBuf.Pos:Len;
	memcpy(Data, pVUart->RxBuf.Data, ReadLen);
	OS_BufferRemove(&pVUart->RxBuf, ReadLen);
	if (!pVUart->RxBuf.Pos && pVUart->RxBuf.MaxLen > VIRTUAL_SERIAL_BUFFER_LEN)
	{
		OS_ReInitBuffer(&pVUart->RxBuf, VIRTUAL_SERIAL_BUFFER_LEN);
	}
READ_END:
	switch(UartID)
	{
	case VIRTUAL_UART0:
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToDeviceEpIndex, 1, 1);
		break;
	}
	return ReadLen;
}

int32_t Core_VUartBufferTx(uint8_t UartID, const uint8_t *Data, uint32_t Len)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];

	switch(UartID)
	{
	case VIRTUAL_UART0:
		if (!pVUart->USBOnOff)
		{
			return -1;
		}
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToHostEpIndex, 0, 0);
		break;
	}

	if (Data && Len)
	{
		OS_BufferWrite(&pVUart->TxCacheBuf, Data, Len);
	}

	if (pVUart->TxBuf.Data || pVUart->TxBuf.MaxLen)
	{
		goto TX_END;
	}

	// 把缓存的Tx指针交给发送的Tx指针，缓存的Tx指针重新建立一个
	Buffer_StaticInit(&pVUart->TxBuf, pVUart->TxCacheBuf.Data, pVUart->TxCacheBuf.Pos);
	OS_InitBuffer(&pVUart->TxCacheBuf, VIRTUAL_SERIAL_BUFFER_LEN);
	USB_StackTxEpData(pVUart->USB_ID, pVUart->ToHostEpIndex, pVUart->TxBuf.Data, pVUart->TxBuf.MaxLen, pVUart->TxBuf.MaxLen, 0);
TX_END:
	switch(UartID)
	{
	case VIRTUAL_UART0:
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToHostEpIndex, 0, 1);
		break;
	}
	return 0;
}
void Core_VUartBufferTxStop(uint8_t UartID)
{
	Virtual_UartCtrlStruct *pVUart = &prvLuatOS_VirtualUart[UartID];
	switch(UartID)
	{
	case VIRTUAL_UART0:
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToHostEpIndex, 0, 0);
		break;
	}
	OS_DeInitBuffer(&pVUart->TxBuf);
	pVUart->TxCacheBuf.Pos = 0;
	USB_StackStopDeviceTx(pVUart->USB_ID, pVUart->ToHostEpIndex, 0);
	switch(UartID)
	{
	case VIRTUAL_UART0:
		USB_StackEpIntOnOff(pVUart->USB_ID, pVUart->ToHostEpIndex, 0, 1);
		break;
	}
}

uint32_t Core_VHIDRxBufferRead(uint8_t USB_ID, uint8_t *Data, uint32_t Len)
{
	uint32_t ReadLen;
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToDeviceEpIndex, 1, 0);
	if (!Data || !Len)
	{
		ReadLen = pVHID->RxBuf.Pos;
		goto READ_END;

	}
	ReadLen = (pVHID->RxBuf.Pos < Len)?pVHID->RxBuf.Pos:Len;
	memcpy(Data, pVHID->RxBuf.Data, ReadLen);
	OS_BufferRemove(&pVHID->RxBuf, ReadLen);
	if (!pVHID->RxBuf.Pos && pVHID->RxBuf.MaxLen > VIRTUAL_VHID_BUFFER_LEN)
	{
		OS_ReInitBuffer(&pVHID->RxBuf, VIRTUAL_VHID_BUFFER_LEN);
	}
READ_END:
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToDeviceEpIndex, 1, 1);
	return ReadLen;
}

void Core_VHIDUploadData(uint8_t USB_ID, uint8_t *Data, uint32_t Len)
{
	if (!prvLuatOS_VirtualHID.IsReady) return;
	USB_HIDKeyValue HIDKey;
	USB_HIDKeyBoradKeyStruct HIDKeyBoard;
	uint16_t Pos;
	volatile uint8_t IsShift, i, LastValue;
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 0);

	if (Data && Len)
	{
		if ((Len*16 + pVHID->TxCacheBuf.Pos) > pVHID->TxCacheBuf.MaxLen)
		{
			OS_ReSizeBuffer(&pVHID->TxCacheBuf, Len*16 + pVHID->TxCacheBuf.Pos);
		}
		Pos = 0;
		while(Pos < Len)
		{
			memset(&HIDKeyBoard, 0, sizeof(USB_HIDKeyBoradKeyStruct));
			HIDKey = USB_HIDGetValueFromAscii(Data[Pos]);
			Pos++;
			IsShift = HIDKey.Shift;
			LastValue = HIDKey.Value;
			HIDKeyBoard.SPECIALHID_KEY_b.RightShift = IsShift;
			HIDKeyBoard.PressKey[0] = HIDKey.Value;
//			DBG("%u,%c,%d,%x,%d", Pos - 1, Data[Pos - 1], 0, HIDKey.Value, HIDKey.Shift);
			if (Pos >= Len)
			{
				OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
				break;
			}
ADD_REST:
			for(i = 1; i < USB_HID_KEY_CACHE; i++)
			{
				HIDKey = USB_HIDGetValueFromAscii(Data[Pos]);
				Pos++;
				if ((IsShift != HIDKey.Shift) || (LastValue == HIDKey.Value) || (LastValue == '\r') || (LastValue == '\n'))
				{
					OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
					memset(&HIDKeyBoard, 0, sizeof(USB_HIDKeyBoradKeyStruct));
					//加入一个抬起的data
					OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
					IsShift = HIDKey.Shift;
					LastValue = HIDKey.Value;
					HIDKeyBoard.SPECIALHID_KEY_b.RightShift = IsShift;
					HIDKeyBoard.PressKey[0] = HIDKey.Value;
//					DBG("%u,%c,%d,%x,%d", Pos - 1, Data[Pos - 1], 0, HIDKey.Value, HIDKey.Shift);
					if (Pos < Len)
					{
						goto ADD_REST;
					}
					else
					{
						break;
					}

				}
				else
				{
					LastValue = HIDKey.Value;
					HIDKeyBoard.PressKey[i] = HIDKey.Value;
//					DBG("%u,%c,%d,%x,%d", Pos - 1, Data[Pos - 1], i, HIDKey.Value, HIDKey.Shift);
					if (Pos >= Len)
					{
						break;
					}
				}
			}
			OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
			//加入一个抬起的data
			memset(&HIDKeyBoard, 0, sizeof(USB_HIDKeyBoradKeyStruct));
			OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
		}
		//加入一个抬起的data
		memset(&HIDKeyBoard, 0, sizeof(USB_HIDKeyBoradKeyStruct));
		OS_BufferWrite(&pVHID->TxCacheBuf, &HIDKeyBoard, sizeof(HIDKeyBoard));
	}
//	pVHID->TxCacheBuf.Pos = 0;
//	return;
	if (pVHID->TxBuf.Data || pVHID->TxBuf.MaxLen)
	{
		goto UPLOAD_END;
	}
	// 把缓存的Tx指针交给发送的Tx指针，缓存的Tx指针重新建立一个
	Buffer_StaticInit(&pVHID->TxBuf, pVHID->TxCacheBuf.Data, pVHID->TxCacheBuf.Pos);
	OS_InitBuffer(&pVHID->TxCacheBuf, VIRTUAL_VHID_BUFFER_LEN);
	USB_StackTxEpData(pVHID->USB_ID, pVHID->ToHostEpIndex, pVHID->TxBuf.Data, pVHID->TxBuf.MaxLen, pVHID->TxBuf.MaxLen, 1);
UPLOAD_END:
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 1);
}

void Core_VHIDSendRawData(uint8_t USB_ID, uint8_t *Data, uint32_t Len)
{
	uint8_t zero[USB_HID_CUST_DATA_CACHE] = {0};
	if (!prvLuatOS_VirtualHID.IsReady) return;
	USB_HIDKeyValue HIDKey;
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 0);
	OS_BufferWrite(&pVHID->TxCacheBuf, Data, Len);
	if (Len % prvCore_HIDEndpointDesc[1].wMaxPacketSize[0])
	{
		OS_BufferWrite(&pVHID->TxCacheBuf, zero, prvCore_HIDEndpointDesc[1].wMaxPacketSize[0] - (Len % prvCore_HIDEndpointDesc[1].wMaxPacketSize[0]));
	}
	Buffer_StaticInit(&pVHID->TxBuf, pVHID->TxCacheBuf.Data, pVHID->TxCacheBuf.Pos);
	OS_InitBuffer(&pVHID->TxCacheBuf, VIRTUAL_VHID_BUFFER_LEN);
	USB_StackTxEpData(pVHID->USB_ID, pVHID->ToHostEpIndex, pVHID->TxBuf.Data, pVHID->TxBuf.MaxLen, pVHID->TxBuf.MaxLen, 1);
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 1);
}

void Core_VHIDUploadStop(uint8_t USB_ID)
{
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 0);
	OS_DeInitBuffer(&pVHID->TxBuf);
	pVHID->TxCacheBuf.Pos = 0;
	USB_StackStopDeviceTx(pVHID->USB_ID, pVHID->ToHostEpIndex, 0);
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 1);
}

#ifdef __BUILD_OS__

static void prvCore_USBAppTask(void *pParam)
{
	OS_EVENT Event;
	USB_EndpointDataStruct *pEpData;
	while(1)
	{
		Task_GetEventByMS(prvUSBApp.hTaskHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case USB_HW_ACTION:
			switch (Event.Param2)
			{
			case SERV_USB_RESET_END:
				DBG("USB%d reset", Event.Param1);
				Task_DelayMS(1);
				USB_StackDeviceAfterDisconnect(Event.Param1);
				break;
			case SERV_USB_RESUME_END:
				Task_DelayMS(20);
				USB_ResumeEnd(Event.Param3);
				break;
			case SERV_USB_SUSPEND:
				DBG("USB%d suspend", Event.Param1);
				break;
			case SERV_USB_RESUME:
				break;
			}
			break;
		case USBD_MSC_CB:
			pEpData = (USB_EndpointDataStruct *)Event.Param1;
//			DBG("%d,%d,%d,%d", pEpData->USB_ID, pEpData->EpIndex, pEpData->IsToDevice, pEpData->Len);
			USB_MSCHandle(pEpData, Event.Param2);
//			USB_StackEpIntOnOff(pEpData->USB_ID, DEVICE_MASS_STORAGE_EP_IN, 0, 0);
//			USB_StackEpIntOnOff(pEpData->USB_ID, DEVICE_MASS_STORAGE_EP_OUT, 1, 0);
//			if (prvUSBApp.BlockCmd)
//			{
//				prvUSBApp.BlockCmd--;
//			}
//			else
//			{
//				DBG("!!!");
//			}
//			USB_StackEpIntOnOff(pEpData->USB_ID, DEVICE_MASS_STORAGE_EP_IN, 0, 1);
//			USB_StackEpIntOnOff(pEpData->USB_ID, DEVICE_MASS_STORAGE_EP_OUT, 1, 1);
			free(pEpData->Data);
			free(Event.Param1);
			WDT_Feed();
			break;
		}
	}
}


void Core_USBAction(uint8_t USB_ID, uint8_t Action, void *pParam)
{
	Task_SendEvent(prvUSBApp.hTaskHandle, USB_HW_ACTION, USB_ID, Action, pParam);
}

extern const USB_StorageSCSITypeDef prvSDHC_SCSIFun;
void Core_UDiskAttachSDHC(uint8_t USB_ID, void *pCtrl)
{
	prvUSBApp.pSDHC = pCtrl;
	prvUSBApp.tSCSI.pSCSIUserFunList = &prvSDHC_SCSIFun;
	prvUSBApp.tSCSI.pUserData = prvUSBApp.pSDHC;
	prvUSBApp.tSCSI.ReadTimeout = 500;
	DBuffer_ReInit(&prvUSBApp.DataBuf, 8 * 1024);
	prvUSBApp.pSDHC->USBDelayTime = 0;	//如果USB不稳定，可调加大USBDelayTime
	prvUSBApp.pSDHC->SCSIDataBuf = &prvUSBApp.DataBuf;
	prvUSBApp.IsUSBAttachSDHC = 1;
}

void Core_UDiskAttachSDHCRecovery(uint8_t USB_ID, void *pCtrl)
{
	if (prvUSBApp.IsUSBAttachSDHC)
	{
		DBG("recovery attach %d, %x", USB_ID, pCtrl);
		prvUSBApp.pSDHC = pCtrl;
		prvUSBApp.tSCSI.pSCSIUserFunList = &prvSDHC_SCSIFun;
		prvUSBApp.tSCSI.pUserData = prvUSBApp.pSDHC;
		prvUSBApp.tSCSI.ReadTimeout = 500;
		DBuffer_ReInit(&prvUSBApp.DataBuf, 8 * 1024);
		prvUSBApp.pSDHC->USBDelayTime = 0;	//如果USB不稳定，可调加大USBDelayTime
		prvUSBApp.pSDHC->SCSIDataBuf = &prvUSBApp.DataBuf;
		if (prvUSBApp.IsStart)
		{
			DBG("reboot usb");
			Core_USBDefaultDeviceStart(USB_ID);
		}
	}
}

void Core_UDiskDetachSDHC(uint8_t USB_ID, void *pCtrl)
{
	if (pCtrl)
	{
		if (prvUSBApp.pSDHC != pCtrl)
		{
			return;
		}
	}

	if (prvUSBApp.IsUSBAttachSDHC)
	{
		prvUSBApp.tSCSI.pSCSIUserFunList = &prvCore_SCSIFun;
		prvUSBApp.tSCSI.pUserData = NULL;
		prvUSBApp.pSDHC = NULL;
		DBuffer_DeInit(&prvUSBApp.DataBuf);
		DBG("detach %d, %x", USB_ID, pCtrl);
		if (!pCtrl)	//NULL表示用户退出
		{
			prvUSBApp.IsUSBAttachSDHC = 0;
			DBG("free attach %d, %x", USB_ID, pCtrl);
		}
	}
	if (prvUSBApp.IsStart)
	{
		DBG("reboot usb");
		Core_USBDefaultDeviceStart(USB_ID);
	}

}

void Core_USBAppGlobalInit(void)
{
	prvUSBApp.hTaskHandle = Task_Create(prvCore_USBAppTask, NULL, 1024, USB_TASK_PRO, "usb app");
}

INIT_TASK_EXPORT(Core_USBAppGlobalInit, "2");
#endif
