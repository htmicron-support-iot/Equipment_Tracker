
#include "HT_Equipment_App.h"

#ifndef MASTER_DEVICE

static uint8_t print_flag = 0;

HT_APP_SlaveFsm slave_state = SM_STATE_SLAVE_WAIT_FOR_MASTER;

static AppliFrame_t xTxFrame, xRxFrame;

volatile FlagStatus xStartRx=RESET, rx_timeout=RESET, exitTime=RESET;
volatile FlagStatus xRxDoneFlag = RESET, xTxDoneFlag=RESET, cmdFlag=RESET;

//uint8_t calib_counter = 0;

void HT_Slave_SetKeyStatus(FlagStatus val) {
	if(val==SET)
		slave_state = SM_STATE_SLAVE_SEND_RESPONSE;

	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

uint8_t HT_APP_ReadBatteryLvl(void) {
	uint32_t vref;
	uint32_t __attribute__((unused)) ad;

	ad = HT_getTemperatureAD();
	vref = HT_getVrefData();

	vref = vref/100;

	return (uint8_t)vref;
}

void HT_Slave_WaitForMasterState(void) {
	printf("Waiting for master...\n");

	memset(&xRxFrame, 0, sizeof(AppliFrame_t));
	print_flag = 0;

	AppliReceiveBuff(MASTER_ADDRESS);
	slave_state = SM_STATE_SLAVE_WAIT_FOR_RX_DONE;
}

void HT_Slave_WaitForRxDoneState(void) {
	uint8_t __attribute__((unused)) len;

	if(!print_flag) {
		printf("Waiting for RX Done...\n");
		print_flag = 1;
	}

	if((RESET != xRxDoneFlag)||(RESET != rx_timeout)||(SET != exitTime)) {
		if((rx_timeout==SET)||(exitTime==RESET)) {
			rx_timeout = RESET;
			slave_state = SM_STATE_SLAVE_WAIT_FOR_MASTER;
		} else if(xRxDoneFlag) {
			xRxDoneFlag = RESET;

			/* Get RX buffer */
			S2LP_GetRxPacket((uint8_t *)&xRxFrame, &len);

			printf("Received from %X -- RX: %d\n", xRxFrame.device_id, xRxFrame.payload_type);

			if(xRxFrame.payload_type == DISCOVERY_PAYLOAD)
				slave_state = SM_STATE_SLAVE_SEND_RESPONSE;
			else if(xRxFrame.payload_type == ACK_PAYLOAD)
				slave_state = SM_STATE_SLAVE_DEEP_SLEEP;
			else {
				slave_state = SM_STATE_SLAVE_CALIBRATION_PROCESS;
				//calib_counter += 1;
			}
		}
	}
}

void HT_Slave_SendResponseState(void) {

	printf("Sending response to %X...\n", MASTER_ADDRESS);

	xTxFrame.payload_type = RESPONSE_PAYLOAD;
	xTxFrame.battery_lvl = HT_APP_ReadBatteryLvl();
	xTxFrame.device_id = MY_ADDRESS;

	HAL_Delay(700*MY_ADDRESS);
	AppliSendBuff(&xTxFrame, MASTER_ADDRESS);

	slave_state = SM_STATE_SLAVE_WAIT_FOR_TX_DONE;
}

void HT_Slave_WaitForTxDoneState(void) {

	printf("Waiting for TX Done...\n");

	if(xTxDoneFlag) {
		xTxDoneFlag = RESET;

		slave_state = (xRxFrame.payload_type != CALIBRATION_PAYLOAD) ? SM_STATE_SLAVE_WAIT_ACK : SM_STATE_SLAVE_WAIT_FOR_MASTER;
	}
}

void HT_Slave_WaitForAckState(void) {
	printf("Waiting for master...\n");

	AppliReceiveBuff(MASTER_ADDRESS);
	slave_state = SM_STATE_SLAVE_WAIT_FOR_RX_DONE;
}

void HT_Slave_DeepSleepState(void) {

	printf("Sleeping...\n");

	HT_McuApi_EnableRTCWkp(MASTER_WKP_TIME-1);

	HT_McuApi_DeepSleepMode();
}

void HT_Slave_Fsm(void) {

	switch(slave_state) {

	case SM_STATE_SLAVE_WAIT_FOR_MASTER:
		HT_Slave_WaitForMasterState();
		break;
	case SM_STATE_SLAVE_WAIT_FOR_RX_DONE:
		HT_Slave_WaitForRxDoneState();
		break;
	case SM_STATE_SLAVE_SEND_RESPONSE:
		HT_Slave_SendResponseState();
		break;
	case SM_STATE_SLAVE_WAIT_FOR_TX_DONE:
		HT_Slave_WaitForTxDoneState();
		break;
	case SM_STATE_SLAVE_WAIT_ACK:
		HT_Slave_WaitForAckState();
		break;
	case SM_STATE_SLAVE_DEEP_SLEEP:
		HT_Slave_DeepSleepState();
		break;
	case SM_STATE_SLAVE_CALIBRATION_PROCESS:
		slave_state = SM_STATE_SLAVE_SEND_RESPONSE;
		break;
	}
}
#endif
