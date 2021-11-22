
#include "HT_Equipment_App.h"

#ifdef MASTER_DEVICE

HT_APP_MasterFsm master_state = SM_STATE_MASTER_CALIBRATION_PROCESS; //SM_STATE_MASTER_RTC_CONFIG;
static AppliFrame_t xTxFrame, xRxFrame;

volatile FlagStatus xStartRx=RESET, rx_timeout=RESET, exitTime=RESET;
volatile FlagStatus xRxDoneFlag = RESET, xTxDoneFlag=RESET, cmdFlag=RESET;

uint8_t table_len = 0;
uint8_t rx_counter = 0;

HT_APP_DeviceTable db_table[MAX_DEVICE_NUMBER];

static uint8_t print_flag = 0;

static RTC_TimeTypeDef sTime;
static RTC_DateTypeDef sDate;

uint8_t usart_callback = 0;
uint8_t usart_buffer[2];

HT_APP_RtcType rtc_flag = RTC_YEAR;

uint8_t entered_flag = 0;
uint8_t empty_flag = 0;

uint8_t rssi_lvl;
uint8_t rssi_thr = 255;

void HT_APP_ResetControlFlags(void) {
	xStartRx = RESET;
	rx_timeout = RESET;
	exitTime = RESET;
	xRxDoneFlag = RESET;
	xTxDoneFlag = RESET;
	cmdFlag = RESET;

	print_flag = 0;
	usart_callback = 0;
	table_len = 0;
	rx_counter = 0;
}

void HT_APP_PrintOptions(void) {

	if(!print_flag) {
		switch(rtc_flag) {

		case RTC_YEAR:
			printf("Year: \n");
			break;
		case RTC_MONTH:
			printf("\nMonth: \n");
			break;
		case RTC_WEEKDAY:
			printf("\nWeekday: \n");
			break;
		case RTC_MONTHDAY:
			printf("\nDay of Month: \n");
			break;
		case RTC_HOUR:
			printf("\nHour: \n");
			break;
		case RTC_MINUTES:
			printf("\nMinutes: \n");
			break;
		case RTC_SECONDS:
			printf("\nSeconds: \n");
			break;
		}

		print_flag = 1;
	}
}

void HT_APP_UpdateDeviceTable(uint8_t id) {
	FLS_RW_StatusTypeDef err;

	err = FlashWrite((TABLE_ADDRESS + (id*4)), sizeof(HT_APP_DeviceTable), (uint8_t *)&db_table[rx_counter], 1);
	if(err != FLS_RW_OK)
		printf("FlashWrite error: %X\n", err);
}

void HT_APP_CleanTable(uint8_t id) {
	FLS_RW_StatusTypeDef err;

	memset(&db_table[id], 0, sizeof(HT_APP_DeviceTable));

	err = FlashWrite((TABLE_ADDRESS + (id*4)), sizeof(HT_APP_DeviceTable), (uint8_t *)&db_table[rx_counter], 1);
	if(err != FLS_RW_OK)
		printf("FlashWrite error: %X\n", err);
}

