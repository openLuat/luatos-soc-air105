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

static Audio_StreamStruct prvAudioStream;
int32_t luat_audio_app_cb(void *pData, void *pParam)
{
    rtos_msg_t msg = {0};
	msg.handler = l_multimedia_raw_handler;
	msg.arg1 = (pParam == INVALID_HANDLE_VALUE)?MULTIMEDIA_CB_AUDIO_DONE:MULTIMEDIA_CB_AUDIO_NEED_DATA;
	msg.arg2 = prvAudioStream.pParam;
	luat_msgbus_put(&msg, 1);

}

int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	prvAudioStream.pParam = multimedia_id;
	prvAudioStream.CB = luat_audio_app_cb;
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
	return Audio_WriteRaw(&prvAudioStream, data, len);
}


int luat_audio_stop_raw(uint8_t multimedia_id)
{
	Audio_Stop(&prvAudioStream);
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
