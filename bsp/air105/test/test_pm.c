#include "user.h"

static int32_t test_done(void *pData, void *pParam)
{
	DBG("RTC唤醒， 测试结束");
	PM_SetDriverRunFlag(PM_DRV_DBG, 1);
}

void prvPM_Test(void *p)
{
	int i;

	DBG("测试在1秒后开始，休眠60秒后RTC唤醒恢复");
	for(i = GPIOA_02; i < GPIO_NONE; i++)
	{
		GPIO_PullConfig(i, 0, 0);
		GPIO_Config(i, 0, 0);
	}
	Task_DelayMS(1000);

	PM_SetDriverRunFlag(PM_DRV_DBG, 0);	//DBG是开机就打开的，如果没有其他外设使用，关闭这个就可以进入低功耗
	RTC_SetAlarm(60, test_done, 1);	//开个RTC唤醒一下，或者键盘，或者GPIO
	while(1)
	{
		Task_DelayMS(1000);
		DBG("UTC %llu", RTC_GetUTC());
	}
}


void PM_TestInit(void)
{
	Task_Create(prvPM_Test, NULL, 1024, SERVICE_TASK_PRO, "pm task");
}

//INIT_TASK_EXPORT(PM_TestInit, "3");
