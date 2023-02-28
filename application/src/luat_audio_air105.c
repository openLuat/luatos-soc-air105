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


#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include <stdlib.h>
#include "luat_multimedia.h"
#include "app_interface.h"
#include "mp3_decode/minimp3.h"
#define LUAT_LOG_TAG "audio"
#include "luat_log.h"
#include "luat_audio.h"
//#define __DECODE_DEBUG__
#ifdef __DECODE_DEBUG__
#define STEP(X)	Stream->DecodeStep=X
#else
#define STEP(X)
#endif
#define MP3_FRAME_LEN 4 * 1152
static Audio_StreamStruct prvAudioStream;

int luat_audio_write_blank_raw(uint32_t multimedia_id, uint8_t cnt);

static int32_t luat_audio_wav_decode(void *pData, void *pParam)
{
	Audio_StreamStruct *Stream = (Audio_StreamStruct *)pData;
	while ( (llist_num(&Stream->DataHead) < 3) && !Stream->IsStop)
	{
		Stream->FileDataBuffer.Pos = luat_fs_fread(Stream->FileDataBuffer.Data, Stream->FileDataBuffer.MaxLen, 1, Stream->fd);
		if (Stream->FileDataBuffer.Pos > 0)
		{
			if (!Stream->IsStop)
			{
				Audio_WriteRaw(Stream, Stream->FileDataBuffer.Data, Stream->FileDataBuffer.Pos, 0);
			}
		}
		else
		{
			return 0;
		}
	}


	return 0;
}

static int32_t luat_audio_mp3_decode(void *pData, void *pParam)
{
	uint32_t pos = 0;
	int read_len;
	int result;
	mp3dec_frame_info_t info;
	mp3dec_t *mp3_decoder = (mp3dec_t *)pParam;
	Audio_StreamStruct *Stream = (Audio_StreamStruct *)pData;
	Stream->AudioDataBuffer.Pos = 0;

	STEP(1);
	while ((llist_num(&Stream->DataHead) < 4) && (Stream->IsFileNotEnd || Stream->FileDataBuffer.Pos > 2)&& !Stream->IsStop)
	{
		STEP(2);
		while (( Stream->AudioDataBuffer.Pos < (Stream->AudioDataBuffer.MaxLen - MP3_FRAME_LEN * 2) ) && (Stream->IsFileNotEnd || Stream->FileDataBuffer.Pos > 2) && !Stream->IsStop)
		{
			STEP(3);
			if (Stream->FileDataBuffer.Pos < MINIMP3_MAX_SAMPLES_PER_FRAME)
			{
				if (Stream->IsFileNotEnd)
				{
					read_len = luat_fs_fread(Stream->FileDataBuffer.Data + Stream->FileDataBuffer.Pos, MINIMP3_MAX_SAMPLES_PER_FRAME, 1, Stream->fd);
					if (read_len > 0)
					{
						Stream->FileDataBuffer.Pos += read_len;
					}
					else
					{
						Stream->IsFileNotEnd = 0;
					}
				}
			}
			pos = 0;
			STEP(4);
			do
			{
				memset(&info, 0, sizeof(info));
				STEP(5);
				result = mp3dec_decode_frame(mp3_decoder, Stream->FileDataBuffer.Data + pos, Stream->FileDataBuffer.Pos - pos,
						(mp3d_sample_t *)&Stream->AudioDataBuffer.Data[Stream->AudioDataBuffer.Pos], &info);
				STEP(6);
				if (result > 0)
				{
					Stream->AudioDataBuffer.Pos += (result * info.channels * 2);
				}
				pos += info.frame_bytes;
				if (Stream->AudioDataBuffer.Pos >= (Stream->AudioDataBuffer.MaxLen - MP3_FRAME_LEN * 2))
				{
					break;
				}
			} while ( ((Stream->FileDataBuffer.Pos - pos) >= (MINIMP3_MAX_SAMPLES_PER_FRAME * Stream->IsFileNotEnd + 1)) && !Stream->IsStop);
			STEP(7);
			OS_BufferRemove(&Stream->FileDataBuffer, pos);
		}
		if (!Stream->IsStop)
		{
			STEP(8);
			Audio_WriteRaw(Stream, Stream->AudioDataBuffer.Data, Stream->AudioDataBuffer.Pos, 0);
			STEP(9);
		}
		Stream->AudioDataBuffer.Pos = 0;
	}
	STEP(0);
	return 0;
}

