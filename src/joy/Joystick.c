/*
MODULO JOYSTICK: 

En este módulo vamos a gestionar el uso del Joystick, este detectera hacia que dirección se pulsa el Joystick y el tipo de pulsación que tiene, larga o corta.
Toda esta información será añmacenada en una estructura de datos y posteriormente será metida en una cola que podrá ser leida por el modulo principal del programa.

*/
#include "Joystick.h" 
 
osThreadId_t tid_Thread_Joystick;     //Identidicador del hilo que manejara el Joystick          
osMessageQueueId_t queue_joystick;    //Identificador de la cola que contendrá los valores de la pulsación (corta o larga y la dirección) 
static uint16_t flags;                       //Variable para almacenar las flags (REB_FLAG, IRQ_FLAG, CHECK_FLAG)
osTimerId_t tim_reb;									//Timer encargado de gestionar los rebotes, durará 50 ms
osTimerId_t tim_pul_L;                //Timer encargado de averiguar si se ha realizado una pulsación larga o corta
static obj_joystick msg;                     //Definimos una estructura que actuara como mensaje, la hemos definido en el .h
uint8_t pulsed = 0;                   //Variable auxiliar para ayudarnos a identificar en todo momento si un pulsador esta pulsado o no 
static uint8_t cnt = 0;                      //Variale auxiliar que nos ayudarara a saber cuantos ciclos del tim_pul_L llevamos para discernir entre una pulsacion corta o larga

void Thread_joy_Joystick (void *argument);     //Declaración de la función con la funcionalidad del hilo (gestionar el joystick)
static void joystick_init(void);               //Declaración de la función que iniciliza los pines del joystick

/*
Callback para tim_reb, su único cometido será el de activar la flag de los rebotes
*/

static void tim_reb_Callback(void* argument){  
	osThreadFlagsSet(tid_Thread_Joystick, REB_FLAG);      
}

/*
Callback para tim_pul_L, su único cometido será el de activar la flag de la pulsación larga
*/

static void tim_pul_L_Callback(void* argument){
  osThreadFlagsSet(tid_Thread_Joystick, CHECK_FLAG);
  
}

/*
Creación de la cola del Joystick
*/

int Init_queue_joystick(void){
	/*
	MUY IMPORTANTE: 
	Creamos la cola, para ello invocamos a la funcion osMessageQueueNew y le pasamos los siguientes parametros:
	1. El numero maximo de mensajes que queremos en la cola, por ejemplo 16 (completamente arbitrario y elegido en base a que no creemos que haya mas de 16 mensajes en esta cola)
	2. El tamaño maximo de datos de cada mensaje, en nuestro caso será el tamaño de la estructura de datos
	3. NULL porque queremos los valores por defecto
	*/
	
	queue_joystick = osMessageQueueNew(16, sizeof(obj_joystick), NULL);
	
	//Gestion de errores
	
	if (queue_joystick == NULL) {
		return (-1);
	}
	return (0);
}

/*
Inicialización del hilo del Joystick
*/

int Init_Thread_Joystick (void) {
	/*
	MUY IMPORTANTE: 
	Creamos el hilo, para ello invocamos a la funcion osThreadNew y le pasamos los siguientes parametros:
	1. La función que queremos implementar
	2. Un puntero a al primer arguemnto (en nuestro caso ninguno)
	3. NULL porque queremos los valores por defecto
	*/
  tid_Thread_Joystick = osThreadNew(Thread_joy_Joystick, NULL, NULL); 
  if (tid_Thread_Joystick == NULL) {
    return(-1);
  }
	
	//Si el hilo se ha inicilizado correctamente, inicializamos la cola
 
  return(Init_queue_joystick());
}


/*
Una vez hemos declarado todo esto podemos definir la funcionalidad de este hilo
*/
 
