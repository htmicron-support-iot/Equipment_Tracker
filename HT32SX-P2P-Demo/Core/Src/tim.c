/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* USER CODE BEGIN 0 */

#include "HT_P2P_s2lp_gpio.h"
#include "HT_P2P_app.h"

extern S2LPIrqs xIrqStatus;
extern volatile FlagStatus rx_timeout, exitTime;
extern volatile FlagStatus xRxDoneFlag;

/* USER CODE END 0 */

TIM_HandleTypeDef htim22;

/* TIM22 init function */
void MX_TIM22_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim22.Instance = TIM22;
  htim22.Init.Prescaler = 16000-1;
  htim22.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim22.Init.Period = 3000-1;
  htim22.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim22.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim22) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim22, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim22, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM22)
  {
  /* USER CODE BEGIN TIM22_MspInit 0 */

  /* USER CODE END TIM22_MspInit 0 */
    /* TIM22 clock enable */
    __HAL_RCC_TIM22_CLK_ENABLE();

    /* TIM22 interrupt Init */
    HAL_NVIC_SetPriority(TIM22_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM22_IRQn);
  /* USER CODE BEGIN TIM22_MspInit 1 */

  /* USER CODE END TIM22_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM22)
  {
  /* USER CODE BEGIN TIM22_MspDeInit 0 */

  /* USER CODE END TIM22_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM22_CLK_DISABLE();

    /* TIM22 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM22_IRQn);
  /* USER CODE BEGIN TIM22_MspDeInit 1 */

  /* USER CODE END TIM22_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */

void HT_TIM_ClearIRQnFlags(void) {
	__HAL_TIM_CLEAR_FLAG(&htim22,TIM_FLAG_CC1);
	__HAL_TIM_CLEAR_FLAG(&htim22,TIM_FLAG_CC2);
	__HAL_TIM_CLEAR_FLAG(&htim22,TIM_FLAG_CC3);
	__HAL_TIM_CLEAR_FLAG(&htim22,TIM_FLAG_CC4);
	__HAL_TIM_CLEAR_FLAG(&htim22, TIM_FLAG_UPDATE);
	__HAL_TIM_CLEAR_FLAG(&htim22, TIM_FLAG_TRIGGER);

	htim22.Instance->SR = 0x00000000;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	HT_TIM_ClearIRQnFlags();

	if(htim->Instance == TIM22){
		printf("Timeout!\n");

		xRxDoneFlag = SET;
		rx_timeout = SET;
	}
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
