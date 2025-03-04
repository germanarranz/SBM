#ifndef __RGB_H
#define __RGB_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

typedef enum{RED, YELLOW, CIAN, BLUE, OFF}col_t;
typedef struct{
  col_t color;
}obj_rgb;

int Init_Thread_rgb(void);
osMessageQueueId_t id_Queue_rgb(void);

#endif

