
#ifndef __HT_MCU_API_H__
#define __HT_MCU_API_H__

#include "main.h"
#include "rtc.h"
#include "dma.h"
#include "usart.h"
#include "HT_P2P_FEM.h"

void HT_McuApi_configPeripherals(void);
void HT_McuApi_EnableRTCWkp(uint32_t seconds);
void HT_McuApi_DeepSleepMode(void);

#endif /* __HT_MCU_API_H__ */