static int32_t luat_audio_pa_on(void *pData, void *pParam)
{
	if (prvAudioStream.UseOutPA)
	{
		GPIO_Output(prvAudioStream.PAPin, prvAudioStream.PAOnLevel);
	}
}

int32_t luat_audio_decode_run(void *pData, void *pParam)
{
	if (!prvAudioStream.IsStop)
	{
		prvAudioStream.Decoder(pData, pParam);
	}
	if (prvAudioStream.waitRequire)
	{
		prvAudioStream.waitRequire--;
	}
}

int32_t luat_audio_app_cb(void *pData, void *pParam)
{
    rtos_msg_t msg = {0};
	msg.handler = l_multimedia_raw_handler;
	msg.arg1 = (pParam == INVALID_HANDLE_VALUE)?MULTIMEDIA_CB_AUDIO_DONE:MULTIMEDIA_CB_AUDIO_NEED_DATA;
	msg.arg2 = prvAudioStream.pParam;
	luat_msgbus_put(&msg, 1);
	return 0;
}

int32_t luat_audio_play_cb(void *pData, void *pParam)
{
	Audio_StreamStruct *Stream = (Audio_StreamStruct *)pData;
	if (pParam == INVALID_HANDLE_VALUE)
	{
	    rtos_msg_t msg = {0};
		msg.handler = l_multimedia_raw_handler;
		msg.arg1 = MULTIMEDIA_CB_AUDIO_DONE;
		msg.arg2 = prvAudioStream.pParam;
		luat_msgbus_put(&msg, 1);
	}
	else
	{
		if (!prvAudioStream.IsStop)
		{
			prvAudioStream.waitRequire++;
			if (prvAudioStream.waitRequire > 2)
			{
				DBG("%d,%d,%u,%u", prvAudioStream.waitRequire, prvAudioStream.DecodeStep,prvAudioStream.FileDataBuffer.Pos,prvAudioStream.AudioDataBuffer.Pos);
			}
			Core_ServiceRunUserAPIWithFile(luat_audio_decode_run, &prvAudioStream, prvAudioStream.CoderParam);
		}
	}
	return 0;
}

int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	prvAudioStream.pParam = multimedia_id;
	if (prvAudioStream.fd) {
		prvAudioStream.CB = luat_audio_play_cb;
	} else {
		prvAudioStream.CB = luat_audio_app_cb;
	}

	prvAudioStream.BitDepth = bits_per_sample;
	prvAudioStream.BusType = AUSTREAM_BUS_DAC;
	prvAudioStream.BusID = 0;
	prvAudioStream.Format = audio_format;
	prvAudioStream.ChannelCount = num_channels;
	prvAudioStream.SampleRate = sample_rate;
	prvAudioStream.IsDataSigned = is_signed;
	GPIO_Iomux(GPIOC_01, 2);
	int result = Audio_StartRaw(&prvAudioStream);
	if (!result)
	{
		if (prvAudioStream.DummyAudioTime)
		{
			luat_audio_write_blank_raw(multimedia_id, prvAudioStream.DummyAudioTime);
		}
		if (prvAudioStream.PADelayTimer)
		{
			Timer_StartMS(prvAudioStream.PADelayTimer, prvAudioStream.PADelayTime, 0);
		}
	}
	return result;
}

int luat_audio_write_blank_raw(uint32_t multimedia_id, uint8_t cnt)
{
	uint32_t total = ((prvAudioStream.SampleRate / 10) * prvAudioStream.ChannelCount * (prvAudioStream.BitDepth / 8) );
	uint8_t *dummy = malloc(total);
	memset(dummy, 0, total);
	for(int i = 0 ; i < cnt; i++)
	{
		Audio_WriteRaw(&prvAudioStream, dummy, total, 1);
	}
	free(dummy);
	return 0;
}

int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len)
{
	if (len)
		return Audio_WriteRaw(&prvAudioStream, data, len, 0);
	return -1;
}


