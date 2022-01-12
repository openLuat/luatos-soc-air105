#include "luat_base.h"
#include "luat_mcu.h"

#include "app_interface.h"

#include "FreeRTOS.h"

int luat_mcu_set_clk(size_t mhz) {
    return 0;
}

int luat_mcu_get_clk(void) {
    return 0;
}

static uint8_t unique_id[16] = {0};
const char* luat_mcu_unique_id(size_t* t) {
    OTP_GetSn(unique_id);
    *t = sizeof(unique_id);
    return unique_id;
}

long luat_mcu_ticks(void) {
    return GetSysTickMS();
}
