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
#include "luat_spi.h"
#include "luat_lcd.h"
#include "luat_timer.h"
#include "luat_malloc.h"
#include "stdlib.h"
#include "string.h"
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "app_interface.h"

#define LUAT_LOG_TAG "luat.spi"
#include "luat_log.h"

typedef struct
{
	uint8_t id;
//	uint8_t mark;
    uint8_t mode;//spi模式
}Spi_Struct;

static Spi_Struct luat_spi[6] ={0};

int32_t luat_spi_cb(void *pData, void *pParam){
//    LLOGD("luat_spi_cb pData:%d pParam:%d ",(int)pData,(int)pParam);
//    switch ((int)pData){
//        case 0:
//            luat_spi[5].mark = 0;
//            break;
//        case 1:
//            luat_spi[0].mark = 0;
//            break;
//        case 2:
//            luat_spi[1].mark = 0;
//            break;
//        case 3:
//            luat_spi[2].mark = 0;
//            break;
//        default:
//            break;
//    }
}

int luat_spi_device_config(luat_spi_device_t* spi_dev) {
    uint8_t spi_mode = SPI_MODE_0;
    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
    SPI_SetNewConfig(luat_spi[spi_dev->bus_id].id, spi_dev->spi_config.bandrate, spi_mode);
    return 0;
}

int luat_spi_bus_setup(luat_spi_device_t* spi_dev){
    int bus_id = spi_dev->bus_id;
    if (bus_id == 0) {
        luat_spi[bus_id].id=SPI_ID0;
        GPIO_Iomux(GPIOB_12, 0);
        GPIO_Iomux(GPIOB_14, 0);
        GPIO_Iomux(GPIOB_15, 0);
    }
    else if (bus_id == 1) {
        luat_spi[bus_id].id=SPI_ID1;
	    GPIO_Iomux(GPIOA_06,3);
	    GPIO_Iomux(GPIOA_08,3);
	    GPIO_Iomux(GPIOA_09,3);
    }
    else if (bus_id == 2) {
        luat_spi[bus_id].id=SPI_ID2;
	    GPIO_Iomux(GPIOB_02,0);
	    GPIO_Iomux(GPIOB_04,0);
	    GPIO_Iomux(GPIOB_05,0);
    }
    else if (bus_id == 5) {
        luat_spi[bus_id].id=HSPI_ID0;
	    GPIO_Iomux(GPIOC_12,3);
	    GPIO_Iomux(GPIOC_13,3);
	    GPIO_Iomux(GPIOC_15,3);
    }
    else {
        return -1;
    }
    uint8_t spi_mode = SPI_MODE_0;
    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
    luat_spi[bus_id].mode=spi_dev->spi_config.mode;
    // LLOGD("SPI_MasterInit luat_bus_%d:%d dataw:%d spi_mode:%d bandrate:%d ",bus_id,luat_spi[bus_id].id, spi_dev->spi_config.dataw, spi_mode, spi_dev->spi_config.bandrate);
    SPI_MasterInit(luat_spi[bus_id].id, spi_dev->spi_config.dataw, spi_mode, spi_dev->spi_config.bandrate, luat_spi_cb, NULL);
    return 0;
}

