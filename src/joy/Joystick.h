/*
MUY IMPORTANTE:
Hay que poner el #ifndef, ya que de no ponerlo se estaria definendo dos veces, cosa que daria un error de compilación
*/

#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

/*
Prototipo de la funcíon para obtener el ID de la cola y parac inicializar el hilo
*/

int Init_Thread_Joystick (void);
osMessageQueueId_t id_Queue_joystick(void);

/*
FLAGS
*/

#define REB_FLAG 0x01
#define IRQ_FLAG 0x02
#define CHECK_FLAG 0x03

/*
MUY IMPORTANTE:

Vamos a definir una estructura de datos para poder guardar los daros y que siempre tengan un tamao fijo,
en un primer lugar definiremos con una enumeración todos los valores posibles y luego los metermemos en una estructura de datos 
para que puedan ser accedidos
*/

typedef enum{RIGHT, LEFT, UP, DOWN, MIDDLE}dir_t;


typedef struct{
	dir_t dir;
	uint8_t tipoP; // tipo de pulsacion, 0 - PULSACION CORTA, 1 - PULSACION LARGA 
}obj_joystick;

#endif
