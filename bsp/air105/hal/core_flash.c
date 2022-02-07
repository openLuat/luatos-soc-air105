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

#include "bl_inc.h"

void __FUNC_IN_RAM__ CACHE_CleanAll(CACHE_TypeDef *Cache)
{
	while (Cache->CACHE_CS & CACHE_IS_BUSY);

	Cache->CACHE_REF = CACHE_REFRESH_ALLTAG;
	Cache->CACHE_REF |= CACHE_REFRESH;
	while ((Cache->CACHE_REF & CACHE_REFRESH));
}
#if 0 //没什么用，没有ROM里的API好用
typedef struct
{
    uint8_t Instruction;
    QSPI_BusModeTypeDef BusMode;
    QSPI_CmdFormatTypeDef CmdFormat;
    uint32_t Address;

    uint32_t WrData;
    uint32_t RdData;

}FLASH_CommandTypeDef;
#define QSPI_FIFO_NUM	32
#define __FLASH_DISABLE_IRQ__

#define FLASH_QSPI_TIMEOUT_DEFAULT_CNT	(19000) //18944
#define FLASH_QSPI_ACCESS_REQ_ENABLE	(0x00000001U)
#define FLASH_QSPI_FLASH_READY_ENABLE	(0x0000006BU)
static int32_t __FUNC_IN_RAM__ prvQSPI_Command(FLASH_CommandTypeDef *cmd, int32_t timeout)
{
	int32_t i;
	int32_t status = -ERROR_OPERATION_FAILED;
	QSPI->REG_WDATA = cmd->WrData;
	QSPI->ADDRES = (QSPI->ADDRES & ~QUADSPI_ADDRESS_ADR) | (cmd->Address << 8);
	QSPI->FCU_CMD = (QSPI->FCU_CMD & ~(QUADSPI_FCU_CMD_CODE | QUADSPI_FCU_CMD_BUS_MODE | QUADSPI_FCU_CMD_CMD_FORMAT | QUADSPI_FCU_CMD_ACCESS_REQ)) | ((cmd->Instruction << 24) |((uint32_t)( cmd->BusMode<< 8)) |((uint32_t)( cmd->CmdFormat << 4))| (FLASH_QSPI_ACCESS_REQ_ENABLE));
	//Wait For CMD done
	for (i = 0; i < timeout; i += 4)
	{
		if (QSPI->INT_RAWSTATUS & QUADSPI_INT_RAWSTATUS_DONE_IR)
		{
			QSPI->INT_CLEAR = QUADSPI_INT_CLEAR_DONE;
			status = ERROR_NONE;
			break;
		}
	}
	cmd->RdData = QSPI->REG_RDATA;
	return status;
}

static int32_t __FUNC_IN_RAM__ prvQSPI_WriteEnable(QSPI_BusModeTypeDef bus_mode)
{
	FLASH_CommandTypeDef sCommand;

	sCommand.Instruction = SPIFLASH_CMD_WREN;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;

	if (QSPI_BUSMODE_444 == bus_mode)
	{
		sCommand.BusMode = QSPI_BUSMODE_444;
	}
	else
	{
		sCommand.BusMode = QSPI_BUSMODE_111;
	}

	if (prvQSPI_Command(&sCommand, FLASH_QSPI_TIMEOUT_DEFAULT_CNT))
	{
		return -ERROR_OPERATION_FAILED;
	}

	return ERROR_NONE;
}

//PP,QPP,Sector Erase,Block Erase, Chip Erase, Write Status Reg, Erase Security Reg
static int32_t __FUNC_IN_RAM__ prvQSPI_IsBusy(QSPI_BusModeTypeDef bus_mode)
{
	FLASH_CommandTypeDef sCommand;

	sCommand.Instruction = SPIFLASH_CMD_RDSR;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_RREG8;

	if (QSPI_BUSMODE_444 == bus_mode)
	{
		sCommand.BusMode = QSPI_BUSMODE_444;
	}
	else
	{
		sCommand.BusMode = QSPI_BUSMODE_111;
	}

	if (prvQSPI_Command(&sCommand, FLASH_QSPI_TIMEOUT_DEFAULT_CNT))
	{
		return -ERROR_OPERATION_FAILED;
	}

	if (sCommand.RdData & BIT0)
	{
		return -ERROR_DEVICE_BUSY;
	}
	return ERROR_NONE;
}