//初始化配置SPI各项参数，并打开SPI
//成功返回0
int luat_spi_setup(luat_spi_t* spi) {
    uint8_t spi_id = spi->id;
    if (spi_id == 0) {
        luat_spi[spi_id].id=SPI_ID0;
        if (spi->cs == 0 || spi->cs == GPIOB_13)
            GPIO_Iomux(GPIOB_13, 0);
        GPIO_Iomux(GPIOB_12, 0);
        GPIO_Iomux(GPIOB_14, 0);
        GPIO_Iomux(GPIOB_15, 0);
    }
    else if (spi_id == 1) {
        luat_spi[spi_id].id=SPI_ID1;
        if (spi->cs == 0 || spi->cs == GPIOA_07)
	        GPIO_Iomux(GPIOA_07,3);
	    GPIO_Iomux(GPIOA_06,3);
	    GPIO_Iomux(GPIOA_08,3);
	    GPIO_Iomux(GPIOA_09,3);
    }
    else if (spi_id == 2) {
        luat_spi[spi_id].id=SPI_ID2;
        if (spi->cs == 0 || spi->cs == GPIOB_03)
	        GPIO_Iomux(GPIOB_03,0);
	    GPIO_Iomux(GPIOB_02,0);
	    GPIO_Iomux(GPIOB_04,0);
	    GPIO_Iomux(GPIOB_05,0);
    }
    else if (spi_id == 5) {
        luat_spi[spi_id].id=HSPI_ID0;
        if (spi->cs == 0 || spi->cs == GPIOC_14)
	        GPIO_Iomux(GPIOC_14,3);
	    GPIO_Iomux(GPIOC_12,3);
	    GPIO_Iomux(GPIOC_13,3);
	    GPIO_Iomux(GPIOC_15,3);
    }
    else {
        return -1;
    }
    uint8_t spi_mode = SPI_MODE_0;
    if(spi->CPHA&&spi->CPOL)spi_mode = SPI_MODE_3;
    else if(spi->CPOL)spi_mode = SPI_MODE_2;
    else if(spi->CPHA)spi_mode = SPI_MODE_1;
    luat_spi[spi_id].mode=spi->mode;
//    LLOGD("SPI_MasterInit luat_spi%d:%d dataw:%d spi_mode:%d bandrate:%d ",spi_id,luat_spi[spi_id].id, spi->dataw, spi_mode, spi->bandrate);
    SPI_MasterInit(luat_spi[spi_id].id, spi->dataw, spi_mode, spi->bandrate, luat_spi_cb, NULL);
    return 0;
}

int luat_spi_config_dma(int spi_id, uint32_t tx_channel, uint32_t rx_channel)
{
//	if (luat_spi[spi_id].id != HSPI_ID0) {
//		SPI_DMATxInit(luat_spi[spi_id].id, (tx_channel >= 8)?ETH_SPI_TX_DMA_STREAM:tx_channel, 0);
//		SPI_DMARxInit(luat_spi[spi_id].id, (rx_channel >= 8)?ETH_SPI_RX_DMA_STREAM:rx_channel, 0);
//	}
//	else
//	{
//		SPI_DMATxInit(luat_spi[spi_id].id, (tx_channel >= 8)?LCD_SPI_TX_DMA_STREAM:tx_channel, 0);
//		SPI_DMARxInit(luat_spi[spi_id].id, (rx_channel >= 8)?LCD_SPI_RX_DMA_STREAM:rx_channel, 0);
//	}
}

//关闭SPI，成功返回0
int luat_spi_close(int spi_id) {
    return 0;
}
//收发SPI数据，返回接收字节数
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length) {
    // LLOGD("SPI_MasterInit luat_spi%d:%d send_buf:%x recv_buf:%x length:%d ",spi_id,luat_spi[spi_id], *send_buf, *recv_buf, length);
    // while(luat_spi[spi_id].mark)
//    luat_spi[spi_id].mark = 1;
    int32_t result = 0;
    if(luat_spi[spi_id].mode==0)
        result = SPI_FlashBlockTransfer(luat_spi[spi_id].id, send_buf, send_length, recv_buf, recv_length);
    else
        result = SPI_BlockTransfer(luat_spi[spi_id].id, send_buf, recv_buf, recv_length);
    if(result)
        return 0;
    return recv_length;
}
//收SPI数据，返回接收字节数
int luat_spi_recv(int spi_id, char* recv_buf, size_t length) {
    // LLOGD("SPI_MasterInit luat_spi%d:%d recv_buf:%x length:%d ",spi_id,luat_spi[spi_id], *recv_buf, length);
    // while(luat_spi[spi_id].mark)
//    luat_spi[spi_id].mark = 1;
    if(SPI_BlockTransfer(luat_spi[spi_id].id, recv_buf, recv_buf, length))
        return 0;
    return length;
}
//发SPI数据，返回发送字节数
int luat_spi_send(int spi_id, const char* send_buf, size_t length) {
    // LLOGD("luat_spi_send luat_spi%d:%d send_buf:%x length:%d ",spi_id,luat_spi[spi_id], *send_buf, length);
    // while(luat_spi[spi_id].mark)
//    luat_spi[spi_id].mark = 1;
    if(SPI_BlockTransfer(luat_spi[spi_id].id, send_buf, NULL, length))
        return 0;
    return length;
}

