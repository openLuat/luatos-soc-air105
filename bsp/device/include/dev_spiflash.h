#ifndef __DEV_SPIFLASH_H__
#define __DEV_SPIFLASH_H__

#define SPIFLASH_CMD_WREN		(0x06)	//写使能
#define SPIFLASH_CMD_WRDI		(0x04)	//写禁止
#define	SPIFLASH_CMD_WRSR		(0X01)	//写状态寄存器
#define SPIFLASH_CMD_SE			(0x20)	//段擦除
#define SPIFLASH_CMD_BE			(0xD8)	//块擦除
#define SPIFLASH_CMD_CE			(0xC7)	//全擦除
#define SPIFLASH_CMD_BE2		(0x52)	//块擦除
#define SPIFLASH_CMD_CE2		(0x60)	//全擦除
#define SPIFLASH_CMD_RDID		(0x9F)	//读JEDEC ID
#define SPIFLASH_CMD_RDSR		(0x05)	//读状态寄存器
#define SPIFLASH_CMD_READ		(0x03)	//读数据
#define SPIFLASH_CMD_FTRD		(0x0B)	//快速读取
#define SPIFLASH_CMD_DRD		(0x3B)	//双线读取
#define SPIFLASH_CMD_TRD		(0x6B)	//三线读取
#define SPIFLASH_CMD_PP			(0x02)	//写flash

#define SPIFLASH_SR_SRWR		(0x80)
#define SPIFLASH_SR_TB			(0x20)
#define SPIFLASH_SR_BP2			(0x10)
#define SPIFLASH_SR_BP1			(0x08)
#define SPIFLASH_SR_BP0			(0x04)
#define SPIFLASH_SR_WEL			(0x02)
#define SPIFLASH_SR_BUSY		(0x01)

#define SPIFLASH_ERASE_CHIP		(1)
#define SPIFLASH_ERASE_BLOCK	(2)
#define SPIFLASH_ERASE_SECTOR	(3)

#define SPIFLASH_RAW_LEN	(256)
#define SPIFLASH_PAGE_LEN	(256)
#define SPIFLASH_SECTOR_LEN	(4096)
#define SPIFLASH_BLOCK_LEN	(65536)
#define SPIFLASH_PP_RUN		(1)
#define SPIFLASH_PP_DONE	(2)
#define SPIFLASH_PP_ERROR	(3)

#define SPI_FLASH_PAGE_SIZE 256
#define SPI_FLASH_SECTOR_SIZE (4 * 1024)
#define SPI_FLASH_BLOCK_SIZE (64 * 1024)
#define SPI_FLASH_BLOCK32K_SIZE (32 * 1024)

#define SPI_FLASH_PAGE_MASK (SPI_FLASH_PAGE_SIZE - 1)
#define SPI_FLASH_SECTOR_MASK (SPI_FLASH_SECTOR_SIZE - 1)
#define SPI_FLASH_BLOCK_MASK (SPI_FLASH_BLOCK_SIZE - 1)
#define SPI_FLASH_BLOCK32K_MASK (SPI_FLASH_BLOCK32K_SIZE - 1)

typedef struct
{
	uint64_t SPIEndTick;					//SPI传输的结束时最大tick
	uint64_t FlashOPEndTick;				//SPI的擦写操作结束时最大tick
	HANDLE NotifyTask;						//设置了NotifyTask，则会在大量传输SPI数据时，休眠任务但是仍然能接收Event并在CB中处理
	CBFuncEx_t TaskCB;
	const uint8_t *TxBuf;							//大量读写数据的缓存
//	uint8_t *RxBuf;							//大量读写数据的缓存
	uint32_t CSDelayUS;
	uint32_t AddrStart;						//读写的初始地址，或者擦除的地址
	uint32_t DataLen;						//大量读写数据的长度
	uint32_t FinishLen;						//已经写入的长度
	uint32_t Size;							//flash的大小KB
	uint32_t EraseSectorWaitTime;
	uint32_t EraseBlockWaitTime;
	uint32_t ProgramWaitTimeUS;
	uint32_t EraseSectorTime;
	uint32_t EraseBlockTime;
	uint32_t ProgramTime;
	uint8_t State;							//SPIFLASH主流程状态
	uint8_t IsPPErase;						//是否是擦除的编程操作
	uint8_t PPDone;							//SPIFLASH子流程中擦除或者编程开始执行，此位置1且flash不忙碌，说明擦除或者编程真正完成
	uint8_t SpiID;
	uint8_t FlashError;
	uint8_t SPIError;						//spi故障位
	uint8_t FlashSR;						//flash最近一次SR
	uint8_t FlashSRReadTime;				//flash SR最多读取次数
	uint8_t FlashID[3];
	uint8_t Tx[SPIFLASH_RAW_LEN + 6];		//SPI发送缓冲，256+6字节
	uint8_t Rx[SPIFLASH_RAW_LEN + 6];		//SPI接收缓冲，256+6字节
	uint8_t IsBlockMode;					//是否在擦读写操作时采用阻塞模式，如果是0，则需要调用SPIFlash_WaitOpDone来获取结果
	uint8_t IsSpiDMAMode;
	uint8_t CSPin;
}SPIFlash_CtrlStruct;


void SPIFlash_Init(SPIFlash_CtrlStruct *Ctrl, void *Config);
int32_t SPIFlash_ID(SPIFlash_CtrlStruct *Ctrl);
int32_t SPIFlash_ReadSR(SPIFlash_CtrlStruct *Ctrl);
int32_t SPIFlash_WriteSR(SPIFlash_CtrlStruct *Ctrl, uint8_t SR);
int32_t SPIFlash_CheckBusy(SPIFlash_CtrlStruct *Ctrl);
uint8_t SPIFlash_WaitOpDone(SPIFlash_CtrlStruct *Ctrl);
int32_t SPIFlash_WriteEnable(SPIFlash_CtrlStruct *Ctrl);
uint8_t SPIFlash_BusyCheck(SPIFlash_CtrlStruct *Ctrl);
uint8_t SPIFlash_WriteBusyCheck(SPIFlash_CtrlStruct *Ctrl);
void SPIFlash_Reset(SPIFlash_CtrlStruct *Ctrl);
int32_t SPIFlash_Read(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, uint8_t *Buf, uint32_t Length, uint8_t FastRead);
int32_t SPIFlash_Write(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, const uint8_t *Buf, uint32_t Length);
int32_t SPIFlash_Erase(SPIFlash_CtrlStruct *Ctrl, uint32_t Address, uint32_t Length);
int32_t SPIFlash_State(SPIFlash_CtrlStruct *Ctrl);

#endif
