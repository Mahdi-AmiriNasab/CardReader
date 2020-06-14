/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "Adafruit_PN532.h"
#include "stdio.h"
//#include "Adafruit_PN532.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// This is the most important switch: It defines if you want to use Mifare Classic or Desfire EV1 cards.
// If you set this define to false the users will only be identified by the UID of a Mifare Classic or Desfire card.
// This mode is only for testing if you have no Desfire cards available.
// Mifare Classic cards have been cracked due to a badly implemented encryption. 
// It is easy to clone a Mifare Classic card (including it's UID).
// You should use Defire EV1 cards for any serious door access system.
// When using Desfire EV1 cards a 16 byte data block is stored in the card's EEPROM memory 
// that can only be read with the application master key.
// To clone a Desfire card it would be necessary to crack a 168 bit 3K3DES or a 128 bit AES key data which is impossible.
// If the Desfire card does not contain the correct data the door will not open even if the UID is correct.
// IMPORTANT: After changing this compiler switch, please execute the CLEAR command!
#define USE_DESFIRE   false

#if USE_DESFIRE
    // This compiler switch defines if you use AES (128 bit) or DES (168 bit) for the PICC master key and the application master key.
    // Cryptographers say that AES is better.
    // But the disadvantage of AES encryption is that it increases the power consumption of the card more than DES.
    // The maximum read distance is 5,3 cm when using 3DES keys and 4,0 cm when using AES keys.
    // (When USE_DESFIRE == false the same Desfire card allows a distance of 6,3 cm.)
    // If the card is too far away from the antenna you get a timeout error at the moment when the Authenticate command is executed.
    // IMPORTANT: Before changing this compiler switch, please execute the RESTORE command on all personalized cards!
    #define USE_AES   false

    // This define should normally be zero
    // If you want to run the selftest (only available if USE_DESFIRE == true) you must set this to a value > 0.
    // Then you can enter TEST into the terminal to execute a selftest that tests ALL functions in the Desfire class.
    // The value that you can specify here is 1 or 2 which will be the debug level for the selftest.
    // At level 2 you see additionally the CMAC and the data sent to and received from the card.
    #define COMPILE_SELFTEST  0
    
    // This define should normally be false
    // If this is true you can use Classic cards / keyfobs additionally to Desfire cards.
    // This means that the code is compiled for Defire cards, but when a Classic card is detected it will also work.
    // This mode is not recommended because Classic cards do not offer the same security as Desfire cards.
    #define ALLOW_ALSO_CLASSIC   true
#endif

// This password will be required when entering via Terminal
// If you define an empty string here, no password is requested.
// If any unauthorized person may access the dooropener hardware phyically you should provide a password!
#define PASSWORD  ""
// The interval of inactivity in minutes after which the password must be entered again (automatic log-off)
#define PASSWORD_TIMEOUT  5

// This Arduino / Teensy pin is connected to the relay that opens the door 1
#define DOOR_1_PIN       20

// This Arduino / Teensy pin is connected to the optional relay that opens the door 2
#define DOOR_2_PIN       21

// This Arduino / Teensy pin is connected to the PN532 RSTPDN pin (reset the PN532)
// When a communication error with the PN532 is detected the board is reset automatically.
#define RESET_PIN         2
// The software SPI SCK  pin (Clock)
#define SPI_CLK_PIN       3
// The software SPI MISO pin (Master In, Slave Out)
#define SPI_MISO_PIN      1
// The software SPI MOSI pin (Master Out, Slave In)
#define SPI_MOSI_PIN      4
// The software SPI SSEL pin (Chip Select)
#define SPI_CS_PIN        0
 
// This Arduino / Teensy pin is connected to the green LED in a two color LED.
// The green LED flashes fast while no card is present and flashes 1 second when opening the door.
#define LED_GREEN_PIN    12

