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
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include "luat_lcd.h"
#include "time.h"
#include <stdlib.h>

#include "app_interface.h"


#define LUAT_LOG_TAG "base"
#include "luat_log.h"

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
void lv_bmp_init(void);
void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

LUAMOD_API int luaopen_usbapp( lua_State *L );

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base}, // _G
  {LUA_LOADLIBNAME, luaopen_package}, // require
  {LUA_COLIBNAME, luaopen_coroutine}, // coroutine协程库
  {LUA_TABLIBNAME, luaopen_table},    // table库,操作table类型的数据结构
  {LUA_IOLIBNAME, luaopen_io},        // io库,操作文件
  {LUA_OSLIBNAME, luaopen_os},        // os库,已精简
  {LUA_STRLIBNAME, luaopen_string},   // string库,字符串操作
  {LUA_MATHLIBNAME, luaopen_math},    // math 数值计算
//  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},     // debug库,已精简
#ifdef LUAT_USE_DBG
#ifndef LUAT_USE_SHELL
#define LUAT_USE_SHELL
#endif
  {"dbg",  luaopen_dbg},               // 调试库
#endif
#if defined(LUA_COMPAT_BITLIB)
  {LUA_BITLIBNAME, luaopen_bit32},    // 不太可能启用
#endif
// 往下是LuatOS定制的库, 如需精简请仔细测试
//----------------------------------------------------------------------
// 核心支撑库, 不可禁用!!
  {"rtos",    luaopen_rtos},              // rtos底层库, 核心功能是队列和定时器
  {"log",     luaopen_log},               // 日志库
  {"timer",   luaopen_timer},             // 延时库
//-----------------------------------------------------------------------
// 设备驱动类, 可按实际情况删减. 即使最精简的固件, 也强烈建议保留uart库
#ifdef LUAT_USE_UART
  {"uart",    luaopen_uart},              // 串口操作
#endif
#ifdef LUAT_USE_GPIO
  {"gpio",    luaopen_gpio},              // GPIO脚的操作
#endif
#ifdef LUAT_USE_I2C
  {"i2c",     luaopen_i2c},               // I2C操作
#endif
#ifdef LUAT_USE_SPI
  {"spi",     luaopen_spi},               // SPI操作
#endif
#ifdef LUAT_USE_ADC
  {"adc",     luaopen_adc},               // ADC模块
#endif
// #ifdef LUAT_USE_SDIO
//   {"sdio",     luaopen_sdio},             // SDIO模块
// #endif
#ifdef LUAT_USE_PWM
  {"pwm",     luaopen_pwm},               // PWM模块
#endif
#ifdef LUAT_USE_WDT
  {"wdt",     luaopen_wdt},               // watchdog模块
#endif
#ifdef LUAT_USE_PM
  {"pm",      luaopen_pm},                // 电源管理模块
#endif
#ifdef LUAT_USE_MCU
  {"mcu",     luaopen_mcu},               // MCU特有的一些操作
#endif
// #ifdef LUAT_USE_HWTIMER
//   {"hwtimer", luaopen_hwtimer},           // 硬件定时器
// #endif
#ifdef LUAT_USE_RTC
  {"rtc", luaopen_rtc},                   // 实时时钟
#endif
  {"pin", luaopen_pin},                   // pin
#ifdef LUAT_USE_DAC
  {"dac", luaopen_dac},
#endif
#ifdef LUAT_USE_KEYBOARD
  {"keyboard", luaopen_keyboard},
#endif
#ifdef LUAT_USE_OTP
  {"otp", luaopen_otp},
#endif
#ifdef LUAT_USE_CAMERA
  {"camera", luaopen_camera},
#endif
//-----------------------------------------------------------------------
// 工具库, 按需选用
#ifdef LUAT_USE_CRYPTO
  {"crypto",luaopen_crypto},            // 加密和hash模块
#endif
#ifdef LUAT_USE_CJSON
  {"json",    luaopen_cjson},          // json的序列化和反序列化
#endif
#ifdef LUAT_USE_ZBUFF
  {"zbuff",   luaopen_zbuff},             // 像C语言语言操作内存块
#endif
#ifdef LUAT_USE_PACK
  {"pack",    luaopen_pack},              // pack.pack/pack.unpack
#endif
  // {"mqttcore",luaopen_mqttcore},          // MQTT 协议封装
  // {"libcoap", luaopen_libcoap},           // 处理COAP消息
#ifdef LUAT_USE_FATFS
  {"fatfs", luaopen_fatfs},
#endif
#ifdef LUAT_USE_LIBGNSS
  {"libgnss", luaopen_libgnss},           // 处理GNSS定位数据
#endif
#ifdef LUAT_USE_FS
  {"fs",      luaopen_fs},                // 文件系统库,在io库之外再提供一些方法
#endif
#ifdef LUAT_USE_SENSOR
  {"sensor",  luaopen_sensor},            // 传感器库,支持DS18B20
#endif
#ifdef LUAT_USE_SFUD
  {"sfud", luaopen_sfud},              // sfud
#endif
#ifdef LUAT_USE_SFD
  {"sfd",  luaopen_sfd},                
#endif
#ifdef LUAT_USE_DISP
  {"disp",  luaopen_disp},              // OLED显示模块,支持SSD1306
#endif
#ifdef LUAT_USE_U8G2
  {"u8g2", luaopen_u8g2},              // u8g2
