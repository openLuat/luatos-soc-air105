#include "user.h"
#include "wave_data.h"
#define ONCE_READ_LEN	(32 * 1024)
extern const uint16_t wavData[WAVE_DATA_SIZE];
static Audio_StreamStruct prvStream;
//下面模拟从文件读取
static uint8_t *pBuf;
static uint32_t TotalLen = WAVE_DATA_SIZE * 2;
static uint32_t ReadLen;
static int32_t Audio_TestCB(void *pData, void *pParam)
{
	if (pParam == INVALID_HANDLE_VALUE)
	{
		DBG("finish!");
		//在这里通知某个线程所有数据都传输完成，可以停止了，如果是DAC，需要用DAC_Stop来等待彻底停止，或者延迟一小段时间，再用DAC_Stop
	}
	if (ReadLen >= TotalLen)
	{
		return 0;
	}
	//在这里通知某个线程需要获取更多数据
	if ((TotalLen - ReadLen) > ONCE_READ_LEN)
	{
		Audio_WriteRaw(&prvStream, pBuf + ReadLen, ONCE_READ_LEN, 0);
		ReadLen += ONCE_READ_LEN;
	}
	else
	{
		Audio_WriteRaw(&prvStream, pBuf + ReadLen, (TotalLen - ReadLen), 0);
		ReadLen = TotalLen;
	}
	return 0;
}

void Audio_Test(void)
{
	prvStream.BitDepth = 16;
	prvStream.BusType = AUSTREAM_BUS_DAC;
	prvStream.BusID = 0;
	prvStream.Format = AUSTREAM_FORMAT_PCM;
	prvStream.CB = Audio_TestCB;
	prvStream.ChannelCount = 1;
	prvStream.SampleRate = SAMPLE_RATE;
	prvStream.IsDataSigned = 1;
	DBG("%d", Audio_StartRaw(&prvStream));
	ReadLen = 0;
	pBuf = wavData;
	//内存充足的情况下，可以连续写入2次，第一块发送完成后，会立刻发送第二块， 同时回调让用户输入更多数据
	Audio_WriteRaw(&prvStream, pBuf, ONCE_READ_LEN, 0);
	Audio_WriteRaw(&prvStream, pBuf + ONCE_READ_LEN, ONCE_READ_LEN, 0);
	ReadLen += 2 * ONCE_READ_LEN;
}