int luat_spi_change_speed(int spi_id, uint32_t speed)
{
	SPI_SetNewConfig(luat_spi[spi_id].id, speed, 0xff);
	return 0;
}

int luat_spi_no_block_transfer(int spi_id, uint8_t *tx_buff, uint8_t *rx_buff, size_t len, void *CB, void *pParam)
{
	if (SPI_IsTransferBusy(luat_spi[spi_id].id)) return -1;
	SPI_SetCallbackFun(luat_spi[spi_id].id, CB, pParam);
	SPI_SetNoBlock(luat_spi[spi_id].id);
	return SPI_Transfer(luat_spi[spi_id].id, tx_buff, rx_buff, len, 0);

}

int luat_lcd_draw_no_block(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color, uint8_t last_flush)
{
	uint32_t retry_cnt = 0;
	uint32_t cache_len = Core_LCDDrawCacheLen();
//	if (last_flush)
//	{
//		LLOGD("lcd flush done!");
//	}
	if (x1 < 0 || y1 < 0 || x2 > conf->w || y2 > conf->h) {
		// 暂不支持屏幕外绘制
	}

	if (conf->port == LUAT_LCD_SPI_DEVICE){
		while (Core_LCDDrawCacheLen() > (conf->buffer_size))
		{
			retry_cnt++;
			luat_timer_mdelay(1);
		}
		if (retry_cnt)
		{
			LLOGD("lcd flush delay %ums, status %u,%u,%u,%d", retry_cnt, cache_len, x2-x1+1, y2-y1+1, last_flush);
		}
		LCD_DrawStruct *draw = zalloc(sizeof(LCD_DrawStruct));
		if (!draw)
		{
			LLOGE("lcd flush no memory");
			return -1;
		}
		luat_spi_device_t* spi_dev = (luat_spi_device_t*)conf->lcd_spi_device;
		int spi_id = spi_dev->bus_id;
	    uint8_t spi_mode = SPI_MODE_0;
	    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
	    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
	    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
	    draw->DCDelay = conf->dc_delay_us;
	    draw->Mode = spi_mode;
	    draw->Speed = spi_dev->spi_config.bandrate;
	    draw->SpiID = luat_spi[spi_id].id;
	    draw->CSPin = spi_dev->spi_config.cs;
	    draw->DCPin = conf->pin_dc;
	    draw->x1 = x1;
	    draw->x2 = x2;
	    draw->y1 = y1;
	    draw->y2 = y2;
	    draw->xoffset = conf->xoffset;
	    draw->yoffset = conf->yoffset;
	    draw->Size = (draw->x2 - draw->x1 + 1) * (draw->y2 - draw->y1 + 1) * 2;
	    draw->ColorMode = COLOR_MODE_RGB_565;
	    draw->Data = malloc(draw->Size);
		if (!draw->Data)
		{
			LLOGE("lcd flush data no memory");
			free(draw);
			return -1;
		}
	    memcpy(draw->Data, color, draw->Size);
	    Core_LCDDraw(draw);
	    return 0;
	}
	else
	{
		return -1;
	}
}

int luat_spi_get_hw_bus(int spi_id)
{
	return luat_spi[spi_id].id;
}