// This Arduino / Teensy pin is connected to the red LED in a two color LED.
// The red LED flashes slowly when a communication error occurred with the PN532 chip and when 
// an unauthorized person tries to open the door.
// It flashes fast when a power failure has been detected. (Charging battery failed)
#define LED_RED_PIN      13

// This Arduino / Teensy pin is connected to the voltage divider that measures the 13,6V battery voltage
#define VOLTAGE_MEASURE_PIN  A5

// If the battery voltage decreases more than (MAX_VOLTAGE_DROP / 10) Volt when opening the door, the battery is old and must be replaced soon.
// If the battery is sane the voltage stays constant even if multiple Amperes are drawn.
// The older the battery gets, the higher becomes it's impedance and the voltage decreases when the door opener solenoid draws current.
// When the battery gets old the red and green LED will blink alternatingly.
#define MAX_VOLTAGE_DROP  10  // 1 Volt

// The pin that connects to the button that opens the door
// This pin is ignored if BUTTON_OPEN_DOOR == NO_DOOR
#define BUTTON_OPEN_PIN  15

// Define which door is opened when the button is pressed (NO_DOOR, DOOR_ONE, DOOR_TWO or DOOR_BOTH)
#define BUTTON_OPEN_DOOR  NO_DOOR

// Use 12 bit resolution for the analog input (ADC)
// The Teensy 3.x boards have a 12 bit ADC.
#define ANALOG_RESOLUTION  12

// The analog reference voltage (float) of the CPU (analogReference(DEFAULT) --> 3.3V, analogReference(INTERNAL1V2) --> 1.2V)
#define ANALOG_REFERENCE   1.2

// This factor (float) is used to calculate the battery voltage.
// If the external voltage divider is 220 kOhm / 15 kOhm the factor is theoretically 15.66666 == (220 + 15) / 15.
// You must fine tune this value until the battery voltage is displayed correctly when you hit Enter in the Terminal.
// Therefor you must unplug the 220V power suppply and measure the real voltage at the battery.
#define VOLTAGE_FACTOR   15.9

// The interval in milliseconds that the relay is powered which opens the door
#define OPEN_INTERVAL   100

// This is the interval that the RF field is switched off to save battery.
// The shorter this interval, the more power is consumed by the PN532.
// The longer  this interval, the longer the user has to wait until the door opens.
// The recommended interval is 1000 ms.
// Please note that the slowness of reading a Desfire card is not caused by this interval.
// The SPI bus speed is throttled to 10 kHz, which allows to transmit the data over a long cable, 
// but this obviously makes reading the card slower.
#define RF_OFF_INTERVAL  1000

// ######################################################################################

#if defined(__MK20DX256__) // the CPU of the Teensy 3.1 / 3.2
    #if !defined(USB_SERIAL)
        #error "Switch the compiler to USB Type = 'Serial'"
    #endif
    #if F_CPU != 24000000
        #error "Switch the compiler to CPU Speed = '24 MHz optimized'"
    #endif
#else
    #warning "This code has not been tested on any other board than Teensy 3.1 / 3.2"
#endif

#if USE_DESFIRE
    #if USE_AES
        #define DESFIRE_KEY_TYPE   AES
        #define DEFAULT_APP_KEY    gi_PN532.AES_DEFAULT_KEY
    #else
        #define DESFIRE_KEY_TYPE   DES
        #define DEFAULT_APP_KEY    gi_PN532.DES3_DEFAULT_KEY
    #endif
    
    #include "Desfire.h"
    #include "Secrets.h"
    #include "Buffer.h"
    Desfire          gi_PN532; // The class instance that communicates with Mifare Desfire cards   
    DESFIRE_KEY_TYPE gi_PiccMasterKey;
#else
    #include "Classic.h"
    Classic          gi_PN532; // The class instance that communicates with Mifare Classic cards
#endif

#include "UserManager.h"

