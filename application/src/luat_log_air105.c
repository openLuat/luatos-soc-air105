#include "luat_base.h"
#include "luat_log.h"
#include "luat_cmux.h"
#include "luat_conf_bsp.h"
#include "app_interface.h"

extern uint8_t cmux_state;
extern uint8_t cmux_log_state;

void DBG_DirectOut(void *Data, uint32_t Len);

void luat_nprint(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_state == 1 && cmux_log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    DBG_DirectOut(s, l);
}

void luat_log_write(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_state == 1 && cmux_log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    DBG_DirectOut(s, l);
}

static void luat_cmux_write_cb(uint8_t *data, uint32_t len){
    luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,data, len);
}

void luat_cmux_log_set(uint8_t state) {
    if (state == 1){
        DBG_SetTxCB(luat_cmux_write_cb);
    }else if(state == 0){
        DBG_SetTxCB(add_printf_data);
    }
    
}