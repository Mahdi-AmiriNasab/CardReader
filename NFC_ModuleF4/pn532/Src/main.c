///* USER CODE BEGIN Header */
///* USER CODE END Header */

///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//#include "usb_device.h"

///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
//#include "usbd_cdc_if.h"
//#include "Adafruit_PN532.h"
//#include "stdio.h"
////#include "Adafruit_PN532.h"

///* USER CODE END Includes */

///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
///* USER CODE END PTD */

///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */
///* USER CODE END PD */

///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */
///* USER CODE END PM */

///* Private variables ---------------------------------------------------------*/
//I2C_HandleTypeDef hi2c1;

//SPI_HandleTypeDef hspi1;

//UART_HandleTypeDef huart2;

///* USER CODE BEGIN PV */
///* USER CODE END PV */

///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_SPI1_Init(void);
//static void MX_I2C1_Init(void);
//static void MX_USART2_UART_Init(void);
///* USER CODE BEGIN PFP */
///* USER CODE END PFP */

///* Private user code ---------------------------------------------------------*/
///* USER CODE BEGIN 0 */
///* USER CODE END 0 */

///**
//  * @brief  The application entry point.
//  * @retval int
//  */
//int main(void)
//{
//  /* USER CODE BEGIN 1 */
//  /* USER CODE END 1 */
//  

//  /* MCU Configuration--------------------------------------------------------*/

//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();

//  /* USER CODE BEGIN Init */
//  /* USER CODE END Init */

//  /* Configure the system clock */
//  SystemClock_Config();

//  /* USER CODE BEGIN SysInit */
//  /* USER CODE END SysInit */

//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  MX_SPI1_Init();
//  MX_USB_DEVICE_Init();
//  MX_I2C1_Init();
//  MX_USART2_UART_Init();
//  /* USER CODE BEGIN 2 */
//  /* USER CODE END 2 */

//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//	uint8_t send[100] = {0x00, 0x00, 0xff, 0x01, 0xff, 0xd4, 0x02, 0x2a, 0x00}
//	, receive[100];
//		
////	send[0] = 0x01;
////	send[1] = 0x00;
////	send[2] = 0x00;
////	send[3] = 0xff;
////	send[4] = 0x01;
////	send[5] = 0xff;
////	send[6] = 0xd4;
////	send[7] = 0x02;
////	send[8] = 0x2a;
////	send[9] = 0x00;
//		
//		Adafruit_PN532 rfid(irq_pin, reset_pin);
//		while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));
//		//HAL_GPIO_WritePin(GPIOD , GPIO_PIN_15, GPIO_PIN_SET);

//	
//	while(1)
//	{
//    /* USER CODE END WHILE */

//    /* USER CODE BEGIN 3 */
//		

//		
////	if(HAL_I2C_Master_Transmit(&hi2c1, 0X48, send, 9, 100) == HAL_OK)
////	{
////		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
////		HAL_Delay(100);
////		HAL_I2C_Master_Receive(&hi2c1, 0x49, receive, 10, 100);
////	}
////	else
////	{
////		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
////	}
////	
//		
////	if(HAL_I2C_Master_Receive(&hi2c1, 0x49, receive, 10, 100) == HAL_OK)
////	{
////		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
////		HAL_Delay(100);
////		//HAL_I2C_Master_Receive(&hi2c1, 0x49, receive, 10, 100);
////	}
////	else
////	{
////		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
////	}
////	
//uint32_t version_number = 0;

//version_number = rfid.getFirmwareVersion();
//if( 0x32010607 == 	version_number)
//	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
//uint8_t lcd[50];
//sprintf((char *)lcd , "Firmware version is: 0x%X\n",version_number);
//	
//CDC_Transmit_FS(lcd ,strlen((const char *)lcd));


//		HAL_Delay(500);
//		HAL_GPIO_WritePin(GPIOD , GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
//		while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));

//    
//	}	
//  /* USER CODE END 3 */
//}

///**
//  * @brief System Clock Configuration
//  * @retval None
//  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

