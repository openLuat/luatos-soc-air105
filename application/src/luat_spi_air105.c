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

#include "app_interface.h"

#define LUAT_LOG_TAG "luat.spi"
#include "luat_log.h"

typedef struct
{
	uint8_t id;
	uint8_t mark;
    uint8_t mode;//spi模式
}Spi_Struct;

static Spi_Struct luat_spi[6] ={0};

int32_t luat_spi_cb(void *pData, void *pParam){
//    LLOGD("luat_spi_cb pData:%d pParam:%d ",(int)pData,(int)pParam);
    switch ((int)pData){
        case 0:
            luat_spi[5].mark = 0;
            break;
        case 1:
            luat_spi[0].mark = 0;
            break;
        case 2:
            luat_spi[1].mark = 0;
            break;
        case 3:
            luat_spi[2].mark = 0;
            break;
        default:
            break;
    }
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
        if (spi_id == 0 || spi->cs == GPIOB_13)
            GPIO_Iomux(GPIOB_13, 0);
        GPIO_Iomux(GPIOB_12, 0);
        GPIO_Iomux(GPIOB_14, 0);
        GPIO_Iomux(GPIOB_15, 0);
    }
    else if (spi_id == 1) {
        luat_spi[spi_id].id=SPI_ID1;
        if (spi_id == 1 || spi->cs == GPIOA_07)
	        GPIO_Iomux(GPIOA_07,3);
	    GPIO_Iomux(GPIOA_06,3);
	    GPIO_Iomux(GPIOA_08,3);
	    GPIO_Iomux(GPIOA_09,3);
    }
    else if (spi_id == 2) {
        luat_spi[spi_id].id=SPI_ID2;
        if (spi_id == 2 || spi->cs == GPIOB_03)
	        GPIO_Iomux(GPIOB_03,0);
	    GPIO_Iomux(GPIOB_02,0);
	    GPIO_Iomux(GPIOB_04,0);
	    GPIO_Iomux(GPIOB_05,0);
    }
    else if (spi_id == 5) {
        luat_spi[spi_id].id=HSPI_ID0;
        if (spi_id == 5 || spi->cs == GPIOC_14)
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
    // LLOGD("SPI_MasterInit luat_spi%d:%d dataw:%d spi_mode:%d bandrate:%d ",spi_id,luat_spi[spi_id], spi->dataw, spi_mode, spi->bandrate);
    SPI_MasterInit(luat_spi[spi_id].id, spi->dataw, spi_mode, spi->bandrate, luat_spi_cb, NULL);
    return 0;
}

//关闭SPI，成功返回0
int luat_spi_close(int spi_id) {
    return 0;
}
//收发SPI数据，返回接收字节数
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length) {
    // LLOGD("SPI_MasterInit luat_spi%d:%d send_buf:%x recv_buf:%x length:%d ",spi_id,luat_spi[spi_id], *send_buf, *recv_buf, length);
    // while(luat_spi[spi_id].mark)
    luat_spi[spi_id].mark = 1;
    if(luat_spi[spi_id].mode==1)
        SPI_FlashBlockTransfer(luat_spi[spi_id].id, send_buf, send_length, recv_buf, recv_length);
    else
    	SPI_BlockTransfer(luat_spi[spi_id].id, send_buf, recv_buf, recv_length);
    return recv_length;
}
//收SPI数据，返回接收字节数
int luat_spi_recv(int spi_id, char* recv_buf, size_t length) {
    // LLOGD("SPI_MasterInit luat_spi%d:%d recv_buf:%x length:%d ",spi_id,luat_spi[spi_id], *recv_buf, length);
    // while(luat_spi[spi_id].mark)
    luat_spi[spi_id].mark = 1;
    SPI_BlockTransfer(luat_spi[spi_id].id, recv_buf, recv_buf, length);
    return length;
}
//发SPI数据，返回发送字节数
int luat_spi_send(int spi_id, const char* send_buf, size_t length) {
    // LLOGD("luat_spi_send luat_spi%d:%d send_buf:%x length:%d ",spi_id,luat_spi[spi_id], *send_buf, length);
    // while(luat_spi[spi_id].mark)
    luat_spi[spi_id].mark = 1;
    SPI_BlockTransfer(luat_spi[spi_id].id, send_buf, NULL, length);
    return length;
}

int luat_lcd_draw_no_block(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color, uint8_t last_flush)
{
	uint32_t retry_cnt = 0;
	uint32_t cache_len = Core_LCDDrawCacheLen();
//	if (last_flush)
//	{
//		LLOGD("lcd flush done!");
//	}

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
		LCD_DrawStruct *draw = OS_Zalloc(sizeof(LCD_DrawStruct));
		if (!draw)
		{
			LLOGE("lcd flush no memory");
			return -1;
		}
		luat_spi_device_t* spi_dev = (luat_spi_device_t*)conf->userdata;
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
	    draw->Data = OS_Malloc(draw->Size);
		if (!draw->Data)
		{
			LLOGE("lcd flush data no memory");
			OS_Free(draw);
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

void luat_lcd_draw_block(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color, uint8_t last_flush)
{
	LCD_DrawStruct draw;
	if (conf->port == LUAT_LCD_SPI_DEVICE){
		luat_spi_device_t* spi_dev = (luat_spi_device_t*)conf->userdata;
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
