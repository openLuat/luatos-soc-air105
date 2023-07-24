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

#define LUAT_BSP_VERSION "V1020"

//------------------------------------------------------
// 以下custom --> 到  <-- custom 之间的内容,是供用户配置的
// 同时也是云编译可配置的部分. 提交代码时切勿删除会修改标识
//custom -->
//------------------------------------------------------

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
#define LUAT_USE_KEYBOARD 1
#define LUAT_USE_DAC 1
#define LUAT_USE_OTP 1
#define LUAT_USE_SOFT_UART 1

#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
#define LUAT_USE_LIBGNSS  1
#define LUAT_USE_FS  1
#define LUAT_USE_SENSOR  1
#define LUAT_USE_SFUD  1
#define LUAT_USE_SFD  1
// #define LUAT_USE_STATEM 1
// #define LUAT_USE_COREMARK 1


// 启用64位虚拟机
// #define LUAT_CONF_VM_64bit


// #define LUAT_USE_YMODEM


// FDB 提供kv数据库, 与nvm库类似
// #define LUAT_USE_FDB 1
// fskv提供与fdb库兼容的API,旨在替换fdb库
#define LUAT_USE_FSKV 1

//#define LUAT_USE_ZLIB
#define LUAT_USE_CAMERA  1
#define LUAT_USE_I2CTOOLS 1
#define LUAT_USE_SOFTKB 1
#define LUAT_USE_PROTOBUF

#define LUAT_USE_FATFS 1
#define LUAT_USE_FATFS_CHINESE
// #define LUAT_FF_USE_LFN 3
// #define LUAT_FF_LFN_UNICODE 3

#define LUAT_USE_USB    1
#define LUAT_USE_MEDIA    1
#define LUAT_USE_IO_QUEUE    1

#define LUAT_USE_W5500  1
#define LUAT_USE_DHCP  1
#define LUAT_USE_DNS  1
#define LUAT_USE_NETWORK 1
#define LUAT_USE_TLS 1
#define LUAT_USE_SNTP 1
#define LUAT_USE_FTP 1

#define LUAT_USE_IOTAUTH 1
#define LUAT_USE_LORA 1
// #define LUAT_USE_MAX30102
// #define LUAT_USE_MLX90640 1
#define LUAT_USE_MINIZ 1
#define LUAT_USE_BIT64 1
//----------------------------
// 高通字体, 需配合芯片使用
#define LUAT_USE_GTFONT 1
#define LUAT_USE_GTFONT_UTF8 1

//----------------------------
// 高级功能, 其中repl推荐开启, shell已废弃
// #define LUAT_USE_SHELL 1
// #define LUAT_USE_DBG 1
#define LUAT_USE_OTA 1
#define LUAT_USE_FOTA 1
#define LUAT_USE_REPL 1

// 多虚拟机支持,实验性,一般不启用
// #define LUAT_USE_VMX 1 
// #define LUAT_USE_NES

// RSA 加解密,加签验签
#define LUAT_USE_RSA 1

// 国密算法 SM2/SM3/SM4
#define LUAT_USE_GMSSL 1

// #define LUAT_USE_ICONV 1

//---------------------
// UI
#define LUAT_USE_LCD 1
#define LUAT_LCD_CMD_DELAY_US 7
#define LUAT_USE_TJPGD 1
#define LUAT_USE_EINK 1

//---------------------
// U8G2
#define LUAT_USE_DISP  1
#define LUAT_USE_U8G2  1

/**************FONT*****************/
#define LUAT_USE_FONTS  1
/**********U8G2&LCD&EINK FONT*************/
#define USE_U8G2_OPPOSANSM_ENGLISH 1
#define USE_U8G2_OPPOSANSM12_CHINESE
//#define USE_U8G2_OPPOSANSM16_CHINESE
// #define USE_U8G2_OPPOSANSM24_CHINESE
// #define USE_U8G2_OPPOSANSM32_CHINESE
// SARASA
// #define USE_U8G2_SARASA_M8_CHINESE
// #define USE_U8G2_SARASA_M10_CHINESE
// #define USE_U8G2_SARASA_M12_CHINESE
// #define USE_U8G2_SARASA_M14_CHINESE
// #define USE_U8G2_SARASA_M16_CHINESE
// #define USE_U8G2_SARASA_M18_CHINESE
// #define USE_U8G2_SARASA_M20_CHINESE
// #define USE_U8G2_SARASA_M22_CHINESE
// #define USE_U8G2_SARASA_M24_CHINESE
// #define USE_U8G2_SARASA_M26_CHINESE
// #define USE_U8G2_SARASA_M28_CHINESE
/**********LVGL FONT*************/
//#define LV_FONT_OPPOSANS_M_12
#define LV_FONT_OPPOSANS_M_16
//---------------------
// LVGL
#define LUAT_USE_LCD
#define LUAT_USE_LVGL

// #define LV_USE_LOG 1

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

#define LUAT_SCRIPT_SIZE            512         //脚本区大小,必须为64KB的倍数
#define LUAT_FS_SIZE                512         //文件系统大小,必须为64KB的倍数


//-------------------------------------------------------------------------------
//<-- custom
//------------------------------------------------------------------------------


// 以下选项仅开发人员可修改, 一般用户切勿自行修改

#ifdef LUAT_HEAP_SIZE_256K
#define LUAT_HEAP_SIZE (256)
#endif

#ifdef LUAT_HEAP_SIZE_300K
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (300)
#endif

#ifdef LUAT_HEAP_SIZE_400K
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (400)
#endif

#ifndef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (200)
#endif

#define FLASH_FS_REGION_SIZE        (LUAT_FS_SIZE + LUAT_SCRIPT_SIZE)

#define LUAT_GPIO_NUMS	32
#define LUAT__UART_TX_NEED_WAIT_DONE
// 内存优化: 减少内存消耗, 会稍微减低性能
#define LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP 1

//----------------------------------
// 使用VFS(虚拟文件系统)和内置库文件, 必须启用
#define LUAT_USE_FS_VFS 1
#define LUAT_USE_VFS_INLINE_LIB 1
//----------------------------------

#define LV_DISP_DEF_REFR_PERIOD gLVFlashTime
extern unsigned int gLVFlashTime;

#define LUAT_LV_DEBUG 0

#define LV_MEM_CUSTOM 1

#define LUAT_USE_LVGL_INDEV 1


#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (240)
#define LV_COLOR_DEPTH          16

#define LV_COLOR_16_SWAP   1
#define __LVGL_SLEEP_ENABLE__

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE  "app_interface.h"         /*Header for the system time function*/
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)GetSysTickMS())     /*Expression evaluating to current system time in ms*/

#define LUAT_USE_LCD_CUSTOM_DRAW
#define time(X)	luat_time(X)

#define LV_USE_PERF_MONITOR     1

#define __LUATOS_TICK_64BIT__
#define LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP

#ifndef __LV_DEBUG__
#undef LV_USE_PERF_MONITOR
#endif

#define LUAT_RT_RET_TYPE int
#define LUAT_RT_RET	0
#define LUAT_RT_CB_PARAM void *pdata, void *param

#define __LUAT_C_CODE_IN_RAM__ __attribute__((section (".RamFunc")))

// 单纯为了生成文档的宏
#define LUAT_USE_PIN 1

#endif
