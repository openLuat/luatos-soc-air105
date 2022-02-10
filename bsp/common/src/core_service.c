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

#ifdef __BUILD_OS__
#include "zbar.h"
#include "symbol.h"
#include "image.h"
extern void SPI_TestInit(uint8_t SpiID, uint32_t Speed, uint32_t TestLen);
extern void SPI_TestOnce(uint8_t SpiID, uint32_t TestLen);
extern void DHT11_TestOnce(uint8_t Pin, CBFuncEx_t CB);
extern void DHT11_TestResult(void);
extern void SHT30_Init(CBFuncEx_t CB, void *pParam);
extern void SHT30_GetResult(CBFuncEx_t CB, void *pParam);
extern void GC032A_TestInit(void);
extern void OV2640_TestInit(void);
enum
{
	DEVICE_INTERFACE_HID_SN,
	DEVICE_INTERFACE_MSC_SN,
	DEVICE_INTERFACE_SERIAL_SN,
	DEVICE_INTERFACE_MAX,
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
	CBFuncEx_t CB;
	uint8_t USB_ID;
	uint8_t IsReady;
	uint8_t ToHostEpIndex;
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

static MSC_SCSICtrlStruct prvLuatOS_SCSI;
static Virtual_HIDCtrlStruct prvLuatOS_VirtualHID;
static Virtual_UDiskCtrlStruct prvLuatOS_VirtualUDisk;
static Virtual_UartCtrlStruct prvLuatOS_VirtualUart[VIRTUAL_UART_MAX];


enum
{
	SERVICE_LCD_DRAW = SERVICE_EVENT_ID_START + 1,
	SERVICE_CAMERA_DRAW,
	SERVICE_USB_ACTION,
	SERVICE_DECODE_QR,
};


typedef struct
{
	HANDLE HardwareHandle;
	HANDLE ServiceHandle;
	HANDLE UserHandle;
	uint64_t LCDDrawRequireByte;
	uint64_t LCDDrawDoneByte;
	uint32_t InitAllocMem;
	uint32_t LastAllocMem;
	uint8_t SleepEnable;

}Service_CtrlStruct;

static Service_CtrlStruct prvService;

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



static const usb_device_descriptor_t prvCore_DeviceDesc =
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
		.bInterfaceProtocol = UIPROTO_BOOT_KEYBOARD,
		.iInterface = USBD_IDX_INTERFACE0_STR + 1,
};

static const usb_endpoint_descriptor_t prvCore_HIDEndpointDesc[2] =
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

static const char prvCore_HIDReportDesc[63] = {
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
	    0x95, 0x06,                    //   REPORT_COUNT (6)
	    0x75, 0x08,                    //   REPORT_SIZE (8)
	    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	    0x25, 0xff,                    //   LOGICAL_MAXIMUM (255)
	    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
	    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
	    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
	    0xc0                           // END_COLLECTION
};

