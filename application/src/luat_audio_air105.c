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
#define MP3_FRAME_LEN 4 * 1152
static Audio_StreamStruct prvAudioStream;

int32_t luat_audio_app_cb(void *pData, void *pParam)
{
    rtos_msg_t msg = {0};
	msg.handler = l_multimedia_raw_handler;
	msg.arg1 = (pParam == INVALID_HANDLE_VALUE)?MULTIMEDIA_CB_AUDIO_DONE:MULTIMEDIA_CB_AUDIO_NEED_DATA;
	msg.arg2 = prvAudioStream.pParam;
	luat_msgbus_put(&msg, 1);

}

int32_t luat_audio_play_cb(void *pData, void *pParam)
{
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

	}

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
	return Audio_StartRaw(&prvAudioStream);
}

int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len)
{
	if (len)
		return Audio_WriteRaw(&prvAudioStream, data, len);
	return -1;
}


int luat_audio_stop_raw(uint8_t multimedia_id)
{
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

int luat_audio_play_file(uint8_t multimedia_id, FILE *fd)
{
	luat_audio_stop_raw(multimedia_id);
	uint32_t jump, i;
	uint16_t temp[16];
	mp3dec_t *mp3_decoder;
	int result;
	int audio_format;
	int num_channels;
	int sample_rate;
	int bits_per_sample = 16;
	uint32_t align;
	int is_signed = 1;
    size_t len;
	mp3dec_frame_info_t info;
	luat_fs_fread(temp, 12, 1, fd);
	if (!memcmp(temp, "ID3", 3))
	{
		jump = 0;
		for(i = 0; i < 4; i++)
		{
			jump <<= 7;
			jump |= temp[6 + i] & 0x7f;
		}
//				LLOGD("jump head %d", jump);
		luat_fs_fseek(fd, jump, SEEK_SET);
		mp3_decoder = luat_heap_malloc(sizeof(mp3_decoder));
		mp3dec_init(mp3_decoder);
		OS_InitBuffer(&prvAudioStream.FileDataBuffer, MP3_FRAME_LEN);
		prvAudioStream.FileDataBuffer.Pos = luat_fs_fread(prvAudioStream.FileDataBuffer.Data, MP3_FRAME_LEN, 1, fd);
		result = mp3dec_decode_frame(mp3_decoder, prvAudioStream.FileDataBuffer.Data, prvAudioStream.FileDataBuffer.Pos, NULL, &info);
		memset(mp3_decoder, 0, sizeof(mp3dec_t));
		audio_format = MULTIMEDIA_DATA_TYPE_PCM;
		num_channels = info.channels;
		sample_rate = info.hz;
	}
	else if (!memcmp(temp, "RIFF", 4) || !memcmp(temp + 8, "WAVE", 4))
	{
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
			len = (align * sample_rate >> 3) & ~(3);
			OS_ReInitBuffer(&prvAudioStream.FileDataBuffer, len);
			prvAudioStream.FileDataBuffer.Data = NULL;
			luat_fs_fread(temp, 8, 1, fd);
			if (!memcmp(temp, "fact", 4))
			{
				memcpy(&len, temp + 4, 4);
				luat_fs_fseek(fd, len, SEEK_CUR);
				luat_fs_fread(temp, 8, 1, fd);
			}
			if (memcmp(temp, "data", 4))
			{
				OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
				return -1;
			}
		}
		else
		{
			OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
			return -1;
		}
	}
	else
	{
		OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
		return -1;
	}
	prvAudioStream.fd = fd;
	result = luat_audio_start_raw(multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
	if (result)
	{
		Audio_Stop(&prvAudioStream);
		OS_DeInitBuffer(&prvAudioStream.FileDataBuffer);
		return -1;
	}
}
