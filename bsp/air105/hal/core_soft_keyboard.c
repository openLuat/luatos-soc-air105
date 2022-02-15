#include "user.h"
typedef struct
{
	CBFuncEx_t CB;
	void *pParam;
	Timer_t *ScanTimer;
	uint32_t ScanPeriod;			//扫描周期，US
	uint8_t ScanTimes;				//一次判决所需周期
	uint8_t PressConfirmTimes;		//判决按下所需周期
	uint8_t InIO[16];
	uint8_t OutIO[16];
	uint8_t InIONum;
	uint8_t OutIONum;
	uint8_t PressKeyIOLevel;
	uint8_t IrqMode;
	uint8_t IsScan;
	uint8_t IsEnable;
	uint8_t Init;
	uint8_t ScanCnt;
	uint8_t KeyPress[256];
	uint8_t KeyState[32];
}SoftKB_CtrlStruct;
static SoftKB_CtrlStruct prvSoftKB;

static int32_t prvSoftKB_DummyCB(void *pData, void *pParam)
{
	DBG("%x", pData);
	return 0;
}


static int32_t prvSoftKB_IOIrqCB(void *pData, void *pParam)
{
	int i;
	for(i = 0; i < prvSoftKB.InIONum; i++)
	{
		GPIO_ExtiConfig(prvSoftKB.InIO[i], 0, 0, 0);

	}
	for(i = 0; i < prvSoftKB.OutIONum; i++)
	{
		GPIO_ODConfig(prvSoftKB.OutIO[i], !prvSoftKB.PressKeyIOLevel);
	}

	if (prvSoftKB.IsEnable)
	{
		Timer_StartUS(prvSoftKB.ScanTimer, prvSoftKB.ScanPeriod, 1);
		prvSoftKB.IsScan = 1;
		prvSoftKB.ScanCnt = 0;
	}
	return 0;
}

static int32_t prvSoftKB_TimerIrqCB(void *pData, void *pParam)
{
	if (!prvSoftKB.IsScan || !prvSoftKB.IsEnable)
	{
		Timer_Stop(prvSoftKB.ScanTimer);
		return -1;
	}
#ifdef __BUILD_OS__
	if (prvSoftKB.IrqMode)
	{
#endif
		SoftKB_ScanOnce();
#ifdef __BUILD_OS__
	}
	else
	{
		Core_ScanKeyBoard();
	}
#endif
	return 0;
}

void SoftKB_Setup(uint32_t ScanPeriodUS, uint8_t ScanTimes, uint8_t PressConfirmTimes, uint8_t IsIrqMode, CBFuncEx_t CB, void *pParam)
{
	prvSoftKB.ScanPeriod = ScanPeriodUS;
	prvSoftKB.ScanTimes = ScanTimes;
	prvSoftKB.PressConfirmTimes = PressConfirmTimes;
	if (CB)
	{
		prvSoftKB.CB = CB;
	}
	else
	{
		prvSoftKB.CB = prvSoftKB_DummyCB;
	}
	prvSoftKB.pParam = pParam;
	prvSoftKB.Init |= 0xf0;
	prvSoftKB.IrqMode = IsIrqMode;
}

void SoftKB_IOConfig(const uint8_t *InIO, uint8_t InIONum, const uint8_t *OutIO, uint8_t OutIONum, uint8_t PressKeyIOLevel)
{
	if (InIONum > 16 || OutIONum > 16) return;
	if (InIO)
	{
		memset(prvSoftKB.InIO, 0xff, sizeof(prvSoftKB.InIO));
		memcpy(prvSoftKB.InIO, InIO, InIONum);
		prvSoftKB.InIONum = InIONum;
		prvSoftKB.Init |= 0x03;
	}
	if (OutIO)
	{
		memset(prvSoftKB.OutIO, 0xff, sizeof(prvSoftKB.OutIO));
		memcpy(prvSoftKB.OutIO, OutIO, OutIONum);
		prvSoftKB.OutIONum = OutIONum;
		prvSoftKB.Init |= 0x0c;
	}
	prvSoftKB.PressKeyIOLevel = PressKeyIOLevel;

}

void SoftKB_Start(void)
{
	int i;
	if (!prvSoftKB.ScanTimer)
	{
		prvSoftKB.ScanTimer = Timer_Create(prvSoftKB_TimerIrqCB, NULL, NULL);
	}
	if (prvSoftKB.Init != 0xff)
	{
		return;
	}
	memset(prvSoftKB.KeyPress, 0, sizeof(prvSoftKB.KeyPress));
	memset(prvSoftKB.KeyState, 0, sizeof(prvSoftKB.KeyState));
	prvSoftKB.IsScan = 0;
	prvSoftKB.ScanCnt = 0;
	for(i = 0; i < prvSoftKB.OutIONum; i++)
	{
		GPIO_ODConfig(prvSoftKB.OutIO[i], prvSoftKB.PressKeyIOLevel);
	}
	for(i = 0; i < prvSoftKB.InIONum; i++)
	{
		GPIO_Config(prvSoftKB.InIO[i], 1, 0);
		GPIO_PullConfig(prvSoftKB.InIO[i], 1, !prvSoftKB.PressKeyIOLevel);
		if (GPIO_Input(prvSoftKB.InIO[i]) == prvSoftKB.PressKeyIOLevel)
		{
			prvSoftKB.IsScan = 1;
		}
		GPIO_ExtiSetCB(prvSoftKB.InIO[i], prvSoftKB_IOIrqCB, NULL);
	}
	if (prvSoftKB.IsScan)
	{
		Timer_StartUS(prvSoftKB.ScanTimer, prvSoftKB.ScanPeriod, 1);
	}
	else
	{
		for(i = 0; i < prvSoftKB.InIONum; i++)
		{
			GPIO_ExtiConfig(prvSoftKB.InIO[i], 0, prvSoftKB.PressKeyIOLevel, !prvSoftKB.PressKeyIOLevel);
		}
	}
	prvSoftKB.IsEnable = 1;
}

