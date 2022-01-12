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

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__
#include "usb_define.h"
enum
{
	USBD_IDX_LANGID_STR = 0,
	USBD_IDX_MANUFACTURER_STR,
	USBD_IDX_PRODUCT_STR,
	USBD_IDX_SERIAL_STR,
	USBD_IDX_CONFIG_STR,
	USBD_IDX_INTERFACE0_STR,

	USBD_BUS_TYPE_SUSPEND = 0,
	USBD_BUS_TYPE_RESUME,
	USBD_BUS_TYPE_RESET,
	USBD_BUS_TYPE_NEW_SOF,
	USBD_BUS_TYPE_DISCONNECT,
	USBD_BUS_TYPE_ENABLE_CONNECT,

	USB_DEVICE_SPEED_FULL_SPEED = 0,
	USB_DEVICE_SPEED_HIGH_SPEED,
	USB_DEVICE_SPEED_SUPER_SPEED,
	USB_DEVICE_SPEED_QTY,
};

#define  USB_LEN_DEV_QUALIFIER_DESC                     0x0AU
#define  USB_LEN_DEV_DESC                               0x12U
#define  USB_LEN_CFG_DESC                               0x09U
#define  USB_LEN_ASSOC_IF_DESC                          0x08U
#define  USB_LEN_IF_DESC                                0x09U
#define  USB_LEN_EP_DESC                                0x07U
#define  USB_LEN_OTG_DESC                               0x03U
#define  USB_LEN_LANGID_STR_DESC                        0x04U
#define  USB_LEN_OTHER_SPEED_DESC_SIZ                   0x09U


typedef struct
{
	usb_interface_descriptor_t *pInterfaceDesc;
	usb_endpoint_descriptor_t *pEndpointDesc;
	usb_endpoint_ss_comp_descriptor_t *pEndpointSSCompDesc;
	CBFuncEx_t GetOtherDesc;
	uint8_t EndpointNum;
}usb_full_interface_t;

typedef struct
{
	usb_config_descriptor_t *pConfigDesc;
	usb_interface_assoc_descriptor_t *pInterfaceAssocDesc;
	usb_full_interface_t *pInterfaceFullDesc;
	uint8_t InterfaceNum;
}usb_full_config_t;

typedef struct
{
	usb_device_qualifier_t *pQualifierDesc;
	usb_full_config_t *pFullConfig[USB_DEVICE_SPEED_QTY];
}USB_FullConfigStruct;



typedef struct
{
	usb_device_descriptor_t *pDeviceDesc;	//静态处理，不会释放内存
	USB_FullConfigStruct *pConfigInfo;	//静态处理，不会释放内存
	USB_FullConfigStruct *pCurConfigInfo;//当前用到的配置
	Buffer_Struct *pString;		//字符串描述, 动态处理
	CBFuncEx_t CB;
	void *pUserData;
	uint8_t ConfigNum;			//配置总数
	uint8_t StringNum;			//字符串描述总数
	uint8_t CurSpeed;
}USB_SetupInfoStruct;	//USB2.0设备配置信息，支持到高速设备，不能支持USB3.0的超高速设备


#endif
