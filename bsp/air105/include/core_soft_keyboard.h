#ifndef __CORE_SOFT_KEYBOARD_H__
#define __CORE_SOFT_KEYBOARD_H__
void SoftKB_Setup(uint32_t ScanPeriodUS, uint8_t ScanTimes, uint8_t PressConfirmTimes, uint8_t IsIrqMode, CBFuncEx_t CB, void *pParam);
void SoftKB_IOConfig(const uint8_t *InIO, uint8_t InIONum, const uint8_t *OutIO, uint8_t OutIONum, uint8_t PressKeyIOLevel);
void SoftKB_Start(void);
void SoftKB_Stop(void);
void SoftKB_ScanOnce(void);
#endif
