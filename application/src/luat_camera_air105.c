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

#include "luat_lcd.h"
#include "luat_camera.h"
#include "luat_msgbus.h"
#include "luat_spi.h"
#include "luat_gpio.h"

#include "app_interface.h"
#include "zbar.h"
#include "symbol.h"
#include "image.h"

#define LUAT_LOG_TAG "camera"
#include "luat_log.h"

typedef struct
{
	Buffer_Struct FileBuffer;
	Buffer_Struct JPEGSavePath;
	uint8_t *DataCache;
	uint32_t TotalSize;
	uint32_t CurSize;
	uint32_t VLen;
	uint32_t drawVLen;
	int32_t YDiff;
	uint16_t Width;
	uint16_t Height;
	uint8_t DataBytes;
	uint8_t IsDecode;
	uint8_t BufferFull;
	uint8_t JPEGQuality;
	uint8_t CaptureMode;
	uint8_t CaptureWait;
	uint8_t JPEGEncodeDone;
}Camera_CtrlStruct;

static Camera_CtrlStruct prvCamera;

static struct luat_camera_conf camera_conf;

static luat_lcd_conf_t* lcd_conf;
static uint8_t draw_lcd = 0;

static void Camera_SaveJPEGData(void *Cxt, void *pData, int Size)
{
	OS_BufferWrite(Cxt, pData, Size);
}

static int32_t Camera_SaveJPEGDone(void *pData, void *pParam)
{
	HANDLE fd;
	if (prvCamera.JPEGSavePath.Data)
	{
		fd = luat_fs_fopen(prvCamera.JPEGSavePath.Data, "w");
	}
	else
	{
		fd = luat_fs_fopen("/capture.jpg", "w");
	}
	if (fd)
	{
		LLOGD("capture data %ubyte", luat_fs_fwrite(prvCamera.FileBuffer.Data, prvCamera.FileBuffer.Pos, 1, fd));
		luat_fs_fclose(fd);
	}
	OS_DeInitBuffer(&prvCamera.FileBuffer);
	prvCamera.CaptureWait = 1;
	prvCamera.JPEGEncodeDone = 1;
	prvCamera.CaptureMode = 0;
	Core_EncodeJPEGSetup(NULL, &prvCamera);
    rtos_msg_t msg = {0};
    {
        msg.handler = l_camera_handler;
        msg.ptr = NULL;
        msg.arg1 = 0;
        msg.arg2 = 1;
        luat_msgbus_put(&msg, 1);
	}

}

void DecodeQR_CBDataFun(uint8_t *Data, uint32_t Len){
    prvCamera.IsDecode = 0;
    rtos_msg_t msg = {0};
	if (Data){
        msg.handler = l_camera_handler;
        msg.ptr = Data;
        msg.arg1 = 0;
        msg.arg2 = Len;
        luat_msgbus_put(&msg, 1);
	}
}

static int32_t Camera_DrawLcd(void *DrawData, uint8_t Scan){
    LCD_DrawStruct *draw = OS_Malloc(sizeof(LCD_DrawStruct));
    if (!draw){
        DBG("lcd flush no memory");
        return -1;
    }
    uint8_t CPHA = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPHA;
    uint8_t CPOL = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPOL;
    draw->DCDelay = lcd_conf->dc_delay_us;
    draw->Mode = SPI_MODE_0;
    if(CPHA&&CPOL)draw->Mode = SPI_MODE_3;
    else if(CPOL)draw->Mode = SPI_MODE_2;
    else if(CPHA)draw->Mode = SPI_MODE_1;
    draw->Speed = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.bandrate;
    if (((luat_spi_device_t*)(lcd_conf->userdata))->bus_id == 5) draw->SpiID = HSPI_ID0;
    else draw->SpiID = ((luat_spi_device_t*)(lcd_conf->userdata))->bus_id;
    draw->CSPin = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.cs;
    draw->DCPin = lcd_conf->pin_dc;
    draw->xoffset = lcd_conf->xoffset;
    draw->yoffset = lcd_conf->yoffset;
    if (Scan == 0){
        draw->x1 = 0;
        draw->x2 = lcd_conf->w -1;
        draw->y1 = prvCamera.VLen;
        draw->y2 = prvCamera.VLen + prvCamera.drawVLen -1;
        draw->Size = (draw->x2 - draw->x1 + 1) * (draw->y2 - draw->y1 + 1) * 2;
        draw->ColorMode = COLOR_MODE_RGB_565;
        draw->Data = OS_Malloc(draw->Size);
        if (!draw->Data){
            DBG("lcd flush data no memory");
            OS_Free(draw);
            return -1;
        }
        memcpy(draw->Data, DrawData, draw->Size);
        Core_LCDDraw(draw);
    }else if(Scan == 1){
        draw->x1 = 0;
        draw->x2 = prvCamera.Width - 1;
        draw->y1 = 0;
        draw->y2 = prvCamera.Height - 1;
        draw->Size = prvCamera.TotalSize;
        draw->ColorMode = COLOR_MODE_GRAY;
        draw->Data = DrawData;
        Core_CameraDraw(draw);
    }else{
        DBG("Scan error");
        OS_Free(draw);
        return -1;
    }
}