// The tick counter starts at zero when the CPU is reset.
// This interval is added to the 64 bit tick count to get a value that does not start at zero,
// because gu64_LastPasswd is initialized with 0 and must always be in the past.
#define PASSWORD_OFFSET_MS   (2 * PASSWORD_TIMEOUT * 60 * 1000)

enum eLED
{
    LED_OFF,
    LED_RED,
    LED_GREEN,
};

enum eBattCheck
{
    BATT_OK,        // The voltage did not drop more than (MAX_VOLTAGE_DROP / 10) Volt when the door was opened the last time.
    BATT_OLD_RED,   // The battery must be replaced soon -> flash red LED.
    BATT_OLD_GREEN, // The battery must be replaced soon -> flash green LED.
};

struct kCard
{
    byte     u8_UidLength;   // UID = 4 or 7 bytes
    byte     u8_KeyVersion;  // for Desfire random ID cards
    bool      b_PN532_Error; // true -> the error comes from the PN532, false -> crypto error
    eCardType e_CardType;    
};

// global variables
char       gs8_CommandBuffer[500];    // Stores commands typed by the user via Terminal and the password
uint32_t   gu32_CommandPos   = 0;     // Index in gs8_CommandBuffer
uint64_t   gu64_LastPasswd   = 0;     // Timestamp when the user has enetered the password successfully
uint64_t   gu64_LastID       = 0;     // The last card UID that has been read by the RFID reader  
bool       gb_InitSuccess    = false; // true if the PN532 has been initialized successfully
eBattCheck ge_BattCheck      = BATT_OK;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
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
  MX_SPI1_Init();
 // MX_USB_DEVICE_Init();
  MX_I2C1_Init();
	MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint8_t send[100] = {0x00, 0x00, 0xff, 0x01, 0xff, 0xd4, 0x02, 0x2a, 0x00}
	, receive[100];
		

	Adafruit_PN532 rfid(irq_pin, reset_pin);
	
		
  uint8_t success;
                      // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
	uint32_t version_number = 0;
	uint8_t lcd[50];

	do 
	{
		version_number = rfid.getFirmwareVersion();
		sprintf((char *)lcd , "No device detected\nPress the blue button to try again\n");
		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
		while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));		
	}while(!version_number);
	
	if( 0x32010607 == 	version_number)
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
		sprintf((char *)lcd , "Firmware version is: 0x%X\n",version_number);
		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
		rfid.setPassiveActivationRetries(0xff);
		rfid.SAMConfig();
		HAL_Delay(30);
	}
	else
	{
		sprintf((char *)lcd , "Device not supported");
		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
		
	}
	while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));
	//HAL_GPIO_WritePin(GPIOD , GPIO_PIN_15, GPIO_PIN_SET);
	
	uint8_t type = 0x00;
	while(1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		

	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;  
	uint8_t response[50];
	uint8_t type [16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x11, 0x12, 0x20, 0x23, 0x40, 0x41, 0x42, 0x80, 0x81, 0x82 };
//	if(rfid.inListPassiveTarget())
//	{
//		sprintf((char *)lcd , "Something has found\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//	}
	
	if(rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 5000))
	{
		HAL_GPIO_WritePin(GPIOD ,LED_GREEN_PIN, GPIO_PIN_SET);
		HAL_Delay(20);
		HAL_GPIO_WritePin(GPIOD ,LED_GREEN_PIN, GPIO_PIN_RESET);
		sprintf((char *)lcd , "\nMIFARE_ISO14443A Found\nUID length: %d \nUID is:",uidLength);
		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);			

		for(uint8_t i = 0; i < uidLength; i++)
		{
			sprintf((char *)lcd, " 0x%02X", uid[i]);
			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
		}
	}
	else
	{
		sprintf((char *)lcd , "\nTime out\n");
		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);		
	}
	/*
	for(uint8_t i = 0; i < 16; i++)
	{
		if(rfid.InAutoPoll(1,1 , &type[i], 1))
		{
			sprintf((char *)lcd , "Type: 0x%02X\nResponse is:", type[i]);
			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
			HAL_Delay(100);
			if(rfid.InAutoPollGetResponse(response))
			{
				for(uint8_t i = 0; i < 20; i++)
					sprintf((char *)&lcd[i] , "0x%x", response[i]);
				HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
			}
			else
			{
				sprintf((char *)lcd , "Nothing found\n");
				HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
			}	
		}
	}
	
	
	*/
