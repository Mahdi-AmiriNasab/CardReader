
#ifndef __DEFINE_H
#define __DEFINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "string.h"
#include "stdio.h"
#include "twi_a.h"
	
//definition
#define CPU_Freq_MHZ 		48
#define ARDUINO 100
#define F 			(const uint8_t *)
#define HEX 		(const uint8_t *) 1
#define millis 				HAL_GetTick
#define delay(ms)			HAL_Delay(ms) 		
#define handle_i2c 	 		hi2c1
#define handle_uart  		huart2
#define reset_pin 			PC6 
#define irq_pin     		PC5 


//extern structures
extern I2C_HandleTypeDef handle_i2c;
extern UART_HandleTypeDef handle_uart;
typedef  uint8_t byte;


	
	
// Classes 
class DBG
{
	public:
		void print (const char* s8_Text, const char* s8_LF);
		bool available(void);
		byte read(void);
		void print (const char *str);
		void printDec(int s32_Data, const char* s8_LF);
		void printHex8(byte u8_Data, const char* s8_LF);
		void printHex16(uint16_t u16_Data, const char* s8_LF);
		void println (const char *str);
		void printHex32(uint32_t u32_Data, const char* s8_LF);
		void println (void);
		void printHexBuf(const byte* u8_Data, const uint32_t u32_DataLen, const char* s8_LF, int s32_Brace1, int s32_Brace2);
		void printInterval(uint64_t u64_Time, const char* s8_LF);

	private:
};






/**********Inline Classes*************/

//    inline void DBG::print (const uint8_t *str, const uint8_t  type)
//    {}

inline void DBG::print (const char *str)
{
	HAL_UART_Transmit(&handle_uart,(uint8_t *)str ,strlen((const char *)str), 100);  
}        
inline void DBG::print (const char* s8_Text, const char* s8_LF) //=NULL
{
	DBG::print(s8_Text);
	if (s8_LF) 
		DBG::print(s8_LF);
}
inline void DBG::printDec(int s32_Data, const char* s8_LF = 0) // =NULL
{
    char s8_Buf[20];
    sprintf(s8_Buf, "%d", s32_Data);
    print(s8_Buf, s8_LF);
}
inline void DBG::printHex8(byte u8_Data, const char* s8_LF = 0) // =NULL
{
    char s8_Buf[20];
    sprintf(s8_Buf, "%02X", u8_Data);
    print(s8_Buf, s8_LF);
}
inline void DBG::printHex16(uint16_t u16_Data, const char* s8_LF = 0) // =NULL
{
    char s8_Buf[20];
    sprintf(s8_Buf, "%04X", u16_Data);
    print(s8_Buf, s8_LF);
}
inline void DBG::printHex32(uint32_t u32_Data, const char* s8_LF = 0) // =NULL
{
    char s8_Buf[20];
    sprintf(s8_Buf, "%08X", (unsigned int)u32_Data);
    print(s8_Buf, s8_LF);
}

// Prints a hexadecimal buffer as 2 digit HEX numbers
// At the byte position s32_Brace1 a "<" will be inserted
// At the byte position s32_Brace2 a ">" will be inserted
// Output will look like: "00 00 FF 03 FD <D5 4B 00> E0 00"
// This is used to mark the data bytes in the packet.
// If the parameters s32_Brace1, s32_Brace2 are -1, they do not appear
inline void DBG::printHexBuf(const byte* u8_Data, const uint32_t u32_DataLen, const char* s8_LF = 0, int s32_Brace1 = 0, int s32_Brace2 = 0)
{
    for (uint32_t i=0; i < u32_DataLen; i++)
    {
        if ((int)i == s32_Brace1)
            print(" <");
        else if ((int)i == s32_Brace2)
            print("> ");
        else if (i > 0)
            print(" ");
        
        printHex8(u8_Data[i]);
    }
    if (s8_LF) print(s8_LF);
}

// Converts an interval in milliseconds into days, hours, minutes and prints it
inline void DBG::printInterval(uint64_t u64_Time, const char* s8_LF)
{
    char Buf[30];
    u64_Time /= 60*1000;
    int s32_Min  = (int)(u64_Time % 60);
    u64_Time /= 60;
    int s32_Hour = (int)(u64_Time % 24);    
    u64_Time /= 24;
    int s32_Days = (int)u64_Time;    
    sprintf(Buf, "%d days, %02d:%02d hours", s32_Days, s32_Hour, s32_Min);
    print(Buf, s8_LF);   
}
inline void DBG::println(const char *str)
{
	HAL_UART_Transmit(&handle_uart,(uint8_t *)str ,strlen((const char *)str), 100); 
	HAL_UART_Transmit(&handle_uart,(uint8_t*)"\n" ,1, 100); 
}
inline bool DBG::available(void)
{
	return __HAL_UART_GET_FLAG(&handle_uart, UART_FLAG_RXNE);
}

inline byte DBG::read(void)
{
	uint8_t read_tmp = 0;
	HAL_UART_Receive(&handle_uart, &read_tmp ,1 , 100);
	return read_tmp;
}




#ifndef CPU_Freq_MHZ 
	#warning "CPU clock not determined"
	#define CPU_Freq_MHZ 	168
#endif

inline void delayMicroseconds(uint32_t delayus)
{
	uint32_t cc = 1.0625 * CPU_Freq_MHZ *delayus;
	while(cc-- > 0);
}