static int32_t prvCamera_DCMICB(void *pData, void *pParam){

    uint8_t zbar_scan = (uint8_t)pParam;
    Buffer_Struct *RxBuf = (Buffer_Struct *)pData;
    if (prvCamera.CaptureMode){
    	if (!pData){
    		if (prvCamera.CaptureWait && prvCamera.JPEGEncodeDone)
    		{
    			prvCamera.CaptureWait = 0;
    			prvCamera.JPEGEncodeDone = 0;
    			Core_EncodeJPEGStart(prvCamera.Width, prvCamera.Height, prvCamera.JPEGQuality, prvCamera.YDiff);
    		}
    		else if (!prvCamera.CaptureWait)
    		{
    			prvCamera.CaptureWait = 1;
    			prvCamera.JPEGEncodeDone = 0;
    			Core_EncodeJPEGEnd(Camera_SaveJPEGDone, 0);
    		}
    	}
    	else
    	{
            if (!prvCamera.CaptureWait)
            {
                uint8_t *data = malloc(RxBuf->MaxLen * 4);
                memcpy(data, RxBuf->Data, RxBuf->MaxLen * 4);
                Core_EncodeJPEGRun(data, RxBuf->MaxLen * 4, zbar_scan?COLOR_MODE_GRAY:COLOR_MODE_RGB_565);
            }
    	}
    	return 0;
    }
    if (zbar_scan == 0){

        if (!pData){
            prvCamera.VLen = 0;
            return 0;
        }
        if (draw_lcd)
        {
            Camera_DrawLcd(RxBuf->Data, zbar_scan);
        }
        prvCamera.VLen += prvCamera.drawVLen;
        return 0;
    }else if (zbar_scan == 1){
        Buffer_Struct *RxBuf = (Buffer_Struct *)pData;
        if (!pData)
        {
            if (!prvCamera.DataCache)
            {
                prvCamera.DataCache = malloc(prvCamera.TotalSize);
                prvCamera.BufferFull = 0;
            }
            else
            {
                if (prvCamera.TotalSize != prvCamera.CurSize)
                {
    //				DBG_ERR("%d, %d", prvCamera.CurSize, prvCamera.TotalSize);
                }
                else if (!prvCamera.IsDecode)
                {
                    prvCamera.IsDecode = 1;
                    if (draw_lcd)
                        Camera_DrawLcd(prvCamera.DataCache, zbar_scan);
                    Core_DecodeQR(prvCamera.DataCache, prvCamera.Width, prvCamera.Height,  DecodeQR_CBDataFun);
                    prvCamera.DataCache = malloc(prvCamera.TotalSize);
                    prvCamera.BufferFull = 0;
                }
                else
                {
                    prvCamera.BufferFull = 1;
                }
            }
            prvCamera.CurSize = 0;
            return 0;
        }
        if (prvCamera.DataCache)
        {
            if (prvCamera.BufferFull)
            {
                if (!prvCamera.IsDecode){
                    prvCamera.IsDecode = 1;
                    if (draw_lcd)
                        Camera_DrawLcd(prvCamera.DataCache, zbar_scan);
                    Core_DecodeQR(prvCamera.DataCache, prvCamera.Width, prvCamera.Height,  DecodeQR_CBDataFun);
                    prvCamera.DataCache = malloc(prvCamera.TotalSize);
                    prvCamera.BufferFull = 0;
                    prvCamera.CurSize = 0;
                }else{
                    return 0;
                }
            }
        }
        else
        {
            prvCamera.DataCache = malloc(prvCamera.TotalSize);
            prvCamera.BufferFull = 0;
            prvCamera.CurSize = 0;
            prvCamera.IsDecode = 0;
        }
        memcpy(&prvCamera.DataCache[prvCamera.CurSize], RxBuf->Data, RxBuf->MaxLen * 4);
        prvCamera.CurSize += RxBuf->MaxLen * 4;
        if (prvCamera.TotalSize < prvCamera.CurSize){
            DBG_ERR("%d,%d", prvCamera.TotalSize, prvCamera.CurSize);
            prvCamera.CurSize = 0;
        }
        return 0;
    }
}