void luat_lcd_draw_block(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color, uint8_t last_flush)
{
	LCD_DrawStruct draw;
	if (conf->port == LUAT_LCD_SPI_DEVICE){
		luat_spi_device_t* spi_dev = (luat_spi_device_t*)conf->lcd_spi_device;
		int spi_id = spi_dev->bus_id;
	    uint8_t spi_mode = SPI_MODE_0;
	    if(spi_dev->spi_config.CPHA&&spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_3;
	    else if(spi_dev->spi_config.CPOL)spi_mode = SPI_MODE_2;
	    else if(spi_dev->spi_config.CPHA)spi_mode = SPI_MODE_1;
	    draw.DCDelay = conf->dc_delay_us;
	    draw.Mode = spi_mode;
	    draw.Speed = spi_dev->spi_config.bandrate;
	    draw.SpiID = luat_spi[spi_id].id;
	    draw.CSPin = spi_dev->spi_config.cs;
	    draw.DCPin = conf->pin_dc;
	    draw.x1 = x1;
	    draw.x2 = x2;
	    draw.y1 = y1;
	    draw.y2 = y2;
	    draw.xoffset = conf->xoffset;
	    draw.yoffset = conf->yoffset;
	    draw.Size = (draw.x2 - draw.x1 + 1) * (draw.y2 - draw.y1 + 1) * 2;
	    draw.ColorMode = COLOR_MODE_RGB_565;
	    draw.Data = color;
	    Core_LCDDrawBlock(&draw);
	}
}

#ifdef LUAT_USE_LCD_CUSTOM_DRAW
int luat_lcd_flush(luat_lcd_conf_t* conf) {
    if (conf->buff == NULL) {
        return 0;
    }
    //LLOGD("luat_lcd_flush range %d %d", conf->flush_y_min, conf->flush_y_max);
    if (conf->flush_y_max < conf->flush_y_min) {
        // 没有需要刷新的内容,直接跳过
        //LLOGD("luat_lcd_flush no need");
        return 0;
    }
    if (conf->is_init_done) {
    	luat_lcd_draw_no_block(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max, &conf->buff[conf->flush_y_min * conf->w], 0);
    }
    else {
        uint32_t size = conf->w * (conf->flush_y_max - conf->flush_y_min + 1) * 2;
        luat_lcd_set_address(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max);
        const char* tmp = (const char*)(conf->buff + conf->flush_y_min * conf->w);
    	if (conf->port == LUAT_LCD_SPI_DEVICE){
    		luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), tmp, size);
    	}else{
    		luat_spi_send(conf->port, tmp, size);
    	}
    }
    // 重置为不需要刷新的状态
    conf->flush_y_max = 0;
    conf->flush_y_min = conf->h;

    return 0;
}

