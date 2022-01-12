#include "app_interface.h"
#include "luat_base.h"

#define LUAT_LOG_TAG "pin"
#include "luat_log.h"

#include "luat_pin.h"

int luat_pin_to_gpio(const char* pin_name) {
    int zone = 0;
    int index = 0;
    int re = 0;
    re = luat_pin_parse(pin_name, &zone, &index);
    if (re < 0) {
        return -1;
    }
    // PA00 ~ PF15
    if (zone < 6 && index < 16) {
        return zone*16 + index;
    }
    return -1;
}