//  /** Configure the main internal regulator output voltage 
//  */
//  __HAL_RCC_PWR_CLK_ENABLE();
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
//  /** Initializes the CPU, AHB and APB busses clocks 
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//  RCC_OscInitStruct.PLL.PLLM = 4;
//  RCC_OscInitStruct.PLL.PLLN = 96;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//  RCC_OscInitStruct.PLL.PLLQ = 4;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /** Initializes the CPU, AHB and APB busses clocks 
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}

///**
//  * @brief I2C1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_I2C1_Init(void)
//{

//  /* USER CODE BEGIN I2C1_Init 0 */

//  /* USER CODE END I2C1_Init 0 */

//  /* USER CODE BEGIN I2C1_Init 1 */

//  /* USER CODE END I2C1_Init 1 */
//  hi2c1.Instance = I2C1;
//  hi2c1.Init.ClockSpeed = 1000;
//  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
//  hi2c1.Init.OwnAddress1 = 0;
//  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//  hi2c1.Init.OwnAddress2 = 0;
//  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN I2C1_Init 2 */

//  /* USER CODE END I2C1_Init 2 */

//}

///**
//  * @brief SPI1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_SPI1_Init(void)
//{

//  /* USER CODE BEGIN SPI1_Init 0 */
//  /* USER CODE END SPI1_Init 0 */

//  /* USER CODE BEGIN SPI1_Init 1 */
//  /* USER CODE END SPI1_Init 1 */
//  /* SPI1 parameter configuration*/
//  hspi1.Instance = SPI1;
//  hspi1.Init.Mode = SPI_MODE_MASTER;
//  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
//  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
//  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
//  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
//  hspi1.Init.NSS = SPI_NSS_SOFT;
//  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
//  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
//  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
//  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
//  hspi1.Init.CRCPolynomial = 10;
//  if (HAL_SPI_Init(&hspi1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN SPI1_Init 2 */
//  /* USER CODE END SPI1_Init 2 */

//}

///**
//  * @brief USART2 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_USART2_UART_Init(void)
//{

//  /* USER CODE BEGIN USART2_Init 0 */

//  /* USER CODE END USART2_Init 0 */

//  /* USER CODE BEGIN USART2_Init 1 */

//  /* USER CODE END USART2_Init 1 */
//  huart2.Instance = USART2;
//  huart2.Init.BaudRate = 115200;
//  huart2.Init.WordLength = UART_WORDLENGTH_8B;
//  huart2.Init.StopBits = UART_STOPBITS_1;
//  huart2.Init.Parity = UART_PARITY_NONE;
//  huart2.Init.Mode = UART_MODE_TX_RX;
//  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
//  if (HAL_UART_Init(&huart2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USART2_Init 2 */

//  /* USER CODE END USART2_Init 2 */

//}

///**
//  * @brief GPIO Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_GPIO_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};

//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  __HAL_RCC_GPIOD_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(CP_SLC_GPIO_Port, CP_SLC_Pin, GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

//  /*Configure GPIO pin : Button_Blue_Pin */
//  GPIO_InitStruct.Pin = Button_Blue_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(Button_Blue_GPIO_Port, &GPIO_InitStruct);

//  /*Configure GPIO pin : CP_SLC_Pin */
//  GPIO_InitStruct.Pin = CP_SLC_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(CP_SLC_GPIO_Port, &GPIO_InitStruct);

//  /*Configure GPIO pins : RSTO_Pin IRQ_Pin */
//  GPIO_InitStruct.Pin = RSTO_Pin|IRQ_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
//  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

//}

///* USER CODE BEGIN 4 */
///* USER CODE END 4 */

///**
//  * @brief  This function is executed in case of error occurrence.
//  * @retval None
//  */
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* USER CODE END Error_Handler_Debug */
//}

//#ifdef  USE_FULL_ASSERT
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t *file, uint32_t line)
//{ 
//  /* USER CODE BEGIN 6 */
//  /* USER CODE END 6 */
//}
//#endif /* USE_FULL_ASSERT */

///************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
