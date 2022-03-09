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

#ifndef __USB_HID_H__
#define __USB_HID_H__
#include "bsp_common.h"
#include "usb_define.h"

//Keybord keyvalue define
#define HID_KEY_NULL 0x00          // NULL
#define HID_KEY_A 0x04             // A
#define HID_KEY_B 0x05             // B
#define HID_KEY_C 0x06             // C
#define HID_KEY_D 0x07             // D
#define HID_KEY_E 0x08             // E
#define HID_KEY_F 0x09             // F
#define HID_KEY_G 0x0A             // G
#define HID_KEY_H 0x0B             // H
#define HID_KEY_I 0x0C             // I
#define HID_KEY_J 0x0D             // J
#define HID_KEY_K 0x0E             // K
#define HID_KEY_L 0x0F             // L
#define HID_KEY_M 0x10             // M
#define HID_KEY_N 0x11             // N
#define HID_KEY_O 0x12             // O
#define HID_KEY_P 0x13             // P
#define HID_KEY_Q 0x14             // Q
#define HID_KEY_R 0x15             // R
#define HID_KEY_S 0x16             // S
#define HID_KEY_T 0x17             // T
#define HID_KEY_U 0x18             // U
#define HID_KEY_V 0x19             // V
#define HID_KEY_W 0x1A             // W
#define HID_KEY_X 0x1B             // X
#define HID_KEY_Y 0x1C             // Y
#define HID_KEY_Z 0x1D             // Z
#define HID_KEY_1 0x1E             // 1 and !
#define HID_KEY_2 0x1F             // 2 and @
#define HID_KEY_3 0x20             // 3 and #
#define HID_KEY_4 0x21             // 4 and $
#define HID_KEY_5 0x22             // 5 and %
#define HID_KEY_6 0x23             // 6 and ^
#define HID_KEY_7 0x24             // 7 and &
#define HID_KEY_8 0x25             // 8 and *
#define HID_KEY_9 0x26             // 9 and (
#define HID_KEY_0 0x27             // 10 and )
#define HID_KEY_ENTER 0x28         // ENTER
#define HID_KEY_ESC 0x29           // ESC
#define HID_KEY_BACKSPACE 0x2A     // BACKSPACE
#define HID_KEY_TAB 0x2B           // TAB
#define HID_KEY_SPACE 0x2C         // SPACE
#define HID_KEY_SUB 0x2D           // - and _
#define HID_KEY_EQUAL 0x2E         // = and +
#define HID_KEY_LEFT_BRACKET 0x2F  // [ and {
#define HID_KEY_RIGHT_BRACKET 0x30 // ] and }
#define HID_KEY_VERTICAL_LINE 0x31 // "\" and |
#define HID_KEY_WAVE 0x32          // ` and ~
#define HID_KEY_SEMICOLON 0x33     // ; and :
#define HID_KEY_QUOTE 0x34         // ' and "
#define HID_KEY_THROW 0x35         // ~ and `
#define HID_KEY_COMMA 0x36         // , and <
#define HID_KEY_DOT 0x37           // . and >
#define HID_KEY_QUESTION 0x38      // / and ?
#define HID_KEY_CAPS_LOCK 0x39     // CAPS
#define HID_KEY_F1 0x3A
#define HID_KEY_F2 0x3B
#define HID_KEY_F3 0x3C
#define HID_KEY_F4 0x3D
#define HID_KEY_F5 0x3E
#define HID_KEY_F6 0x3F
#define HID_KEY_F7 0x40
#define HID_KEY_F8 0x41
#define HID_KEY_F9 0x42
#define HID_KEY_F10 0x43
#define HID_KEY_F11 0x44
#define HID_KEY_F12 0x45
#define HID_KEY_PRT_SCR 0x46
#define HID_KEY_SCOLL_LOCK 0x47
#define HID_KEY_PAUSE 0x48
#define HID_KEY_INS 0x49
#define HID_KEY_HOME 0x4A
#define HID_KEY_PAGEUP 0x4B
#define HID_KEY_DEL 0x4C
#define HID_KEY_END 0x4D
#define HID_KEY_PAGEDOWN 0x4E
#define HID_KEY_RIGHT_ARROW 0x4F
#define HID_KEY_LEFT_ARROW 0x50
#define HID_KEY_DOWN_ARROW 0x51
#define HID_KEY_UP_ARROW 0x52
//Num Pad
#define HID_KEY_PAD_NUMLOCK 0x53
#define HID_KEY_PAD_DIV 0x54
#define HID_KEY_PAD_MUL 0x55
#define HID_KEY_PAD_SUB 0x56
#define HID_KEY_PAD_ADD 0x57
#define HID_KEY_PAD_ENTER 0x58
#define HID_KEY_PAD_1 0x59
#define HID_KEY_PAD_2 0x5A
#define HID_KEY_PAD_3 0x5B
#define HID_KEY_PAD_4 0x5C
#define HID_KEY_PAD_5 0x5D
#define HID_KEY_PAD_6 0x5E
#define HID_KEY_PAD_7 0x5F
#define HID_KEY_PAD_8 0x60
#define HID_KEY_PAD_9 0x61
#define HID_KEY_PAD_0 0x62
#define HID_KEY_PAD_DOT 0x63
#define HID_KEY_PRESSED 0x00
#define HID_KEY_RELEASED 0x01
// Control
#define HID_KEY_LCTRL 0xE0  // left ctrl // #define HID_KEY_LCTRL 0x01
#define HID_KEY_LALT 0xE2   // left Alt // #define HID_KEY_LALT 0x04
#define HID_KEY_LSHFIT 0xE1 // left Shift // #define HID_KEY_LSHFIT 0x02
#define HID_KEY_LWIN 0xE3   // left windows // #define HID_KEY_LWIN 0x08
#define HID_KEY_RWIN 0xE7   // right windows // #define HID_KEY_RWIN 0x80
#define HID_KEY_RSHIFT 0xE5 // right Shift // #define HID_KEY_RSHIFT 0x20
#define HID_KEY_RALT 0xE6   // right Alt // #define HID_KEY_RALT 0x40
#define HID_KEY_RCTRL 0xE4  // right Ctrl // #define HID_KEY_RCTRL 0x10
#define HID_KEY_APP 0x65    // Application // #define HID_KEY_APP 0x65
#define HID_KEY_K14 0x89    // international key
#define HID_KEY_KR_L 0x91
#define HID_KEY_K107 0x85
#define HID_KEY_K45 0x64
#define HID_KEY_K42 0x32
#define HID_KEY_K131 0x8b
#define HID_KEY_K132 0x8a
#define HID_KEY_K133 0x88
#define HID_KEY_K56 0x87
#define HID_KEY_KR_R 0x90