void HT_Master_SetKeyStatus(FlagStatus val) {
	if(val==SET)
		master_state = SM_STATE_MASTER_SEND_BROADCAST;

	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void HT_APP_RetrieveTableData(HT_APP_DeviceTable *table) {
	FLS_RW_StatusTypeDef err;
	uint8_t i;

	for(i = 0; i < MAX_DEVICE_NUMBER; i++) {

		err = FlashRead(TABLE_ADDRESS + (i*4), sizeof(HT_APP_DeviceTable), (uint8_t *)&table[i]);

		if(err == FLS_RW_OK) {
			if(table[i].device_id != EMPTY_ADDR)
				table_len++;

		} else {
			printf("FlashRead Error: %X\n", err);
		}
	}

	FlashRead(CALIBRATION_ADDRESS, 1, &rssi_thr);
}

void HT_APP_ShiftDeviceTable(HT_APP_DeviceTable *table) {
	FLS_RW_StatusTypeDef err;
	uint8_t i;

	for(i = 0; i < (MAX_DEVICE_NUMBER-1); i++) {

		err = FlashRead(TABLE_ADDRESS + (i*4), sizeof(HT_APP_DeviceTable), (uint8_t *)&table[i]);
		if(err == FLS_RW_OK) {
			if(table[i].device_id == EMPTY_ADDR) {
				FlashRead(TABLE_ADDRESS + ((i+1)*4), sizeof(HT_APP_DeviceTable), (uint8_t *)&table[i]); /* read next position */
				HAL_Delay(10);
				FlashWrite(TABLE_ADDRESS + (i*4), sizeof(HT_APP_DeviceTable), (uint8_t *)&table[i], 1);
			}
		} else {
			printf("FlashRead Error: %X\n", err);
		}

	}
}

void HT_APP_DisableTimer(void) {
	HAL_TIM_Base_Stop_IT(&htim22);
	__HAL_TIM_SET_COUNTER(&htim22, 0);
}

void HT_APP_EnableTimer(void) {
	HT_TIM_ClearIRQnFlags();
	HAL_TIM_Base_Start_IT(&htim22);
}

void HT_APP_SendBroadcastState(void) {

	printf("Sending broadcast...\n");
	xTxFrame.payload_type = DISCOVERY_PAYLOAD;
	xTxFrame.battery_lvl = 0;
	xTxFrame.device_id = MY_ADDRESS;

	AppliSendBuff(&xTxFrame, BROADCAST_ADDRESS);

	master_state = SM_STATE_MASTER_WAIT_FOR_TX_DONE;
}

void HT_APP_WaitForTxDoneState(void) {
	printf("Waiting for tx done...\n");

	if(xTxDoneFlag) {
		xTxDoneFlag = RESET;

		if(xTxFrame.payload_type == DISCOVERY_PAYLOAD)
			master_state = SM_STATE_MASTER_WAIT_RETRIEVE_DATA;
		else
			master_state = SM_STATE_MASTER_SHOW_RESULTS;
	}
}

void HT_APP_RetrieveDataState(void) {
	printf("Retrieving data...\n");

	/* Get table context and how many devices are registered */
	HT_APP_RetrieveTableData(db_table);

	master_state = SM_STATE_MASTER_WAIT_FOR_RESPONSE;
}

void HT_APP_WaitForResponseState(void) {
	printf("Wait for response...\n");

	HT_APP_EnableTimer();

	/* It is going to wait in the device table order */
	AppliReceiveBuff((db_table[rx_counter].device_id) != 0 ? db_table[rx_counter].device_id : BROADCAST_ADDRESS);
	master_state = SM_STATE_MASTER_WAIT_FOR_RX_DONE;
}

void HT_APP_WaitForRXDoneState(void) {
	uint8_t __attribute__((unused)) len;

	GPIOA->BSRR = 1 << 5;

	if(!print_flag) {
		printf("Waiting for RX Done...\n");
		print_flag = 1;
	}

	if((RESET != xRxDoneFlag)||(RESET != rx_timeout)||(SET != exitTime)) {
		if((rx_timeout==SET)||(exitTime==RESET)) {
			HT_APP_DisableTimer();

			rx_timeout = RESET;

			master_state = SM_STATE_MASTER_SHOW_RESULTS;

			if(db_table[rx_counter].last_event != EVENT_EMPTY && table_len > 0)
				db_table[rx_counter].last_event = EVENT_OUT;
			else if(table_len == 0 && !entered_flag)
				db_table[rx_counter].last_event = EVENT_EMPTY;
			else
				db_table[rx_counter].last_event = EVENT_STAY+1;

		} else if(xRxDoneFlag) {

			HT_APP_DisableTimer();

			xRxDoneFlag = RESET;

			/* Get RX buffer */
			S2LP_GetRxPacket((uint8_t *)&xRxFrame, &len);

			S2LPSpiReadRegisters(RSSI_LEVEL_ADDR, 1, &rssi_lvl);
			printf("\nRSSI: %d\n", rssi_lvl);

			if(rssi_lvl >= (rssi_thr-RSSI_OFFSET)) {
				master_state = SM_STATE_MASTER_SAVE_CONTEXT;
			} else {
				printf("\nDropping packet...\n");
				master_state = SM_STATE_MASTER_WAIT_FOR_RESPONSE;
			}

			print_flag = 0;
		}
	}

	//HAL_Delay(50);
	GPIOA->BSRR = 1 << 21;
}

void HT_APP_SaveContextState(void) {
	FLS_RW_StatusTypeDef err;

	printf("Saving context...\n");

	if(xRxFrame.device_id != MASTER_ADDRESS) {
		if(db_table[rx_counter].device_id == xRxFrame.device_id) { /* Verify if the respective buffer was received from the expected device */
			db_table[rx_counter].last_event = EVENT_STAY;

			err = FlashWrite((TABLE_ADDRESS + (rx_counter*4)), sizeof(HT_APP_DeviceTable), (uint8_t *)&db_table[rx_counter], 1);
			if(err != FLS_RW_OK) {
				printf("FlashWrite error: %X\n", err);
			}

		} else if(db_table[rx_counter].device_id ==  0) { /* empty column */

			db_table[rx_counter].last_event = EVENT_IN;
			db_table[rx_counter].device_id = xRxFrame.device_id;

			err = FlashWrite((TABLE_ADDRESS + (rx_counter*4)), sizeof(HT_APP_DeviceTable), (uint8_t *)&db_table[rx_counter], 1);

			if(err != FLS_RW_OK) {
				printf("FlashWrite error: %X\n", err);
			}
		} else {
			printf("Response time error! Check the slave time!\n");
		}

		master_state = SM_STATE_MASTER_SEND_ACK;
	} else {
		printf("\nDropping packet from another master device...\n");
		master_state = SM_STATE_CALIB_RECEIVE_RESPONSE;
	}
}

void HT_APP_SendAckState(void) {
	xTxFrame.payload_type = ACK_PAYLOAD;
	xTxFrame.battery_lvl = 0;
	xTxFrame.device_id = MY_ADDRESS;

	printf("Sending ack to %X...\n", xRxFrame.device_id);

	/* Send to where the previous buffer was received */
	AppliSendBuff(&xTxFrame, xRxFrame.device_id);

	master_state = SM_STATE_MASTER_WAIT_FOR_TX_DONE;
}

void HT_APP_ShowResultsState(void) {

	HAL_RTC_GetTime(&hrtc, &sTime, FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, FORMAT_BIN);

	switch((HT_APP_Event)db_table[rx_counter].last_event) {

	case EVENT_IN:
		entered_flag = 1;

		printf("\nDevice ID: %X\n", db_table[rx_counter].device_id);
		printf("Battery Level: %d\n", xRxFrame.battery_lvl);

		printf("Date: %02d/%02d/20%02d\n", sDate.Date, sDate.Month, sDate.Year);
		printf("Time: %02d:%02d:%02d\n",sTime.Hours,sTime.Minutes,sTime.Seconds);

		printf("Has just ENTERED this room.\n\n");

		HT_APP_UpdateDeviceTable(rx_counter);
		break;
	case EVENT_OUT:
		entered_flag = 0;

		printf("\nDevice ID: %X\n", db_table[rx_counter].device_id);
		printf("Battery Level: %d\n", xRxFrame.battery_lvl);

		printf("Date: %02d/%02d/20%02d\n", sDate.Date, sDate.Month, sDate.Year);
		printf("Time: %02d:%02d:%02d\n",sTime.Hours,sTime.Minutes,sTime.Seconds);

		printf("Has just LEFT this room.\n\n");
		HT_APP_CleanTable(rx_counter);
		break;
	case EVENT_STAY:
		printf("\nDevice ID: %X\n", db_table[rx_counter].device_id);
		printf("Battery Level: %d\n", xRxFrame.battery_lvl);

		printf("Date: %02d/%02d/20%02d\n", sDate.Date, sDate.Month, sDate.Year);
		printf("Time: %02d:%02d:%02d\n",sTime.Hours,sTime.Minutes,sTime.Seconds);

		printf("Is in this room.\n\n");
		break;

	case EVENT_EMPTY:
		if(!empty_flag) {
			printf("\nThis room is EMPTY!\n\n");
			empty_flag = 1;
		}

		break;
	default:
		break;
	}

	rx_counter++;

	if(rx_counter < MAX_DEVICE_NUMBER) {
		master_state = SM_STATE_MASTER_WAIT_FOR_RESPONSE;
	} else {
		rx_counter = 0;
		master_state = SM_STATE_MASTER_DEEP_SLEEP;
	}

	memset(&db_table, 0, sizeof(HT_APP_DeviceTable));
}

void HT_APP_DeepSleepState(void) {
	empty_flag = 0;

	printf("Sleeping...\n");

	//HT_APP_ShiftDeviceTable(db_table);

	HT_APP_ResetControlFlags();

	HT_McuApi_EnableRTCWkp(MASTER_WKP_TIME);
	HT_McuApi_DeepSleepMode();
}

void HT_APP_RtcConfigState(void) {

	HT_APP_PrintOptions();

	if(usart_callback) {
		usart_callback = 0;
		print_flag = 0;

		switch(rtc_flag) {

		case RTC_YEAR:
			sDate.Year = atoi((char *)usart_buffer);
			break;
		case RTC_MONTH:
			sDate.Month = atoi((char *)usart_buffer);
			break;
		case RTC_WEEKDAY:
			sDate.WeekDay = atoi((char *)usart_buffer);
			break;
		case RTC_MONTHDAY:
			sDate.Date = atoi((char *)usart_buffer);
			break;
		case RTC_HOUR:
			sTime.Hours = atoi((char *)usart_buffer);
			break;
		case RTC_MINUTES:
			sTime.Minutes = atoi((char *)usart_buffer);
			break;
		case RTC_SECONDS:
			sTime.Seconds = atoi((char *)usart_buffer);

			HAL_RTC_SetTime(&hrtc, &sTime, FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &sDate, FORMAT_BIN);
			break;
		}

		rtc_flag++;
	}

	master_state = (rtc_flag != (RTC_SECONDS+1)) ? SM_STATE_MASTER_RTC_CONFIG : SM_STATE_MASTER_SEND_BROADCAST;
}

void HT_APP_CalibrationProcessState(void) {
	HT_APP_CalibrationFsm state = SM_STATE_CALIB_SEND_BROADCAST;
	uint8_t rssi;
	uint8_t cnt = 0;
	uint8_t calib_end = 0;
	uint8_t __attribute__ ((unused)) len;

	xTxFrame.payload_type = CALIBRATION_PAYLOAD;
	xTxFrame.battery_lvl = 0;
	xTxFrame.device_id = MY_ADDRESS;

	while(!usart_callback)
		__NOP();

	usart_callback = 0;

	printf("Calibrating ...\n");
	while(!calib_end) {
		switch(state) {
		case SM_STATE_CALIB_SEND_BROADCAST:
			printf("Sending broadcast...\n");

			AppliSendBuff(&xTxFrame, BROADCAST_ADDRESS);
			state = SM_STATE_CALIB_WAIT_TX_DONE;
			HAL_Delay(1000);

			break;
		case SM_STATE_CALIB_WAIT_TX_DONE:
			if(xTxDoneFlag) {
				xTxDoneFlag = RESET;
				state = SM_STATE_CALIB_RECEIVE_RESPONSE;
			}

			break;

		case SM_STATE_CALIB_RECEIVE_RESPONSE:
			printf("Waiting for response...\n");

			AppliReceiveBuff(BROADCAST_ADDRESS);
			state = SM_STATE_CALIB_WAIT_RX_DONE;

			break;
		case SM_STATE_CALIB_WAIT_RX_DONE:

			if((RESET != xRxDoneFlag)||(RESET != rx_timeout)||(SET != exitTime)) {
				if((rx_timeout==SET)||(exitTime==RESET)) {
					state = SM_STATE_CALIB_SEND_BROADCAST;
					HT_APP_ResetControlFlags();

				} else if(xRxDoneFlag) {
					printf("\nResponse received!\n");

					S2LP_GetRxPacket((uint8_t *)&xRxFrame, &len);
					if(xRxFrame.device_id != MASTER_ADDRESS) {
						xRxDoneFlag = RESET;
						S2LPSpiReadRegisters(RSSI_LEVEL_ADDR, 1, &rssi);
						printf("RSSI: %d\n", rssi);

						state = SM_STATE_CALIB_CALCULATE;
						cnt += 1;
					} else {
						printf("Dropping packet...\n");
						state = SM_STATE_CALIB_SEND_BROADCAST;
						HT_APP_ResetControlFlags();
					}

				}
			}

			break;
		case SM_STATE_CALIB_CALCULATE:

			rssi_thr = (rssi < rssi_thr) ? rssi : rssi_thr;
			printf("RSSI thr: %d\n", rssi_thr);

			FlashWrite(CALIBRATION_ADDRESS, 1, &rssi_thr, 1);

			if(cnt < CALIBRATION_REPEAT)
				state = SM_STATE_CALIB_SEND_BROADCAST;
			else
				calib_end = 1;

			break;
		}
	}

	printf("Finishing calibration process...\n");
	master_state = SM_STATE_MASTER_RTC_CONFIG;
}

void HT_Master_Fsm(void) {

	switch(master_state) {
	case SM_STATE_MASTER_SEND_BROADCAST:
		HT_APP_SendBroadcastState();
		break;
	case SM_STATE_MASTER_WAIT_FOR_TX_DONE:
		HT_APP_WaitForTxDoneState();
		break;
	case SM_STATE_MASTER_WAIT_RETRIEVE_DATA:
		HT_APP_RetrieveDataState();
		break;
	case SM_STATE_MASTER_WAIT_FOR_RESPONSE:
		HT_APP_WaitForResponseState();
		break;
	case SM_STATE_MASTER_WAIT_FOR_RX_DONE:
		HT_APP_WaitForRXDoneState();
		break;
	case SM_STATE_MASTER_SAVE_CONTEXT:
		HT_APP_SaveContextState();
		break;
	case SM_STATE_MASTER_SEND_ACK:
		HT_APP_SendAckState();
		break;
	case SM_STATE_MASTER_SHOW_RESULTS:
		HT_APP_ShowResultsState();
		break;
	case SM_STATE_MASTER_DEEP_SLEEP:
		HT_APP_DeepSleepState();
		break;
	case SM_STATE_MASTER_RTC_CONFIG:
		HT_APP_RtcConfigState();
		break;
	case SM_STATE_MASTER_CALIBRATION_PROCESS:
		HT_APP_CalibrationProcessState();
		break;
	}
}

#endif