void Thread_joy_Joystick (void *argument) {
	
	//Inicializamos los pines del Joystick
	
	joystick_init();
	
	/*
	Creamos los timers para gestionar los rebotes y la discriminación entre `pulsaciones largas y cortas
	*/
	
	/*
	El de los rebotes será un timer que llamara a la Callback de los rebotes y será one-shot
	*/
	
	tim_reb = osTimerNew(tim_reb_Callback, osTimerOnce, (void*)0, NULL); 
	
	/*
	Mientras que el de las pulsaciones largs y cortas también llamará a su propia callback pero este será periodico (estará todo el rato ejecurtandose)
	*/
	
  tim_pul_L = osTimerNew(tim_pul_L_Callback, osTimerPeriodic, (void*)0, NULL);
	
  while (1) {
		
		/*
		Eseperamos a REB_FLAG o IRQ_FLAG o CHECK_FLAG, todo el tiempo que haga falta y guardamos su valor en la variable flags 
		*/
		
		flags = osThreadFlagsWait((REB_FLAG | IRQ_FLAG | CHECK_FLAG),osFlagsWaitAny, osWaitForever);
		
		if(flags == 0x02){
			
			/*
			Si se detecta una interrupción en el pulsador se ejecuta el timer tim_reb por 50ms
			*/
			
			osTimerStart(tim_reb, 50U);
		}
		
		if(flags == 0x01){
			
			/*
			Una vez ya se hayan eliminado los rebotes de la pulsación, podemos pasar a discriminar si la pulsación en el pulsador es corta o larga:
			*/
      
      if ((HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15) == GPIO_PIN_SET) |
				(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12) == GPIO_PIN_SET) |
				(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14) == GPIO_PIN_SET) |
			  (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_SET) |
			  (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_SET)){ //Si el pulsador esta pulsado pasan cosas....
           
				
					 //La variable pulsed se activa
           pulsed = 1;
					 //cnt se inicializa a 0
           cnt = 0;
					 /*
				   MUY IMPORTANTE:
					 La estrategia que vamos a seguir para discriminar entre pulsaciones largas y cortas consiste en tener un timer periodioco (tim_pul_L) contando ciclos de 50ms todo el rato, si cuando llegue a 20
					 el pin sigue estando activo, podremos afirmar que la pulsación ha durado mas de 1s (20*50ms = 1s) y cambiaremos el valor en la estructura de datos
   				 */
           osTimerStart(tim_pul_L, 50U);
         }
         
       }
		
			 /*
			 El timer periodico que hemos declarado (tim_pul_L) activa una Callback que asu vez activa la flag de CHECK_FLAG, que mediante este siguiente if, comprueba cada 50ms el estado del pin
			 */
      
       if(flags == 0x03){ 
				 
         if(cnt == 20){  //Si la cuenta de ciclos ha llegado a 20*50ms = 1s, podemos afirmar que la pulsación ha sido larga y procedemos a cargarla en la estructura de datos.
					 
					 //Reseteamos la cuenta
					 cnt = 0;
					 
					 //Cargamos en la estructura de datos que ha sido larga:
           msg.tipoP = 1;
					 
					 //Paramos el timer a la espera de que se vuelva a iniciar con una nueva pulsación
           osTimerStop(tim_pul_L);
					 
					 /*Finalmente metemos el mensaje a la cola del RTOS, para ello invocamos la siguiente función osMessageQueuePut y le pasamos los siguientes argumentos:
					 1. El osMessageQueueId_t de la cola a la que lo queremos meter 
					 2. Puntero al mensaje que euremos meter
					 3. Prioridad del mensaje (en nuestro caso NULL)
					 4. Timeout del mensaje (en nuestro caso NULL)
					 */
           osMessageQueuePut(queue_joystick, &msg, 0U, 0U);
					
         }else if((HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15) == GPIO_PIN_SET)){ 
					 
					 /*
					 Si el pulsador del medio sigue pulsado incrementamos el numero de ciclos que contamos para saber si es larga o corta (Solo es sensible al pulsador de en medio, ya que es para el unico que importan las 
					 pulsaciones largas)
					 */
         
         cnt++;
       }
        else if(pulsed == 1){
					
					/*
					Si ha sido pulsado pero ya no lo esta, eso significa que la pulsación ha sido corta, por lo que hay que almacenarlo en la estructura
					*/
					
					//Ha sido pulsación corta
          msg.tipoP = 0;
					
					//Paramos el timer a la espera de que se vuelva a iniciar con una nueva pulsación
          osTimerStop(tim_pul_L);
					
					/*Finalmente metemos el mensaje a la cola del RTOS, para ello invocamos la siguiente función osMessageQueuePut y le pasamos los siguientes argumentos:
					1. El osMessageQueueId_t de la cola a la que lo queremos meter 
					2. Puntero al mensaje que euremos meter
					3. Prioridad del mensaje (en nuestro caso NULL)
				  4. Timeout del mensaje (en nuestro caso NULL)
					*/
          osMessageQueuePut(queue_joystick, &msg, 0U, 0U);
					
        }
        
      }
       
    }
  
  }
			
	
	/*
	Inicilaizacion del GPIO del JOYSTICK
	*/

static void joystick_init(void){
	
	static GPIO_InitTypeDef GPIO_InitStruct;
	
	__HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_14;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	
}


void EXTI15_10_IRQHandler(void){  //Ha de ser responsivo con todos estos pines
	
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
	
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	
	//Pinemos la variable pulsed a 0, porque aun no hemos quitado los rebotes 
	
	pulsed  = 0;
	
	//Almacenamos la direccion de pulsación en la estructura de datos, ¡OJO! no es que lo metamos en la cola ya, para ello primero hay que quitar los rebotes y determinar si ha sido larga o corta

		
	switch(GPIO_Pin){
		
		case GPIO_PIN_12:
			msg.dir = DOWN;
		break;
		
		case GPIO_PIN_14:
			msg.dir = LEFT;
		break;
		
		case GPIO_PIN_15:
			msg.dir = MIDDLE;
		break;
		
		case GPIO_PIN_10:
			msg.dir = UP;
		break;
		
		case GPIO_PIN_11:
			msg.dir = RIGHT;
		break;
		
	}
	
	osThreadFlagsSet(tid_Thread_Joystick, IRQ_FLAG);  //Activamos la flag de interrupciones
	
}

/*
Vamos a necesitar una función que nos devuelva la osMessageQueueId_t de la cola del Joystick para poder descargar la cola desde el modulo principal
*/

osMessageQueueId_t id_Queue_joystick(void){
	return queue_joystick;
}