int luat_audio_stop_raw(uint8_t multimedia_id)
{
	if (prvAudioStream.UseOutPA)
	{
		GPIO_Output(prvAudioStream.PAPin, !prvAudioStream.PAOnLevel);
	}
	if (prvAudioStream.PADelayTimer)
	{
		Timer_Stop(prvAudioStream.PADelayTimer);
	}
	Audio_Stop(&prvAudioStream);
	OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
	OS_DeInitBuffer(&prvAudioStream.AudioDataBuffer);
	if (prvAudioStream.fd)
	{
		luat_fs_fclose(prvAudioStream.fd);
		prvAudioStream.fd = NULL;
	}
	if (prvAudioStream.CoderParam)
	{
		luat_heap_free(prvAudioStream.CoderParam);
		prvAudioStream.CoderParam = NULL;
	}
	prvAudioStream.IsStop = 0;
	prvAudioStream.IsPlaying = 0;
	prvAudioStream.waitRequire = 0;

	return ERROR_NONE;
}

int luat_audio_pause_raw(uint8_t multimedia_id, uint8_t is_pause)
{
	if (is_pause)
	{
		Audio_Pause(&prvAudioStream);
	}
	else
	{
		Audio_Resume(&prvAudioStream);
	}
	return ERROR_NONE;
}

int luat_audio_play_stop(uint8_t multimedia_id)
{
	prvAudioStream.IsStop = 1;
	prvAudioStream.LastError = -1;
}

uint8_t luat_audio_is_finish(uint8_t multimedia_id)
{
	ASSERT(prvAudioStream.waitRequire < 5);
	return !prvAudioStream.waitRequire;
}

int luat_audio_play_multi_files(uint8_t multimedia_id, uData_t *info, uint32_t files_num, uint8_t error_stop)
{
	return -1;
}

