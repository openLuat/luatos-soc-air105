
#include "luat_base.h"
#include "luat_wdt.h"
#include "app_interface.h"
extern uint8_t gMainWDTEnable;
int luat_wdt_init(size_t timeout) {
	gMainWDTEnable = 0;
    return 0;
}

int luat_wdt_set_timeout(size_t timeout) {
	WDT_SetTimeout(timeout);
    return 0;
}

int luat_wdt_feed(void) {
	WDT_Feed();
    return 0;
}

int luat_wdt_close(void) {
	gMainWDTEnable = 1;
    return 0;
}
