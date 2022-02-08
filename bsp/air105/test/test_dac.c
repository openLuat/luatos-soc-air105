#include "user.h"
#include "wave_data.h"
extern uint16_t wavData[WAVE_DATA_SIZE/2];

static int DAC_TestCB(void *pData, void *pParam)
{
	DBG("%d", DAC_CheckRun((uint32_t)pData));
	DAC_Send(0, wavData, WAVE_DATA_SIZE/2, DAC_TestCB, 0);
}

void DAC_Test(void)
{
	GPIO_Iomux(GPIOC_00, 2);
	wave_DataHandle();
	DAC_DMAInit(0, DAC_TX_DMA_STREAM);
	DAC_Setup(0, SAMPLE_RATE, 1);
	DAC_Send(0, wavData, WAVE_DATA_SIZE/2, DAC_TestCB, 0);

}
