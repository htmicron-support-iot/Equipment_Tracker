/**
 *
 * Copyright (c) 2020 HT Micron Semicondutors S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HT_P2P_app.h"
#include "HT_Equipment_App.h"

SM_State_t SM_State =  SM_STATE_START_RX;

extern volatile FlagStatus xStartRx, rx_timeout, exitTime;
extern volatile FlagStatus xRxDoneFlag, xTxDoneFlag, cmdFlag;

uint16_t exitCounter = 0;
uint16_t txCounter = 0;

S2LPIrqs xIrqStatus;

PktBasicInit xBasicInit={
		PREAMBLE_LENGTH,
		SYNC_LENGTH,
		SYNC_WORD,
		VARIABLE_LENGTH,
		EXTENDED_LENGTH_FIELD,
		CRC_MODE,
		EN_ADDRESS,
		EN_FEC,
		EN_WHITENING
};

PktBasicAddressesInit xAddressInit={
		EN_FILT_MY_ADDRESS,
		MY_ADDRESS,
		EN_FILT_MULTICAST_ADDRESS,
		MULTICAST_ADDRESS,
		EN_FILT_BROADCAST_ADDRESS,
		BROADCAST_ADDRESS
};

SGpioInit xGpioIRQ={
		S2LP_GPIO_3,
		S2LP_GPIO_MODE_DIGITAL_OUTPUT_LP,
		S2LP_GPIO_DIG_OUT_IRQ
};

SRadioInit xRadioInit = {
		BASE_FREQUENCY,
		MODULATION_SELECT,
		DATARATE,
		FREQ_DEVIATION,
		BANDWIDTH
};


void HT_P2P_Init(void) {
	/* S2LP IRQ config */
	S2LPGpioInit(&xGpioIRQ);

	/* S2LP Radio config */
	S2LPRadioInit(&xRadioInit);

	/* S2LP Radio set power */
	S2LPRadioSetMaxPALevel(S_DISABLE);

	FEM_Init();
#ifdef BASE_FREQ_433
	Config_RangeExt(PA_SHUTDOWN);
#endif

	S2LP_PacketConfig();

	S2LPRadioSetRssiThreshdBm(RSSI_THRESHOLD);
}

void HAL_Radio_Init(void) {
	S2LPInterfaceInit();
}

void BasicProtocolInit(void) {
	/* Radio Packet config */
	S2LPPktBasicInit(&xBasicInit);
}

void AppliSendBuff(AppliFrame_t *xTxFrame, uint8_t dst_addr) {
	uint8_t tmp;

	S2LPPktBasicAddressesInit(&xAddressInit);

	S2LPGpioIrqDeInit(NULL);

	S2LP_EnableTxIrq();

	/* payload length config */
	S2LP_SetPayloadlength(sizeof(AppliFrame_t));

	/* rx timeout config */
	//S2LP_SetRxTimeout(RECEIVE_TIMEOUT);

	/* IRQ registers blanking */
	S2LPGpioIrqClearStatus();

	/* Destination address. It could be also changed to BROADCAST_ADDRESS or MULTICAST_ADDRESS. */
	S2LP_SetDestinationAddress(dst_addr);

#ifndef BASE_FREQ_433
	Config_RangeExt(PA_TX);
#endif

	/* S2LP Boost mode*/
	tmp = 0x72; /* </ Turn off boost mode (0x72 turn on -- 0x12 turn off)*/
	S2LPSpiWriteRegisters(0x79, sizeof(tmp), &tmp);

//	tmp = 0x1E;
//	S2LPSpiWriteRegisters(0x5A, sizeof(tmp), &tmp);

	/* send the TX command */
	S2LP_StartTx((uint8_t *)xTxFrame, sizeof(AppliFrame_t));
}

void AppliReceiveBuff(uint8_t src_addr) {
	/*float rRSSIValue = 0;*/
	exitTime = SET;
	exitCounter = TIME_TO_EXIT_RX;

	S2LPPktBasicAddressesInit(&xAddressInit);

	/* S2LP IRQs disable */
	S2LPGpioIrqDeInit(NULL);

	/* S2LP IRQs enable */
	S2LP_EnableRxIrq();

	/* payload length config */
	S2LP_SetPayloadlength(sizeof(AppliFrame_t));

	S2LPTimerSetRxTimerMs(1000); //700.0
	//SET_INFINITE_RX_TIMEOUT();

	/* destination address */
	S2LP_SetDestinationAddress(src_addr);

	/* IRQ registers blanking */
	S2LPGpioIrqClearStatus();

#ifndef BASE_FREQ_433
	Config_RangeExt(PA_RX);
#endif

	/* RX command */
	S2LP_StartRx();
}

void P2PInterruptHandler(void) {

	S2LPGpioIrqGetStatus(&xIrqStatus);

	/* Check the S2LP TX_DATA_SENT IRQ flag */
	if(xIrqStatus.IRQ_TX_DATA_SENT)
		xTxDoneFlag = SET;

	/* Check the S2LP RX_DATA_READY IRQ flag */
	if((xIrqStatus.IRQ_RX_DATA_READY))
		xRxDoneFlag = SET;

	/* Restart receive after receive timeout*/
	if (xIrqStatus.IRQ_RX_TIMEOUT) {
		rx_timeout = SET;
		S2LPCmdStrobeSabort();
		//S2LPTimerSetRxTimerMs
		//S2LPCmdStrobeRx();
	}

	/* Check the S2LP RX_DATA_DISC IRQ flag */
//	if(xIrqStatus.IRQ_RX_DATA_DISC) {
//		/* RX command - to ensure the device will be ready for the next reception */
//		S2LPCmdStrobeRx();
//	}
}

