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

#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "app_interface.h"

#define LUAT_LOG_TAG "luat.usbapp"
#include "luat_log.h"
extern void *luat_spi_get_sdhc_ctrl(void);

int32_t luat_usb_app_vhid_cb(void *pData, void *pParam);
void luat_usb_app_set_vid_pid(uint8_t usb_id, uint16_t vid, uint16_t pid)
{
	Core_USBSetID(usb_id, vid, pid);

}
void uat_usb_app_set_hid_mode(uint8_t usb_id, uint8_t hid_mode, uint8_t buff_size)
{
	Core_USBSetHIDMode(usb_id, hid_mode, buff_size);
}
//打开luatos内置usb device config，实现虚拟MSC，HID和串口的复合设备功能，串口收发见luat_uart
void luat_usb_app_start(uint8_t usb_id)
{
	Core_USBDefaultDeviceStart(usb_id);
	Core_VHIDInit(usb_id, luat_usb_app_vhid_cb);
}

void luat_usb_app_stop(uint8_t usb_id)
{
	USB_StackStop(usb_id);
	USB_StackClearSetup(usb_id);
}

void luat_usb_app_vhid_tx(uint8_t usb_id, uint8_t *data, uint32_t len)
{
	Core_VHIDSendRawData(usb_id, data, len);
}

uint32_t luat_usb_app_vhid_rx(uint8_t usb_id, uint8_t *data, uint32_t len)
{
	return Core_VHIDRxBufferRead(usb_id, data, len);
}

void luat_usb_app_vhid_upload(uint8_t usb_id, uint8_t *key_data, uint32_t len)
{
	Core_VHIDUploadData(usb_id, key_data, len);
}

void luat_usb_app_vhid_cancel_upload(uint8_t usb_id)
{
	Core_VHIDUploadStop(usb_id);
}

void luat_usb_udisk_attach_sdhc(uint8_t usb_id)
{
	Core_UDiskAttachSDHC(usb_id, luat_spi_get_sdhc_ctrl());
}

void luat_usb_udisk_detach_sdhc(uint8_t usb_id)
{
	Core_UDiskDetachSDHC(usb_id);
}
