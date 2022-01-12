#include "luat_adc.h"
#include "luat_base.h"

#include "core_adc.h"
#include "platform_define.h"

#define LUAT_LOG_TAG "luat.adc"
#include "luat_log.h"

int luat_adc_open(int ch, void *args)
{
    switch (ch)
    {
    case 0:
        return 0;
    case 1:
        GPIO_Iomux(GPIOC_00, 2);
        break;
    case 2:
        GPIO_Iomux(GPIOC_01, 2);
        break;
    case 3:
        return -1;
    case 4:
        GPIO_Iomux(GPIOC_03, 2);
        break;
    case 5:
        GPIO_Iomux(GPIOC_04, 2);
        break;
    case 6:
        GPIO_Iomux(GPIOC_05, 2);
        break;
    default:
        return -1;
    }
    ADC_ChannelOnOff(ch, 1);
    return 0;
}

int luat_adc_read(int ch, int *val, int *val2)
{
    int voltage = 0;
    voltage = ADC_GetChannelValue(ch);
    *val = voltage;
    *val2 = voltage;
    return 0;
}

int luat_adc_close(int ch){
    if (ch)
        ADC_ChannelOnOff(ch, 0);
    return 0;
}
