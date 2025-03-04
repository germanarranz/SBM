/*
.h estandar de un hilo, sus librerías y su funcion de inicialización para ser llamada desde el main
*/

#ifndef CLOCK_H
#define CLOCK_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

int Init_Thread_clock(void);
void poner_hora(uint8_t hr, uint8_t min, uint8_t seg);

#endif

