#ifndef __POT_H
#define __POT_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

#define FLAG_TEMP 0x01


typedef struct {
	double pot1; //GIRO MAXIMO: 5�C a 30�C
	double pot2; //GIRO MAXIMO: 5�C a 30�C
} obj_pot;

osMessageQueueId_t id_queue_pot(void);
int Init_Thread_pot(void);

#endif

