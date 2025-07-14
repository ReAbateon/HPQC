/*
 * bootloader.h
 *
 *  Created on: May 18, 2025
 *      Author: loren
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include "stm32h7xx_hal.h"

#define USE_CHECKSUM 1

#define USE_WRITE_PROTECTION 0

#define SET_VECTOR_TABLE 1

#define CLEAR_RESET_FLAGS 1

#define VERSION_AREA ((uint32_t)0x08020000)

#define CHCKSUM_AREA ((uint32_t)0x08020020)

#define SIZE_AREA ((uint32_t)0x08020040)

#define VERSION_AREA_POINTER ((uint32_t*)0x08020000)

#define CHCKSUM_AREA_POINTER ((uint32_t*)0x08020020)

#define SIZE_AREA_POINTER ((uint32_t*)0x08020040)

#define VERSION_OLD_AREA (uint32_t)0x08020060

#define CHCKSUM_OLD_AREA (uint32_t)0x08020080

#define SIZE_OLD_AREA (uint32_t)0x080200A0

#define VERSION_OLD_AREA_POINTER (uint32_t*)0x08020060

#define CHCKSUM_OLD_AREA_POINTER (uint32_t*)0x08020080

#define SIZE_OLD_AREA_POINTER (uint32_t*)0x080200A0

#define APP_1_ADDRESS (uint32_t)0x08040000

#define END_APP_1_ADDRESS (uint32_t)0x080FFFFF

#define APP_2_ADDRESS (uint32_t)0x08100000

#define END_APP_2_ADDRESS (uint32_t)0x081BFFFF

#define FLAG_FLASH_ADDRESS (uint32_t)0x081E0000

/** Size of application in DWORD (32bits or 4bytes) */
#define APP_SIZE (uint32_t)(((END_APP_1_ADDRESS - APP_1_ADDRESS) + 3) / 4)

#define RAM_D1_START	0x24000000
#define RAM_D1_SIZE 	(uint32_t)0x80000	//512 KB

/** Bootloader error codes */
enum eBootloaderErrorCodes
{
    BL_OK = 0,      /*!< No error */
    BL_NO_APP,      /*!< No application found in flash */
    BL_SIZE_ERROR,  /*!< New application is too large for flash */
    BL_CHKS_ERROR,  /*!< Application checksum error */
    BL_ERASE_ERROR, /*!< Flash erase error */
    BL_WRITE_ERROR, /*!< Flash write error */
    BL_OBP_ERROR,   /*!< Flash option bytes programming error */
	BL_FLAG_ERROR,	/*!< Flag rewrite error */
	BL_VER_ERROR,	/*!< Update Version error */
	BL_VERIFY_ERROR /*!< Verify written data error */
};

enum eFlashProtectionTypes
{
    BL_PROTECTION_NONE  = 0,   /*!< No flash protection */
    BL_PROTECTION_WRP   = 0x1, /*!< Flash write protection */
    BL_PROTECTION_RDP   = 0x2 /*!< Flash read protection */
};

extern uint8_t firmware_receiving;
extern uint32_t last_receive_time;
extern uint8_t ack_send;
extern uint8_t check_res;

uint8_t Bootloader_CheckForApplication(void);
uint8_t Bootloader_Erase(uint32_t Bank, uint32_t Sector, uint32_t NbSector);
uint8_t Bootloader_CheckSum(void);

//uint8_t Bootloader_GetProtectionStatus(void);
//uint8_t Bootloader_ConfigProtection(uint8_t protection);

uint8_t Bootloader_Write_Flash(uint8_t* data, uint32_t len);
uint8_t Bootloader_Write_Header(uint8_t* data, uint32_t len);
uint8_t Bootloader_CheckVersion (uint8_t* data, uint32_t len);
uint8_t Bootloader_CheckSize(uint8_t* data, uint32_t len);

uint8_t  Flash_WriteFlag(uint32_t flag_value);
uint32_t Flash_ReadFlag(void);

void Bootloader_CRC_Init(void);
void Bootloader_JumpToApplication(void);
void Bootloader_PreviousVersion(void);
void Bootloader_Error_Handler(void);

#endif /* INC_BOOTLOADER_H_ */