#define USB_HID_DESC_LEN                          9U

#define USB_HID_DESCRIPTOR_TYPE                   0x21U
#define USB_HID_REPORT_DESC                       0x22U
#define USB_HID_PHYSICAL_DESC                     0x23U

#define USB_HID_REQ_SET_PROTOCOL                       0x0BU
#define USB_HID_REQ_GET_PROTOCOL                       0x03U

#define USB_HID_REQ_SET_IDLE                           0x0AU
#define USB_HID_REQ_GET_IDLE                           0x02U

#define USB_HID_REQ_SET_REPORT                         0x09U
#define USB_HID_REQ_GET_REPORT                         0x01U

enum
{
	USB_HID_NOT_READY,
	USB_HID_READY,
	USB_HID_SEND_DONE,
};

typedef struct
{
	uint8_t Value;
	uint8_t Shift;
}USB_HIDKeyValue;

typedef struct {
	uint8_t	bLength;
	uint8_t	bDescriptorType;
	uint8_t	bcdHID[2];
	uint8_t	bCountryCode;
	uint8_t	bNumDescriptors;
	uint8_t	bReportDescriptorType;
	uint8_t	wDescriptorLength[2];
}usb_hid_descriptor_t;

typedef struct
{
	union
	{
		uint8_t SpecialKey;
		struct
		{
			uint8_t LeftControl:1;
			uint8_t LeftShift  :1;
			uint8_t LeftAlt:1;
			uint8_t LeftGUI:1;
			uint8_t RightControl:1;
			uint8_t RightShift :1;
			uint8_t RightAlt :1;
			uint8_t RightGUI :1;
		}SPECIALHID_KEY_b;

	};
	uint8_t bZero;
	uint8_t PressKey[6];
}USB_HIDKeyBoradKeyStruct;

typedef struct
{
	union
	{
		uint8_t Led;
		struct
		{
			uint8_t NumsLock:1;
			uint8_t CapsLock:1;
			uint8_t l2:1;
			uint8_t l3:1;
			uint8_t Kana:1;
			uint8_t zero :3;
		}SPECIALHID_KEY_b;
	};
}USB_HIDKeyBoradLedStruct;

USB_HIDKeyValue USB_HIDGetValueFromAscii(uint8_t ascii);
#endif