int luat_lcd_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color) {
    if (x1 >= conf->w || y1 >= conf->h || x2 < 0 || y2 < 0 || x2 < x1 || y2 < y1) {
        // LLOGE("out of lcd buff range %d %d %d %d", x1, y1, x2, y2);
        // LLOGE("out of lcd buff range %d %d %d %d %d", x1 >= conf->w, y1 >= conf->h, y2 < 0, x2 < x1, y2 < y1);
        return 0;
    }
    if (y2 >= conf->h) {
        y2 = conf->h - 1;
    }
    // 直接刷屏模式
    if (conf->buff == NULL) {
        // 常规数据, 整体传输
        if (x1 >= 0 && y1 >= 0 && x2 <= conf->w && y2 <= conf->h) {
			if (conf->is_init_done) {
				return luat_lcd_draw_no_block(conf, x1, y1, x2, y2, color, 0);
			}
            uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
            // LLOGD("draw %dx%d %dx%d %d", x1, y1, x2, y2, size);
            luat_lcd_set_address(conf, x1, y1, x2, y2);
	        if (conf->port == LUAT_LCD_SPI_DEVICE){
		        luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)color, size* sizeof(luat_color_t));
	        }else{
		        luat_spi_send(conf->port, (const char*)color, size * sizeof(luat_color_t));
	        }
        }
        // 超出边界的数据, 按行传输
        else {
            int line_size = (x2 - x1 + 1);
            // LLOGD("want draw %dx%d %dx%d %d", x1, y1, x2, y2, line_size);
            luat_color_t* ptr = (luat_color_t*)color;
            for (int i = y1; i <= y2; i++)
            {
                if (i < 0) {
                    ptr += line_size;
                    continue;
                }
                luat_color_t* line = ptr;
                int lsize = line_size;
                int tmp_x1 = x1;
                int tmp_x2 = x2;
                if (x1 < 0) {
                    line += ( - x1);
                    lsize += (x1);
                    tmp_x1 = 0;
                }
                if (x2 > conf->w) {
                    lsize -= (conf->w - x2);
                    tmp_x2 = conf->w;
                }
				// LLOGD("action draw %dx%d %dx%d %d", tmp_x1, i, tmp_x2, i, lsize);
				if (conf->is_init_done) {
					luat_lcd_draw_no_block(conf, tmp_x1, i, tmp_x2, i, line, 0);
				}
				else {
                	luat_lcd_set_address(conf, tmp_x1, i, tmp_x2, i);
	            	if (conf->port == LUAT_LCD_SPI_DEVICE){
		            	luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)line, lsize * sizeof(luat_color_t));
	            	} else{
		            	luat_spi_send(conf->port, (const char*)line, lsize * sizeof(luat_color_t));
	            	}
				}
                ptr += line_size;
            }
        }
        return 0;
    }
    // buff模式
    luat_color_t* dst = (conf->buff);
    size_t lsize = (x2 - x1 + 1);
    for (int16_t x = x1; x <= x2; x++)
    {
        if (x < 0 || x >= conf->w)
            continue;
        for (int16_t y = y1; y <= y2; y++)
        {
            if (y < 0 || y >= conf->h)
                continue;
            memcpy((char*)(dst + (conf->w * y + x)), (char*)(color + (lsize * (y-y1) + (x-x1))), sizeof(luat_color_t));
        }
    }
    // 存储需要刷新的区域
    if (y1 < conf->flush_y_min) {
        if (y1 >= 0)
            conf->flush_y_min = y1;
        else
            conf->flush_y_min = 0;
    }
    if (y2 > conf->flush_y_max) {
        conf->flush_y_max = y2;
    }
    return 0;
}
#endif

#if 1
static void *luat_fatfs_spi_ctrl;
static HANDLE luat_fatfs_locker;
static void sdhc_spi_check(luat_fatfs_spi_t* userdata)
{
	if (!luat_fatfs_spi_ctrl)
	{
		if(userdata->type == 1){
			luat_fatfs_spi_ctrl = SDHC_SpiCreate(luat_spi[userdata->spi_device->bus_id].id, userdata->spi_device->spi_config.cs);
			luat_spi_device_config(userdata->spi_device);
		}else{
			luat_fatfs_spi_ctrl = SDHC_SpiCreate(luat_spi[userdata->spi_id].id, userdata->spi_cs);
		}
	}
}

static DSTATUS sdhc_spi_initialize(luat_fatfs_spi_t* userdata)
{
	int i;
	if (!luat_fatfs_locker)
	{
		luat_fatfs_locker = OS_MutexCreateUnlock();
	}
	OS_MutexLock(luat_fatfs_locker);
	if (luat_fatfs_spi_ctrl)
	{
		for(i = 0; i < USB_MAX; i++)
		{
			Core_UDiskDetachSDHC(i, luat_fatfs_spi_ctrl);
		}
		SDHC_SpiRelease(luat_fatfs_spi_ctrl);
		free(luat_fatfs_spi_ctrl);
		luat_fatfs_spi_ctrl = NULL;
	}
	sdhc_spi_check(userdata);
	SDHC_SpiInitCard(luat_fatfs_spi_ctrl);
	if(userdata->type == 1){
		userdata->spi_device->spi_config.bandrate = userdata->fast_speed;
		luat_spi_device_config(userdata->spi_device);
	}else{
		SPI_SetNewConfig(luat_spi[userdata->spi_id].id, userdata->fast_speed, 0);
	}
	if (SDHC_IsReady(luat_fatfs_spi_ctrl))
	{
		for(i = 0; i < USB_MAX; i++)
		{
			Core_UDiskAttachSDHCRecovery(i, luat_fatfs_spi_ctrl);
		}
	}
	OS_MutexRelease(luat_fatfs_locker);
	return SDHC_IsReady(luat_fatfs_spi_ctrl)?0:STA_NOINIT;
}

