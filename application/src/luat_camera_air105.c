#include "app_interface.h"
#include "luat_base.h"

#include "luat_lcd.h"
#include "luat_camera.h"
#include "luat_msgbus.h"
#include "luat_spi.h"
#include "luat_gpio.h"

#include "user.h"
#include "zbar.h"
#include "symbol.h"
#include "image.h"

#define LUAT_LOG_TAG "camera"
#include "luat_log.h"

static luat_lcd_conf_t* lcd_conf;

#define DECODE_DONE (USER_EVENT_ID_START + 100)

typedef struct
{
	HANDLE NotifyTaskHandler;
	uint8_t TestBuf[4096];
	uint8_t *DataCache;
	uint32_t TotalSize;
	uint32_t CurSize;
	uint16_t Width;
	uint16_t Height;
	uint8_t DataBytes;
	uint8_t IsDecode;
	uint8_t BufferFull;
}QR_DecodeCtrlStruct;

static QR_DecodeCtrlStruct prvDecodeQR;

static uint32_t prvVLen;
static uint32_t drawVLen;

static int32_t prvCamera_DCMICB(void *pData, void *pParam){
    uint8_t zbar_scan = (uint8_t)pParam;
    if (zbar_scan == 0){
        Buffer_Struct *RxBuf = (Buffer_Struct *)pData;
        if (!pData){
            prvVLen = 0;
            return 0;
        }
        LCD_DrawStruct *draw = OS_Malloc(sizeof(LCD_DrawStruct));
        if (!draw){
            DBG("lcd flush no memory");
            return -1;
        }
        uint8_t CPHA = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPHA;
        uint8_t CPOL = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPOL;
        draw->Mode = SPI_MODE_0;
        if(CPHA&&CPOL)draw->Mode = SPI_MODE_3;
        else if(CPOL)draw->Mode = SPI_MODE_1;
        else if(CPHA)draw->Mode = SPI_MODE_2;
		draw->Speed = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.bandrate;
        if (((luat_spi_device_t*)(lcd_conf->userdata))->bus_id == 5) draw->SpiID = HSPI_ID0;
        else draw->SpiID = ((luat_spi_device_t*)(lcd_conf->userdata))->bus_id;
		draw->CSPin = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.cs;
		draw->DCPin = lcd_conf->pin_dc;
        draw->x1 = 0;
        draw->x2 = 239;
        draw->y1 = prvVLen;
        draw->y2 = prvVLen + drawVLen -1;
        draw->xoffset = lcd_conf->xoffset;
		draw->yoffset = lcd_conf->yoffset;
        draw->Size = (draw->x2 - draw->x1 + 1) * (draw->y2 - draw->y1 + 1) * 2;
        draw->ColorMode = COLOR_MODE_RGB_565;
        draw->Data = OS_Malloc(draw->Size);
        if (!draw->Data){
            DBG("lcd flush data no memory");
            OS_Free(draw);
            return -1;
        }
    //	DBG_HexPrintf(RxBuf->Data, 8);
        memcpy(draw->Data, RxBuf->Data, draw->Size);
        Core_LCDDraw(draw);
        prvVLen += drawVLen;
        return 0;
    }else if (zbar_scan == 1){
        Buffer_Struct *RxBuf = (Buffer_Struct *)pData;
        if (!pData)
        {
            if (!prvDecodeQR.DataCache)
            {
                prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
                prvDecodeQR.BufferFull = 0;
            }
            else
            {
                if (prvDecodeQR.TotalSize != prvDecodeQR.CurSize)
                {
    //				DBG_ERR("%d, %d", prvDecodeQR.CurSize, prvDecodeQR.TotalSize);
                }
                else if (!prvDecodeQR.IsDecode)
                {
                    prvDecodeQR.IsDecode = 1;
                    Task_SendEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, prvDecodeQR.DataCache, 0, 0);
                    prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
                    prvDecodeQR.BufferFull = 0;
                }
                else
                {
                    prvDecodeQR.BufferFull = 1;
                }
            }
            prvDecodeQR.CurSize = 0;
            return 0;
        }
        if (prvDecodeQR.DataCache)
        {
            if (prvDecodeQR.BufferFull)
            {
                if (!prvDecodeQR.IsDecode){
                    prvDecodeQR.IsDecode = 1;
                    Task_SendEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, prvDecodeQR.DataCache, 0, 0);
                    prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
                    prvDecodeQR.BufferFull = 0;
                    prvDecodeQR.CurSize = 0;
                }else{
                    return 0;
                }
            }
        }
        else
        {
            prvDecodeQR.DataCache = malloc(prvDecodeQR.TotalSize);
            prvDecodeQR.BufferFull = 0;
            prvDecodeQR.CurSize = 0;
        }
        memcpy(&prvDecodeQR.DataCache[prvDecodeQR.CurSize], RxBuf->Data, RxBuf->MaxLen * 4);
        prvDecodeQR.CurSize += RxBuf->MaxLen * 4;
        if (prvDecodeQR.TotalSize < prvDecodeQR.CurSize){
            DBG_ERR("%d,%d", prvDecodeQR.TotalSize, prvDecodeQR.CurSize);
            prvDecodeQR.CurSize = 0;
        }
        return 0;
    }
}

