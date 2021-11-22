
#ifndef __HT_EQUIPMENT_APP_H__
#define __HT_EQUIPMENT_APP_H__

#include "main.h"
#include "tim.h"
#include "HT_P2P_app.h"
#include "HT_eeprom_utils.h"
#include "HT_mcu_api.h"
#include "adc.h"
#include "stdlib.h"

#define TABLE_ADDRESS 			0x08080000
#define CALIBRATION_ADDRESS 	0x08080030

#define MAX_DEVICE_NUMBER	5
#define TABLE_SIZE 			MAX_DEVICE_NUMBER*4 		/* 4 = 1 page */
#define EMPTY_ADDR			0x00

#define HT_RX_TIMEOUT		1000000

#define MASTER_WKP_TIME		10			/* sec */

#define RSSI_OFFSET			1

#define CALIBRATION_REPEAT 	5

typedef enum {
	EVENT_EMPTY = 0,
	EVENT_IN,
	EVENT_OUT,
	EVENT_STAY
} HT_APP_Event;

typedef enum {
	ACK_PAYLOAD = 0,
	DISCOVERY_PAYLOAD,
	RESPONSE_PAYLOAD,
	CALIBRATION_PAYLOAD
} HT_APP_PayloadType;

typedef enum {
	SM_STATE_MASTER_SEND_BROADCAST = 0,
	SM_STATE_MASTER_WAIT_FOR_TX_DONE,
	SM_STATE_MASTER_WAIT_RETRIEVE_DATA,
	SM_STATE_MASTER_WAIT_FOR_RESPONSE,
	SM_STATE_MASTER_SAVE_CONTEXT,
	SM_STATE_MASTER_SEND_ACK,
	SM_STATE_MASTER_SHOW_RESULTS,
	SM_STATE_MASTER_WAIT_FOR_RX_DONE,
	SM_STATE_MASTER_DEEP_SLEEP,
	SM_STATE_MASTER_RTC_CONFIG,
	SM_STATE_MASTER_CALIBRATION_PROCESS
} HT_APP_MasterFsm;

typedef enum {
	SM_STATE_SLAVE_WAIT_FOR_MASTER = 0,
	SM_STATE_SLAVE_WAIT_FOR_RX_DONE,
	SM_STATE_SLAVE_SEND_RESPONSE,
	SM_STATE_SLAVE_WAIT_FOR_TX_DONE,
	SM_STATE_SLAVE_WAIT_ACK,
	SM_STATE_SLAVE_DEEP_SLEEP,
	SM_STATE_SLAVE_CALIBRATION_PROCESS
} HT_APP_SlaveFsm;

typedef enum {
	SM_STATE_CALIB_SEND_BROADCAST,
	SM_STATE_CALIB_WAIT_TX_DONE,
	SM_STATE_CALIB_RECEIVE_RESPONSE,
	SM_STATE_CALIB_WAIT_RX_DONE,
	SM_STATE_CALIB_CALCULATE
} HT_APP_CalibrationFsm;

typedef enum {
	RTC_YEAR = 0,
	RTC_MONTH,
	RTC_WEEKDAY,
	RTC_MONTHDAY,
	RTC_HOUR,
	RTC_MINUTES,
	RTC_SECONDS
} HT_APP_RtcType;

typedef struct {
	uint8_t device_id;
	uint8_t last_event;
} HT_APP_DeviceTable;

void HT_Master_Fsm(void);
void HT_Master_SetKeyStatus(FlagStatus val);

void HT_Slave_Fsm(void);
void HT_Slave_SetKeyStatus(FlagStatus val);

void HT_APP_EnableTimer(void);

#endif /* __HT_EQUIPMENT_APP_H__ */
