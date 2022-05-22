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

#include "user.h"

#define	SHT30_ADDR	  		(0x44)					//7位地址0x44

#define noACK 0                                 //用于判断是否结束通讯
#define ACK   1                                 //结束数据传输

//命令
#define SHT30_SINGLE_H_MEASURE_EN					0x2C06
#define SHT30_SINGLE_M_MEASURE_EN					0x2C0D
#define SHT30_SINGLE_L_MEASURE_EN					0x2C10
#define SHT30_SINGLE_H_MEASURE_DIS				0x2400
#define SHT30_SINGLE_M_MEASURE_DIS				0x240B
#define SHT30_SINGLE_L_MEASURE_DIS				0x2416

#define SHT30_PERIODOC_H_MEASURE_500MS		0x2032
#define SHT30_PERIODOC_M_MEASURE_500MS		0x2024
#define SHT30_PERIODOC_L_MEASURE_500MS		0x202F
#define SHT30_PERIODOC_H_MEASURE_1S				0x2130
#define SHT30_PERIODOC_M_MEASURE_1S				0x2126
#define SHT30_PERIODOC_L_MEASURE_1S				0x212D
#define SHT30_PERIODOC_H_MEASURE_2S				0x2236
#define SHT30_PERIODOC_M_MEASURE_2S				0x2220
#define SHT30_PERIODOC_L_MEASURE_2S				0x222B
#define SHT30_PERIODOC_H_MEASURE_4S				0x2334
#define SHT30_PERIODOC_M_MEASURE_4S				0x2322
#define SHT30_PERIODOC_L_MEASURE_4S				0x2329
#define SHT30_PERIODOC_H_MEASURE_10S			0x2737
#define SHT30_PERIODOC_M_MEASURE_10S			0x2721
#define SHT30_PERIODOC_L_MEASURE_10S			0x272A

#define SHT30_PERIODOC_MEASURE_READ			0xE000			//重复测量读取数据

#define	SHT30_SOFT_RESET									0x30A2			//软复位

#define SHT30_HEATER_EN										0x306D			//加热使能
#define SHT30_HEATER_DIS									0x3066			//加热关闭

#define SHT30_READ_STATUS									0xF32D			//读状态寄存器
#define SHT30_CLEAR_STATUS								0x3041			//清状态寄存器

// CRC
#define POLYNOMIAL  0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

void SHT30_Init(CBFuncEx_t CB, void *pParam)
{
	uint8_t Reg[2];
	uint8_t I2CID = I2C_ID0;
	GPIO_Iomux(GPIOE_06, 2);
	GPIO_Iomux(GPIOE_07, 2);
	I2C_MasterSetup(I2CID, 400000);
	BytesPutBe16(Reg, SHT30_SINGLE_M_MEASURE_EN);
	I2C_BlockWrite(I2CID, SHT30_ADDR, Reg, 2, 100, CB, pParam);
}

void SHT30_GetResult(CBFuncEx_t CB, void *pParam)
{
	uint8_t Reg[2];
	uint8_t Data[9];
	uint8_t I2CID = I2C_ID0;
	uint16_t T,H;
	uint8_t vTemSymbol, crc8;
	int32_t vTem, vHum;
	float vTemp = 0.00;

	Reg[0] = 0;
	if (!I2C_BlockRead(I2CID, SHT30_ADDR, Reg, 1, Data, 6, 100, CB, pParam))
	{
		T = BytesGetBe16(Data);
		H = BytesGetBe16(Data + 3);
		crc8 = CRC8Cal(Data, 2, 0xff, POLYNOMIAL, 0);
		if (crc8 != Data[2])
		{
			DBG("t data check error %x %x", crc8, Data[2]);
			goto CHECK_NEXT;
		}
		crc8 = CRC8Cal(Data + 3, 2, 0xff, POLYNOMIAL, 0);
		if (crc8 != Data[5])
		{
			DBG("h data check error %x %x", crc8, Data[5]);
			goto CHECK_NEXT;
		}
		vTemp = 175.0*T/(65536-1);
		if (vTemp >= 45)
		{
			vTem = (vTemp - 45.0)*10.0;
		}
		else
		{
			vTem = (45.0 - vTemp)*10.0;
			vTem = -vTem;
		}
		vTemp = 100.0*H/(65536.0-1.0);
		vHum = (vTemp*10);
		DBG("温度 %d, 湿度 %d",vTem, vHum);
	}
	else
	{
		DBG("i2c fail!");
	}
CHECK_NEXT:
	BytesPutBe16(Reg, SHT30_SINGLE_M_MEASURE_EN);
	I2C_BlockWrite(I2CID, SHT30_ADDR, Reg, 2, 100, CB, pParam);
}
