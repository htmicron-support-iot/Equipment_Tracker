
#ifndef __HT_EEPROM_UTILS_H__
#define __HT_EEPROM_UTILS_H__

#include "main.h"

//#define FLASH_USER_START_ADDR   	nvm_data				/* Start @ of user Flash area */
//#define FLASH_USER_END_ADDR		nvm_data+(sizeof(nvm_data))	/* End @ of user Flash area */

  /**
  * @brief board_data is the part of the FLASH where Sigfox board info are stored.
  * In this area are stored ID, PAC, RCZ, the Sigfox key and other informations.
  * The key could be encrypted or not.
  * Use the functions in the @nvm_api module in order to read/update these information.
  */
//#define FLASH_BOARD_START_ADDR	board_data					/* Start @ of board data Flash area */
//#define FLASH_BOARD_END_ADDR		board_data+(sizeof(board_data))	/* End @ of board data Flash area */

#define FLASH_TYPEERASE_PAGES           ((uint32_t)0x00U)  /*!<Page erase only*/
#define FLASH_ERASE_VALUE 0x00

#define MAX_NO_OF_PAGES		32	/* Pages for sector */

/**
* @brief  FlashRead Status Enum
*/
typedef enum
{
  FLS_RW_OK		= 0x00,
  FLS_RW_ERROR	= 0x01,
  FLS_RW_OUT_OF_RANGE	= 0x02
} FLS_RW_StatusTypeDef;

/*
 * @brief  Reads cNbBytes from FLASH memory starting at nAddress and store a pointer to that in pcBuffer.
 *
 * @param nAddress starting address
 * @cNbBytes number of bytes to read
 * @pcBuffer the pointer to the data
 * @return The staus of the operation
*/
FLS_RW_StatusTypeDef FlashRead(uint32_t nAddress,  uint16_t cNbBytes,  uint8_t* pcBuffer);

/*
 * @brief  Writes cNbBytes pointed to pcBuffer to FLASH memory starting at nAddress.
 *
 * @param nAddress starting address
 * @cNbBytes number of bytes to write
 * @pcBuffer the pointer to the data
 * @eraseBeforeWrite if set to 1 erase the page before write bytes
 * @return The staus of the operation
*/
FLS_RW_StatusTypeDef FlashWrite(uint32_t nAddress, uint16_t cNbBytes, uint8_t* pcBuffer, uint8_t eraseBeforeWrite);

/*
 * @brief  Erase nPages from FLASH starting at nAddress
 *
 * @param nAddress starting address
 * @nPages number of pages to erase
 * @return The staus of the operation
*/
FLS_RW_StatusTypeDef FlashErase(uint32_t nAddress, uint32_t nPages);

/*
 * @brief  Erase a page from Data Eeprom.
 *
 * @param nAddress flash address
 * @return FLS_RW_OK if address is valid
*/
FLS_RW_StatusTypeDef DataEepromErase(uint32_t nAddress);

#endif /* __HT_EEPROM_UTILS_H__ */