int luat_camera_init(luat_camera_conf_t *conf){
    
	GPIO_Iomux(GPIOD_01, 3);
	GPIO_Iomux(GPIOD_02, 3);
	GPIO_Iomux(GPIOD_03, 3);
	GPIO_Iomux(GPIOD_08, 3);
	GPIO_Iomux(GPIOD_09, 3);
	GPIO_Iomux(GPIOD_10, 3);
	GPIO_Iomux(GPIOD_11, 3);
	GPIO_Iomux(GPIOE_00, 3);
    GPIO_Iomux(GPIOE_01, 3);
	GPIO_Iomux(GPIOE_02, 3);
	GPIO_Iomux(GPIOE_03, 3);

    memcpy(&camera_conf, conf, sizeof(luat_camera_conf_t));
    lcd_conf = conf->lcd_conf;
    draw_lcd = conf->draw_lcd;
    prvCamera.Width = lcd_conf->w;
    prvCamera.Height = lcd_conf->h;
    if (conf->zbar_scan == 1){
        prvCamera.DataCache = NULL;

        prvCamera.TotalSize = prvCamera.Width * prvCamera.Height;
        prvCamera.DataBytes = 1;
    }

    GPIO_Iomux(GPIOA_05, 2);
    HWTimer_SetPWM(conf->pwm_id, conf->pwm_period, conf->pwm_pulse, 0);

    luat_i2c_setup(conf->i2c_id,1,NULL);
    for(size_t i = 0; i < conf->init_cmd_size; i++){
        luat_i2c_send(conf->i2c_id, conf->i2c_addr, &(conf->init_cmd[i]), 2);
        i++;
	}

    DCMI_Setup(0, 0, 0, 8, 0);
	DCMI_SetCallback(prvCamera_DCMICB, conf->zbar_scan);

//    if (conf->zbar_scan == 0){
//        DCMI_SetCROPConfig(1, (conf->sensor_height-lcd_conf->h)/2, ((conf->sensor_width-lcd_conf->w)/2)*2, lcd_conf->h - 1, 2*lcd_conf->w - 1);
//        DCMI_CaptureSwitch(1, 0,lcd_conf->w, lcd_conf->h, 2, &prvCamera.drawVLen);
//        prvCamera.VLen = 0;
//    }else if(conf->zbar_scan == 1){
//        DCMI_SetCROPConfig(1, (conf->sensor_height-prvCamera.Height)/2, ((conf->sensor_width-prvCamera.Width)/2)*prvCamera.DataBytes, prvCamera.Height - 1, prvCamera.DataBytes*prvCamera.Width - 1);
//        DCMI_CaptureSwitch(1, 0,lcd_conf->w, lcd_conf->h, prvCamera.DataBytes, &prvCamera.drawVLen);
//    }
    return 0;
}

int luat_camera_start(int id)
{
	if (prvCamera.DataCache)
	{
		DCMI_CaptureSwitch(0, 0, 0, 0, 0, NULL);
		free(prvCamera.DataCache);
	}
    if (camera_conf.zbar_scan == 0){
        DCMI_SetCROPConfig(1, (camera_conf.sensor_height-lcd_conf->h)/2, ((camera_conf.sensor_width-lcd_conf->w)/2)*2, lcd_conf->h - 1, 2*lcd_conf->w - 1);
        DCMI_CaptureSwitch(1, 0, lcd_conf->w, lcd_conf->h, 2, &prvCamera.drawVLen);
        prvCamera.CaptureMode = 0;
        prvCamera.VLen = 0;
    }else if(camera_conf.zbar_scan == 1){
        DCMI_SetCROPConfig(1, (camera_conf.sensor_height-prvCamera.Height)/2, ((camera_conf.sensor_width-prvCamera.Width)/2)*prvCamera.DataBytes, prvCamera.Height - 1, prvCamera.DataBytes*prvCamera.Width - 1);
        DCMI_CaptureSwitch(1, 0,lcd_conf->w, lcd_conf->h, prvCamera.DataBytes, &prvCamera.drawVLen);
    }
    return 0;
}

int luat_camera_capture(int id, int y_diff, uint8_t quality, const char *path)
{
	DCMI_CaptureSwitch(0, 0, 0, 0, 0, NULL);
	if (prvCamera.DataCache)
	{
		free(prvCamera.DataCache);
		prvCamera.DataCache = NULL;
	}

	if (path)
	{
		OS_ReInitBuffer(&prvCamera.JPEGSavePath, strlen(path) + 1);
		memcpy(prvCamera.JPEGSavePath.Data, path, strlen(path));
	}
	OS_ReInitBuffer(&prvCamera.FileBuffer, 16 * 1024);
	Core_EncodeJPEGSetup(Camera_SaveJPEGData, &prvCamera);
	luat_camera_start(id);
	prvCamera.YDiff = y_diff;
	prvCamera.JPEGQuality = quality;
	prvCamera.CaptureMode = 1;
	prvCamera.CaptureWait = 1;
	prvCamera.JPEGEncodeDone = 1;

    return 0;
}

int luat_camera_stop(int id)
{
	DCMI_CaptureSwitch(0, 0, 0, 0, 0, NULL);
	if (prvCamera.DataCache)
	{
		free(prvCamera.DataCache);
		prvCamera.DataCache = NULL;
	}
	OS_DeInitBuffer(&prvCamera.FileBuffer);
    return 0;
}
