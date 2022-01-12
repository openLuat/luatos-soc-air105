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

#ifndef __USB_MSC_SCSI_H__
#define __USB_MSC_SCSI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bsp_common.h"

#define MODE_SENSE6_LEN                    0x17U
#define MODE_SENSE10_LEN                   0x1BU
#define LENGTH_INQUIRY_PAGE00              0x06U
#define LENGTH_INQUIRY_PAGE80              0x08U
#define LENGTH_FORMAT_CAPACITIES           0x14U


#define SENSE_LIST_DEEPTH                           8U

/* SCSI Commands */
#define SCSI_FORMAT_UNIT                            0x04U
#define SCSI_INQUIRY                                0x12U
#define SCSI_MODE_SELECT6                           0x15U
#define SCSI_MODE_SELECT10                          0x55U
#define SCSI_MODE_SENSE6                            0x1AU
#define SCSI_MODE_SENSE10                           0x5AU
#define SCSI_ALLOW_MEDIUM_REMOVAL                   0x1EU
#define SCSI_READ6                                  0x08U
#define SCSI_READ10                                 0x28U
#define SCSI_READ12                                 0xA8U
#define SCSI_READ16                                 0x88U

#define SCSI_READ_CAPACITY10                        0x25U
#define SCSI_READ_CAPACITY16                        0x9EU

#define SCSI_REQUEST_SENSE                          0x03U
#define SCSI_START_STOP_UNIT                        0x1BU
#define SCSI_TEST_UNIT_READY                        0x00U
#define SCSI_WRITE6                                 0x0AU
#define SCSI_WRITE10                                0x2AU
#define SCSI_WRITE12                                0xAAU
#define SCSI_WRITE16                                0x8AU

#define SCSI_VERIFY10                               0x2FU
#define SCSI_VERIFY12                               0xAFU
#define SCSI_VERIFY16                               0x8FU

#define SCSI_SEND_DIAGNOSTIC                        0x1DU
#define SCSI_READ_FORMAT_CAPACITIES                 0x23U
#define SCSI_SEEK10                					0x2BU
#define SCSI_WRITE_VERIFY             				0x2EU

#define SENSE_KEY_NO_SENSE                                    0U
#define SENSE_KEY_RECOVERED_ERROR                             1U
#define SENSE_KEY_NOT_READY                                   2U
#define SENSE_KEY_MEDIUM_ERROR                                3U
#define SENSE_KEY_HARDWARE_ERROR                              4U
#define SENSE_KEY_ILLEGAL_REQUEST                             5U
#define SENSE_KEY_UNIT_ATTENTION                              6U
#define SENSE_KEY_DATA_PROTECT                                7U
#define SENSE_KEY_BLANK_CHECK                                 8U
#define SENSE_KEY_VENDOR_SPECIFIC                             9U
#define SENSE_KEY_COPY_ABORTED                                10U
#define SENSE_KEY_ABORTED_COMMAND                             11U
#define SENSE_KEY_VOLUME_OVERFLOW                             13U
#define SENSE_KEY_MISCOMPARE                                  14U


#define INVALID_COMMAND_OPERATION_CODE              0x20U
#define INVALID_FIELED_IN_COMMAND                   0x24U
#define PARAMETER_LIST_LENGTH_ERROR                 0x1AU
#define INVALID_FIELD_IN_PARAMETER_LIST             0x26U
#define ADDRESS_OUT_OF_RANGE                        0x21U
#define MEDIUM_NOT_PRESENT                          0x3AU
#define MEDIUM_HAVE_CHANGED                         0x28U
#define WRITE_PROTECTED                             0x27U
#define UNRECOVERED_READ_ERROR                      0x11U
#define WRITE_FAULT                                 0x03U

#define READ_FORMAT_CAPACITY_DATA_LEN               0x0CU
#define READ_CAPACITY10_DATA_LEN                    0x08U
#define REQUEST_SENSE_DATA_LEN                      0x12U
#define STANDARD_INQUIRY_DATA_LEN                   0x24U
#define BLKVFY                                      0x04U

#define SCSI_MEDIUM_UNLOCKED                        0x00U
#define SCSI_MEDIUM_LOCKED                          0x01U
#define SCSI_MEDIUM_EJECTED                         0x02U



typedef struct
{
	uint8_t Skey;
	uint8_t ASC;
	uint8_t ASCQ;
	PV_Union uData;
} SCSI_SenseTypeDef;

typedef struct
{
	union
	{
		uint8_t Byte0;
		struct
		{
			uint8_t Type:5;
			uint8_t Zero:3;
		}Byte0_b;
	};
	union
	{
		uint8_t Byte1;
		struct
		{
			uint8_t Zero:7;
			uint8_t RMB:1;
		}Byte1_b;
	};
	union
	{
		uint8_t Byte2;
		struct
		{
			uint8_t ANSI:3;
			uint8_t ECMA:3;
			uint8_t ISO:2;
		}Byte2_b;
	};
	union
	{
		uint8_t Byte3;
		struct
		{
			uint8_t ResponseDataFormat:4;
			uint8_t Zero:4;
		}Byte3_b;
	};
	uint8_t RestLen;
	uint8_t Zero[3];
	uint8_t VendorInfo[8];
	uint8_t ProductInfo[16];
	uint8_t ProductVersion[4];
} SCSI_InquiryDataTypeDef;


/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_MSC_SCSI_H */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

