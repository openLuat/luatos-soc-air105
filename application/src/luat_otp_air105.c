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
