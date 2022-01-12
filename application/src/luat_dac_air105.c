#include "luat_base.h"
#include "luat_dac.h"
#include "app_interface.h"

int luat_dac_setup(uint32_t ch, uint32_t freq, uint32_t mode) {
    if (ch != 0)
        return -1;
    GPIO_Iomux(GPIOC_01, 2);
    DAC_Init(0);
    DAC_Setup(freq, mode);
    return 0;
}

int luat_dac_write(uint32_t ch, uint16_t* buff, size_t len) {
    if (ch != 0)
        return -1;
    DAC_Send(buff, len, NULL, NULL);
    return 0;
}

int luat_dac_close(uint32_t ch) {
    return 0; // 暂不支持关闭
}