static int32_t prvCore_GetHIDDesc(void *pData, void *pParam)
{
	if (pData)
	{
		memcpy(pData, &prvCore_HIDDesc, sizeof(usb_hid_descriptor_t));
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

static const uint8_t prvCore_MSCPage00InquiryData[LENGTH_INQUIRY_PAGE00] =
{
	0x00,
	0x00,
	0x00,
	(LENGTH_INQUIRY_PAGE00 - 4U),
	0x00,
	0x80
};

/* USB Mass storage VPD Page 0x80 Inquiry Data for Unit Serial Number */
static const uint8_t prvCore_MSCPage80InquiryData[LENGTH_INQUIRY_PAGE80] =
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
static const uint8_t prvCore_MSCModeSense6data[MODE_SENSE6_LEN] =
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
static const uint8_t prvCore_MSCModeSense10data[MODE_SENSE10_LEN] =
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

static int32_t prvCore_SCSIInit(uint8_t LUN, void *pUserData)
{
	return 0;
}

static int32_t prvCore_SCSIGetCapacity(uint8_t LUN, uint32_t *BlockNum, uint32_t *BlockSize, void *pUserData)
{
	*BlockNum = prvLuatOS_VirtualUDisk.BlockTotalNums;
	*BlockSize = prvLuatOS_VirtualUDisk.BlockSize;
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
	if ((BlockAddress + BlockNums) * prvLuatOS_VirtualUDisk.BlockSize > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	prvLuatOS_VirtualUDisk.CurAddress = BlockAddress * prvLuatOS_VirtualUDisk.BlockSize;
	return 0;
}

static int32_t prvCore_SCSIRead(uint8_t LUN, uint32_t Len, uint8_t **pOutData, uint32_t *OutLen, void *pUserData)
{
	if (Len + prvLuatOS_VirtualUDisk.CurAddress > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	*pOutData = &prvLuatOS_VirtualUDisk.VirtualData[prvLuatOS_VirtualUDisk.CurAddress];
	*OutLen = Len;
	prvLuatOS_VirtualUDisk.CurAddress += Len;
	return 0;
}

static int32_t prvCore_SCSIPreWrite(uint8_t LUN, uint32_t BlockAddress, uint32_t BlockNums, void *pUserData)
{
	if ((BlockAddress + BlockNums) * prvLuatOS_VirtualUDisk.BlockSize > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	prvLuatOS_VirtualUDisk.CurAddress = BlockAddress * prvLuatOS_VirtualUDisk.BlockSize;
	return 0;
}

static int32_t prvCore_SCSIWrite(uint8_t LUN, uint8_t *Data, uint32_t Len, void *pUserData)
{
	if (Len + prvLuatOS_VirtualUDisk.CurAddress > VIRTUAL_UDISK_LEN)
	{
		return -1;
	}
	memcpy(&prvLuatOS_VirtualUDisk.VirtualData[prvLuatOS_VirtualUDisk.CurAddress], Data, Len);
	prvLuatOS_VirtualUDisk.CurAddress += Len;
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
	USB_StorageSCSITypeDef *pUserFun = (USB_StorageSCSITypeDef *)prvLuatOS_SCSI.pSCSIUserFunList;
	uint16_t wLength, wValue;
	Virtual_UartCtrlStruct *pVUart;
	DBG("%d,%d,%d,%d", pEpData->USB_ID, pEpData->EpIndex, pEpData->IsToDevice, pEpData->Len);
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
					USB_StackTxEpData(pEpData->USB_ID, pEpData->EpIndex, prvCore_HIDReportDesc, sizeof(prvCore_HIDReportDesc), wLength, 1);
					Result = ERROR_NONE;
					break;
				}
				break;
			case MSC_GET_MAX_LUN:
				pUserFun->GetMaxLUN(&prvLuatOS_SCSI.LogicalUnitNum, prvLuatOS_SCSI.pUserData);
				USB_StackTxEpData(pEpData->USB_ID, pEpData->EpIndex, &prvLuatOS_SCSI.LogicalUnitNum, 1, wLength, 1);

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
	USB_MSCHandle(pEpData, &prvLuatOS_SCSI);
	return ERROR_NONE;
}

static int32_t prvCore_HIDCB(void *pData, void *pParam)
{
	USB_EndpointDataStruct *pEpData = (USB_EndpointDataStruct *)pData;
	uint32_t USB_ID = (uint32_t)pParam;
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	if (pEpData->IsToDevice)
	{
		prvCore_VHIDSetReady(1);
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
		USB_MSCReset(&prvLuatOS_SCSI);
		for(i = 0; i < VIRTUAL_UART_MAX; i++)
		{
			prvLuatOS_VirtualUart[i].USBOnOff = 0;
			prvLuatOS_VirtualUart[i].CB(i, UART_CB_ERROR);
		}
		break;
	case USBD_BUS_TYPE_ENABLE_CONNECT:
		USB_StackStartDevice(uPV.u8[0]);
		break;
	}
}

void Core_USBDefaultDeviceStart(uint8_t USB_ID)
{
	uint8_t langid[2] = {0x09, 0x04};
	int i;
	prvLuatOS_VirtualHID.USB_ID = USB_ID;
	prvLuatOS_VirtualHID.IsReady = 0;
	prvLuatOS_VirtualHID.ToHostEpIndex = DEVICE_HID_KEYBOARD_EP_IN;
	prvLuatOS_VirtualUDisk.BlockTotalNums = VIRTUAL_UDISK_BLOCKS_NUMS;
	prvLuatOS_VirtualUDisk.BlockSize = VIRTUAL_UDISK_BLOCKS_LEN;
	prvLuatOS_SCSI.LogicalUnitNum = 0;
	prvLuatOS_SCSI.ToHostEpIndex = DEVICE_MASS_STORAGE_EP_IN;
	prvLuatOS_SCSI.ToDeviceEpIndex = DEVICE_MASS_STORAGE_EP_OUT;
	prvLuatOS_SCSI.pSCSIUserFunList = &prvCore_SCSIFun;
	if (!prvLuatOS_VirtualHID.CB)
	{
		prvLuatOS_VirtualHID.CB = prvCore_VHIDDummyCB;
	}
	OS_ReInitBuffer(&prvLuatOS_VirtualHID.TxCacheBuf, VIRTUAL_VHID_BUFFER_LEN);
	USB_StackPowerOnOff(USB_ID, 1);
	USB_StackStop(USB_ID);
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
	prvLuatOS_VirtualUDisk.VirtualData = prvLuatOS_VirtualUart[VIRTUAL_UART0].TxCacheBuf.Data;
	USB_MSCReset(&prvLuatOS_SCSI);

	USB_StackStartDevice(USB_ID);
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
	if (!Len)
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

void Core_VHIDUploadData(uint8_t USB_ID, uint8_t *Data, uint16_t Len)
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
ADD_REST:
			for(i = 1; i < 4; i++)
			{
				HIDKey = USB_HIDGetValueFromAscii(Data[Pos]);
				Pos++;
				if ((IsShift != HIDKey.Shift))
				{
					//shift发生切换，必须换新的data
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
				else if (LastValue == HIDKey.Value)
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

void Core_VHIDUploadStop(uint8_t USB_ID)
{
	Virtual_HIDCtrlStruct *pVHID = &prvLuatOS_VirtualHID;
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 0);
	OS_DeInitBuffer(&pVHID->TxBuf);
	pVHID->TxCacheBuf.Pos = 0;
	USB_StackStopDeviceTx(pVHID->USB_ID, pVHID->ToHostEpIndex, 0);
	USB_StackEpIntOnOff(pVHID->USB_ID, pVHID->ToHostEpIndex, 0, 1);
}

static void prvCore_LCDCmd(LCD_DrawStruct *Draw, uint8_t Cmd)
{
    GPIO_Output(Draw->DCPin, 0);
    if (Draw->DCDelay)
    {
    	SysTickDelay(Draw->DCDelay * CORE_TICK_1US);
    }
    GPIO_Output(Draw->CSPin, 0);
    SPI_BlockTransfer(Draw->SpiID, &Cmd, NULL, 1);
    GPIO_Output(Draw->CSPin, 1);
    GPIO_Output(Draw->DCPin, 1);
//    SysTickDelay(300);
}

static void prvCore_LCDData(LCD_DrawStruct *Draw, uint8_t *Data, uint32_t Len)
{
    GPIO_Output(Draw->CSPin, 0);
    SPI_BlockTransfer(Draw->SpiID, Data, NULL, Len);
    GPIO_Output(Draw->CSPin, 1);
}

static uint16_t RGB888toRGB565(uint8_t red, uint8_t green, uint8_t blue, uint8_t IsBe)
{
	uint16_t B = (blue >> 3) & 0x001F;
	uint16_t G = ((green >> 2) << 5) & 0x07E0;
	uint16_t R = ((red >> 3) << 11) & 0xF800;
	uint16_t C = (R | G | B);
	if (IsBe)
	{
		C = (C >> 8) | ((C & 0x00ff) << 8);
	}
	return C;
}

static void prvHW_Task(void* params)
{
	uint64_t StartUS;
	OS_EVENT Event;
	int32_t Result;
	LCD_DrawStruct *Draw;
	PV_Union uPV;
	uint16_t uColor[256];
	uint32_t i, j, Size;
	for(i = 0; i < 256; i++)
	{
		uColor[i] = RGB888toRGB565(i, i, i, 1);
	}
	while(1)
	{
		Result = Task_GetEvent(prvService.HardwareHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case SERVICE_LCD_DRAW:

			Draw = (LCD_DrawStruct *)Event.Param1;
			if (Draw->Speed > 30000000)
			{
				SPI_SetTxOnlyFlag(Draw->SpiID, 1);
			}
			SPI_SetNewConfig(Draw->SpiID, Draw->Speed, Draw->Mode);
			SPI_DMATxInit(Draw->SpiID, LCD_SPI_TX_DMA_STREAM, 0);
			SPI_DMARxInit(Draw->SpiID, LCD_SPI_RX_DMA_STREAM, 0);
			prvCore_LCDCmd(Draw, 0x2a);
			BytesPutBe16(uPV.u8, Draw->x1 + Draw->xoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->x2 + Draw->xoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2b);
			BytesPutBe16(uPV.u8, Draw->y1 + Draw->yoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->y2 + Draw->yoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2c);
			GPIO_Output(Draw->CSPin, 0);
//			StartUS = GetSysTickUS();
//			SPI_Transfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size, 2);

			SPI_BlockTransfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size);
//			DBG("%u, %u", Draw->Size, (uint32_t)(GetSysTickUS() - StartUS));
			prvService.LCDDrawDoneByte += Draw->Size;
			free(Draw->Data);
			GPIO_Output(Draw->CSPin, 1);
			free(Draw);
			SPI_SetTxOnlyFlag(Draw->SpiID, 0);
			break;
		case SERVICE_CAMERA_DRAW:
			Draw = (LCD_DrawStruct *)Event.Param1;

			SPI_SetTxOnlyFlag(Draw->SpiID, 1);
			SPI_SetNewConfig(Draw->SpiID, Draw->Speed, Draw->Mode);
			SPI_DMATxInit(Draw->SpiID, LCD_SPI_TX_DMA_STREAM, 0);
			SPI_DMARxInit(Draw->SpiID, LCD_SPI_RX_DMA_STREAM, 0);
			prvCore_LCDCmd(Draw, 0x2a);
			BytesPutBe16(uPV.u8, Draw->x1 + Draw->xoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->x2 + Draw->xoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2b);
			BytesPutBe16(uPV.u8, Draw->y1 + Draw->yoffset);
			BytesPutBe16(uPV.u8 + 2, Draw->y2 + Draw->yoffset);
			prvCore_LCDData(Draw, uPV.u8, 4);
			prvCore_LCDCmd(Draw, 0x2c);
			GPIO_Output(Draw->CSPin, 0);
			switch(Draw->ColorMode)
			{
			case COLOR_MODE_RGB_565:
				SPI_BlockTransfer(Draw->SpiID, Draw->Data, Draw->Data, Draw->Size);
				break;
			case COLOR_MODE_GRAY:
				Size = Draw->Size/10;
				uPV.pu16 = malloc(Size * 2);
				for(i = 0; i < 10; i++)
				{
					for(j = 0; j < Size; j++)
					{
						uPV.pu16[j] = uColor[Draw->Data[i * Size + j]];
					}
					SPI_BlockTransfer(Draw->SpiID, uPV.pu8, uPV.pu8, Size * 2);
				}
				free(uPV.pu16);
				break;
			}
			free(Draw);
			GPIO_Output(Draw->CSPin, 1);
			SPI_SetTxOnlyFlag(Draw->SpiID, 0);
			break;
		}
	}
}



static void prvService_Task(void* params)
{
	zbar_image_scanner_t *zbar_scanner;
	zbar_image_t *zbar_image;
	zbar_symbol_t *zbar_symbol;
	CBDataFun_t CBDataFun;
	PV_Union uPV;
	OS_EVENT Event;
//	Audio_Test();
	while(1)
	{
		Task_GetEventByMS(prvService.ServiceHandle, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		switch(Event.ID)
		{
		case SERVICE_USB_ACTION:
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
		case SERVICE_DECODE_QR:
			if (Event.Param3)
			{
				CBDataFun = (CBDataFun_t)(Event.Param3);
				uPV.u32 = Event.Param2;
				zbar_scanner = zbar_image_scanner_create();
				zbar_image_scanner_set_config(zbar_scanner, 0, ZBAR_CFG_ENABLE, 1);
				zbar_image = zbar_image_create();
				zbar_image_set_format(zbar_image, *(int*)"Y800");
				zbar_image_set_size(zbar_image, uPV.u16[0], uPV.u16[1]);
				zbar_image_set_data(zbar_image, Event.Param1, uPV.u16[0] * uPV.u16[1], zbar_image_free_data);
				if (zbar_scan_image(zbar_scanner, zbar_image) > 0)
				{
					zbar_symbol = (zbar_symbol_t *)zbar_image_first_symbol(zbar_image);
					free(Event.Param1);
					CBDataFun(zbar_symbol->data, zbar_symbol->datalen);
				}
				else
				{
					free(Event.Param1);
					CBDataFun(NULL, 0);
				}
				zbar_image_destroy(zbar_image);
				zbar_image_scanner_destroy(zbar_scanner);
			}
			else
			{
				free(Event.Param1);
			}
			break;
		}
	}
}

extern int luat_fs_init(void);
static void prvLuatOS_Task(void* params)
{
	// 文件系统初始化
	// luat_fs_init();

	// 载入LuatOS主入口
	luat_main();
	// 永不进入的代码, 避免编译警告.
//	for(;;)
//	{
//		vTaskDelayUntil( &xLastWakeTime, 2000 * portTICK_RATE_MS );
//	}
}
extern void luat_base_init(void);

uint32_t Core_LCDDrawCacheLen(void)
{
	if (prvService.LCDDrawRequireByte > prvService.LCDDrawDoneByte)
	{
		return (uint32_t)(prvService.LCDDrawRequireByte - prvService.LCDDrawDoneByte);
	}
	else
	{
		return 0;
	}
}

void Core_LCDDraw(LCD_DrawStruct *Draw)
{
	prvService.LCDDrawRequireByte += Draw->Size;
	Task_SendEvent(prvService.HardwareHandle, SERVICE_LCD_DRAW, Draw, 0, 0);
}

void Core_CameraDraw(LCD_DrawStruct *Draw)
{
	Task_SendEvent(prvService.HardwareHandle, SERVICE_CAMERA_DRAW, Draw, 0, 0);
}

void Core_DecodeQR(uint8_t *ImageData, uint16_t ImageW, uint16_t ImageH,  CBDataFun_t CB)
{
	PV_Union uPV;
	uPV.u16[0] = ImageW;
	uPV.u16[1] = ImageH;
	Task_SendEvent(prvService.ServiceHandle, SERVICE_DECODE_QR, ImageData, uPV.u32, CB);
}

void Core_USBAction(uint8_t USB_ID, uint8_t Action, void *pParam)
{
	Task_SendEvent(prvService.ServiceHandle, SERVICE_USB_ACTION, USB_ID, Action, pParam);
}

void Core_PrintMemInfo(void)
{
	uint32_t curalloc, totfree, maxfree;
	OS_MemInfo(&curalloc, &totfree, &maxfree);
	DBG("curalloc %uB, totfree %uB, maxfree %uB", curalloc, totfree, maxfree);
}

static uint8_t disassembly_ins_is_bl_blx(uint32_t addr) {
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK         0xF800
#define BL_INS_HIGH         0xF800
#define BL_INS_LOW          0xF000
#define BLX_INX_MASK        0xFF00
#define BLX_INX             0x4700

    if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW) {
        return 1;
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return 1;
    } else {
        return 0;
    }
}

static void prvCore_PrintTaskStack(HANDLE TaskHandle)
{
	uint32_t SP, StackAddress, Len, i, PC;
	uint32_t Buffer[16];
	char *Name = vTaskInfo(TaskHandle, &SP, &StackAddress, &Len);
	Len *= 4;
	DBG_Trace("%s call stack info:", Name);
	for(i = 0; i < 16, SP < StackAddress + Len; SP += 4)
	{
		PC = *((uint32_t *) SP) - 4;
		if ((PC > __FLASH_APP_START_ADDR__) && (PC < (__FLASH_BASE_ADDR__ + __CORE_FLASH_BLOCK_NUM__ * __FLASH_BLOCK_SIZE__)))
		{

	        if (PC % 2 == 0) {
	            continue;
	        }
	        PC = *((uint32_t *) SP) - 1;
	        if (disassembly_ins_is_bl_blx(PC - 4))
	        {
				DBG_Trace("%x", PC);
				i++;
	        }
		}
	}
}

void Core_PrintServiceStack(void)
{
	prvCore_PrintTaskStack(prvService.HardwareHandle);
	prvCore_PrintTaskStack(prvService.UserHandle);
	prvCore_PrintTaskStack(prvService.ServiceHandle);
}

void Core_SetSleepEnable(uint8_t OnOff)
{
	prvService.SleepEnable = OnOff;
}

uint8_t Core_GetSleepFlag(void)
{
	return prvService.SleepEnable;
}

void Core_DebugMem(uint8_t HeapID, const char *FuncName, uint32_t Line)
{
	int32_t curalloc, totfree, maxfree;
	OS_MemInfo(&curalloc, &totfree, &maxfree);
	if (!prvService.InitAllocMem)
	{
		prvService.InitAllocMem = curalloc;
		prvService.LastAllocMem = curalloc;
		DBG_Printf("base %d\r\n", prvService.InitAllocMem);
	}
	else
	{
		DBG_Printf("%s %u:total %d last %d\r\n", FuncName, Line, curalloc - prvService.InitAllocMem, curalloc - prvService.LastAllocMem);
		prvService.LastAllocMem = curalloc;
	}
}

void Core_HWTaskInit(void)
{
	prvService.HardwareHandle = Task_Create(prvHW_Task, NULL, 4 * 1024, HW_TASK_PRO, "HW task");
}

void Core_ServiceInit(void)
{
	prvService.SleepEnable = 0;
	prvService.ServiceHandle = Task_Create(prvService_Task, NULL, 8 * 1024, SERVICE_TASK_PRO, "Serv task");
}

void Core_UserTaskInit(void)
{
#ifdef __LUATOS__
	prvService.UserHandle = Task_Create(prvLuatOS_Task, NULL, 16*1024, LUATOS_TASK_PRO, "luatos task");
	luat_base_init();
#endif
}

INIT_TASK_EXPORT(Core_HWTaskInit, "0");
INIT_TASK_EXPORT(Core_ServiceInit, "1");
INIT_TASK_EXPORT(Core_UserTaskInit, "2");
#endif
