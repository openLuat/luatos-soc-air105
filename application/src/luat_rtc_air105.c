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
#include "luat_rtc.h"
#include "app_interface.h"

int luat_rtc_set(struct tm *tblock) {
	Date_UserDataStruct Date;
	Time_UserDataStruct Time;

    Time.Sec = tblock->tm_sec;
    Time.Min = tblock->tm_min;
    Time.Hour = tblock->tm_hour;
    //tblock->tm_wday = uTime.Time.Week;

    Date.Year = tblock->tm_year;
    Date.Mon = tblock->tm_mon;
    Date.Day = tblock->tm_mday;

    RTC_SetDateTime(&Date, &Time, 1);
    return 0;
}

void luat_rtc_set_tamp32(uint32_t tamp) {
	RTC_SetUTC(tamp, 1);
}

int luat_rtc_get(struct tm *tblock) {
	Date_UserDataStruct Date;
	Time_UserDataStruct Time;
    
    RTC_GetDateTime(&Date, &Time);
    
    tblock->tm_sec = Time.Sec;
    tblock->tm_min = Time.Min;
    tblock->tm_hour = Time.Hour;
    tblock->tm_wday = Time.Week;

    tblock->tm_year = Date.Year;
    tblock->tm_mon = Date.Mon;
    tblock->tm_mday = Date.Day;
    return 0;
}

int luat_rtc_timer_start(int id, struct tm *tblock) {
    return -1; // 暂不支持
}

int luat_rtc_timer_stop(int id) {
    return -1; // 暂不支持
}

uint32_t luat_get_utc(uint32_t *tamp)
{
	uint32_t sec = RTC_GetUTC();
	if (tamp)
	{
		*tamp = sec;
	}
	return sec;
}