static void zbar_task(void *pData){
    rtos_msg_t msg = {0};
	OS_EVENT Event;
	unsigned int len;
	int ret;
	char *string;
	zbar_t *zbar;
	LCD_DrawStruct *draw;
	while (1) {
		Task_GetEvent(prvDecodeQR.NotifyTaskHandler, DECODE_DONE, &Event, NULL, 0);
		draw = malloc(sizeof(LCD_DrawStruct));
        uint8_t CPHA = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPHA;
        uint8_t CPOL = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.CPOL;
        draw->Mode = SPI_MODE_0;
        if(CPHA&&CPOL)draw->Mode = SPI_MODE_3;
        else if(CPOL)draw->Mode = SPI_MODE_1;
        else if(CPHA)draw->Mode = SPI_MODE_2;
		draw->Speed = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.bandrate;
        if (((luat_spi_device_t*)(lcd_conf->userdata))->bus_id == 5) draw->SpiID = HSPI_ID0;
        else draw->SpiID = ((luat_spi_device_t*)(lcd_conf->userdata))->bus_id;
		draw->CSPin = ((luat_spi_device_t*)(lcd_conf->userdata))->spi_config.cs;
		draw->DCPin = lcd_conf->pin_dc;
		draw->x1 = 0;
		draw->x2 = prvDecodeQR.Width - 1;
		draw->y1 = 0;
		draw->y2 = prvDecodeQR.Height - 1;
		draw->xoffset = lcd_conf->xoffset;
		draw->yoffset = lcd_conf->yoffset;
		draw->Size = prvDecodeQR.TotalSize;
		draw->ColorMode = COLOR_MODE_GRAY;
		draw->Data = Event.Param1;
		Core_CameraDraw(draw);
		zbar = zbar_create();
		ret = zbar_run(zbar, prvDecodeQR.Width, prvDecodeQR.Height, Event.Param1);
		if (ret > 0){
			string = zbar_get_data_ptr(zbar, &len);
            msg.handler = l_camera_handler;
            msg.ptr = string;
            msg.arg1 = 0;
            msg.arg2 = len;
            luat_msgbus_put(&msg, 1);
		}
		zbar_destory(zbar);
		free(Event.Param1);
		prvDecodeQR.IsDecode = 0;
	}
}


int luat_camera_init(luat_camera_conf_t *conf){
    lcd_conf = conf->lcd_conf;

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

    luat_i2c_setup(conf->i2c_id,1,NULL);
	
    if (conf->zbar_scan == 1){
        prvDecodeQR.DataCache = NULL;
        prvDecodeQR.Width = lcd_conf->w;
        prvDecodeQR.Height = lcd_conf->h;
        prvDecodeQR.TotalSize = prvDecodeQR.Width * prvDecodeQR.Height;
        prvDecodeQR.DataBytes = 1;
        prvDecodeQR.NotifyTaskHandler = Task_Create(zbar_task, NULL, 8 * 1024, 1, "test zbar");
    }
    GPIO_Iomux(GPIOA_05, 2);
    HWTimer_SetPWM(conf->pwm_id, conf->pwm_period, conf->pwm_pulse, 0);

    luat_gpio_mode(conf->camera_pwdn, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
    luat_gpio_mode(conf->camera_rst, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, Luat_GPIO_HIGH);
	luat_timer_us_delay(10);
    luat_gpio_set(conf->camera_rst, Luat_GPIO_LOW);

    for(size_t i = 0; i < conf->init_cmd_size; i++){
        luat_i2c_send(conf->i2c_id, conf->i2c_addr, &(conf->init_cmd[i]), 2);
        i++;
	}

    DCMI_Setup(0, 0, 0, 8, 0);
	DCMI_SetCallback(prvCamera_DCMICB, conf->zbar_scan);

    if (conf->zbar_scan == 0){
        DCMI_SetCROPConfig(1, (conf->sensor_height-lcd_conf->h)/2, ((conf->sensor_width-lcd_conf->w)/2)*2, lcd_conf->h - 1, 2*lcd_conf->w - 1);
        DCMI_CaptureSwitch(1, 0,lcd_conf->w, lcd_conf->h, 2, &drawVLen);	
        prvVLen = 0;
    }else if(conf->zbar_scan == 1){
        DCMI_SetCROPConfig(1, (conf->sensor_height-prvDecodeQR.Height)/2, ((conf->sensor_width-prvDecodeQR.Width)/2)*prvDecodeQR.DataBytes, prvDecodeQR.Height - 1, prvDecodeQR.DataBytes*prvDecodeQR.Width - 1);
        DCMI_CaptureSwitch(1, 0,lcd_conf->w, lcd_conf->h, prvDecodeQR.DataBytes, &drawVLen);	
    }
}
