#ifndef __WAVE_DATA_H_
#define __WAVE_DATA_H_

#include <stdint.h>

#define SAMPLE_RATE          22050      /* sample rate��22050Hz */
#define WAVE_DATA_SIZE       (0x19442)

extern uint16_t wavData[WAVE_DATA_SIZE >> 1];

void wave_DataHandle(void);

#endif   ///< __WAVE_DATA_H_

