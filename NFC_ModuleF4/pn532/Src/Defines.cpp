
#include "defines.h"

uint32_t max(uint32_t par1, uint32_t par2)
{
	if(par1 > par2)
		return par1;
	else
		return par2;	
}
	

uint32_t min(uint32_t par1, uint32_t par2)
{
	if(par1 < par2)
		return par1;
	else
		return par2;	
}
	
 uint8_t digitalWrite(uint8_t pin_number,  PS state)
{
	
    uint16_t temp = 0;
    if(pin_number > 80)
        return false;
    
    if(pin_number > PD15) // GPIOE
    {
        temp =  pin_number - PE0;
        temp = 1 << temp;
		HAL_GPIO_WritePin(GPIOE, temp, state ? GPIO_PIN_SET : GPIO_PIN_RESET); 
    }
    else if(pin_number > PC15) // GPIOD
    {   
        temp =  pin_number - PD0;
		temp = 1 << temp;
        HAL_GPIO_WritePin(GPIOD, temp, state ? GPIO_PIN_SET : GPIO_PIN_RESET); 
    }
    else if(pin_number > PB15) // GPIOC
    {
        temp =  pin_number - PC0;
        temp = 1 << temp;
        HAL_GPIO_WritePin(GPIOC, temp, state ? GPIO_PIN_SET : GPIO_PIN_RESET); 
    }
    else if(pin_number > PA15) // GPIOB
    {
        temp =  pin_number - PB0;
		temp = 1 << temp;
        HAL_GPIO_WritePin(GPIOB, temp, state ? GPIO_PIN_SET : GPIO_PIN_RESET); 
    }
    else                        // GPIOA
    {
        temp =  pin_number - PA0;
        temp = 1 << temp;
         HAL_GPIO_WritePin(GPIOA, temp, state ? GPIO_PIN_SET : GPIO_PIN_RESET); 
    }
    return true;
}

uint8_t digitalRead(uint8_t pin_number)
{ 
    uint16_t temp = 0;
    if(pin_number > 80)
        return 0;
    
    if(pin_number > PD15) // GPIOE
    {
        temp =  pin_number - PE0;
        temp = 1 << temp;
        return HAL_GPIO_ReadPin(GPIOE, temp); 
    }
    else if(pin_number > PC15) // GPIOD
    {   
		temp =  pin_number - PD0;
		temp = 1 << temp;
		return  HAL_GPIO_ReadPin(GPIOD, temp); 
    }
    else if(pin_number > PB15) // GPIOC
    {
        temp =  pin_number - PC0;
        temp = 1 << temp;
       return  HAL_GPIO_ReadPin(GPIOC, temp); 
    }
    else if(pin_number > PA15) // GPIOB
    {
        temp =  pin_number - PB0;
		temp = 1 << temp;
		return  HAL_GPIO_ReadPin(GPIOB, temp); 
    }
    else                       // GPIOA
    {
        temp =  pin_number - PA0;
        temp = 1 << temp;
        return  HAL_GPIO_ReadPin(GPIOA, temp); 
    }

}






        