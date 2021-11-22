
#include "HT_eeprom_utils.h"

static FLASH_EraseInitTypeDef EraseInitStruct; /* Variable used for Erase procedure */

static uint32_t GetNumberOfPagesByBytes(uint32_t nBytesCount) {
	uint32_t nRet, nTmp;

	nTmp = nBytesCount % FLASH_PAGE_SIZE;
	nRet = (!nTmp)?(nBytesCount/FLASH_PAGE_SIZE):(nBytesCount/FLASH_PAGE_SIZE)+1;

	return nRet;
}

FLS_RW_StatusTypeDef FlashRead(uint32_t nAddress, uint16_t cNbBytes, uint8_t* pcBuffer) {
	FLS_RW_StatusTypeDef frRetStatus = FLS_RW_OK;
	volatile uint64_t tmp;
	uint8_t i, count4;
	uint16_t pageIdx;

	if(pcBuffer == NULL)
		frRetStatus = FLS_RW_ERROR;

	if(frRetStatus == FLS_RW_OK)
	{
		for(i=0; i<cNbBytes; i++)
		{
			count4  = (i-((i/4)*4));	/* Counts 0...3 and restarts */
			pageIdx = (i/4)*4;		/* Every 4 bytes moves ahead */


			tmp = *((__IO uint32_t *)(nAddress+pageIdx));
			pcBuffer[i] = (tmp&(0xFF000000>>(count4*8)))>>(24-(count4*8));

		}
	}

	return frRetStatus;
}

FLS_RW_StatusTypeDef FlashWrite(uint32_t nAddress, uint16_t cNbBytes, uint8_t* pcBuffer, uint8_t eraseBeforeWrite) {
	uint8_t i, count4;
	uint16_t pageIdx;
	uint32_t temp_word;
	uint32_t __attribute__ ((unused)) error_code;

	FLS_RW_StatusTypeDef frRetStatus = FLS_RW_OK;

	if(GetNumberOfPagesByBytes(cNbBytes) > MAX_NO_OF_PAGES)
		frRetStatus = FLS_RW_OUT_OF_RANGE;

	if(pcBuffer == NULL)
		frRetStatus = FLS_RW_ERROR;

	if (eraseBeforeWrite) {
		frRetStatus = FlashErase(nAddress, GetNumberOfPagesByBytes(cNbBytes));
	}

	if(frRetStatus == FLS_RW_OK)
	{
		temp_word = 0;

		/* Unlock the Flash to enable the flash control register access */
		HAL_FLASHEx_DATAEEPROM_Unlock();

		for(i=0; i<cNbBytes; i++)
		{
			count4  = i-((i/4)*4);	/* Counts 0...3 and restarts */
			pageIdx = (i/4)*4; 	/* Every 4 bytes writes page */

			temp_word |= ((uint32_t)pcBuffer[i])<<(24-(8*count4));

			if((i == cNbBytes-1) || count4 == 3 )/* Write every 4 bytes or if bytes in args are less than 4 */
			{
				if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAM_WORD, nAddress+pageIdx, temp_word) == HAL_OK)
				{
					temp_word = 0;
				}
				else
				{
					error_code = HAL_FLASH_GetError();
					frRetStatus = FLS_RW_ERROR;

					break;
				}
			}
		}

		HAL_FLASHEx_DATAEEPROM_Lock();
	}

	return frRetStatus;
}

FLS_RW_StatusTypeDef DataEepromErase(uint32_t nAddress) {
	uint32_t __attribute__ ((unused)) error_code;
	FLS_RW_StatusTypeDef frRetStatus = FLS_RW_ERROR;

	/* Unlock the Flash to enable the flash control register access */
	if(HAL_FLASHEx_DATAEEPROM_Unlock() == HAL_OK)
	{
		if(HAL_FLASHEx_DATAEEPROM_Erase(nAddress) == HAL_OK)
		{
			if(HAL_FLASHEx_DATAEEPROM_Lock() == HAL_OK)
				frRetStatus = FLS_RW_OK;
		}
	}

	if(frRetStatus == FLS_RW_ERROR)
	{
		error_code = HAL_FLASH_GetError();
	}

	return FLS_RW_OK;
}

FLS_RW_StatusTypeDef FlashErase(uint32_t nAddress, uint32_t nPages) {
	uint32_t __attribute__ ((unused)) error_code;
	FLS_RW_StatusTypeDef frRetStatus = FLS_RW_ERROR;

	/* Unlock the Flash to enable the flash control register access */
	if(HAL_FLASHEx_DATAEEPROM_Unlock() == HAL_OK)
	{

		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = nAddress;
		EraseInitStruct.NbPages     = nPages;

		if(HAL_FLASHEx_DATAEEPROM_Erase(nAddress) == HAL_OK)
		{
			if(HAL_FLASHEx_DATAEEPROM_Lock() == HAL_OK)
				frRetStatus = FLS_RW_OK;
		}
	}

	if(frRetStatus == FLS_RW_ERROR)
	{
		error_code = HAL_FLASH_GetError();
	}

	return FLS_RW_OK;
}