void HAL_SYSTICK_Callback(void) {
	if(exitTime) {
		/*Decreament the counter to check when 3 seconds has been elapsed*/
		exitCounter--;
		/*3 seconds has been elapsed*/
		if(exitCounter <= TIME_UP)
			exitTime = RESET;
	}
}

#ifdef P2P_BASIC_PROTOCOL

void Set_KeyStatus(FlagStatus val) {
	if(val==SET)
		SM_State = SM_STATE_SEND_DATA;

	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void P2P_StartRx(void) {
	printf("Start RX...\n");
	AppliReceiveBuff();
	SM_State = SM_STATE_WAIT_FOR_RX_DONE;
}

void P2P_WaitForRxDone(void) {
	if((RESET != xRxDoneFlag)||(RESET != rx_timeout)||(SET != exitTime)) {
		if((rx_timeout==SET)||(exitTime==RESET)) {
			rx_timeout = RESET;
			SM_State = SM_STATE_START_RX;
		} else if(xRxDoneFlag) {
			xRxDoneFlag=RESET;
			SM_State = SM_STATE_DATA_RECEIVED;
		}
	}
}

void P2P_DataReceived(void) {
	uint8_t len = sizeof(AppliFrame_t);
	uint8_t tmp;

	S2LP_GetRxPacket((uint8_t *)&xRxFrame, &len);

	if(xRxFrame.Cmd != ACK_OK)
		printf("%s\n", (char *)xRxFrame.DataBuff);

	if(xRxFrame.Cmd == LED_TOGGLE)
		SM_State = SM_STATE_TOGGLE_LED;

	if(xRxFrame.Cmd == ACK_OK)
		SM_State = SM_STATE_ACK_RECEIVED;

	S2LPSpiReadRegisters(RSSI_LEVEL_ADDR, 1, &tmp);
	printf("RSSI: %d\n", tmp);
}

void P2P_SendAck(uint8_t *pTxBuff) {
	printf("Sending ack...\n");

	xTxFrame.Cmd = ACK_OK;
	xTxFrame.CmdLen = 0x01;
	xTxFrame.Cmdtag = xRxFrame.Cmdtag;
	xTxFrame.CmdType = APPLI_CMD;
	xTxFrame.DataLen = sizeof(AppliFrame_t);
	xTxFrame.device_id = MY_ADDRESS;

	memcpy(xTxFrame.DataBuff, pTxBuff, TX_BUFFER_SIZE);

	HAL_Delay(DELAY_TX_LED_GLOW);
	AppliSendBuff(&xTxFrame);
	SM_State = SM_STATE_WAIT_FOR_TX_DONE;
}

void P2P_SendData(uint8_t *pTxBuff) {
	printf("Sending data...\n");

	xTxFrame.Cmd = LED_TOGGLE;
	xTxFrame.CmdLen = 0x01;
	xTxFrame.Cmdtag = txCounter++;
	xTxFrame.CmdType = APPLI_CMD;
	xTxFrame.DataLen = sizeof(AppliFrame_t);
	xTxFrame.device_id = MY_ADDRESS;

	memcpy(xTxFrame.DataBuff, pTxBuff, TX_BUFFER_SIZE);

	AppliSendBuff(&xTxFrame);
	SM_State = SM_STATE_WAIT_FOR_TX_DONE;
}

void P2P_WaitForTxDone(void) {

	if(xTxDoneFlag) {
		xTxDoneFlag = RESET;

		if(xTxFrame.Cmd == LED_TOGGLE)
			SM_State = SM_STATE_START_RX;
		else if(xTxFrame.Cmd == ACK_OK)
			SM_State = SM_STATE_IDLE;
	}

}

void P2P_AckReceived(void) {

	printf("Ack received...\n");

	SM_State = SM_STATE_IDLE;
}

void P2P_ToggleLed(void) {
	uint8_t  dest_addr;

	GPIOA->BSRR = 1 << 5; /* turn on led */

	dest_addr = S2LPGetReceivedDestinationAddress();

	if ((dest_addr == MULTICAST_ADDRESS) || (dest_addr == BROADCAST_ADDRESS)) {
		/* in that case do not send ACK to avoid RF collisions between several RF ACK messages */
		HAL_Delay(200);
		SM_State = SM_STATE_IDLE;
	} else {
		SM_State = SM_STATE_SEND_ACK;
	}
}

void P2P_Idle(void) {
	GPIOA->BSRR = 1 << 21; /*turn off led */

	SM_State = SM_STATE_START_RX;
}

void P2P_Process(uint8_t *pTxBuff) {

	switch(SM_State) {
	case SM_STATE_START_RX:
		P2P_StartRx();
		break;
	case SM_STATE_WAIT_FOR_RX_DONE:
		P2P_WaitForRxDone();
		break;
	case SM_STATE_DATA_RECEIVED:
		P2P_DataReceived();
		break;
	case SM_STATE_SEND_ACK:
		P2P_SendAck(pTxBuff);
		break;
	case SM_STATE_SEND_DATA:
		P2P_SendData(pTxBuff);
		break;
	case SM_STATE_WAIT_FOR_TX_DONE:
		P2P_WaitForTxDone();
		break;
	case SM_STATE_ACK_RECEIVED:
		P2P_AckReceived();
		break;
	case SM_STATE_TOGGLE_LED:
		P2P_ToggleLed();
		break;
	case SM_STATE_IDLE:
		P2P_Idle();
		break;
	}

}

#endif

/************************ HT Micron Semicondutors S.A *****END OF FILE****/
