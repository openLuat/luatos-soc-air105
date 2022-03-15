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


enum
{
	EV_USER_DHT11_TEST_DONE = USER_EVENT_ID_START + 1,
};
typedef struct
{
	uint32_t LastValue;
	uint8_t Data[6];
	uint8_t Pos;
	uint8_t BitPos;
	uint8_t BytePos;
}DHT11_CtrlStruct;


static int32_t DHT11_ReadBit(void *pData, void *pParam)
{
	OPQueue_CmdStruct *Cmd = (OPQueue_CmdStruct *)pData;
	DHT11_CtrlStruct *DHT11 = (DHT11_CtrlStruct *)pParam;
	uint32_t diff;
	switch(DHT11->Pos)
	{
	case 0:
//		DBG("%u", Cmd->uParam.MaxCnt);
		break;
	case 1:
		DHT11->LastValue = Cmd->uParam.MaxCnt;
		DHT11->BitPos = 7;
		DHT11->BytePos = 0;
		DHT11->Data[DHT11->BytePos] = 0;
		break;
	default:
		diff = (Cmd->uParam.MaxCnt - DHT11->LastValue);
		DHT11->LastValue = Cmd->uParam.MaxCnt;
//		DBG("%u,%u,%u,%u,%u", DHT11->BytePos, DHT11->BitPos, diff, Cmd->uParam.MaxCnt,Cmd->PinOrDelay);
		if (diff > (100 * SYS_TIMER_1US))
		{
			DHT11->Data[DHT11->BytePos] |= (1 << DHT11->BitPos);
		}
		if (DHT11->BitPos)
		{
			DHT11->BitPos--;
		}
		else
		{
			DHT11->BitPos = 7;
			DHT11->BytePos++;
			DHT11->Data[DHT11->BytePos] = 0;
		}
	}
	DHT11->Pos++;
}
static int32_t prvDHT11_TestCB(void *pData, void *pParam)
{
	Task_SendEvent(pParam, EV_USER_DHT11_TEST_DONE, 0, 0, 0);
}

void DHT11_TestOnce(uint8_t Pin)
{
	uint8_t HWTimerID = HW_TIMER0;
	uint32_t i;
	OPQueue_CmdStruct OPCmd;
	HANDLE TaskHandle = Task_GetCurrent();
	HWTimer_InitOperationQueue(HWTimerID, 100, 1, prvDHT11_TestCB, TaskHandle);
	OPCmd.Operation = OP_QUEUE_CMD_SET_GPIO_DIR_IN;
	OPCmd.PinOrDelay = Pin;
	OPCmd.uArg.IOArg.Level = 0;
	OPCmd.uArg.IOArg.PullMode = OP_QUEUE_CMD_IO_PULL_UP;
	HWTimer_AddOperation(HWTimerID, &OPCmd);
	OPCmd.Operation = OP_QUEUE_CMD_ONE_TIME_DELAY;
	OPCmd.PinOrDelay = 0;
	OPCmd.uArg.Time = 60000;
	HWTimer_AddOperation(HWTimerID, &OPCmd);

	OPCmd.Operation = OP_QUEUE_CMD_SET_GPIO_DIR_OUT;
	OPCmd.PinOrDelay = Pin;
	OPCmd.uArg.IOArg.Level = 0;
	OPCmd.uArg.IOArg.PullMode = OP_QUEUE_CMD_IO_PULL_NONE;
	HWTimer_AddOperation(HWTimerID, &OPCmd);
	OPCmd.Operation = OP_QUEUE_CMD_ONE_TIME_DELAY;
	OPCmd.PinOrDelay = 0;
	OPCmd.uArg.Time = 18000;
	HWTimer_AddOperation(HWTimerID, &OPCmd);

	OPCmd.Operation = OP_QUEUE_CMD_CAPTURE_SET;
	OPCmd.PinOrDelay = Pin;
	OPCmd.uArg.ExitArg.ExtiMode = OP_QUEUE_CMD_IO_EXTI_DOWN;
	OPCmd.uArg.ExitArg.PullMode = OP_QUEUE_CMD_IO_PULL_UP;
	OPCmd.uParam.MaxCnt = 100 * SYS_TIMER_1MS;
	HWTimer_AddOperation(HWTimerID, &OPCmd);

	OPCmd.Operation = OP_QUEUE_CMD_CAPTURE;
	OPCmd.PinOrDelay = 0xff;
	OPCmd.uArg.IOArg.Level = 0xff;
	OPCmd.uParam.MaxCnt = 0;
	for(i = 0; i < 42; i++) //40bit data+2bit start+1bit end
	{
		HWTimer_AddOperation(HWTimerID, &OPCmd);
	}
	OPCmd.Operation = OP_QUEUE_CMD_CAPTURE_END;
	OPCmd.PinOrDelay = Pin;
	HWTimer_AddOperation(HWTimerID, &OPCmd);
	OPCmd.Operation = OP_QUEUE_CMD_SET_GPIO_DIR_IN;
	OPCmd.PinOrDelay = Pin;
	OPCmd.uArg.IOArg.Level = 0;
	OPCmd.uArg.IOArg.PullMode = OP_QUEUE_CMD_IO_PULL_UP;
	HWTimer_AddOperation(HWTimerID, &OPCmd);
	HWTimer_StartOperationQueue(HWTimerID);
}

void DHT11_TestResult(void)
{
	uint8_t HWTimerID = HW_TIMER0;
	DHT11_CtrlStruct DHT11;
	DHT11.Pos = 0;
	HWTimer_GetOperationQueueCaptureResult(HWTimerID, DHT11_ReadBit, &DHT11);
	DHT11.Data[5] = SumCheck(DHT11.Data, 4);
	if (DHT11.Data[4] == DHT11.Data[5])
	{
		DBG("湿度 %d.%d, 温度 %d.%d", DHT11.Data[0], DHT11.Data[1], DHT11.Data[2], DHT11.Data[3]);
	}
	else
	{
		DBG("check error %x,%x", DHT11.Data[4], DHT11.Data[5]);
		DBG_HexPrintf(DHT11.Data, 4);
	}
	HWTimer_Stop(HWTimerID);
}

static void prvDHT11_Test(void *p)
{
	OS_EVENT Event;
	while(1)
	{
		DBG("test start");
		DHT11_TestOnce(GPIOD_06);
		Task_GetEvent(Task_GetCurrent(), EV_USER_DHT11_TEST_DONE, &Event, NULL, 200 * SYS_TIMER_1MS);
		DHT11_TestResult();
		Task_DelayMS(1000);
	}

}

void DHT11_TestInit(void)
{
	Task_Create(prvDHT11_Test, NULL, 1024, SERVICE_TASK_PRO, "dht11 task");
}

//INIT_TASK_EXPORT(DHT11_TestInit, "3");