static DSTATUS sdhc_spi_status(luat_fatfs_spi_t* userdata)
{
	OS_MutexLock(luat_fatfs_locker);
	sdhc_spi_check(userdata);
	OS_MutexRelease(luat_fatfs_locker);
	return SDHC_IsReady(luat_fatfs_spi_ctrl)?0:STA_NOINIT;
}

static DRESULT sdhc_spi_read(luat_fatfs_spi_t* userdata, uint8_t* buff, uint32_t sector, uint32_t count)
{
	OS_MutexLock(luat_fatfs_locker);
	sdhc_spi_check(userdata);
	if (!SDHC_IsReady(luat_fatfs_spi_ctrl))
	{
		OS_MutexRelease(luat_fatfs_locker);
		return RES_NOTRDY;
	}
	SDHC_SpiReadBlocks(luat_fatfs_spi_ctrl, buff, sector, count);
	OS_MutexRelease(luat_fatfs_locker);
	return SDHC_IsReady(luat_fatfs_spi_ctrl)?RES_OK:RES_ERROR;
}

static DRESULT sdhc_spi_write(luat_fatfs_spi_t* userdata, const uint8_t* buff, uint32_t sector, uint32_t count)
{
	OS_MutexLock(luat_fatfs_locker);
	sdhc_spi_check(userdata);
	if (!SDHC_IsReady(luat_fatfs_spi_ctrl))
	{
		OS_MutexRelease(luat_fatfs_locker);
		return RES_NOTRDY;
	}
	SDHC_SpiWriteBlocks(luat_fatfs_spi_ctrl, buff, sector, count);
	OS_MutexRelease(luat_fatfs_locker);
	return SDHC_IsReady(luat_fatfs_spi_ctrl)?RES_OK:RES_ERROR;
}

static DRESULT sdhc_spi_ioctl(luat_fatfs_spi_t* userdata, uint8_t ctrl, void* buff)
{
	OS_MutexLock(luat_fatfs_locker);
	sdhc_spi_check(userdata);
	if (!SDHC_IsReady(luat_fatfs_spi_ctrl))
	{
		OS_MutexRelease(luat_fatfs_locker);
		return RES_NOTRDY;
	}
	SDHC_SpiReadCardConfig(luat_fatfs_spi_ctrl);
	OS_MutexRelease(luat_fatfs_locker);
	switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			return RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			*(uint32_t*)buff = SDHC_GetLogBlockNbr(luat_fatfs_spi_ctrl);
			return RES_OK;
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(uint32_t*)buff = 128;
			return RES_OK;
			break;

		default:
			return RES_PARERR;
	}
	return RES_PARERR;
}

static const block_disk_opts_t sdhc_spi_disk_opts = {
    .initialize = sdhc_spi_initialize,
    .status = sdhc_spi_status,
    .read = sdhc_spi_read,
    .write = sdhc_spi_write,
    .ioctl = sdhc_spi_ioctl,
};

void luat_spi_set_sdhc_ctrl(block_disk_t *disk)
{
	disk->opts = &sdhc_spi_disk_opts;
}

void *luat_spi_get_sdhc_ctrl(void)
{
	return luat_fatfs_spi_ctrl;
}

#else
void *luat_spi_get_sdhc_ctrl(void)
{
	return NULL;
}
#endif
