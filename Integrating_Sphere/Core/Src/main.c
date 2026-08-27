/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - VEML7700 Ambient Light Sensor
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VEML7700_ADDR          (0x10 << 1) // 0x20 in 8-bit HAL format
#define VEML7700_REG_ALS_CONF  0x00        // ALS Configuration Register
#define VEML7700_REG_ALS_DATA  0x04        // ALS Output Data Register
#define VEML7700_REG_WHITE     0x05        // White Channel Data Register
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint16_t raw_als = 0;
float lux = 0.0f;
char msg_buffer[128];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
HAL_StatusTypeDef VEML7700_Init(void);
float VEML7700_Read_Lux(void);
void Send_Output(char *str);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* -------------------------------------------------------------------------- */
/* Helper to send text over both USB CDC and USART1                           */
/* -------------------------------------------------------------------------- */
void Send_Output(char *str)
{
    uint16_t len = (uint16_t)strlen(str);

    // Send via USB CDC (Virtual COM Port)
    CDC_Transmit_FS((uint8_t *)str, len);

    // Also send via Hardware UART1 (PA9)
    HAL_UART_Transmit(&huart1, (uint8_t *)str, len, 100);
}

/* -------------------------------------------------------------------------- */
/* VEML7700 Initialization (Gain: 1x, Integration Time: 100ms, Power ON)     */
/* -------------------------------------------------------------------------- */
HAL_StatusTypeDef VEML7700_Init(void)
{
    // Register 0x00: Bit 0 = 0 (Power ON), Bits 9:6 = 0000 (100ms), Bits 12:11 = 00 (Gain 1x)
    uint8_t config_data[2] = {0x00, 0x00};

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        &hi2c1,
        VEML7700_ADDR,
        VEML7700_REG_ALS_CONF,
        I2C_MEMADD_SIZE_8BIT,
        config_data,
        2,
        HAL_MAX_DELAY
    );

    HAL_Delay(10); // Sensor power-up time
    return status;
}

/* -------------------------------------------------------------------------- */
/* Read Ambient Light Sensor Register and Convert to Lux                      */
/* -------------------------------------------------------------------------- */
float VEML7700_Read_Lux(void)
{
    uint8_t buffer[2] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &hi2c1,
        VEML7700_ADDR,
        VEML7700_REG_ALS_DATA,
        I2C_MEMADD_SIZE_8BIT,
        buffer,
        2,
        HAL_MAX_DELAY
    );

    if (status != HAL_OK)
    {
        return -1.0f; // Communication error
    }

    // 16-bit word format (LSB in buffer[0], MSB in buffer[1])
    raw_als = (uint16_t)((buffer[1] << 8) | buffer[0]);

    // For Gain = 1x and Integration Time = 100ms: Resolution = 0.0576 lx/count
    return (float)raw_als * 0.0576f;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* Force USB Re-enumeration (helps PC detect USB device reliably) */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_Delay(10);

  /* Initialize USB Device */
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN 2 */
  HAL_Delay(1500); // Give the PC USB Host time to enumerate the Virtual COM Port

  // Send startup greeting
  Send_Output("\r\n========================================\r\n");
  Send_Output(" STM32F103C8T6 + VEML7700 Ambient Light \r\n");
  Send_Output("========================================\r\n");

  // Initialize VEML7700
  if (VEML7700_Init() != HAL_OK)
  {
      Send_Output("[ERROR] VEML7700 initialization failed!\r\n");
      Send_Output("Please check I2C wiring (PB6=SCL, PB7=SDA) & pull-ups.\r\n");
      Error_Handler();
  }
  else
  {
      Send_Output("[OK] VEML7700 initialized successfully.\r\n\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 1. Read lux from sensor
      lux = VEML7700_Read_Lux();

      // 2. Format output string (Separating integer and fractional avoids GCC float issues)
      if (lux < 0.0f)
      {
          snprintf(msg_buffer, sizeof(msg_buffer), "[I2C ERROR] Failed to read sensor!\r\n");
      }
      else
      {
          int lux_int = (int)lux;
          int lux_dec = (int)((lux - (float)lux_int) * 100.0f);
          if (lux_dec < 0) lux_dec = -lux_dec;

          snprintf(
              msg_buffer,
              sizeof(msg_buffer),
              "Raw ALS: %-5u | Lux: %d.%02d lx\r\n",
              raw_als,
              lux_int,
              lux_dec
          );
      }

      // 3. Transmit data to PC
      Send_Output(msg_buffer);

      // 4. Delay 500ms between updates
      HAL_Delay(500);

    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */
  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */
  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */
  /* USER CODE END I2C1_Init 2 */
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  /* USER CODE END USART1_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