uint32_t min(uint32_t par1, uint32_t par2);
uint32_t max(uint32_t par1, uint32_t par2);



#define CLOCK_SPEED_MHZ   96


#define BOARD_LED_PIN           PD12
#define BOARD_LED_PIN2          PD13
#define BOARD_LED_PIN3          PD14
#define BOARD_LED_PIN4          PD15

#define BOARD_BUTTON_PIN        PA0

#define BOARD_USB_DM_PIN		PA11
#define BOARD_USB_DP_PIN		PA12

#define BOARD_NR_USARTS         3
#define BOARD_USART1_TX_PIN     PA9
#define BOARD_USART1_RX_PIN     PA10
#define BOARD_USART2_TX_PIN     PA2
#define BOARD_USART2_RX_PIN     PA3
#define BOARD_USART6_TX_PIN     PC6
#define BOARD_USART6_RX_PIN     PC7

#define BOARD_NR_I2C            3
#define BOARD_I2C1_SCL_PIN      PB6
#define BOARD_I2C1_SDA_PIN      PB7
#define BOARD_I2C1A_SCL_PIN     PB8
#define BOARD_I2C1A_SDA_PIN     PB9
#define BOARD_I2C2_SCL_PIN      PB10
#define BOARD_I2C2_SDA_PIN      PB11
#ifdef PACKAGE_LQFP144
#define BOARD_I2C2A_SCL_PIN     PF1
#define BOARD_I2C2A_SDA_PIN     PF0
#endif
#define BOARD_I2C3_SCL_PIN      PA8
#define BOARD_I2C3_SDA_PIN      PC9
#ifdef PACKAGE_LQFP144
#define BOARD_I2C3A_SCL_PIN     PH7
#define BOARD_I2C3A_SDA_PIN     PH8
#endif

#define BOARD_NR_SPI            5
#define BOARD_SPI1_NSS_PIN      PA4
#define BOARD_SPI1_SCK_PIN      PA5
#define BOARD_SPI1_MISO_PIN     PA6
#define BOARD_SPI1_MOSI_PIN     PA7
#define BOARD_SPI1A_NSS_PIN      PA15
#define BOARD_SPI1A_SCK_PIN      PB3
#define BOARD_SPI1A_MISO_PIN     PB4
#define BOARD_SPI1A_MOSI_PIN     PB5

#define BOARD_SPI2_NSS_PIN      PB12
#define BOARD_SPI2_SCK_PIN      PB13
#define BOARD_SPI2_MISO_PIN     PB14
#define BOARD_SPI2_MOSI_PIN     PB15
#define BOARD_SPI2A_NSS_PIN      PB9
#define BOARD_SPI2A_SCK_PIN      PB10 // PC7 // PB7
#define BOARD_SPI2A_MISO_PIN     PC2
#define BOARD_SPI2A_MOSI_PIN     PC3

#define BOARD_SPI3_NSS_PIN      PA15
#define BOARD_SPI3_SCK_PIN      PB3
#define BOARD_SPI3_MISO_PIN     PB4
#define BOARD_SPI3_MOSI_PIN     PB5
#define BOARD_SPI3A_NSS_PIN      PA4
#define BOARD_SPI3A_SCK_PIN      PC10
#define BOARD_SPI3A_MISO_PIN     PC11
#define BOARD_SPI3A_MOSI_PIN     PC12

#define BOARD_SPI4_NSS_PIN      PE4
#define BOARD_SPI4_SCK_PIN      PE2
#define BOARD_SPI4_MISO_PIN     PE5
#define BOARD_SPI4_MOSI_PIN     PE6

#define BOARD_SPI5_NSS_PIN      PE11
#define BOARD_SPI5_SCK_PIN      PE12
#define BOARD_SPI5_MISO_PIN     PE13
#define BOARD_SPI5_MOSI_PIN     PE14

#define BOARD_NR_PWM_PINS       22
#define BOARD_NR_ADC_PINS       16
#define BOARD_NR_USED_PINS      43 // ala42 not set yet
#define BOARD_JTMS_SWDIO_PIN    PA13
#define BOARD_JTCK_SWCLK_PIN    PA14
#define BOARD_JTDI_PIN          PA15
#define BOARD_JTDO_PIN          PB3
#define BOARD_NJTRST_PIN        PB4



enum PinNumber 
{
	PA0 = 0,PA1,PA2,PA3,PA4,PA5,PA6,PA7,PA8,PA9,PA10,PA11,PA12,PA13,PA14,PA15,
		PB0,PB1,PB2,PB3,PB4,PB5,PB6,PB7,PB8,PB9,PB10,PB11,PB12,PB13,PB14,PB15,
		PC0,PC1,PC2,PC3,PC4,PC5,PC6,PC7,PC8,PC9,PC10,PC11,PC12,PC13,PC14,PC15,
		PD0,PD1,PD2,PD3,PD4,PD5,PD6,PD7,PD8,PD9,PD10,PD11,PD12,PD13,PD14,PD15,
		PE0,PE1,PE2,PE3,PE4,PE5,PE6,PE7,PE8,PE9,PE10,PE11,PE12,PE13,PE14,PE15,
		BOARD_NR_GPIO_PINS
};

typedef enum 
{
    HIGH =  1,
    LOW =   0, 
}PS;


uint8_t digitalRead(uint8_t pin_number);
uint8_t digitalWrite(uint8_t pin_number,  PS state);


#ifdef __cplusplus
}
#endif

#endif 
