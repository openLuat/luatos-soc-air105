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
