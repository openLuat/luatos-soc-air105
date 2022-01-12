#include "luat_base.h"
#include "luat_otp.h"

#include "app_interface.h"

int luat_otp_read(int zone, char* buff, size_t offset, size_t len) {
    if (zone < 0 || zone > 2)
        return -1;
    int addr = zone * 1024 + offset;
    OTP_Read((uint32_t)addr, (const uint32_t *)buff, len / 4);
    return len;
}

int luat_otp_write(int zone, char* buff, size_t offset, size_t len) {
    if (zone < 0 || zone > 2)
        return -1;
    int addr = zone * 1024 + offset;
    OTP_Write(addr, (const uint32_t *)buff, len / 4);
    return 0;
}

int luat_otp_erase(int zone, size_t offset, size_t len) {
    return 0; // 无需主动擦除
}

int luat_otp_lock(int zone) {
    OTP_Lock();
    return 0;
}