//	type = 0x00;
//	if(rfid.InAutoPoll(1,1 , &type, 1))
//	{
//		sprintf((char *)lcd , "Type: 0x00\nResponse is:");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		HAL_Delay(100);
//		if(rfid.InAutoPollGetResponse(response))
//		{
//			for(uint8_t i = 0; i < 20; i++)
//				sprintf((char *)&lcd[i] , "0x%x", response[i]);
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}
//		else
//		{
//			sprintf((char *)lcd , "Nothing found\n");
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}	
//	}
//	type = 0x10;
//	if(rfid.InAutoPoll(1,1 , &type, 1))
//	{
//		sprintf((char *)lcd , "Type: 0x10\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		HAL_Delay(100);
//		if(rfid.InAutoPollGetResponse(response))
//		{
//			for(uint8_t i = 0; i < 20; i++)
//				sprintf((char *)&lcd[i] , "0x%x", response[i]);
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}
//		else
//		{
//			sprintf((char *)lcd , "Nothing found\n");
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}	
//	}
//	type = 0x03;
//	if(rfid.InAutoPoll(1,1 , &type, 1))
//	{
//		sprintf((char *)lcd , "Type: 0x03\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		HAL_Delay(100);
//		if(rfid.InAutoPollGetResponse(response))
//		{
//			for(uint8_t i = 0; i < 20; i++)
//				sprintf((char *)&lcd[i] , "0x%x", response[i]);
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}
//		else
//		{
//			sprintf((char *)lcd , "Nothing found\n");
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}	
//	}
//	type = 0x23;
//	if(rfid.InAutoPoll(1,1 , &type, 1))
//	{
//		sprintf((char *)lcd , "Type: 0x23\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		HAL_Delay(100);
//		if(rfid.InAutoPollGetResponse(response))
//		{
//			for(uint8_t i = 0; i < 20; i++)
//				sprintf((char *)&lcd[i] , "0x%x", response[i]);
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}
//		else
//		{
//			sprintf((char *)lcd , "Nothing found\n");
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}	
//	}
//	type = 0x20;
//	if(rfid.InAutoPoll(1,1 , &type, 1))
//	{
//		sprintf((char *)lcd , "Type: 0x20\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		HAL_Delay(100);
//		if(rfid.InAutoPollGetResponse(response))
//		{
//			for(uint8_t i = 0; i < 20; i++)
//				sprintf((char *)&lcd[i] , "0x%x", response[i]);
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}
//		else
//		{
//			sprintf((char *)lcd , "Nothing found\n");
//			HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//		}	
//	}
//	
//		sprintf((char *)lcd , "Nothing found :)\n");
//		HAL_UART_Transmit(&huart2, lcd, strlen((const char *)lcd), 100);
//	
//	


HAL_GPIO_WritePin(GPIOD , GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));





    
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
  hi2c1.Init.ClockSpeed = 1000;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */

}
/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CP_SLC_GPIO_Port, CP_SLC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : Button_Blue_Pin */
  GPIO_InitStruct.Pin = Button_Blue_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button_Blue_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CP_SLC_Pin */
  GPIO_InitStruct.Pin = CP_SLC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CP_SLC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RSTO_Pin IRQ_Pin */
  GPIO_InitStruct.Pin = RSTO_Pin|IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
  /* USER CODE END Error_Handler_Debug */
}


#ifdef  USE_FULL_ASSERT
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