int32_t __FUNC_IN_RAM__ Flash_Erase(uint32_t Address, uint32_t Length)
{
	FLASH_CommandTypeDef sCommand;
	uint32_t TotalLen = 0;
	uint32_t FlashAddress, DummyLen;
	Address &= (uint32_t)(0x00FFFFFF);
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);
	sCommand.BusMode = QSPI_BUSMODE_111;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24;
	while (TotalLen < Length)
	{
		if (prvQSPI_WriteEnable(sCommand.BusMode) != 0)
		{
			return -ERROR_OPERATION_FAILED;
		}
		FlashAddress = Address + TotalLen;
		DummyLen = Length - TotalLen;
		if (!(FlashAddress & SPI_FLASH_BLOCK_MASK) && (DummyLen >= SPI_FLASH_BLOCK_SIZE))
		{
			sCommand.Instruction = SPIFLASH_CMD_BE;
			TotalLen += SPI_FLASH_BLOCK_SIZE;
		}
		else
		{
			sCommand.Instruction = SPIFLASH_CMD_SE;
			TotalLen += SPI_FLASH_SECTOR_SIZE;
		}
		sCommand.Address = FlashAddress;
#if (defined __FLASH_DISABLE_IRQ__)
		__disable_irq();
#endif
		if (prvQSPI_Command(&sCommand, FLASH_QSPI_TIMEOUT_DEFAULT_CNT))
		{
#if (defined __FLASH_DISABLE_IRQ__)
			CACHE_CleanAll(CACHE);
			__enable_irq();
#endif
			return -ERROR_OPERATION_FAILED;
		}

		while(prvQSPI_IsBusy(sCommand.BusMode));
#if (defined __FLASH_DISABLE_IRQ__)
		__enable_irq();
#endif

	}
#if (defined __FLASH_DISABLE_IRQ__)
	CACHE_CleanAll(CACHE);
#endif
	return ERROR_NONE;
}

