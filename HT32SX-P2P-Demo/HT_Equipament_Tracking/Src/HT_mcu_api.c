

#include "HT_mcu_api.h"
#include "HT_P2P_s2lp_config.h"
#include "HT_Equipment_App.h"

uint8_t deepSleepModeFlag = 0;

extern HT_APP_MasterFsm master_state;

void HT_McuApi_ConfigPeripherals(void) {

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();

	MX_SPI1_Init();
	HAL_SPI_MspInit(&hspi1);
	__HAL_SPI_DISABLE(&hspi1);
	__HAL_SPI_ENABLE(&hspi1);

	MX_ADC_Init();
	HAL_ADC_MspInit(&hadc);
	__HAL_ADC_DISABLE(&hadc);
	__HAL_ADC_ENABLE(&hadc);

	MX_USART1_UART_Init();
	HAL_UART_MspInit(&huart1);
	__HAL_UART_DISABLE(&huart1);
	__HAL_UART_ENABLE(&huart1);
}

void HT_McuApi_EnterGpioLowPower(void) {
	/* Set all the GPIOs in low power mode (input analog) */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;

	GPIO_InitStructure.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = GPIO_PIN_All & (~GPIO_PIN_15);
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = GPIO_PIN_All & (~GPIO_PIN_8) & (~GPIO_PIN_2);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

//	HAL_SPI_MspDeInit(&hspi1);
//	HAL_UART_MspDeInit(&huart1);
//	HAL_ADC_MspDeInit(&hadc);
}

void HT_McuApi_DeepSleepMode(void) {
	Config_RangeExt(PA_SHUTDOWN);
	S2LPEnterShutdown();

#ifndef MASTER_DEVICE
	HAL_PWR_EnterSTANDBYMode();
#else

	deepSleepModeFlag = 1;

	HT_McuApi_EnterGpioLowPower();

	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
#endif
}

#ifdef MASTER_DEVICE

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {

	/* Clear Wake Up Flag */
	if(deepSleepModeFlag) {
		HAL_RTCEx_DeactivateWakeUpTimer(hrtc);

		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		HT_McuApi_ConfigPeripherals();

		deepSleepModeFlag = 0;
		master_state = SM_STATE_MASTER_SEND_BROADCAST;
	}
}

#endif

void HT_McuApi_EnableRTCWkp(uint32_t seconds) {

	/* Disable all Wakeup IT */
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	HAL_NVIC_SetPriority(RTC_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(RTC_IRQn);

	/* Set wakeup IT */
	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, seconds, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
}
