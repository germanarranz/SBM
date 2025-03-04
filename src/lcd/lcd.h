#ifndef LCD_H
#define LCD_H

/*
.h estandar de un hilo, en el definimos la inicialización del thread, la del test (solo en el ejemplo) la estructura de los mensajes,
las flags y las librerias necesarias. Cabe destacar el uso de string para strlen y de stdio.h
*/

#include "cmsis_os2.h"
#include "Driver_SPI.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include <stdio.h>


#define RESET_FLAG 0x01  //Flag para el timer de reset del LCD

typedef struct{  //Estructura del mensaje que se va meter a la cola
  
  char l_1[32];
  char l_2[32];
  
}obj_lcd;

int Init_Thread_lcd (void);  //Inicialización del thread del LCD
osMessageQueueId_t id_Queue_lcd(void);  //Funcion para poder acceder a la cola desde diferentes módulos

#endif

