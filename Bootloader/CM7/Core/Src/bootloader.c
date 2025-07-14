/*
 * bootloader.c
 *
 *  Created on: May 18, 2025
 *      Author: loren
 */

#include "bootloader.h"
#include "main.h"
#include <string.h>

static uint32_t flash_ptr = APP_1_ADDRESS;

typedef void (*pFunction)(void);


CRC_HandleTypeDef hcrc;
uint8_t firmware_receiving = 0;

uint8_t Bootloader_CheckForApplication(void){
	return (((*(uint32_t*)(APP_1_ADDRESS)) - RAM_D1_START) <= RAM_D1_SIZE) ? BL_OK
                                                                : BL_NO_APP;
}

uint8_t Bootloader_Erase(uint32_t Bank, uint32_t Sector, uint32_t NbSector){
	HAL_StatusTypeDef status;
	FLASH_EraseInitTypeDef eraseInitStruct;
	uint32_t sectorError = 0;

	eraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	eraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	eraseInitStruct.Banks = Bank;
	eraseInitStruct.Sector = Sector;
	eraseInitStruct.NbSectors = NbSector;

	HAL_FLASH_Unlock();

	status = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);

	HAL_FLASH_Lock();

	return (status == HAL_OK) ? BL_OK : BL_ERASE_ERROR;
}

void Bootloader_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Bootloader_Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}


uint8_t Bootloader_CheckSum(void){
	uint32_t app_size_byte = *SIZE_AREA_POINTER;
	uint32_t app_size_word = (app_size_byte +3)/4;

	uint32_t calculatedCrc = HAL_CRC_Calculate(&hcrc, (uint32_t*)(APP_1_ADDRESS), app_size_word);

	if(calculatedCrc == *CHCKSUM_AREA_POINTER){
		return BL_OK;
	}else{
		return BL_CHKS_ERROR;
	}

	return BL_OK;
}


/*
uint8_t Bootloader_GetProtectionStatus(void){
	 FLASH_OBProgramInitTypeDef obInit;
	 uint8_t protection = BL_PROTECTION_NONE;

	 // Sblocca accesso agli Option Bytes
	 HAL_FLASH_Unlock();
	 HAL_FLASH_OB_Unlock();

	 // Ottiene lo stato corrente degli Option Bytes
	 HAL_FLASHEx_OBGetConfig(&obInit);

	 // Controlla Read-Out Protection (RDP)
	 if (obInit.RDPLevel == OB_RDP_LEVEL_0){
		 printf("RDP not active (Flash readable)\n");
		 fflush(stdout);
	 }
	 else{
		 printf("RDP attivo! (Flash not readable)\n");
		 fflush(stdout);
		 protection |= BL_PROTECTION_RDP;
	 }

	 // Controllo Write Protection
	 if (obInit.WRPSector != 0x00){
		 printf("Write Protection is active on sectors: 0x%lx\n", obInit.WRPSector);
		 fflush(stdout);
		 protection |= BL_PROTECTION_WRP;
	 }
	 else{
		 printf("No Write Protection Active\n");
		 fflush(stdout);
	 }

	 return protection;
}
*/

/*
uint8_t Bootloader_ConfigProtection(uint8_t protection){
	FLASH_OBProgramInitTypeDef OBInit;
	HAL_StatusTypeDef status = HAL_ERROR;

	status = HAL_FLASH_Unlock();
	status = HAL_FLASH_OB_Unlock();


	OBInit.OptionType = OPTIONBYTE_WRP;
	OBInit.WRPSector = OB_WRP_SECTOR_All;

	if(protection & BL_PROTECTION_WRP){
		printf("Disabilito\n");
		fflush(stdout);
		OBInit.WRPState = OB_WRPSTATE_DISABLE;
	}else{
		printf("Abilito\n");
		fflush(stdout);
		OBInit.WRPState = OB_WRPSTATE_ENABLE;
	}

	status = HAL_FLASHEx_OBProgram(&OBInit);
	printf("OB Program: %d\n", status);
	fflush(stdout);

	if(status == HAL_OK){
		printf("Sto qua dentro\n");
		fflush(stdout);
		status = HAL_FLASH_OB_Launch();
		printf("OB Launch: %d\n", status);
		fflush(stdout);
	}

	 status = HAL_FLASH_OB_Lock();
	 status = HAL_FLASH_Lock();

	 return (status == HAL_OK) ? BL_OK : BL_OBP_ERROR;
}
*/

uint8_t Bootloader_Write_Flash(uint8_t* data, uint32_t len){

	uint8_t flash_word[32];
	for(uint32_t i = 0; i < len; i+=32){
		memset(flash_word, 0xFF, sizeof(flash_word));
		if((len-i) > 32){
			memcpy(&flash_word, &data[i], 32);
		}else{
			memcpy(&flash_word, &data[i], len-i);
		}

		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flash_ptr, (uint32_t)flash_word) ==  HAL_OK){
			if (memcmp((void*)flash_ptr, flash_word, 32) != 0) {
			    HAL_FLASH_Lock();
			    return BL_VERIFY_ERROR;
			}else{
				flash_ptr += 32;
			}
		}else{
			HAL_FLASH_Lock();
			return BL_WRITE_ERROR;
		}
	}

	return BL_OK;
}

