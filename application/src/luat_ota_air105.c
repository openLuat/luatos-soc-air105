
#include "luat_base.h"
#include "luat_ota.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include "luat_malloc.h"
#include "luat_flash.h"

extern const size_t script_luadb_start_addr;

int luat_ota_exec(void) {
    return luat_ota(script_luadb_start_addr);
}
