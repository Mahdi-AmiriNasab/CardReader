#include "defines.h"



 uint8_t digitalWrite(PinNumber pin_number,  Pinstate state)
{
    uint16_t temp = 0;
    if(pin_number > 80)
        return 0;
    
    if(pin_number > PD15) // PORTE
    {
        temp =  pin_number - PE0;
        HAL_GPIO_WritePin(PORTE, temp, state); 
    }
    else if(pin_number > PC15) // PORTD
    {   
        temp =  pin_number - PD0;
        HAL_GPIO_WritePin(PORTE, temp, state); 
    }
    else if(pin_number > PB15) // PORTC
    {
        temp =  pin_number - PC0;
        HAL_GPIO_WritePin(PORTE, temp, state); 
    }
    else if(pin_number > PA15) // PORTB
    {
        temp =  pin_number - PB0;
        HAL_GPIO_WritePin(PORTE, temp, state); 
    }
    else                        // PORTA
    {
        temp =  pin_number - PA0;
         HAL_GPIO_WritePin(PORTE, temp, state); 
    }

}