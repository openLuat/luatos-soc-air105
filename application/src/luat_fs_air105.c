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

#include "app_interface.h"
#include "luat_base.h"
#include "luat_fs.h"
#include "luat_ota.h"


struct lfs_config mcu_flash_lfs_cfg;
struct lfs LFS;

#define LFS_BLOCK_DEVICE_READ_SIZE      (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (__FLASH_PAGE_SIZE__)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (16)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (256)
// 根据头文件的定义, 算出脚本区和文件系统区的绝对地址
const size_t script_luadb_start_addr = (__FLASH_BASE_ADDR__ + __FLASH_MAX_SIZE__ - FLASH_FS_REGION_SIZE * 1024);
const size_t lfs_fs_start_addr = (__FLASH_BASE_ADDR__ + __FLASH_MAX_SIZE__ - LUAT_FS_SIZE * 1024);
static HANDLE lfs_locker;
static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t start_address = block * __FLASH_SECTOR_SIZE__ + off + lfs_fs_start_addr;
	memcpy(buffer, start_address, size);
//	DBG("%x, %u", start_address, size);
//	DBG_HexPrintf(buffer, 16);
	return LFS_ERR_OK;
}
static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
	int result;
	uint32_t start_address = block * __FLASH_SECTOR_SIZE__ + off + lfs_fs_start_addr;
//	DBG("%x", start_address);
	OS_MutexLock(lfs_locker);
	if (size & 0x03)
	{
		uint32_t len = (size & 0xfc) + 4;
		DBG("%u not align to 4, change to ", size, len);
		uint8_t *program_cache = malloc(len);
		memset(program_cache, 0xff, len);
		memcpy(program_cache, buffer, size);
		result = Flash_Program(start_address, program_cache, size);
		free(program_cache);
	}
	else
	{
		result = Flash_Program(start_address, buffer, size);
	}
	OS_MutexRelease(lfs_locker);
	return result?LFS_ERR_IO:LFS_ERR_OK;
}

static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	uint32_t start_address = block * __FLASH_SECTOR_SIZE__ + lfs_fs_start_addr;
//	DBG("%x", start_address);
	OS_MutexLock(lfs_locker);
	if (Flash_Erase(start_address, __FLASH_SECTOR_SIZE__))
	{
		OS_MutexRelease(lfs_locker);
		return LFS_ERR_IO;
	}
	OS_MutexRelease(lfs_locker);
	return LFS_ERR_OK;
}

static int block_device_sync(const struct lfs_config *cfg)
{
	//DBG_Trace("block_device_sync");
    return 0;
}

#define LFS_BLOCK_DEVICE_READ_SIZE      (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (__FLASH_PAGE_SIZE__)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (16)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (256)

#ifdef __BUILD_APP__
//__attribute__ ((aligned (8))) static char lfs_cache_buff[LFS_BLOCK_DEVICE_CACHE_SIZE];
__attribute__ ((aligned (8))) static char lfs_read_buff[LFS_BLOCK_DEVICE_READ_SIZE];
__attribute__ ((aligned (8))) static char lfs_prog_buff[LFS_BLOCK_DEVICE_PROG_SIZE];
__attribute__ ((aligned (8))) static char lfs_lookahead_buff[16];
#endif


void FileSystem_Init(void)
{
	struct lfs_config *config = &mcu_flash_lfs_cfg;
	//DBG_INFO("ID:%02x,%02x,%02x,Size:%uKB", Ctrl->FlashID[0], Ctrl->FlashID[1], Ctrl->FlashID[2], Ctrl->Size);
	config->context = NULL;
	config->cache_size = LFS_BLOCK_DEVICE_READ_SIZE;
	config->prog_size = __FLASH_PAGE_SIZE__;
	config->block_size = __FLASH_SECTOR_SIZE__;
	config->read_size = LFS_BLOCK_DEVICE_READ_SIZE;
	config->block_cycles = 200;
	config->lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD;
	//config->block_count = (Ctrl->Size / 4) - __CORE_FLASH_SECTOR_NUM__ - __SCRIPT_FLASH_SECTOR_NUM__;
	config->block_count = 512 / 4 ;
	config->name_max = 63;
	config->read = block_device_read;
	config->prog = block_device_prog;
	config->erase = block_device_erase;
	config->sync  = block_device_sync;

	//config->buffer = (void*)lfs_cache_buff;
	config->read_buffer = (void*)lfs_read_buff;
	config->prog_buffer = (void*)lfs_prog_buff;
	config->lookahead_buffer = (void*)lfs_lookahead_buff;

	if (!lfs_locker)
	{
		lfs_locker = OS_MutexCreateUnlock();
	}
/*
 * 正式加入luatos代码可以开启
 */
	int re = lfs_mount(&LFS, &mcu_flash_lfs_cfg);
	if (re) {
		DBG_INFO("lfs, mount fail=%d", re);
		re = lfs_format(&LFS, &mcu_flash_lfs_cfg);
		if (re) {
			DBG_INFO("lfs, format fail=%d", re);
		}
		else {
			lfs_mount(&LFS, &mcu_flash_lfs_cfg);
		}
	}
}

extern const struct luat_vfs_filesystem vfs_fs_lfs2;
extern const struct luat_vfs_filesystem vfs_fs_luadb;
extern const struct luat_vfs_filesystem vfs_fs_ram;
extern size_t luat_luadb_act_size;

#ifdef LUAT_USE_VFS_INLINE_LIB
//extern const char luadb_inline[];
// extern const char luadb_inline_sys[];
#endif

int luat_fs_init(void) {
	FileSystem_Init();
	#ifdef LUAT_USE_FS_VFS
	// vfs进行必要的初始化
	luat_vfs_init(NULL);

	// 注册vfs for posix 实现
	luat_vfs_reg(&vfs_fs_lfs2);
	luat_vfs_reg(&vfs_fs_luadb);
	// 指向3M + 512k的littefs 文件区
	luat_fs_conf_t conf = {
		.busname = (char*)&LFS,
		.type = "lfs2",
		.filesystem = "lfs2",
		.mount_point = "",
	};
	luat_fs_mount(&conf);

    luat_vfs_reg(&vfs_fs_ram);
    luat_fs_conf_t conf3 = {
		.busname = NULL,
		.type = "ram",
		.filesystem = "ram",
		.mount_point = "/ram/"
	};
	luat_fs_mount(&conf3);

	// 指向3M 的脚本区, luadb格式
	luat_fs_conf_t conf2 = {
		.busname = (char*)script_luadb_start_addr,
		.type = "luadb",
		.filesystem = "luadb",
		.mount_point = "/luadb/",
	};
	luat_luadb_act_size = LUAT_SCRIPT_SIZE;
	#ifdef LUAT_USE_FSKV
	luat_luadb_act_size -= 64 * 1024;
	#endif
	luat_fs_mount(&conf2);
	// #endif
	#endif

	#ifdef LUAT_USE_LVGL
	luat_lv_fs_init();
	//lv_bmp_init();
	lv_png_init();
	lv_split_jpeg_init();
	#endif
	return 0;
}
