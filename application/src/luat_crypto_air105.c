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
#include "luat_crypto.h"

#include "app_interface.h"

#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"
//#include "mbedtls/config.h"
#include "mbedtls/cipher.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md5.h"

int luat_crypto_trng(char* buff, size_t len) {
    size_t t = 0;
    uint32_t tmp[4];
    int i;
    char *temp = tmp;
    while (len > t) {
        RNG_GetData(tmp);
        if ((len - t) >=16)
        {
        	memcpy(buff + t, temp, 16);
        	t += 16;
        }
        else
        {
        	i = 0;
        	while (len > t)
        	{
        		buff[t] = temp[i];
        		t++;
        		i++;
        	}
        }
    }
    return 0;
}

struct tm *mbedtls_platform_gmtime_r( const mbedtls_time_t *tt,
                                      struct tm *tm_buf )
{
	Date_UserDataStruct Date;
	Time_UserDataStruct Time;
	Tamp2UTC(*tt, &Date, &Time, 0);
	tm_buf->tm_year = Date.Year - 1900;
	tm_buf->tm_mon = Date.Mon - 1;
	tm_buf->tm_mday = Date.Day;
	tm_buf->tm_hour = Time.Hour;
	tm_buf->tm_min = Time.Min;
	tm_buf->tm_sec = Time.Sec;
	return tm_buf;

}
