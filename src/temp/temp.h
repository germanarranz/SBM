/*
.h estandar de un hilo. Flags, librerías, funcion de inicialización, funcion de acceso a cola y la estructura de datos que queremos
*/

#ifndef TEMP_H
#define TEMP_H

#include "cmsis_os2.h" 
#include "stm32f4xx_hal.h"
#include "Driver_I2C.h"

#define I2C_FLAG 0x01
#define TIM_FLAG 0x02

int Init_Thread_temp (void);
osMessageQueueId_t id_Queue_temp(void);

typedef struct{
	
	float temp;
	
}obj_temp;

#endif

