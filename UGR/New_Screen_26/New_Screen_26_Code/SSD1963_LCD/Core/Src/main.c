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
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
  #include <string.h>
  #include "gfx.h"
  #include "ssd1963.h"
  #include "gt911.h" //touchscreen

  #include "dashboard/endurance_dashboard.h"
  #include "dashboard/pedal_graph_dashboard.h"
  #include "dashboard/dashboard_tests.h"

  //sd card
  #include "sd_card.h"
  #include "fatfs.h"
  #include "ff.h"

  //Logos
  #include "logos/ugr_logo.h"

  //USB
  #include "usbd_core.h"
  extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

//USB REQUESTS

typedef enum {
    SCREEN_REQ_NONE = 0,
    SCREEN_REQ_LOGO,
    SCREEN_REQ_ENDURANCE,
    SCREEN_REQ_PEDAL
} ScreenRequest;

static volatile uint8_t g_enter_dfu_requested = 0;
static volatile uint8_t g_screen_change_requested = 0;
static volatile ScreenRequest g_requested_screen = SCREEN_REQ_NONE;

void Request_EnterDfu(void)
{
    g_enter_dfu_requested = 1;
}

void Request_LogoScreen(void)
{
    g_requested_screen = SCREEN_REQ_LOGO;
    g_screen_change_requested = 1;
}

void Request_EnduranceScreen(void)
{
    g_requested_screen = SCREEN_REQ_ENDURANCE;
    g_screen_change_requested = 1;
}

void Request_PedalScreen(void)
{
    g_requested_screen = SCREEN_REQ_PEDAL;
    g_screen_change_requested = 1;
}

/* USER CODE END PFP */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
    const char *fr_str(FRESULT fr)
    {
        switch (fr)
        {
            case FR_OK: return "FR_OK";
            case FR_DISK_ERR: return "FR_DISK_ERR";
            case FR_INT_ERR: return "FR_INT_ERR";
            case FR_NOT_READY: return "FR_NOT_READY";
            case FR_NO_FILE: return "FR_NO_FILE";
            case FR_NO_PATH: return "FR_NO_PATH";
            case FR_INVALID_NAME: return "FR_INVALID_NAME";
            case FR_DENIED: return "FR_DENIED";
            case FR_EXIST: return "FR_EXIST";
            case FR_INVALID_OBJECT: return "FR_INVALID_OBJECT";
            case FR_WRITE_PROTECTED: return "FR_WRITE_PROTECTED";
            case FR_INVALID_DRIVE: return "FR_INVALID_DRIVE";
            case FR_NOT_ENABLED: return "FR_NOT_ENABLED";
            case FR_NO_FILESYSTEM: return "FR_NO_FILESYSTEM";
            case FR_MKFS_ABORTED: return "FR_MKFS_ABORTED";
            case FR_TIMEOUT: return "FR_TIMEOUT";
            case FR_LOCKED: return "FR_LOCKED";
            case FR_NOT_ENOUGH_CORE: return "FR_NOT_ENOUGH_CORE";
            case FR_TOO_MANY_OPEN_FILES: return "FR_TOO_MANY_OPEN_FILES";
            case FR_INVALID_PARAMETER: return "FR_INVALID_PARAMETER";
            default: return "FR_UNKNOWN";
        }
    }
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
  #define SCREEN_TEST_MODE 1 //Set to 1 to have the screen keep incrementing all values to test

  #if SCREEN_TEST_MODE
    static DashboardTestSim dashboard_test_sim;
    #endif

  #define CAN_ID_DASHBOARD      0x100
  #define CAN_DASHBOARD_DLC     5 //Length of data

  #define DASH_IDX_BATTERY      0
  #define DASH_IDX_CELL_TEMP    1
  #define DASH_IDX_WATER_TEMP   2
  #define DASH_IDX_SPEED_L      3
  #define DASH_IDX_SPEED_H      4

    /* 
    ===========================================
    CAN message format (ID 0x100):
    byte0 = battery_charge
    byte1 = cell_temperature
    byte2 = water_temperature
    byte3 = speed low byte
    byte4 = speed high byte
    ===========================================
    */ 

  #define CAN2_RX_QUEUE_SIZE 16

  #define LCD_BRIGHTNESS 10 // adjust as needed (0-100%)

  #define TOUCH_COOLDOWN_MS 250

  typedef struct
  {
      CAN_RxHeaderTypeDef hdr;
      uint8_t data[8];
  } CAN2_RxMessage;

static Dashboard d = {
    .lap = 0,
    .battery_charge = 0,
    .cell_temperature = 0,
    .water_temperature = 0, //obsolete
    .speed = 0,
    .throttle_percent = 0,
    .brake_percent = 0
};

enum Screen_State {
    ENDURANCE,
    PEDAL,
    LOGO
};

