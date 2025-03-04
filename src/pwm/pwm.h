#ifndef PWM_H
#define PWM_H
#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "stm32f4xx_hal.h"

typedef enum{LOW, MED_LOW, MED_HIGH, HIGH}pow_t;
typedef struct{
  pow_t pow;
}obj_pwm;

int Init_Th_pwm (void);
osMessageQueueId_t id_Queue_pwm(void);


#endif

