#include "luat_base.h"
#include "luat_pm.h"
#include "user.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

int luat_pm_request(int mode) {
    if (mode != LUAT_PM_SLEEP_MODE_LIGHT || mode == LUAT_PM_SLEEP_MODE_DEEP) {
        LLOGW("only pm.LIGHT/pm.DEEP supported");
        return -1;
    }
    // 如果是深度睡眠, 关闭GPIO
    if (mode == LUAT_PM_SLEEP_MODE_DEEP) {
	    for(int i = GPIOA_02; i < GPIO_NONE; i++)
	    {
		    GPIO_PullConfig(i, 0, 0);
		    GPIO_Config(i, 0, 0);
	    }
    }
    PM_SetDriverRunFlag(PM_DRV_DBG, 0);
    return 0;
}

int luat_pm_release(int mode) {
    return 0;
}

int luat_pm_dtimer_start(int id, size_t timeout) {
    if (id != 0)
        return -1;
    RTC_SetAlarm(timeout / 1000, NULL, NULL); // 无需回调, 或者改成回调里重启, 也不太对的样
    return 0;
}

int luat_pm_dtimer_stop(int id) {
    if (id != 0)
        return -1;
    RTC_SetAlarm(0, NULL, NULL); // 设置为0就是关闭
    return 0;
}

int luat_pm_dtimer_check(int id) {
    return -1;
}

// void luat_pm_cb(int event, int arg, void* args);

int luat_pm_last_state(int *lastState, int *rtcOrPad) {
    return 0;
}

int luat_pm_force(int mode) {
    return luat_pm_request(mode);
}

int luat_pm_check(void) {
    return 0;
}

int luat_pm_dtimer_list(size_t* count, size_t* list) {
    *count = 0;
    return 0;
}

int luat_pm_dtimer_wakeup_id(int* id) {
    return -1;
}

