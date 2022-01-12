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

static const USB_HIDKeyValue prvKeyList[128] =
{
		{HID_KEY_SPACE, 0},	//00
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_BACKSPACE, 0},	//08
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},		//0a
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_ENTER, 0},		//0d
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},		//10
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},		//1a
		{HID_KEY_ESC, 0},		//1b
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},
		{HID_KEY_SPACE, 0},		//20
		{HID_KEY_1, 1},		//!
		{HID_KEY_QUOTE, 1},		//"
		{HID_KEY_3, 1},		//#
		{HID_KEY_4, 1},		//$
		{HID_KEY_5, 1},		//%
		{HID_KEY_7, 1},		//&
		{HID_KEY_QUOTE, 0},		//'
		{HID_KEY_9, 1},		//(
		{HID_KEY_0, 1},		//)
		{HID_KEY_8, 1},		//*
		{HID_KEY_EQUAL, 1},		//+
		{HID_KEY_COMMA, 0},		//,
		{HID_KEY_SUB, 0},		//-
		{HID_KEY_DOT, 0},		//.
		{HID_KEY_QUESTION, 0},		///
		{HID_KEY_0, 0},		//0
		{HID_KEY_1, 0},		//1
		{HID_KEY_2, 0},		//2
		{HID_KEY_3, 0},		//3
		{HID_KEY_4, 0},		//4
		{HID_KEY_5, 0},		//5
		{HID_KEY_6, 0},		//6
		{HID_KEY_7, 0},		//7
		{HID_KEY_8, 0},		//8
		{HID_KEY_9, 0},		//9
		{HID_KEY_SEMICOLON, 1},		//:
		{HID_KEY_SEMICOLON, 0},		//;
		{HID_KEY_COMMA, 1},		// 	<
		{HID_KEY_EQUAL, 0},		//=
		{HID_KEY_DOT, 0},		//>
		{HID_KEY_QUESTION, 1},		// 	?
		{HID_KEY_2, 1},		//@
		{HID_KEY_A, 1},		//A
		{HID_KEY_B, 1},		//B
		{HID_KEY_C, 1},		//C
		{HID_KEY_D, 1},		//D
		{HID_KEY_E, 1},		//E
		{HID_KEY_F, 1},		//F
		{HID_KEY_G, 1},		//G
		{HID_KEY_H, 1},		//H
		{HID_KEY_I, 1},		//I
		{HID_KEY_J, 1},		//J
		{HID_KEY_K, 1},		//K
		{HID_KEY_L, 1},		//L
		{HID_KEY_M, 1},		//M
		{HID_KEY_N, 1},		//N
		{HID_KEY_O, 1},		//O
		{HID_KEY_P, 1},		//P
		{HID_KEY_Q, 1},		//Q
		{HID_KEY_R, 1},		//R
		{HID_KEY_S, 1},		//S
		{HID_KEY_T, 1},		//T
		{HID_KEY_U, 1},		//U
		{HID_KEY_V, 1},		//V
		{HID_KEY_W, 1},		//W
		{HID_KEY_X, 1},		//X
		{HID_KEY_Y, 1},		//Y
		{HID_KEY_Z, 1},		//Z
		{HID_KEY_LEFT_BRACKET, 0},		// 	[
		{HID_KEY_VERTICAL_LINE, 0},		// "\"
		{HID_KEY_RIGHT_BRACKET, 0},		// 	]
		{HID_KEY_6, 1},		// 	^
		{HID_KEY_SUB, 1},		// 	_
		{HID_KEY_WAVE, 0},		// 	`
		{HID_KEY_A, 0},		//A
		{HID_KEY_B, 0},		//B
		{HID_KEY_C, 0},		//C
		{HID_KEY_D, 0},		//D
		{HID_KEY_E, 0},		//E
		{HID_KEY_F, 0},		//F
		{HID_KEY_G, 0},		//G
		{HID_KEY_H, 0},		//H
		{HID_KEY_I, 0},		//I
		{HID_KEY_J, 0},		//J
		{HID_KEY_K, 0},		//K
		{HID_KEY_L, 0},		//L
		{HID_KEY_M, 0},		//M
		{HID_KEY_N, 0},		//N
		{HID_KEY_O, 0},		//O
		{HID_KEY_P, 0},		//P
		{HID_KEY_Q, 0},		//Q
		{HID_KEY_R, 0},		//R
		{HID_KEY_S, 0},		//S
		{HID_KEY_T, 0},		//T
		{HID_KEY_U, 0},		//U
		{HID_KEY_V, 0},		//V
		{HID_KEY_W, 0},		//W
		{HID_KEY_X, 0},		//X
		{HID_KEY_Y, 0},		//Y
		{HID_KEY_Z, 0},		//Z
		{HID_KEY_LEFT_BRACKET, 1},		// 	{
		{HID_KEY_VERTICAL_LINE, 1},		// 	|
		{HID_KEY_RIGHT_BRACKET, 1},		// 	}
		{HID_KEY_WAVE, 1},		// 	~
		{HID_KEY_PAD_DOT, 0},		// 	DEL
};

USB_HIDKeyValue USB_HIDGetValueFromAscii(uint8_t ascii)
{
	if (ascii & 0x80)
	{
		return prvKeyList[0];
	}
	else
	{
		return prvKeyList[ascii];
	}
}
