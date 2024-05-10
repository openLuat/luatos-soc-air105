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
#include "luat_bget.h"
#define LUAT_LOG_TAG "heap"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "app_interface.h"

#ifndef	LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE		200
#endif

#if (LUAT_HEAP_SIZE > 400) || (LUAT_HEAP_SIZE < 100)
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE 		200
#endif

static luat_bget_t luavm_pool;
static uint64_t luavm_pool_data[LUAT_HEAP_SIZE / 8 * 1024];
//------------------------------------------------
//  管理系统内存

void* luat_heap_malloc(size_t len) {
    return malloc(len);
}

void luat_heap_free(void* ptr) {
    free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    return calloc(count,_size);
}

void* luat_heap_zalloc(size_t _size) {
    return zalloc(_size);
}

//------------------------------------------------
void luat_vm_pool_init(void)
{
	luat_bget_init(&luavm_pool);
	luat_bpool(&luavm_pool, luavm_pool_data, sizeof(luavm_pool_data));
}
//------------------------------------------------

void *luat_vm_malloc(size_t nsize)
{
	return luat_bget(&luavm_pool, nsize);
}

void luat_vm_free(void *ptr)
{
	return luat_brel(&luavm_pool, ptr);
}
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
//                free(ptr);
//                return NULL;
//            }
//        }
//        else {
//            // 申请内存块
//            ptr = malloc(nsize);
//            LLOGD("malloc %p type=%d size=%d", ptr, osize, nsize);
//            return ptr;
//        }
//    }
    if (nsize)
    {
    	void* ptmp = luat_bget(&luavm_pool, nsize);
    	if (ptmp)
    	{
    		if (ptr)
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
        			luat_brel(&luavm_pool, ptr);
        		}
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
		luat_brel(&luavm_pool, ptr);
	}
    return NULL;
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
	long curalloc, totfree, maxfree;
	unsigned long nget, nrel;
	luat_bstats(&luavm_pool, &curalloc, &totfree, &maxfree, &nget, &nrel);
	*used = curalloc;
	*max_used = luat_bstatsmaxget(&luavm_pool);
    *total = curalloc + totfree;
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
