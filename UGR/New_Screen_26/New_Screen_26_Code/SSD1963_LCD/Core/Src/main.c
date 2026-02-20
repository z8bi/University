/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "dashboard.h"
#include "gfx.h"
#include "ssd1963.h"

//Logos
#include "logos/ugr_logo.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// Simple loopback test state
static volatile uint8_t  can2_rx_flag = 0;
static volatile uint16_t can2_rx_u16  = 0;

static CAN_RxHeaderTypeDef can2_rx_hdr;
static uint8_t             can2_rx_data[8];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FMC_Init();
  MX_CAN2_Init();
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_TIM12_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  __enable_irq();

  // Start backlight PWM (your external brightness control on PB15)
  HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, __HAL_TIM_GET_AUTORELOAD(&htim12) / 2); // 50%
  SSD1963_Init();


  //CAN TEST
  CAN2_SetLoopbackAndStart();

  //================== UGR LOGO TEST ==================
  //draw_UGR_logo();

  /* USER CODE END 2 */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    //================== CAN TEST ==================
    /*
    uint8_t ok = CAN2_LoopbackSendU16(123, 50);

    if (ok) SSD1963_Fill(RGB565(0, 200, 0));   // green = CAN loopback OK
    else    SSD1963_Fill(RGB565(200, 0, 0));   // red = fail / timeout
    HAL_Delay(1000);

    //================== DASHBOARD TEST WITH CAN ==================
    
    
    static Dashboard d = {0};   // persistent state for dash_update

    if (can2_rx_flag) {
        uint16_t v;

        __disable_irq();
        v = can2_rx_u16;
        can2_rx_flag = 0;
        __enable_irq();

        d.cell_temperature  = (int)(v / 1000);   // 0..65  (upper part)
        d.water_temperature = (int)(v % 1000);   // 0..999 (lower part)

        dash_update(&d);
    }
    */

    // 1) Solid fills (quick sanity)
    SSD1963_Fill(RGB565(255, 0, 0));     // red
    HAL_Delay(250);
    SSD1963_Fill(RGB565(0, 255, 0));     // green
    HAL_Delay(250);
    SSD1963_Fill(RGB565(0, 0, 255));     // blue
    HAL_Delay(250);
    SSD1963_Fill(RGB565(255, 255, 255)); // white
    HAL_Delay(250);

    // 2) Color bars (checks RGB order & gradients)
    SSD1963_TestColorBars();
    HAL_Delay(800);

    // 3) Checkerboard (checks address windowing / tearing)
    SSD1963_TestCheckerboard(24);
    HAL_Delay(800);

    // 4) Moving rect (checks X/Y direction)
    SSD1963_Fill(RGB565(0,0,0));
    for (uint16_t x = 0; x < 800; x += 20) {
      SSD1963_Fill(RGB565(0,0,0));
      SSD1963_FillRect(x, 200, 80, 80, RGB565(255, 120, 0));
      HAL_Delay(20);
    }
    /* USER CODE BEGIN 3 */
    //
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void CAN2_SetLoopbackAndStart(void)
{
  // Switch to internal loopback and re-init CAN
  hcan2.Init.Mode = CAN_MODE_LOOPBACK;
  if (HAL_CAN_Init(&hcan2) != HAL_OK) {
    Error_Handler();
  }

  // Accept-all filter -> FIFO0
  // Filter banks are shared; 14 is commonly safe if nothing else uses filters.
  CAN_FilterTypeDef f;
  memset(&f, 0, sizeof(f));

  f.FilterActivation     = ENABLE;
  f.FilterMode           = CAN_FILTERMODE_IDMASK;
  f.FilterScale          = CAN_FILTERSCALE_32BIT;
  f.FilterIdHigh         = 0x0000;
  f.FilterIdLow          = 0x0000;
  f.FilterMaskIdHigh     = 0x0000;
  f.FilterMaskIdLow      = 0x0000;
  f.FilterFIFOAssignment = CAN_RX_FIFO0;

  f.FilterBank           = 14;
  f.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan2, &f) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_CAN_Start(&hcan2) != HAL_OK) {
    Error_Handler();
  }

  // Enable RX FIFO0 interrupt callback
  if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }
}

uint8_t CAN2_LoopbackSendU16(uint16_t value, uint32_t timeout_ms)
{
  CAN_TxHeaderTypeDef txh;
  memset(&txh, 0, sizeof(txh));

  uint32_t mailbox;
  uint8_t data[2];

  data[0] = (uint8_t)(value & 0xFF);
  data[1] = (uint8_t)((value >> 8) & 0xFF);

  // reset rx state
  can2_rx_flag = 0;
  can2_rx_u16  = 0;

  txh.IDE = CAN_ID_STD;
  txh.StdId = 0x123;
  txh.RTR = CAN_RTR_DATA;
  txh.DLC = 2;
  txh.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(&hcan2, &txh, data, &mailbox) != HAL_OK) {
    return 0;
  }

  uint32_t t0 = HAL_GetTick();
  while (!can2_rx_flag) {
    if ((HAL_GetTick() - t0) > timeout_ms) return 0;
  }

  return (can2_rx_u16 == value) ? 1 : 0;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance != CAN2) return;

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can2_rx_hdr, can2_rx_data) == HAL_OK) {
    if (can2_rx_hdr.DLC >= 2) {
      can2_rx_u16 = (uint16_t)can2_rx_data[0] | ((uint16_t)can2_rx_data[1] << 8);
    }
    can2_rx_flag = 1;
  }
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x60000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