uint8_t Bootloader_Write_Header(uint8_t* data, uint32_t len){
	flash_ptr = APP_1_ADDRESS;
	uint8_t flash_word[32];
	memset(flash_word, 0xFF, sizeof(flash_word));
	//se è già avvenuta almeno una scrittura allora è necessario salvare i vecchi parametri
	if(Flash_ReadFlag() != 0xFFFFFFFF){
		uint32_t old_version = *VERSION_AREA_POINTER;
		uint32_t old_checksum = *CHCKSUM_AREA_POINTER;
		uint32_t old_size = *SIZE_AREA_POINTER;

		Bootloader_Erase(FLASH_BANK_1, FLASH_SECTOR_1, 1);

		HAL_FLASH_Unlock();

		memcpy(&flash_word, &old_version, 4);
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, VERSION_OLD_AREA, (uint32_t)flash_word) !=  HAL_OK){
			HAL_FLASH_Lock();
			return BL_WRITE_ERROR;
		}

		memcpy(&flash_word, &old_checksum, 4);
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,CHCKSUM_OLD_AREA, (uint32_t)flash_word) !=  HAL_OK){
			HAL_FLASH_Lock();
			return BL_WRITE_ERROR;
		}

		memcpy(&flash_word, &old_size, 4);
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,SIZE_OLD_AREA, (uint32_t)flash_word) !=  HAL_OK){
			HAL_FLASH_Lock();
			return BL_WRITE_ERROR;
		}

		HAL_FLASH_Lock();
	}


	HAL_FLASH_Unlock();
	memcpy(&flash_word, &data[0], 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, VERSION_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return BL_WRITE_ERROR;
	}

	memcpy(&flash_word, &data[4], 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, CHCKSUM_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return BL_WRITE_ERROR;
	}

	memcpy(&flash_word, &data[8], 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, SIZE_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return BL_WRITE_ERROR;
	}
	HAL_FLASH_Lock();

	return BL_OK;
}

uint8_t Bootloader_CheckVersion (uint8_t* data, uint32_t len){
	uint32_t act_ver = *VERSION_AREA_POINTER;
	uint32_t flash_word;
	memcpy(&flash_word, &data[0], 4);

	if((act_ver == 0xFFFFFFFF) || (flash_word > act_ver)){
		return BL_OK;
	}else{
		return BL_VER_ERROR;
	}
}

uint8_t Bootloader_CheckSize(uint8_t* data, uint32_t len){
	uint32_t flash_word;
	memcpy(&flash_word, &data[8], 4);
	uint32_t app_size_word = (flash_word + 3)/4;

	if(app_size_word >= APP_SIZE){
		return BL_SIZE_ERROR;
	}else{
		return BL_OK;
	}

}

uint8_t  Flash_WriteFlag(uint32_t flag_value){

	HAL_FLASH_Unlock();
	uint8_t flash_word [32];
	memset(flash_word, 0xFF, sizeof(flash_word));
	memcpy(&flash_word, &flag_value, 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, FLAG_FLASH_ADDRESS, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return BL_FLAG_ERROR;
	}

	HAL_FLASH_Lock();

	return BL_OK;
}

uint32_t Flash_ReadFlag(void){
	return *(uint32_t*)FLAG_FLASH_ADDRESS;
}

void Bootloader_PreviousVersion(void){
	uint32_t version = *VERSION_OLD_AREA_POINTER;
	uint32_t chcksum = *CHCKSUM_OLD_AREA_POINTER;
	uint32_t size = *SIZE_OLD_AREA_POINTER;

	Bootloader_Erase(FLASH_BANK_1, FLASH_SECTOR_1, 7);

	uint8_t flash_word[32];
	memset(flash_word, 0xFF, sizeof(flash_word));

	HAL_FLASH_Unlock();

	memcpy(&flash_word, &version, 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, VERSION_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return;
	}
	memcpy(&flash_word, &chcksum, 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, CHCKSUM_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return;
	}

	memcpy(&flash_word, &size, 4);
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, SIZE_AREA, (uint32_t)flash_word) !=  HAL_OK){
		HAL_FLASH_Lock();
		return;
	}

	uint32_t src = APP_2_ADDRESS;
	for(uint32_t offset = 0; offset < APP_SIZE; offset += 32){
		uint8_t data[32];
		memcpy(data, (void*)src, 32);
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, APP_1_ADDRESS + offset, (uint32_t)data) != HAL_OK){
			return;
		}
		src += 32;
	}

	HAL_FLASH_Lock();
}

void Bootloader_JumpToApplication(void){
	uint32_t JumpAddress = *(__IO uint32_t*)(APP_1_ADDRESS + 4);
	pFunction Jump = (pFunction)JumpAddress;

	HAL_RCC_DeInit();
	HAL_DeInit();

	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL  = 0;

	#if(SET_VECTOR_TABLE)
		SCB->VTOR = APP_1_ADDRESS;
	#endif

	__set_MSP(*(__IO uint32_t*)(APP_1_ADDRESS));

	Jump();
}

void Bootloader_Error_Handler(void){
	__disable_irq();
	LED_All_Off();
	LD3_Red_On();
	while(1){

	}
}