int32_t __FUNC_IN_RAM__ Flash_Program(uint32_t Address, const uint8_t *pBuf, uint32_t Len)
{
	FLASH_CommandTypeDef sCommand;
	uint32_t FinishLen = 0, DummyLen, ProgramLen, i;
	int32_t status;
	uint32_t PageData[64];
    /* Initialize the adress variables */
	sCommand.Instruction = QUAD_INPUT_PAGE_PROG_CMD;
	sCommand.BusMode = QSPI_BUSMODE_114;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;
	Address &= 0x00ffffff;

	while(FinishLen < Len)
	{
        if (prvQSPI_WriteEnable(QSPI_BUSMODE_111))
        {
			return -ERROR_OPERATION_FAILED;
        }
#if (defined __FLASH_DISABLE_IRQ__)
        __disable_irq();
        ProgramLen = ((Len - FinishLen) > 4)?4:(Len - FinishLen);
#else
        ProgramLen = ((Len - FinishLen) > QSPI_FIFO_NUM)?QSPI_FIFO_NUM:(Len - FinishLen);
#endif
    	for(i = 0; i < ProgramLen; i+=4)
    	{
    		PageData[i >> 2] = BytesGetLe32(pBuf + i);
    	}

		QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
		QSPI->BYTE_NUM = (ProgramLen << 16);
		sCommand.Address = Address + FinishLen;

		DummyLen = 0;
		while((DummyLen < ProgramLen) && !(QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_TFFL))
		{
			QSPI->WR_FIFO = PageData[DummyLen >> 2];
			DummyLen += 4;
		}

		QSPI->ADDRES = (QSPI->ADDRES & ~QUADSPI_ADDRESS_ADR) | (sCommand.Address << 8);
		QSPI->FCU_CMD = (QSPI->FCU_CMD & ~(QUADSPI_FCU_CMD_CODE | QUADSPI_FCU_CMD_BUS_MODE | QUADSPI_FCU_CMD_CMD_FORMAT | QUADSPI_FCU_CMD_ACCESS_REQ)) | ((sCommand.Instruction << 24) |((uint32_t)( sCommand.BusMode<< 8)) |((uint32_t)( sCommand.CmdFormat << 4))| (FLASH_QSPI_ACCESS_REQ_ENABLE));
		while(DummyLen < ProgramLen)
		{
			while(QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_TFFL)
			{

			}
			QSPI->WR_FIFO = PageData[DummyLen >> 2];
			DummyLen += 4;
		}
//		DBG("%u", DummyLen);
		status = -ERROR_OPERATION_FAILED;
		for (i = 0; i < FLASH_QSPI_TIMEOUT_DEFAULT_CNT; i += 4)
		{
			if (QSPI->INT_RAWSTATUS & QUADSPI_INT_RAWSTATUS_DONE_IR)
			{
				QSPI->INT_CLEAR = QUADSPI_INT_CLEAR_DONE;
				status = ERROR_NONE;
				break;
			}
		}

		if (status)
		{
#if (defined __FLASH_DISABLE_IRQ__)
			CACHE_CleanAll(CACHE);
			__enable_irq();
#endif
			return status;
		}
		while (prvQSPI_IsBusy(QSPI_BUSMODE_111));
#if (defined __FLASH_DISABLE_IRQ__)
		__enable_irq();
#endif
		FinishLen += ProgramLen;
	}
#if (defined __FLASH_DISABLE_IRQ__)
	CACHE_CleanAll(CACHE);
#endif
	return ERROR_NONE;
}
#endif
/**
  * @brief  Flash Erase Sector.
  * @param  sectorAddress: The sector address to be erased
  * @retval FLASH Status:  The returned value can be: QSPI_STATUS_ERROR, QSPI_STATUS_OK
  */
uint8_t FLASH_EraseSector(uint32_t sectorAddress)
{
    uint8_t ret;
	__disable_irq();
	//__disable_fault_irq();

    ret = ROM_QSPI_EraseSector(NULL, sectorAddress);

	//__enable_fault_irq();
	__enable_irq();

    return ret;
}

/**
  * @brief  Flash Program Interface.
  * @param  addr:          specifies the address to be programmed.
  * @param  size:          specifies the size to be programmed.
  * @param  buffer:        pointer to the data to be programmed, need word aligned
  * @retval FLASH Status:  The returned value can be: QSPI_STATUS_ERROR, QSPI_STATUS_OK
  */
uint8_t FLASH_ProgramPage(uint32_t addr, uint32_t size, uint8_t *buffer)
{
    uint8_t ret;
	QSPI_CommandTypeDef cmdType;

    cmdType.Instruction = QUAD_INPUT_PAGE_PROG_CMD;
    cmdType.BusMode = QSPI_BUSMODE_114;
    cmdType.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;

	__disable_irq();
	//__disable_fault_irq();

    ret = ROM_QSPI_ProgramPage(&cmdType, DMA_Channel_1, addr, size, buffer);

	//__enable_fault_irq();
	__enable_irq();

    return ret;
}

