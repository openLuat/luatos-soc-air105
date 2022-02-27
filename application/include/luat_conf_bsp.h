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


#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

#define LUAT_BSP_VERSION "V0005"


#define LUAT_USE_FS_VFS 1
#define LUAT_USE_VFS_INLINE_LIB 1


#define LUAT_USE_UART 1
#define LUAT_USE_GPIO 1
#define LUAT_USE_I2C  1
#define LUAT_USE_SPI  1
#define LUAT_USE_ADC  1
#define LUAT_USE_PWM  1
#define LUAT_USE_WDT  1
#define LUAT_USE_PM  1
#define LUAT_USE_MCU  1
// #define LUAT_USE_HWTIMER  1
#define LUAT_USE_RTC 1
// #define LUAT_USE_SDIO 1
#define LUAT_USE_KEYBOARD 1
#define LUAT_USE_DAC 1
#define LUAT_USE_OTP 1

#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
//#define LUAT_USE_LIBGNSS  1
#define LUAT_USE_FS  1
#define LUAT_USE_SENSOR  1
#define LUAT_USE_SFUD  1
#define LUAT_USE_SFD  1
// #define LUAT_USE_STATEM 1
// #define LUAT_USE_COREMARK 1
#define LUAT_USE_FDB 1
// #define LUAT_USE_ZLIB 
#define LUAT_USE_CAMERA  1
#define LUAT_USE_FATFS 1

//----------------------------
// 高通字体, 需配合芯片使用
// #define LUAT_USE_GTFONT 1
// #define LUAT_USE_GTFONT_UTF8

//----------------------------
// 高级功能, 其中shell是推荐启用, 除非你打算uart0也读数据
#define LUAT_USE_SHELL 
#define LUAT_USE_DBG

#define LUAT_USE_OTA

// 多虚拟机支持,实验性,一般不启用 
// #define LUAT_USE_VMX 1 
// #define LUAT_USE_NES

//---------------------
// UI
#define LUAT_USE_LCD
#define LUAT_LCD_CMD_DELAY_US 7
#define LUAT_USE_EINK

//---------------------
// U8G2
#define LUAT_USE_DISP 
#define LUAT_USE_U8G2
#define U8G2_USE_SH1106
#define U8G2_USE_ST7567

/**************FONT*****************/
/**********U8G2&LCD FONT*************/
// #define USE_U8G2_UNIFONT_SYMBOLS
#define USE_U8G2_OPPOSANSM12_CHINESE
//#define USE_U8G2_OPPOSANSM16_CHINESE
// #define USE_U8G2_OPPOSANSM24_CHINESE
// #define USE_U8G2_OPPOSANSM32_CHINESE
/**********LVGL FONT*************/
//#define LV_FONT_OPPOSANS_M_12
#define LV_FONT_OPPOSANS_M_16
//---------------------
// LVGL
#define LUAT_USE_LCD
#define LUAT_USE_LVGL
#define LV_DISP_DEF_REFR_PERIOD gLVFlashTime
extern unsigned int gLVFlashTime;
#define LUAT_LV_DEBUG 0
#define LV_USE_LOG 1
#define LV_MEM_CUSTOM 1

#define LUAT_USE_LVGL_INDEV 1

#define LUAT_USE_LVGL_ARC   //圆弧 无依赖
#define LUAT_USE_LVGL_BAR   //进度条 无依赖
#define LUAT_USE_LVGL_BTN   //按钮 依赖容器CONT
#define LUAT_USE_LVGL_BTNMATRIX   //按钮矩阵 无依赖
#define LUAT_USE_LVGL_CALENDAR   //日历 无依赖
#define LUAT_USE_LVGL_CANVAS   //画布 依赖图片IMG
#define LUAT_USE_LVGL_CHECKBOX   //复选框 依赖按钮BTN 标签LABEL
#define LUAT_USE_LVGL_CHART   //图表 无依赖
#define LUAT_USE_LVGL_CONT   //容器 无依赖
#define LUAT_USE_LVGL_CPICKER   //颜色选择器 无依赖
#define LUAT_USE_LVGL_DROPDOWN   //下拉列表 依赖页面PAGE 标签LABEL
#define LUAT_USE_LVGL_GAUGE   //仪表 依赖进度条BAR 仪表(弧形刻度)LINEMETER
#define LUAT_USE_LVGL_IMG   //图片 依赖标签LABEL
#define LUAT_USE_LVGL_IMGBTN   //图片按钮 依赖按钮BTN
#define LUAT_USE_LVGL_KEYBOARD   //键盘 依赖图片按钮IMGBTN
#define LUAT_USE_LVGL_LABEL   //标签 无依赖
#define LUAT_USE_LVGL_LED   //LED 无依赖
#define LUAT_USE_LVGL_LINE   //线 无依赖
#define LUAT_USE_LVGL_LIST   //列表 依赖页面PAGE 按钮BTN 标签LABEL
#define LUAT_USE_LVGL_LINEMETER   //仪表(弧形刻度) 无依赖
#define LUAT_USE_LVGL_OBJMASK   //对象蒙版 无依赖
#define LUAT_USE_LVGL_MSGBOX   //消息框 依赖图片按钮IMGBTN 标签LABEL
#define LUAT_USE_LVGL_PAGE   //页面 依赖容器CONT
#define LUAT_USE_LVGL_SPINNER   //旋转器 依赖圆弧ARC 动画ANIM
#define LUAT_USE_LVGL_ROLLER   //滚筒 无依赖
#define LUAT_USE_LVGL_SLIDER   //滑杆 依赖进度条BAR
#define LUAT_USE_LVGL_SPINBOX   //数字调整框 无依赖
#define LUAT_USE_LVGL_SWITCH   //开关 依赖滑杆SLIDER
#define LUAT_USE_LVGL_TEXTAREA   //文本框 依赖标签LABEL 页面PAGE
#define LUAT_USE_LVGL_TABLE   //表格 依赖标签LABEL
#define LUAT_USE_LVGL_TABVIEW   //页签 依赖页面PAGE 图片按钮IMGBTN
#define LUAT_USE_LVGL_TILEVIEW   //平铺视图 依赖页面PAGE
#define LUAT_USE_LVGL_WIN   //窗口 依赖容器CONT 按钮BTN 标签LABEL 图片IMG 页面PAGE

#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (240)
#define LV_COLOR_DEPTH          16

#define LV_COLOR_16_SWAP   1
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE  "app_interface.h"         /*Header for the system time function*/
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)GetSysTickMS())     /*Expression evaluating to current system time in ms*/

#define LV_NO_BLOCK_FLUSH
#define time(X)	luat_time(X)
#endif

#define LV_USE_PERF_MONITOR     1
#ifndef __DEBUG__
#undef LV_USE_PERF_MONITOR
#endif
//#define LV_ATTRIBUTE_FAST_MEM	__attribute__((section (".RamFunc")))
