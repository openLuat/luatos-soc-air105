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


// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"

#define LUAT_LOG_TAG "heap"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "app_interface.h"

//------------------------------------------------
//  管理系统内存

void* luat_heap_malloc(size_t len) {
    return OS_Malloc(len);
}

void luat_heap_free(void* ptr) {
    OS_Free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    void* tmp = luat_heap_malloc(len);
    if (tmp && ptr) {
        memcpy(tmp, ptr, len);
    }
    return tmp;
}

void* luat_heap_calloc(size_t count, size_t _size) {
    void *ptr = luat_heap_malloc(count * _size);
    if (ptr) {
        memset(ptr, 0, _size);
    }
    return ptr;
}
//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------
void* luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
//    if (0) {
//        if (ptr) {
//            if (nsize) {
//                // 缩放内存块
//                LLOGD("realloc %p from %d to %d", ptr, osize, nsize);
//            }
//            else {
//                // 释放内存块
//                LLOGD("free %p ", ptr);
//                OS_Free(ptr);
//                return NULL;
//            }
//        }
//        else {
//            // 申请内存块
//            ptr = OS_Malloc(nsize);
//            LLOGD("malloc %p type=%d size=%d", ptr, osize, nsize);
//            return ptr;
//        }
//    }
    if (nsize)
    {
    	void* ptmp = pvPortMalloc(nsize);
    	if (ptmp)
    	{
    		if (osize > nsize)
    		{
    			memcpy(ptmp, ptr, nsize);
    		}
    		else
    		{
    			memcpy(ptmp, ptr, osize);
    		}
    		if (((uint32_t)ptr & __SRAM_BASE_ADDR__) == __SRAM_BASE_ADDR__)
    		{
    			vPortFree(ptr);
    		}
    		return ptmp;
    	}
    	else if (osize >= nsize)
    	{
    		return ptr;
    	}
    }
	if (((uint32_t)ptr & __SRAM_BASE_ADDR__) == __SRAM_BASE_ADDR__)
	{
		vPortFree(ptr);
	}
    return NULL;
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
	*used = configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize();
	*max_used = configTOTAL_HEAP_SIZE - xPortGetMinimumEverFreeHeapSize();
    *total = configTOTAL_HEAP_SIZE;
}

void luat_meminfo_sys(size_t *total, size_t *used, size_t *max_used) {
	long curalloc, totfree, maxfree;
	unsigned long nget, nrel;
	bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
	*used = curalloc;
	*max_used = bstatsmaxget();
    *total = curalloc + totfree;
}

//-----------------------------------------------------------------------------
