/*
En este modulo vamos a menjar un PIN PWM (PE9) que nos entregara una potencia modulable dependiendo de el mensaje que lea desde la cola. Este será el output final de nuestra apliacion  
*/

#include "pwm.h"

/*
Crearemos un hilo, una cola y un hilo de prueba
*/

static osThreadId_t tid_Th_pwm;                        // thread id
static osMessageQueueId_t queue_pwm;

/*
Estructura que nos manejara el Timer para el PWM
*/

static TIM_HandleTypeDef htim1;
static osThreadId_t tid_Thread_test_pwm;
 
static void Th_pwm (void *argument);                   // thread function

static obj_pwm msg;

	int Init_queue_pwm(void){
  queue_pwm = osMessageQueueNew(8, sizeof(obj_pwm), NULL);
  if(queue_pwm == NULL)
  {
    return(-1);
  }
  return(0);
}

static void pwm_init (TIM_HandleTypeDef *htim){
  
  static GPIO_InitTypeDef timer1 = {0};
	static TIM_OC_InitTypeDef timOC = {0};
	
	// PE9 AF1 TIM1-CH1
  
  //Encdendemos el reloj del Puerto E
	__HAL_RCC_GPIOE_CLK_ENABLE();
  
  //Utilizaremos el Pin 9 del puerto E
	timer1.Pin= GPIO_PIN_9;
  //Lo usaremos en Outpur
	timer1.Mode= GPIO_MODE_AF_PP;
  //Con funcion alternativa de TIM1
  timer1.Alternate= GPIO_AF1_TIM1;
  //SIn PULL de ningun tipo
	timer1.Pull= GPIO_NOPULL;
  //En frecuencia baja
  timer1.Speed= GPIO_SPEED_FREQ_LOW;
  //Lo inicializamos
  HAL_GPIO_Init(GPIOE, &timer1);
	
  //Encendemos el Reloj del TIM1
	__HAL_RCC_TIM1_CLK_ENABLE();
  //Lo instanciamos
	htim -> Instance= TIM1;
  
  //Para lograr una frecuencia de 1KHz configuramemos la señal entrante del APB2 (es el que alimenta el TIM1 (168MHz)) tenemos que dividir entre 4 y luego contar 42000 pulsos
	htim -> Init.Prescaler= 3;
	htim -> Init.Period= 41999;
  //Es contador ascendente y no tiene division
	htim -> Init.CounterMode= TIM_COUNTERMODE_UP;
	htim -> Init.ClockDivision= TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_PWM_Init(htim);
	
  
  //Lo ponemos en PWM
	timOC.OCMode= TIM_OCMODE_PWM1;
	timOC.OCPolarity= TIM_OCPOLARITY_HIGH;
	timOC.OCFastMode= TIM_OCFAST_DISABLE;
	timOC.Pulse= 0; // -- CICLO DE TRABAJO
	HAL_TIM_PWM_ConfigChannel(htim, &timOC, TIM_CHANNEL_1);
  
}

int Init_Th_pwm (void) {
 
  tid_Th_pwm = osThreadNew(Th_pwm, NULL, NULL);
  if (tid_Th_pwm == NULL) {
    return(-1);
  }
 
  return(Init_queue_pwm());
}
 
/*
Como el PWM tiene 4 modos diferentes el mensaje que leera de la cola tambien tendra 4 estados (LOW, MED_LOW, MED_HIGH, HIGH), dependeinedo de cual lea modificara el PWM de una manera u otra
*/

static void Th_pwm (void *argument) {
  
 //Inicializamos todo
	static TIM_HandleTypeDef htim = {0};
	static TIM_OC_InitTypeDef timOC = {0};
	
  //Inicializamos el PWM
	pwm_init(&htim);

    while (1) {
      //Si la lectura es correcta:
        if (osMessageQueueGet(queue_pwm, &msg, NULL, osWaitForever) == osOK) {
          
          //Dependiendo del estado que se le pase modificamos el ciclo de trabajo del PWM
					if (msg.pow == HIGH){
            
            //?Paramos el PWM 
						HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
						timOC.OCMode= TIM_OCMODE_PWM1;
						timOC.OCPolarity= TIM_OCPOLARITY_HIGH;
						timOC.OCFastMode= TIM_OCFAST_DISABLE;
            
            //Cambiamos el Ciclo de trabajo
						timOC.Pulse= 41999; // 100%
						HAL_TIM_PWM_ConfigChannel(&htim, &timOC, TIM_CHANNEL_1);
            
            //Lo volvemos a empezar
						HAL_TIM_PWM_Start(&htim,TIM_CHANNEL_1);
					}
					else if(msg.pow == MED_HIGH){
						HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
						timOC.OCMode= TIM_OCMODE_PWM1;
						timOC.OCPolarity= TIM_OCPOLARITY_HIGH;
						timOC.OCFastMode= TIM_OCFAST_DISABLE;
						timOC.Pulse= 33599; // 80%
						HAL_TIM_PWM_ConfigChannel(&htim, &timOC, TIM_CHANNEL_1);
						HAL_TIM_PWM_Start(&htim,TIM_CHANNEL_1);
					}
					else if(msg.pow == MED_LOW){
						HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
						timOC.OCMode= TIM_OCMODE_PWM1;
						timOC.OCPolarity= TIM_OCPOLARITY_HIGH;
						timOC.OCFastMode= TIM_OCFAST_DISABLE;
						timOC.Pulse= 16799; // 40%
						HAL_TIM_PWM_ConfigChannel(&htim, &timOC, TIM_CHANNEL_1);
						HAL_TIM_PWM_Start(&htim,TIM_CHANNEL_1);
					}
					else{
						HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
						timOC.OCMode= TIM_OCMODE_PWM1;
						timOC.OCPolarity= TIM_OCPOLARITY_HIGH;
						timOC.OCFastMode= TIM_OCFAST_DISABLE;
						timOC.Pulse=0; // 0%
						HAL_TIM_PWM_ConfigChannel(&htim, &timOC, TIM_CHANNEL_1);
						HAL_TIM_PWM_Start(&htim,TIM_CHANNEL_1);
					}
								
				}
		}
  
  }

	
osMessageQueueId_t id_Queue_pwm(void){
	return queue_pwm;
}
	