#endif

#ifdef LUAT_USE_EINK
  {"eink",  luaopen_eink},              // 电子墨水屏,试验阶段
#endif

#ifdef LUAT_USE_LVGL
#ifndef LUAT_USE_LCD
#define LUAT_USE_LCD
#endif
  {"lvgl",   luaopen_lvgl},
#endif

#ifdef LUAT_USE_LCD
  {"lcd",    luaopen_lcd},
#endif
#ifdef LUAT_USE_STATEM
  {"statem",    luaopen_statem},
#endif
#ifdef LUAT_USE_GTFONT
  {"gtfont",    luaopen_gtfont},
#endif
#ifdef LUAT_USE_COREMARK
  {"coremark", luaopen_coremark},
#endif
#ifdef LUAT_USE_FDB
  {"fdb", luaopen_fdb},
#endif
#ifdef LUAT_USE_ZLIB
  {"zlib", luaopen_zlib},
#endif
#ifdef LUAT_USE_VMX   
  {"vmx",       luaopen_vmx}, 
#endif 
#ifdef LUAT_USE_NES   
  {"nes", luaopen_nes}, 
#endif
#ifdef LUAT_USE_SOFTKB
  {"softkb", luaopen_softkb}, 
#endif
#ifdef LUAT_USE_YMODEM
  {"ymodem", luaopen_ymodem},
#endif
#ifdef LUAT_USE_W5500
  {"w5500", luaopen_w5500},
  {"network", luaopen_network_adapter},
#endif
// #ifdef LUAT_USE_FOTA
  {"fota", luaopen_fota},
// #endif
#ifdef LUAT_USE_LORA
  {"lora", luaopen_lora},
#endif
  {"usbapp", luaopen_usbapp},
  {"audio", luaopen_multimedia_audio},
  {"codec", luaopen_multimedia_codec},
  {"ioqueue", luaopen_io_queue},
  {NULL, NULL}
};


void luat_lvgl_tick_sleep(uint8_t OnOff);
// 按不同的rtconfig加载不同的库函数
void luat_openlibs(lua_State *L) {
    // 初始化队列服务
    luat_msgbus_init();
    //print_list_mem("done>luat_msgbus_init");
    // 加载系统库
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }
}

void luat_os_reboot(int code) {
	SystemReset();
}

const char* luat_os_bsp(void) {
    return "AIR105";
}

void vConfigureTimerForRunTimeStats( void ) {}

/** 设备进入待机模式 */
void luat_os_standy(int timeout) {
    return; // nop
}

void luat_ota_reboot(int timeout_ms) {
	luat_lvgl_tick_sleep(1);
  if (timeout_ms > 0)
    luat_timer_mdelay(timeout_ms);
  SystemReset();
}

#ifdef LUAT_USE_LVGL
#include "app_interface.h"
#define LVGL_TICK_PERIOD	10
unsigned int gLVFlashTime;
static timer_t *lv_timer;
static uint32_t lvgl_tick_cnt;
static int luat_lvg_handler(lua_State* L, void* ptr) {
//	DBG("%u", lv_tick_get());
	if (lvgl_tick_cnt) lvgl_tick_cnt--;
    lv_task_handler();
    return 0;
}
static int32_t _lvgl_handler(void *pData, void *pParam) {
	if (lvgl_tick_cnt < 10)
	{
		lvgl_tick_cnt++;
	    rtos_msg_t msg = {0};
	    msg.handler = luat_lvg_handler;
	    luat_msgbus_put(&msg, 0);
	}
	return 0;
}
void luat_lvgl_tick_sleep(uint8_t OnOff)
{
	if (!OnOff)
	{
		Timer_StartMS(lv_timer, LVGL_TICK_PERIOD, 1);
	}
	else
	{
		Timer_Stop(lv_timer);
	}
}
#else
void luat_lvgl_tick_sleep(uint8_t OnOff)
{

}
#endif

void luat_shell_poweron(int _drv);

void luat_base_init(void)
{
	luat_vm_pool_init();

#ifdef LUAT_USE_SHELL
  luat_shell_poweron(0);
#endif

#ifdef LUAT_USE_LVGL
  gLVFlashTime = 33;
	lv_init();
	lv_timer = Timer_Create(_lvgl_handler, NULL, NULL);
#ifdef __LVGL_SLEEP_ENABLE__
    luat_lvgl_tick_sleep(1);
#else
	Timer_StartMS(lv_timer, LVGL_TICK_PERIOD, 1);
#endif
#endif
}

time_t luat_time(time_t *_Time) {
  if (_Time != NULL) {
    *_Time = RTC_GetUTC();
  }
  return RTC_GetUTC();
}


static uint32_t cri;
static uint8_t cri_flag;
static uint8_t disable_all;	//如果设置成1，将关闭总中断，非常危险，谨慎使用

void luat_os_set_cri_all(uint8_t onoff)
{
	disable_all = onoff;
}

//进入临界保护，不可重入
void luat_os_entry_cri(void) {
	if (disable_all) {
		__disable_irq();
		return;
	}
	if (!cri_flag) {
		cri = OS_EnterCritical();
		cri_flag = 1;
	}
}

//退出临界保护，不可重入
void luat_os_exit_cri(void) {
	if (disable_all) {
		__enable_irq();
		return;
	}
	if (cri_flag) {
		cri_flag = 0;
		OS_ExitCritical(cri);
	}
}
