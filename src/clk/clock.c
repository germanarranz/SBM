/*
MÓDULO DE LA HORA: clock.c y clock.h

Tenenos que codificar un codigo el cual sepa llevar la cuenta de la hora con una resolución de un segundo.
Despues este modulo deberá cambiar las variables globales del sistema para que los demás modulos (en concreto el LCD)
sepa que hora es y lo pueda representar por pantalla.

El enfoque que le vamos a dar es de una aplicación que simplemente cuente segundos mediante un timer del RTOS2, y luego mediante 
el uso de funciones que serán definidas, el código será capaz de transformar esa variable de segundos en el formato que deseamos (HH/MM/SS)

Todo esto será codificado en un hilo que se ejecutará en concurrencia con el resto de módulos del código

*/

#include "clock.h"                         
 
static uint32_t cnt;                                  //Variable auxiliar para medir los segundos que han pasado

static void Thread_clock(void *argument);      //Inicialización del thread 
static void tim_sec_Callback(void *argument);  //Callback del timer que cuenta segundos      
static void cnt_to_hhmmss(uint32_t cnt);       //Funcion que convierte los segundos al formato HH/MM/SS
static osTimerId_t tim_sec;                    //Timer para contar segundos

osThreadId_t tid_Thread_clock;                 //Identificador del Clock

uint8_t hora;                                  //Variables auxiliares para gardar horas minutos y segundos
uint8_t min;
uint8_t seg;

/*
Callback del tim_sec que se encarga de incrementar la cuenta de los segundos y resetearla cuando llega a las 23:59:59
*/

static void tim_sec_Callback(void *argument){
  
	//Si llega a las 23:59:59 lo pone a 0
	
  if (cnt == 86399){
    
    cnt = 0;
  //Si no ha llegado a las 23:59:59, sigue contando segundos
}else
    cnt++;
	
	//Pasa el valor de segundos a HH/MM/SS
  cnt_to_hhmmss(cnt);
}

/*
Inicialización del thread
*/
int Init_Thread_clock (void) {
 
  tid_Thread_clock = osThreadNew(Thread_clock, NULL, NULL);
  if (tid_Thread_clock == NULL) {
    return(-1);
  }
 
  return(0);
}

//Funcionalidad del thread 
 
static void Thread_clock (void *argument) {
	
	//Inicializa la cuenta a 0 (00:00:00)
  cnt = 0;
	
	//Crea el timer que cuenta segundos, activara la tim_sec_Callback y será periodico (como no podría ser de otra manera)
  tim_sec = osTimerNew(tim_sec_Callback, osTimerPeriodic, NULL, NULL);
	
	//Empieza a contar
  osTimerStart(tim_sec, 1000U);
	
	
}

//Función que pasa de segundos a HH/MM/SS

void cnt_to_hhmmss(uint32_t cnt){

hora = cnt/3600;
min = (cnt - (hora*3600))/60;
seg = (cnt - (hora*3600) - (min*60));

}

//Funcion que pasa de HHMMSS a la cuenta, lo usamos para poder modificar la hora desde el programa principal (cnt se pasa por puntero porque hay que modificar un paraemtro del módulo)

void hhmmss_to_cnt(uint32_t* cnt, uint8_t hr, uint8_t min, uint8_t seg){
	*cnt = (hr * 3600) + (min * 60) + seg;
	
}

/*
Nos enfrentamos a un desafio cuando nos toca implemntar funcionalidades que pueden cambiar la hora desde el modulo principal, asi que hemos d eencontrar una manera de aboradarlas. La solucion que hemos encontrado es la de 
crear una funcion que cuando se le llame, pare el timer que aumenta la cnt para llevar la hora, actualice la cuenta a su nuevo estado y vuelva a inicilizar el timer. Asi nos ahorramos tener que gestionar regiones críticas
*/

void poner_hora(uint8_t hr, uint8_t min, uint8_t seg){
	osTimerStop(tim_sec);
	hhmmss_to_cnt(&cnt, hr, min, seg);
	osTimerStart(tim_sec, 1000U);
}




