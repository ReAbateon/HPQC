/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
#define LD1_Green_Port 		GPIOB
#define LD1_Green_Pin		GPIO_PIN_0
#define LD2_Yellow_Port 	GPIOE
#define LD2_Yellow_Pin		GPIO_PIN_1
#define LD3_Red_Port 		GPIOB
#define LD3_Red_Pin			GPIO_PIN_14

#define LD1_Green_On()		HAL_GPIO_WritePin(LD1_Green_Port, LD1_Green_Pin, GPIO_PIN_SET);  		// accende LD1
#define LD1_Green_Off()		HAL_GPIO_WritePin(LD1_Green_Port, LD1_Green_Pin, GPIO_PIN_RESET); 	 	// spegne LD1
#define LD2_Yellow_On()		HAL_GPIO_WritePin(LD2_Yellow_Port, LD2_Yellow_Pin, GPIO_PIN_SET);  		// accende LD2
#define LD2_Yellow_Off()	HAL_GPIO_WritePin(LD2_Yellow_Port, LD2_Yellow_Pin, GPIO_PIN_RESET);  	// spegne LD2
#define LD3_Red_On()		HAL_GPIO_WritePin(LD3_Red_Port, LD3_Red_Pin, GPIO_PIN_SET);  			// accende LD3
#define LD3_Red_Off()		HAL_GPIO_WritePin(LD3_Red_Port, LD3_Red_Pin, GPIO_PIN_RESET); 			// spegne LD3

#define LED_All_On() \
    do               \
    {                \
    	LD1_Green_On();  \
    	LD2_Yellow_On();  \
    	LD3_Red_On(); \
    } while(0)

#define LED_All_Off() \
    do               \
    {                \
    	LD1_Green_Off();  \
    	LD2_Yellow_Off();  \
    	LD3_Red_Off(); \
    } while(0)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
