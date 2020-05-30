
#ifndef __DEFINE_H
#define __DEFINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "twi_a.h"
//extern structures
extern I2C_HandleTypeDef hi2c1;

	
	
	//definition
	#define handle_i2c  hi2c1
	
	#define reset_pin 	RSTO_Pin 
	#define reset_port 	RSTO_GPIO_Port 
	
	#define irq_pin 		IRQ_Pin 
	#define irq_port 		IRQ_GPIO_Port 

#ifdef __cplusplus
}
#endif

#endif 
