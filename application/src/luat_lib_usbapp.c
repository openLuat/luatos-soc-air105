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



/*
@module  usbapp
@summary USB功能操作
@version 1.0
@date    2022.01.17
*/
#include "luat_base.h"
#include "luat_msgbus.h"
#include "app_interface.h"

static int l_usb_app_vhid_cb(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "USB_HID_INC");
        lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    return 0;
}

int32_t luat_usb_app_vhid_cb(void *pData, void *pParam)
{
	//在这里把底层的HID消息转换为LUAT消息并上传，同时删除本注释
    rtos_msg_t msg;
    msg.handler = l_usb_app_vhid_cb;
	switch((uint32_t)pParam)
	{
	case USB_HID_NOT_READY:
	case USB_HID_READY:
	case USB_HID_SEND_DONE:
	    msg.arg1 = (uint32_t)pParam;
        luat_msgbus_put(&msg, 0);
		break;
	}
    return 0;
}

/*
启动USB设备
@api usbapp.start(id)
@int 设备id,默认为0
@return bool 成功返回true,否则返回false
@usage
-- 启动USB
usbapp.start(0)
*/
static int l_usb_start(lua_State* L) {
    luat_usb_app_start(USB_ID0);
    lua_pushboolean(L, 1);
    return 1;
}

/*
关闭USB设备
@api usbapp.stop(id)
@int 设备id,默认为0
@return bool 成功返回true,否则返回false
@usage
-- 关闭USB
usbapp.stop(0)
*/
static int l_usb_stop(lua_State* L) {
    luat_usb_app_stop(USB_ID0);
    lua_pushboolean(L, 1);
    return 1;
}

/*
USB HID设备上传数据
@api usbapp.vhid_upload(id, data)
@int 设备id,默认为0
@string 数据. 注意, HID的可用字符是有限制的, 基本上只有可见字符是支持的, 不支持的字符会替换为空格.
@return bool 成功返回true,否则返回false
@usage
-- HID上传数据
usbapp.vhid_upload(0, "1234") -- usb hid会模拟敲出1234
*/
static int l_usb_vhid_upload(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    if (len > 0) {
        luat_usb_app_vhid_upload(USB_ID0, data, len);
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/*
USB HID设备取消上传数据
@api usbapp.vhid_cancel_upload(id)
@int 设备id,默认为0
@return nil 无返回值
@usage
-- 取消上传数据,通常不需要
usbapp.vhid_cancel_upload(0)
*/
static int l_usb_vhid_cancel_upload(lua_State* L) {
    luat_usb_app_vhid_cancel_upload(USB_ID0);
    return 0;
}


#include "rotable.h"
static const rotable_Reg reg_usbapp[] =
{
    { "start",              l_usb_start,                0},
    { "start",              l_usb_stop,                 0},
    { "vhid_upload",        l_usb_vhid_upload,          0},
    { "vhid_cancel_upload", l_usb_vhid_cancel_upload,   0},

    { "HID_NOT_READY",      NULL,   USB_HID_NOT_READY},
    { "HID_READY",          NULL,   USB_HID_READY},
    { "HID_SEND_DONE",      NULL,   USB_HID_SEND_DONE},
	{ NULL,        NULL,   0}
};

LUAMOD_API int luaopen_usbapp( lua_State *L ) {
    luat_newlib(L, reg_usbapp);
    return 1;
}