void SoftKB_Stop(void)
{
	int i;
	if (!prvSoftKB.ScanTimer)
	{
		prvSoftKB.ScanTimer = Timer_Create(prvSoftKB_TimerIrqCB, NULL, NULL);
	}
	else
	{
		Timer_Stop(prvSoftKB.ScanTimer);
	}
	for(i = 0; i < prvSoftKB.InIONum; i++)
	{
		GPIO_ExtiConfig(prvSoftKB.InIO[i], 0, 0, 0);
		GPIO_ExtiSetCB(prvSoftKB.InIO[i], NULL, NULL);
	}
	for(i = 0; i < prvSoftKB.OutIONum; i++)
	{
		GPIO_Output(prvSoftKB.OutIO[i], prvSoftKB.PressKeyIOLevel);
	}
	prvSoftKB.IsEnable = 0;
}

void SoftKB_ScanOnce(void)
{
	int i, j;
	uint32_t KeySn;
	if (!prvSoftKB.IsEnable || !prvSoftKB.IsScan)
	{
		return;
	}
	for(i = 0; i < prvSoftKB.OutIONum; i++)
	{
		GPIO_Output(prvSoftKB.OutIO[i], prvSoftKB.PressKeyIOLevel);
		for(j = 0; j < prvSoftKB.InIONum; j++)
		{
			//DBG("%d,%d,%d", i, j, GPIO_Input(prvSoftKB.InIO[j]));
			prvSoftKB.KeyPress[(i << 4) + j] += (GPIO_Input(prvSoftKB.InIO[j]) == prvSoftKB.PressKeyIOLevel);
		}
		GPIO_Output(prvSoftKB.OutIO[i], !prvSoftKB.PressKeyIOLevel);
	}
	prvSoftKB.ScanCnt++;
	if (prvSoftKB.ScanCnt >= prvSoftKB.ScanTimes)
	{
		prvSoftKB.ScanCnt = 0;
		for(i = 0; i < prvSoftKB.OutIONum; i++)
		{
			for(j = 0; j < prvSoftKB.InIONum; j++)
			{
				KeySn = (i << 4) + j;
				if (prvSoftKB.KeyPress[KeySn] >= prvSoftKB.PressConfirmTimes)
				{
					if (!BSP_TestBit(prvSoftKB.KeyState, KeySn))
					{
						BSP_SetBit(prvSoftKB.KeyState, KeySn, 1);
						prvSoftKB.CB(KeySn|(1 << 16), prvSoftKB.pParam);
					}
				}
				else
				{
					if (BSP_TestBit(prvSoftKB.KeyState, KeySn))
					{
						BSP_SetBit(prvSoftKB.KeyState, KeySn, 0);
						prvSoftKB.CB(KeySn, prvSoftKB.pParam);
					}
				}
			}
		}
		memset(prvSoftKB.KeyPress, 0, sizeof(prvSoftKB.KeyPress));
		for(i = 0; i < sizeof(prvSoftKB.KeyState); i++)
		{
			if (prvSoftKB.KeyState[i])
			{
				return;
			}
		}
		Timer_Stop(prvSoftKB.ScanTimer);
		prvSoftKB.IsScan = 0;
		for(i = 0; i < prvSoftKB.OutIONum; i++)
		{
			GPIO_Output(prvSoftKB.OutIO[i], prvSoftKB.PressKeyIOLevel);
		}
		for(i = 0; i < prvSoftKB.InIONum; i++)
		{
			GPIO_ExtiConfig(prvSoftKB.InIO[i], 0, prvSoftKB.PressKeyIOLevel, !prvSoftKB.PressKeyIOLevel);
		}

	}
}


void SoftKB_SetCB(CBFuncEx_t CB, void *pParam)
{
	if (CB)
	{
		prvSoftKB.CB = CB;
	}
	else
	{
		prvSoftKB.CB = prvSoftKB_DummyCB;
	}
	prvSoftKB.pParam = pParam;
}

void SoftKB_Test(void)
{
	uint8_t In[4] = {GPIOD_10, GPIOE_00, GPIOE_01, GPIOE_02};
	uint8_t Out[4] = {GPIOD_12, GPIOD_13, GPIOD_14, GPIOD_15};
	SoftKB_Setup(6250, 4, 2, 0, NULL, NULL);
	SoftKB_IOConfig(In, 4, Out, 4, 0);
	SoftKB_Start();
}

//INIT_TASK_EXPORT(SoftKB_Test, "3");
