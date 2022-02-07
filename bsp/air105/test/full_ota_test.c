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
static uint32_t CRC32_Table[256];
static uint32_t DataBuf[16 * 1024];
static uint32_t DataBufBak[16 * 1024];
void OutFlash_Test(void)
{
	CoreUpgrade_HeadStruct Head;
	uint32_t copyaddr, flag, crc32;
	uint32_t AppInfo[4];
	PV_Union uPV;
	CRC32_CreateTable(CRC32_Table, CRC32_GEN);
	memcpy(AppInfo, __FLASH_APP_START_ADDR__, sizeof(AppInfo));
	if (AppInfo[2] == __APP_START_MAGIC__)
	{
		DBG("test ok!");
		return;
	}
	SPIFlash_CtrlStruct SPIFlash;
	memset(&SPIFlash, 0, sizeof(SPIFlash));
	SPIFlash.SpiID = SPI_ID1;
	crc32 = CRC32_Cal(CRC32_Table, __FLASH_APP_START_ADDR__, 1024 * 1024, 0xffffffff);
    GPIO_Iomux(GPIOA_06,3);
    GPIO_Iomux(GPIOA_08,3);
    GPIO_Iomux(GPIOA_09,3);
//    GPIO_Iomux(GPIOA_07,3);
    GPIO_Config(GPIOA_07, 0, 1);
    SPI_MasterInit(SPIFlash.SpiID, 8, SPI_MODE_0, 24000000, NULL, NULL);
    SPIFlash.CSPin = GPIOA_07;
    SPIFlash.IsBlockMode = 1;
    SPIFlash.IsSpiDMAMode = 1;
	SPI_DMATxInit(SPIFlash.SpiID, FLASH_SPI_TX_DMA_STREAM, 0);
	SPI_DMARxInit(SPIFlash.SpiID, FLASH_SPI_RX_DMA_STREAM, 0);
	SPIFlash_Init(&SPIFlash, NULL);
	//把app区的程序拷贝到备份区，现在就拷贝1MB
	flag = 0;
	Head.DataCRC32 = 0xffffffff;
	for(copyaddr = __FLASH_APP_START_ADDR__; copyaddr < (__FLASH_APP_START_ADDR__ + 1024 * 1024); copyaddr += 65536)
	{
		memcpy(DataBuf, copyaddr, 65536);
		if (!flag)
		{
			flag = 1;
			DataBuf[2] = __APP_START_MAGIC__;
		}
		SPIFlash_Erase(&SPIFlash, copyaddr - __FLASH_APP_START_ADDR__, 65536);
		SPIFlash_Write(&SPIFlash, copyaddr - __FLASH_APP_START_ADDR__, DataBuf, 65536);
		SPIFlash_Read(&SPIFlash, copyaddr - __FLASH_APP_START_ADDR__, DataBufBak, 65536, 1);
		if (memcmp(DataBuf, DataBufBak, 65536))
		{
			DBG("%x, %x, %x, %x", DataBuf[0], DataBuf[256], DataBufBak[0], DataBufBak[256]);
		}
		Head.DataCRC32 = CRC32_Cal(CRC32_Table, DataBuf, 65536, Head.DataCRC32);
		WDT_Feed();
	}
	Head.MaigcNum = __APP_START_MAGIC__;
	uPV.u8[0] = CORE_OTA_MODE_FULL;
	uPV.u8[1] = CORE_OTA_OUT_SPI_FLASH;
	uPV.u8[2] = SPI_ID1;
	Head.Param1 = uPV.u32;
	uPV.u8[0] = GPIOA_08;
	uPV.u8[1] = GPIOA_09;
	uPV.u8[2] = GPIOA_06;
	uPV.u8[3] = GPIOA_07;
	Head.Param2 = uPV.u32;
	Head.DataStartAddress = 0;
	Head.DataLen = 1024 * 1024;
	Head.CRC32 = CRC32_Cal(CRC32_Table, &Head.Param1, sizeof(Head) - 8, 0xffffffff);
	Flash_EraseSector(__FLASH_OTA_INFO_ADDR__, 0);
	Flash_ProgramData(__FLASH_OTA_INFO_ADDR__, &Head, sizeof(Head), 0);
	DBG("reset to ota");
	Task_DelayMS(10);
	SystemReset();
}

void InFlash_Test(void)
{
	CoreUpgrade_HeadStruct Head;
	uint32_t copyaddr, flag, crc32[0];
	uint32_t AppInfo[4];
	PV_Union uPV;
	CRC32_CreateTable(CRC32_Table, CRC32_GEN);
	memcpy(AppInfo, __FLASH_APP_START_ADDR__, sizeof(AppInfo));
	if (AppInfo[2] == __APP_START_MAGIC__)
	{
		DBG("test ok!");
		return;
	}
	//把app区的程序拷贝到备份区，现在就拷贝1MB
	flag = 0;
	for(copyaddr = (__FLASH_APP_START_ADDR__ + 1024 * 1024); copyaddr < (__FLASH_APP_START_ADDR__ + 2 * 1024 * 1024); copyaddr += __FLASH_BLOCK_SIZE__)
	{
		DBG("%x", copyaddr);
		memcpy(DataBuf, copyaddr - 1024 * 1024, __FLASH_BLOCK_SIZE__);
		if (!flag)
		{
			flag = 1;
			DataBuf[2] = __APP_START_MAGIC__;
		}

		Flash_Erase(copyaddr, __FLASH_BLOCK_SIZE__);
		Flash_Program(copyaddr, DataBuf, __FLASH_BLOCK_SIZE__);

		WDT_Feed();
	}
	Head.MaigcNum = __APP_START_MAGIC__;
	uPV.u8[0] = CORE_OTA_MODE_FULL;
	uPV.u8[1] = CORE_OTA_IN_FLASH;
	Head.Param1 = uPV.u32;
	Head.DataStartAddress = __FLASH_APP_START_ADDR__ + 1024 * 1024;
	Head.DataLen = 1024 * 1024;
	Head.DataCRC32 = CRC32_Cal(CRC32_Table, __FLASH_APP_START_ADDR__ + 1024 * 1024, 1024 * 1024, 0xffffffff);
	Head.CRC32 = CRC32_Cal(CRC32_Table, &Head.Param1, sizeof(Head) - 8, 0xffffffff);
	Flash_EraseSector(__FLASH_OTA_INFO_ADDR__, 0);
	Flash_ProgramData(__FLASH_OTA_INFO_ADDR__, &Head, sizeof(Head), 0);
	DBG("reset to ota");
	Task_DelayMS(10);
	SystemReset();
//	crc32[0] = CRC32_Cal(CRC32_Table, __FLASH_APP_START_ADDR__, 1024 * 1024, 0xffffffff);
//	crc32[1] = CRC32_Cal(CRC32_Table, __FLASH_APP_START_ADDR__ + 1024 * 1024, 1024 * 1024, 0xffffffff);
//	DBG("%x,%x,%u",crc32[0], crc32[1], crc32[1] - crc32[0]);
}
