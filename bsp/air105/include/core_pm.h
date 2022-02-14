#ifndef __CORE_PM_H__
#define __CORE_PM_H__
enum
{
	PM_HW_UART_0,
	PM_HW_UART_1,
	PM_HW_UART_2,
	PM_HW_UART_3,
	PM_HW_UART_4,
	PM_HW_UART_5,
	PM_HW_UART_6,
	PM_HW_UART_7,
	PM_HW_SPI_0,
	PM_HW_SPI_1,
	PM_HW_SPI_2,
	PM_HW_SPI_3,
	PM_HW_SPI_4,
	PM_HW_SPI_5,
	PM_HW_HSPI,
	PM_HW_QSPI,
	PM_HW_TIMER_0,
	PM_HW_TIMER_1,
	PM_HW_TIMER_2,
	PM_HW_TIMER_3,
	PM_HW_TIMER_4,
	PM_HW_TIMER_5,
	PM_HW_TIMER_6,
	PM_HW_TIMER_7,
	PM_HW_I2C_0,
	PM_HW_I2C_1,
	PM_HW_I2C_2,
	PM_HW_I2C_3,
	PM_HW_I2C_4,
	PM_HW_I2C_5,
	PM_HW_DAC_0,
	PM_HW_DCMI_0,
	PM_HW_MAX = 32,
	PM_DRV_USB = 0,
	PM_DRV_DBG,
	PM_DRV_USER,
	PM_DRV_MAX = 32,
};

void PM_Init(void);
void PM_SetHardwareRunFlag(uint32_t PmHWSn, uint32_t OnOff);
void PM_SetDriverRunFlag(uint32_t PmDrvSn, uint32_t OnOff);
int32_t PM_Sleep(void);
#endif
