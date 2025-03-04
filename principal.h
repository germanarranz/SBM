#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include "src/clk/clock.h"
#include "src/com/com.h"
#include "src/joy/Joystick.h"
#include "src/lcd/lcd.h"
#include "src/pot/pot.h"
#include "src/pwm/pwm.h"
#include "src/temp/temp.h"
#include "cmsis_os2.h"
#include "src/rgb/rgb.h"

#define FLAG_DISP 0x01

typedef struct{
  char mesure[36];
}mesure_t;

typedef struct{
  mesure_t medidas[10];
  uint8_t ini;
  uint8_t fin;
  uint8_t cnt;
}buf_medidas;

typedef enum {HOR_D, HOR_S, MIN_D, MIN_S, SEG_D, SEG_S, TEMP_D, TEMP_U, TEMP_DEC}t_dep;
typedef enum {REP, ACTIVO, TEST, DEBUG}t_estado;

int Init_Thread_principal(void);


#endif