int32_t Flash_Erase(uint32_t Address, uint32_t Length)
{
	uint32_t TotalLen = 0;
	QSPI_CommandTypeDef sCommand;
	sCommand.BusMode = QSPI_BUSMODE_111;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24;
	uint32_t FlashAddress, DummyLen;
	uint8_t ret;
	while (TotalLen < Length)
	{
		FlashAddress = Address + TotalLen;
		DummyLen = Length - TotalLen;
		if (!(FlashAddress & SPI_FLASH_BLOCK_MASK) && (DummyLen >= SPI_FLASH_BLOCK_SIZE))
		{
			sCommand.Instruction = SPIFLASH_CMD_BE;
			TotalLen += SPI_FLASH_BLOCK_SIZE;
		}
		else
		{
			sCommand.Instruction = SPIFLASH_CMD_SE;
			TotalLen += SPI_FLASH_SECTOR_SIZE;
		}
		__disable_irq();
		//__disable_fault_irq();

	    ret = ROM_QSPI_EraseSector(&sCommand, FlashAddress);

		//__enable_fault_irq();
		__enable_irq();
	}
	CACHE_CleanAll(CACHE);
	return ERROR_NONE;
}

int32_t Flash_Program(uint32_t Address, const uint8_t *pBuf, uint32_t Len)
{
	uint32_t size = (Len + (4 - 1)) & (~(4 - 1));
	uint32_t Pos = 0;
    uint8_t ret;
	QSPI_CommandTypeDef cmdType;

    cmdType.Instruction = QUAD_INPUT_PAGE_PROG_CMD;
    cmdType.BusMode = QSPI_BUSMODE_114;
    cmdType.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;


	while(Pos < size)
	{
		if ((size - Pos) > __FLASH_PAGE_SIZE__)
		{


			__disable_irq();
			//__disable_fault_irq();

		    ret = ROM_QSPI_ProgramPage(&cmdType, DMA_Channel_1, Address + Pos, __FLASH_PAGE_SIZE__, pBuf + Pos);

			//__enable_fault_irq();
			__enable_irq();
			Pos += __FLASH_PAGE_SIZE__;
		}
		else
		{
			__disable_irq();
			//__disable_fault_irq();

		    ret = ROM_QSPI_ProgramPage(&cmdType, DMA_Channel_1, Address + Pos, (size - Pos), pBuf + Pos);

			//__enable_fault_irq();
			__enable_irq();
			Pos += (size - Pos);
		}
	}
	CACHE_CleanAll(CACHE);
	return ERROR_NONE;
}

int Flash_EraseSector(uint32_t address, uint8_t NeedCheck)
{
	uint8_t buf[__FLASH_PAGE_SIZE__];
	uint32_t i;
	uint8_t retry = 1;
	void *res;
	memset(buf, 0xff, __FLASH_PAGE_SIZE__);
BL_ERASESECTOR_AGAIN:
	FLASH_EraseSector(address);
#if (defined __BUILD_OS__) || (defined __BUILD_APP__)
	CACHE_CleanAll(CACHE);
#endif

	if (!NeedCheck) return ERROR_NONE;
	for(i = 0; i < 4096; i+=__FLASH_PAGE_SIZE__)
	{
		res = memcmp(address + i, buf, __FLASH_PAGE_SIZE__);
		if (res)
		{
			DBG_INFO("%x", res);
			if (retry)
			{
				retry = 0;
				goto BL_ERASESECTOR_AGAIN;
			}
			else
			{
				return -1;
			}
		}
	}
	return 0;
}

int Flash_ProgramData(uint32_t address, uint32_t *Data, uint32_t Len, uint8_t NeedCheck)
{
	void *res;
	uint32_t size = (Len + (4 - 1)) & (~(4 - 1));
	FLASH_ProgramPage(address, size, Data);
#if (defined __BUILD_OS__) || (defined __BUILD_APP__)
	CACHE_CleanAll(CACHE);
#endif
	if (!NeedCheck) return ERROR_NONE;
	res = memcmp(address, Data, Len);
	if (res)
	{
		DBG_INFO("%x", res);
		FLASH_ProgramPage(address, size, Data);
#if (defined __BUILD_OS__) || (defined __BUILD_APP__)
		CACHE_CleanAll(CACHE);
#endif
		res = memcmp(address, Data, size);
		if (res)
		{
			DBG_INFO("%x", res);
			return -1;
		}
	}
	return 0;
}