int luat_audio_play_file(uint8_t multimedia_id, const char *path)
{
	if (prvAudioStream.IsPlaying)
	{
		luat_audio_stop_raw(multimedia_id);
	}

	uint32_t jump, i;
	uint8_t temp[16];
	mp3dec_t *mp3_decoder;
	int result;
	int audio_format = MULTIMEDIA_DATA_TYPE_PCM;
	int num_channels;
	int sample_rate;
	int bits_per_sample = 16;
	uint32_t align;
	int is_signed = 1;
    size_t len;
	mp3dec_frame_info_t info;
	FILE *fd = luat_fs_fopen(path, "r");
	if (!fd)
	{
		return -1;
	}
	luat_fs_fread(temp, 12, 1, fd);
	if (!memcmp(temp, "ID3", 3) || (temp[0] == 0xff))
	{
		if (temp[0] != 0xff)
		{
			jump = 0;
			for(i = 0; i < 4; i++)
			{
				jump <<= 7;
				jump |= temp[6 + i] & 0x7f;
			}
	//		LLOGD("jump head %d", jump);
			luat_fs_fseek(fd, jump, SEEK_SET);
		}
		else
		{
			luat_fs_fseek(fd, 0, SEEK_SET);
		}
		mp3_decoder = luat_heap_malloc(sizeof(mp3dec_t));
		memset(mp3_decoder, 0, sizeof(mp3dec_t));
		mp3dec_init(mp3_decoder);
		OS_InitBuffer(&prvAudioStream.FileDataBuffer, MP3_FRAME_LEN);
		prvAudioStream.FileDataBuffer.Pos = luat_fs_fread(prvAudioStream.FileDataBuffer.Data, MP3_FRAME_LEN, 1, fd);
		result = mp3dec_decode_frame(mp3_decoder, prvAudioStream.FileDataBuffer.Data, prvAudioStream.FileDataBuffer.Pos, NULL, &info);
		if (result)
		{
			prvAudioStream.CoderParam = mp3_decoder;

			memset(mp3_decoder, 0, sizeof(mp3dec_t));
			num_channels = info.channels;
			sample_rate = info.hz;
			len = (num_channels * sample_rate >> 2);	//一次时间为1/16秒
			OS_ReInitBuffer(&prvAudioStream.AudioDataBuffer, len + MP3_FRAME_LEN * 2);	//防止溢出，需要多一点空间
			prvAudioStream.Decoder = luat_audio_mp3_decode;
		}
		else
		{
			luat_heap_free(mp3_decoder);
		}
	}
	else if (!memcmp(temp, "RIFF", 4) || !memcmp(temp + 8, "WAVE", 4))
	{
		result = 0;
		prvAudioStream.CoderParam = NULL;
		OS_DeInitBuffer(&prvAudioStream.AudioDataBuffer);
		luat_fs_fread(temp, 8, 1, fd);
		if (!memcmp(temp, "fmt ", 4))
		{
			memcpy(&len, temp + 4, 4);
			OS_InitBuffer(&prvAudioStream.FileDataBuffer, len);
			luat_fs_fread(prvAudioStream.FileDataBuffer.Data, len, 1, fd);
			audio_format = prvAudioStream.FileDataBuffer.Data[0];
			num_channels = prvAudioStream.FileDataBuffer.Data[2];
			memcpy(&sample_rate, prvAudioStream.FileDataBuffer.Data + 4, 4);
			align = prvAudioStream.FileDataBuffer.Data[12];
			bits_per_sample = prvAudioStream.FileDataBuffer.Data[14];
			len = ((align * sample_rate) >> 3) & ~(3);	//一次时间为1/8秒
			OS_ReInitBuffer(&prvAudioStream.FileDataBuffer, len);
			luat_fs_fread(temp, 8, 1, fd);
			if (!memcmp(temp, "fact", 4))
			{
				memcpy(&len, temp + 4, 4);
				luat_fs_fseek(fd, len, SEEK_CUR);
				luat_fs_fread(temp, 8, 1, fd);
			}
			if (!memcmp(temp, "data", 4))
			{
				result = 1;
				prvAudioStream.Decoder = luat_audio_wav_decode;
			}
		}
	}
	else
	{
		result = 0;
	}
	if (result)
	{
		prvAudioStream.fd = fd;
		result = luat_audio_start_raw(multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
		if (!result)
		{
			LLOGD("decode %s ok,param,%d,%d,%d,%d,%d,%d,%u,%u", path,multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed, prvAudioStream.FileDataBuffer.MaxLen, prvAudioStream.AudioDataBuffer.MaxLen);
			prvAudioStream.IsPlaying = 1;
			prvAudioStream.pParam = multimedia_id;
			prvAudioStream.IsFileNotEnd = 1;
			prvAudioStream.Decoder(&prvAudioStream, prvAudioStream.CoderParam);
			prvAudioStream.LastError = 0;
			if (!llist_num(&prvAudioStream.DataHead))
			{
				prvAudioStream.fd = NULL;
			}
			else
			{
				return 0;
			}

		}
		else
		{
			prvAudioStream.fd = 0;
		}
	}
	luat_fs_fclose(fd);
	OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
	OS_DeInitBuffer(&prvAudioStream.AudioDataBuffer);
	if (prvAudioStream.CoderParam)
	{
		luat_heap_free(prvAudioStream.CoderParam);
		prvAudioStream.CoderParam = NULL;
	}
	return -1;
}

void luat_audio_config_pa(uint8_t multimedia_id, uint32_t pin, int level, uint32_t dummy_time_len, uint32_t pa_delay_time)
{
	if (pin < GPIO_NONE)
	{
		prvAudioStream.PAPin = pin;
		prvAudioStream.PAOnLevel = level;
		prvAudioStream.UseOutPA = 1;
		GPIO_Config(prvAudioStream.PAPin, 0, !prvAudioStream.PAOnLevel);
	}
	else
	{
		prvAudioStream.UseOutPA = 0;
	}
	prvAudioStream.DummyAudioTime = dummy_time_len;
	prvAudioStream.PADelayTime = pa_delay_time;
	if (!prvAudioStream.PADelayTimer && prvAudioStream.PADelayTime)
	{
		prvAudioStream.PADelayTimer = Timer_Create(luat_audio_pa_on, NULL, NULL);
	}
}

void luat_audio_config_dac(uint8_t multimedia_id, int pin, int level, uint32_t dac_off_delay_time)
{

}

uint16_t luat_audio_vol(uint8_t multimedia_id, uint16_t vol)
{
	return 100;
}

int luat_audio_play_get_last_error(uint8_t multimedia_id)
{
	return prvAudioStream.LastError;
}
