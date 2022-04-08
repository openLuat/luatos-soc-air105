#ifndef __DEV_SDHC_SPI_H__
#define __DEV_SDHC_SPI_H__

#define __SDHC_BLOCK_LEN__	(512)

typedef struct
{
  uint8_t  Reserved1:2;               /* Reserved */
  uint16_t DeviceSize:12;             /* Device Size */
  uint8_t  MaxRdCurrentVDDMin:3;      /* Max. read current @ VDD min */
  uint8_t  MaxRdCurrentVDDMax:3;      /* Max. read current @ VDD max */
  uint8_t  MaxWrCurrentVDDMin:3;      /* Max. write current @ VDD min */
  uint8_t  MaxWrCurrentVDDMax:3;      /* Max. write current @ VDD max */
  uint8_t  DeviceSizeMul:3;           /* Device size multiplier */
} struct_v1;


typedef struct
{
  uint8_t  Reserved1:6;               /* Reserved */
  uint32_t DeviceSize:22;             /* Device Size */
  uint8_t  Reserved2:1;               /* Reserved */
} struct_v2;

/**
  * @brief  Card Specific Data: CSD Register
  */
typedef struct
{
  /* Header part */
  uint8_t  CSDStruct:2;            /* CSD structure */
  uint8_t  Reserved1:6;            /* Reserved */
  uint8_t  TAAC:8;                 /* Data read access-time 1 */
  uint8_t  NSAC:8;                 /* Data read access-time 2 in CLK cycles */
  uint8_t  MaxBusClkFrec:8;        /* Max. bus clock frequency */
  uint16_t CardComdClasses:12;      /* Card command classes */
  uint8_t  RdBlockLen:4;           /* Max. read data block length */
  uint8_t  PartBlockRead:1;        /* Partial blocks for read allowed */
  uint8_t  WrBlockMisalign:1;      /* Write block misalignment */
  uint8_t  RdBlockMisalign:1;      /* Read block misalignment */
  uint8_t  DSRImpl:1;              /* DSR implemented */

  /* v1 or v2 struct */
  union csd_version {
    struct_v1 v1;
    struct_v2 v2;
  } version;

  uint8_t  EraseSingleBlockEnable:1;  /* Erase single block enable */
  uint8_t  EraseSectorSize:7;         /* Erase group size multiplier */
  uint8_t  WrProtectGrSize:7;         /* Write protect group size */
  uint8_t  WrProtectGrEnable:1;       /* Write protect group enable */
  uint8_t  Reserved2:2;               /* Reserved */
  uint8_t  WrSpeedFact:3;             /* Write speed factor */
  uint8_t  MaxWrBlockLen:4;           /* Max. write data block length */
  uint8_t  WriteBlockPartial:1;       /* Partial blocks for write allowed */
  uint8_t  Reserved3:5;               /* Reserved */
  uint8_t  FileFormatGrouop:1;        /* File format group */
  uint8_t  CopyFlag:1;                /* Copy flag (OTP) */
  uint8_t  PermWrProtect:1;           /* Permanent write protection */
  uint8_t  TempWrProtect:1;           /* Temporary write protection */
  uint8_t  FileFormat:2;              /* File Format */
  uint8_t  Reserved4:2;               /* Reserved */
  uint8_t  crc:7;                     /* Reserved */
  uint8_t  Reserved5:1;               /* always 1*/

} SD_CSD;

/**
  * @brief  Card Identification Data: CID Register
  */
typedef struct
{
  __IO uint8_t  ManufacturerID;       /* ManufacturerID */
  __IO uint16_t OEM_AppliID;          /* OEM/Application ID */
  __IO uint32_t ProdName1;            /* Product Name part1 */
  __IO uint8_t  ProdName2;            /* Product Name part2*/
  __IO uint8_t  ProdRev;              /* Product Revision */
  __IO uint32_t ProdSN;               /* Product Serial Number */
  __IO uint8_t  Reserved1;            /* Reserved1 */
  __IO uint16_t ManufactDate;         /* Manufacturing Date */
  __IO uint8_t  CID_CRC;              /* CID CRC */
  __IO uint8_t  Reserved2;            /* always 1 */
} SD_CID;

/**
  * @brief SD Card information
  */
typedef struct
{
  SD_CSD Csd;
  SD_CID Cid;
  uint64_t CardCapacity;              /*!< Card Capacity */
  uint32_t LogBlockNbr;               /*!< Specifies the Card logical Capacity in blocks   */
  uint32_t CardBlockSize;             /*!< Card Block Size */
  uint32_t LogBlockSize;              /*!< Specifies logical block size in bytes           */
} SD_CardInfo;

typedef struct
{
	SD_CardInfo Info;
	uint64_t SDHCReadBlockTo;
	uint64_t SDHCWriteBlockTo;
	Buffer_Struct DataBuf;
	Buffer_Struct CacheBuf;
	HANDLE NotifyTask;						//设置了NotifyTask，则会在大量传输SPI数据时，休眠任务但是仍然能接收Event并在CB中处理
	CBFuncEx_t TaskCB;
	HANDLE RWMutex;
	uint32_t Size;							//flash的大小KB
	uint32_t OCR;
	DBuffer_Struct *SCSIDataBuf;
	uint32_t PreCurBlock;
	uint32_t PreEndBlock;
	uint32_t CurBlock;
	uint32_t EndBlock;
	uint16_t WriteWaitCnt;
	uint8_t CSPin;
	uint8_t SpiID;
	uint8_t IsSpiDMAMode;
	uint8_t SDHCState;
	uint8_t IsInitDone;
	uint8_t IsCRCCheck;
	uint8_t SDHCError;
	uint8_t SPIError;
	uint8_t ExternResult[8];
	uint8_t ExternLen;
	uint8_t TempData[__SDHC_BLOCK_LEN__ + 8];
	uint8_t IsPrintData;
	uint8_t IsMMC;
	uint8_t USBDelayTime;
	uint8_t WaitFree;
}SDHC_SPICtrlStruct;



void SDHC_SpiInitCard(void *pSDHC);
void SDHC_SpiReadCardConfig(void *pSDHC);
void SDHC_SpiReadCardInfo(void *pSDHC);
void SDHC_SpiWriteBlocks(void *pSDHC, const uint8_t *Buf, uint32_t StartLBA, uint32_t BlockNums);
void SDHC_SpiReadBlocks(void *pSDHC, uint8_t *Buf, uint32_t StartLBA, uint32_t BlockNums);
void *SDHC_SpiCreate(uint8_t SpiID, uint8_t CSPin);
void SDHC_SpiRelease(void *pSDHC);
uint8_t SDHC_IsReady(void *pSDHC);
uint32_t SDHC_GetLogBlockNbr(void *pSDHC);
#endif
