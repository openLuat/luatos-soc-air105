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

static uint8_t *prvTxBuf;
static uint8_t *prvRxBuf;
static uint32_t prvLen;

static int32_t prvSPI_CB(void *pData, void *pParam)
{
	uint32_t i;
	if (!memcmp(prvTxBuf, prvRxBuf, prvLen))
	{
		DBG("spi test ok");
	}
	for(i = 0; i < prvLen; i++)
	{
		if (prvTxBuf[i] != prvRxBuf[i])
		{
			DBG("spi test fail %u, %x, %x", i, prvTxBuf[i], prvRxBuf[i]);
			break;
		}
	}
}

void SPI_TestInit(uint8_t SpiID, uint32_t Speed, uint32_t TestLen)
{
	GPIO_Iomux(GPIOC_12, 3);
	GPIO_Iomux(GPIOC_13, 3);
	GPIO_Iomux(GPIOB_14, 0);
	GPIO_Iomux(GPIOB_15, 0);
	SPI_MasterInit(SpiID, 8, SPI_MODE_0, Speed, prvSPI_CB, NULL);
	SPI_DMATxInit(SpiID, DMA1_STREAM_6, 0);
	SPI_DMARxInit(SpiID, DMA1_STREAM_7, 0);
	prvTxBuf = OS_Malloc(TestLen);
	prvRxBuf = OS_Malloc(TestLen);
}

void SPI_TestOnce(uint8_t SpiID, uint32_t TestLen)
{
	uint32_t i;
	for(i = 0; i < TestLen; i++)
	{
		prvTxBuf[i] = i + 2;
	}
	memset(prvRxBuf, 0xff, TestLen);
	if (memcmp(prvTxBuf, prvRxBuf, TestLen))
	{
		DBG("spi test ready");
	}
	else
	{
		DBG("spi test not ready");
	}
	prvLen = TestLen;
	SPI_Transfer(SpiID, prvTxBuf, prvRxBuf, TestLen, 1);
}
