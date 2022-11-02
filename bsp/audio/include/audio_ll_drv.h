#ifndef __AUDIO_LL_DRV_H__
#define __AUDIO_LL_DRV_H__

typedef enum
{
    AUSTREAM_FORMAT_UNKNOWN, ///< placeholder for unknown format
    AUSTREAM_FORMAT_PCM,     ///< raw PCM data
    AUSTREAM_FORMAT_WAVPCM,  ///< WAV, PCM inside
    AUSTREAM_FORMAT_MP3,     ///< MP3
    AUSTREAM_FORMAT_AMRNB,   ///< AMR-NB
    AUSTREAM_FORMAT_AMRWB,   ///< AMR_WB
    AUSTREAM_FORMAT_SBC,     ///< bt SBC
} auStreamFormat_t;

typedef enum
{
    AUSTREAM_BUS_DAC,
	AUSTREAM_BUS_I2S,
} auStreamBusType_t;

typedef struct
{
	CBFuncEx_t CB;	//pData是自身Audio_StreamStruct指针
	CBFuncEx_t Decoder;
	CBFuncEx_t Encoder;
	void *pParam;
	void *fd;
	void *CoderParam;
	void *UserParam;
	Timer_t *PADelayTimer;
	Buffer_Struct FileDataBuffer;
	Buffer_Struct AudioDataBuffer;
	llist_head DataHead;
	uint32_t SampleRate;
	uint32_t waitRequire;
	uint32_t DummyAudioTime;
	uint32_t PADelayTime;
	uint8_t BitDepth;
	uint8_t ChannelCount;	//声道，目前只有1或者2
	auStreamFormat_t Format;
	auStreamBusType_t BusType;	//音频总线类型，DAC, IIS之类的
	uint8_t BusID;		//音频总线ID
	uint8_t IsDataSigned;	//数据是否是有符号的
	uint8_t IsHardwareRun;
	uint8_t IsPause;
	uint8_t IsStop;
	uint8_t IsPlaying;
	uint8_t IsFileNotEnd;
	uint8_t DecodeStep;
	uint8_t UseOutPA;
	uint8_t PAPin;
	uint8_t PAOnLevel;
}Audio_StreamStruct;

void Audio_GlobalInit(void);
/**
 * @brief 开始播放原始音频流
 *
 * @param pStream 原始音频流数据结构, 底层不保存这个结构，需要用户保存
 * @return =0 成功 < 0失败错误码
 */
int32_t Audio_StartRaw(Audio_StreamStruct *pStream);

int32_t Audio_WriteRaw(Audio_StreamStruct *pStream, uint8_t *pByteData, uint32_t ByteLen, uint8_t AddHead);

void Audio_Stop(Audio_StreamStruct *pStream);

void Audio_Pause(Audio_StreamStruct *pStream);
void Audio_Resume(Audio_StreamStruct *pStream);
#endif
