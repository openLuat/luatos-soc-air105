#include "user.h"

static int32_t prvEventCB(void *pData, void *pParam)
{
	DBG("!");
	return 0;
}

void prvSDHC_Test(void *p)
{
	int i, ok;
	uint32_t *Buf = malloc(65536);
	uint32_t *Buf2 = malloc(65536);
	SDHC_SPICtrlStruct SDHC;
	memset(&SDHC, 0, sizeof(SDHC));
	SDHC.SpiID = SPI_ID0;
	SDHC.CSPin = GPIOC_13;
	SDHC.IsSpiDMAMode = 0;
//	SDHC.NotifyTask = Task_GetCurrent();
//	SDHC.TaskCB = prvEventCB;
    SDHC.SDHCReadBlockTo = 5 * CORE_TICK_1MS;
    SDHC.SDHCWriteBlockTo = 25 * CORE_TICK_1MS;
    SDHC.IsPrintData = 0;
    GPIO_Iomux(GPIOC_12,2);
    GPIO_Iomux(GPIOC_14,2);
    GPIO_Iomux(GPIOC_15,2);
    GPIO_Config(SDHC.CSPin, 0, 1);
    SPI_MasterInit(SDHC.SpiID, 8, SPI_MODE_0, 400000, NULL, NULL);
#ifdef __BUILD_OS__
    SDHC.RWMutex = OS_MutexCreateUnlock();
#endif
    SDHC_SpiInitCard(&SDHC);
    if (SDHC.IsInitDone)
    {
    	ok = 1;
    	SPI_SetNewConfig(SDHC.SpiID, 24000000, SPI_MODE_0);
    	SDHC_SpiReadCardConfig(&SDHC);
    	DBG("卡容量 %ublock", SDHC.Info.LogBlockNbr);
    	for(i = 0; i < 16384; i++)
    	{
    		Buf[i] = i;
    	}
    	SDHC_SpiWriteBlocks(&SDHC, Buf, 0x1000, 128);
    	SDHC_SpiReadBlocks(&SDHC, Buf2, 0x1000, 128);
    	for(i = 0; i < 16384; i++)
    	{
    		if (Buf[i] != Buf2[i])
    		{
    			DBG("error %u,%u,%u", i, Buf[i], Buf2[i]);
    			ok = 0;
    			break;
    		}
    	}
    	if (ok)
    	{
    		DBG("TEST OK!");
    	}
    }

	while(1)
	{
		Task_DelayMS(1000);

	}
}

void SDHC_TestInit(void)
{
	Task_Create(prvSDHC_Test, NULL, 1024, SERVICE_TASK_PRO, "sdhc task");
}

//INIT_TASK_EXPORT(SDHC_TestInit, "3");
