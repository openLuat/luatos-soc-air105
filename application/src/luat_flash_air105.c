
#include "app_interface.h"
#include "luat_base.h"
#include "luat_flash.h"

#include "luat_log.h"
#include "luat_timer.h"
#include "stdio.h"
#include "luat_ota.h"

#include "core_flash.h"


int luat_flash_read(char* buff, size_t addr, size_t len) {
    if (addr == 0)
        return 0;
    memcpy((uint8_t*)addr, buff, len);
    return len;
}

int luat_flash_write(char* buff, size_t addr, size_t len) {
	return Flash_Program( addr, buff, len);
}

int luat_flash_erase(size_t addr, size_t len) {
    Flash_Erase(addr, len);
    return 0;
}
