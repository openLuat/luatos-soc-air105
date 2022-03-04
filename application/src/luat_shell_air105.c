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
#include "luat_shell.h"

#include "luat_uart.h"
#include "app_interface.h"


#define LUAT_LOG_TAG "luat.shell"
#include "luat_log.h"
enum
{
	SHELL_READ_DATA = USER_EVENT_ID_START + 1,
};
static HANDLE prvhShell;
void luat_shell_write(char* buff, size_t len) {
    DBG_DirectOut(buff, len);
}

void luat_shell_notify_recv(void) {
}


static int32_t luat_shell_uart_cb(void *pData, void *pParam){
    Task_SendEvent(prvhShell, SHELL_READ_DATA, 0, 0, 0);
    return -1;
}

static void luat_shell(void *sdata)
{
	OS_EVENT Event;
	char buff[512];
	int ret = 0;
	int len = 1;
	while (1) {
		// printf("tls_os_queue_receive \r");
		Task_GetEvent(prvhShell, CORE_EVENT_ID_ANY, &Event, NULL, 0);
		len = 1;
		while (len > 0 && len < 512) {
			memset(buff, 0, 512);
            len = Uart_RxBufferRead(0, buff, 512);
            if (len > 0 && len < 512){
                buff[len] = 0x00; // 确保结尾
                luat_shell_push(buff, len);
            }
		}
		// printf("shell loop end\r");
	}
}

void luat_shell_poweron(int _drv) {
	DBG_SetRxCB(luat_shell_uart_cb);
	if (!prvhShell)
	{
		prvhShell = Task_Create(luat_shell,
                NULL, 
                2048,
                1,
                "luat_shell");
	}

}