//==================STARTING STATE==============
static enum Screen_State screen_state = ENDURANCE;
static volatile uint8_t screen_updated = 0;
static volatile uint8_t updated = 0;


static uint8_t last_touch = 0;
static uint32_t next_touch_allowed = 0;
static GT911_State touch;
static uint8_t touch_ok = 0;


  static volatile uint8_t can2_rx_head = 0;
  static volatile uint8_t can2_rx_tail = 0;
  static volatile uint8_t can2_rx_overflow = 0;

  static CAN2_RxMessage can2_rx_queue[CAN2_RX_QUEUE_SIZE];
  static uint8_t CAN2_RxQueue_Pop(CAN2_RxMessage *msg);
  static uint8_t CAN2_RxQueue_Push(const CAN_RxHeaderTypeDef *hdr, const uint8_t *data);
  void CAN2_Start(void);

  //LCD Brightness control using TIM12 CH2 PWM on PB15
  static inline void LCD_SetBrightness(uint8_t percent)
  {
      if (percent > 100) percent = 100;

      uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim12);
      __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (arr * percent) / 100);
  }

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

static void JumpToSystemBootloader(void);
void Request_EnterDfu(void);
void Request_LogoScreen(void);
void Request_EnduranceScreen(void);
void Request_PedalScreen(void);

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
  MX_FATFS_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

    // Initialize GT911 touchscreen
    touch_ok = (GT911_Init(&hi2c1) == HAL_OK) ? 1 : 0;

    //CAN INIT
    CAN2_Start();
    __enable_irq(); 

    // Start backlight PWM (your external brightness control on PB15)
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
    LCD_SetBrightness(LCD_BRIGHTNESS); // adjust as needed (0-100%)
    
    //Main LOGO screen initialization
    SSD1963_Init();
    SSD1963_Fill(RGB565(0, 0, 0));
    

    //DRAW BIG UGR LOGO FROM SD CARD IF PRESENT
    if (f_mount(&USERFatFS, USERPath, 1) == FR_OK)
    {   
        draw_big_UGR_logo();
        HAL_Delay(2000);
        screen_state = PEDAL;
        screen_updated = 1;
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    
    while (1)
    {
        /*
        //=========================================================
        //=================TOUCH SCREEN WHILE PART=================
        //=========================================================
         
        if (touch_ok)
        {
            if (GT911_ReadState(&touch) == HAL_OK)
            {
                if (touch.touched && !last_touch)
                {
                    uint32_t now = HAL_GetTick();

                    if (now >= next_touch_allowed)
                    {
                        UI_Area a = (screen_state == ENDURANCE)
                                ? dash_get_area(DASH_AREA_SMALL_LOGO)
                                : dash_get_area(DASH_AREA_BIG_LOGO);

                        uint16_t tx = touch.x;
                        uint16_t ty = touch.y;

                        if (tx >= a.x1 && tx <= a.x2 &&
                            ty >= a.y1 && ty <= a.y2)
                        {
                            screen_state = (screen_state == ENDURANCE) ? LOGO : ENDURANCE;
                            SSD1963_Fill(RGB565(0, 0, 0));

                            if (screen_state == ENDURANCE)
                            {
                                dash_init(d.battery_charge,
                                        d.cell_temperature,
                                        d.water_temperature,
                                        d.speed);
                            }
                            else
                            {
                                //draw_big_UGR_logo();
                            }

                            screen_updated = 1;
                            next_touch_allowed = now + TOUCH_COOLDOWN_MS;
                        }
                    }
                }

                last_touch = touch.touched;
            }
        }
        */

        //==================================================================
        //========GO TO BOOTLOADER OR SWITCH SCREEN FROM USB INPUT==========
        //==================================================================

        if (g_enter_dfu_requested)
        {
            HAL_Delay(20);
            JumpToSystemBootloader();
        }

        if (g_screen_change_requested)
        {
            g_screen_change_requested = 0;

            SSD1963_Fill(RGB565(0, 0, 0));

            switch (g_requested_screen)
            {
                case SCREEN_REQ_LOGO:
                    screen_state = LOGO;
                    draw_big_UGR_logo();
                    screen_updated = 0;
                    break;

                case SCREEN_REQ_ENDURANCE:
                    screen_state = ENDURANCE;
                    endurance_dash_init(
                            d.battery_charge,
                            d.cell_temperature,
                            d.lap,
                            d.speed); 
                    updated = 1;
                    break;

                case SCREEN_REQ_PEDAL:
                    screen_state = PEDAL;
                    pedal_graph_dash_init(
                            d.battery_charge,
                            d.cell_temperature,
                            d.lap,
                            d.speed); 
                    updated = 1;
                    break;

                case SCREEN_REQ_NONE:
                default:
                    break;
            }

            g_requested_screen = SCREEN_REQ_NONE;
        }

        //==================================================================
        //=================State machine for screen updates=================
        //==================================================================
        
        switch(screen_state)
        {
            case ENDURANCE:

                if (updated) {
                    updated = 0;
                    endurance_dash_update(&d);
                }

                break;

            case PEDAL:

                if (updated) {
                    updated = 0;
                    pedal_graph_dash_update(&d);
                }

                break;

            case LOGO:
                if (screen_updated) {
                    screen_updated = 0;
                    draw_big_UGR_logo();
                }
                
                break;
        }
        
        //==================================================================
        //=================CAN RX SECTION - IMPORTANT!!!!!!=================
        //==================================================================

        CAN2_RxMessage msg;
        uint8_t has_msg;

        do
        {
            __disable_irq();
            has_msg = CAN2_RxQueue_Pop(&msg);
            __enable_irq();

            if (has_msg)
            {
                if (msg.hdr.IDE == CAN_ID_STD && msg.hdr.StdId == CAN_ID_DASHBOARD)
                {
                    if (msg.hdr.DLC >= CAN_DASHBOARD_DLC) {
                        d.battery_charge    = msg.data[DASH_IDX_BATTERY];
                        d.cell_temperature  = msg.data[DASH_IDX_CELL_TEMP];
                        d.water_temperature = msg.data[DASH_IDX_WATER_TEMP];
                        d.speed = (uint16_t)msg.data[DASH_IDX_SPEED_L] |
                                ((uint16_t)msg.data[DASH_IDX_SPEED_H] << 8);
                        updated = 1;
                    }
                }
            }

        } while (has_msg);

        //==================================================================
        //==========THIS ONLY RUNS IF TESTING THE SCREEN'S WIDGETS==========
        //==================================================================

        #if SCREEN_TEST_MODE
        static uint32_t last_screen_test = 0;

        if (HAL_GetTick() - last_screen_test >= 20)
        {
            last_screen_test = HAL_GetTick();

            dashboard_test_sim_step(&dashboard_test_sim, &d, last_screen_test);

            updated = 1;
        }
        #endif

        HAL_Delay(1);
        
    }
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

static void JumpToSystemBootloader(void)
{
    void (*SysMemBootJump)(void);
    uint32_t sysmem_addr = 0x1FF00000U;   // verify for your exact STM32F767 variant

    __disable_irq();

    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_BOOT;

    __DSB();
    __ISB();

    __set_MSP(*(volatile uint32_t *)sysmem_addr);
    SysMemBootJump = (void (*)(void))(*(volatile uint32_t *)(sysmem_addr + 4U));
    SysMemBootJump();

    while (1) {
    }
}

void CAN2_Start(void)
{
    CAN_FilterTypeDef f;
    memset(&f, 0, sizeof(f));

    f.FilterActivation     = ENABLE;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterBank           = 14;
    f.SlaveStartFilterBank = 14;

    // Standard ID exact match for 0x100
    f.FilterIdHigh         = (CAN_ID_DASHBOARD << 5);
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = (0x7FF << 5); // All 11 bits of the ID matter (7FF is 0111 1111 1111)
    f.FilterMaskIdLow      = 0x0000;

    if (HAL_CAN_ConfigFilter(&hcan2, &f) != HAL_OK)
        Error_Handler();

    if (HAL_CAN_Start(&hcan2) != HAL_OK)
        Error_Handler();

    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
        Error_Handler();
}


  void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
  {
      if (hcan->Instance != CAN2) return;

      CAN_RxHeaderTypeDef rxh;
      uint8_t data[8];

      while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0)
      {
          if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxh, data) != HAL_OK) {
              return;
          }

          CAN2_RxQueue_Push(&rxh, data);
      }
  }

  static uint8_t CAN2_RxQueue_Push(const CAN_RxHeaderTypeDef *hdr, const uint8_t *data)
  {
      uint8_t next = (uint8_t)((can2_rx_head + 1) % CAN2_RX_QUEUE_SIZE);

      if (next == can2_rx_tail) {
          can2_rx_overflow = 1;   // queue full
          return 0;
      }

      can2_rx_queue[can2_rx_head].hdr = *hdr;
      for (int i = 0; i < 8; i++) {
          can2_rx_queue[can2_rx_head].data[i] = data[i];
      }

      can2_rx_head = next;
      return 1;
  }

  static uint8_t CAN2_RxQueue_Pop(CAN2_RxMessage *msg)
  {
      if (can2_rx_head == can2_rx_tail) {
          return 0;   // empty
      }

      *msg = can2_rx_queue[can2_rx_tail];
      can2_rx_tail = (uint8_t)((can2_rx_tail + 1) % CAN2_RX_QUEUE_SIZE);
      return 1;
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
