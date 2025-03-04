#include "pot.h"
#include "math.h"
/*----------------------------------------------------------------------------
 *      Thread 'Potenciometro 1 y 2': Recoge valores del potenciometro y los 
				pasa a valores de entre 5ºC y 30ºC por una cola
 *---------------------------------------------------------------------------*/
 
#define POT_REF 3.3f
#define RESOLUTION_12B 4096U

osThreadId_t id_pot_th;                        // thread id
osMessageQueueId_t queue_pot;
osTimerId_t tim_temp_pot;

 
void Th_pot1_pot2 (void *argument);                   // thread function
int Init_MsgQueue_pot(void);
static void ADC1_pins_F429ZI_config(ADC_HandleTypeDef *hadc1, ADC_HandleTypeDef *hadc2);
static void tim_temp_Callback(void* argument);

osMessageQueueId_t id_queue_pot(void){
	
	return queue_pot;
}


// --- Inicializacion de la cola
int Init_MsgQueue_pot(void){
  queue_pot = osMessageQueueNew(16, sizeof(obj_pot), NULL);
  if(queue_pot == NULL)
    return (-1); 
  return(0);
}


// --- Inicializacion del Thread
int Init_Thread_pot (void) {
  id_pot_th = osThreadNew(Th_pot1_pot2, NULL, NULL);
  if (id_pot_th == NULL) {
    return(-1);
  }
  return(Init_MsgQueue_pot());
}




// --- Inicializar el potenciometro
static void ADC1_pins_F429ZI_config(ADC_HandleTypeDef *hadc1, ADC_HandleTypeDef *hadc2){ 
    GPIO_InitTypeDef GPIO_InitStruct1 = {0};
    GPIO_InitTypeDef GPIO_InitStruct2 = {0};

    /* Configuración de PA3 (ADC2) POTENCIOMETRO 1*/
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct1.Pin = GPIO_PIN_3;
    GPIO_InitStruct1.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct1.Pull = GPIO_NOPULL;
    GPIO_InitStruct1.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct1);
		
		 /* Configuración de PC0 (ADC1) POTENCIOMETRO 2 */ 
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct2.Pin = GPIO_PIN_0;
    GPIO_InitStruct2.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct2.Pull = GPIO_NOPULL;
    GPIO_InitStruct2.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct2);
	}

	
	// proceso de una conversion
	int ADC_Init_Single_Conversion(ADC_HandleTypeDef *hadc, ADC_TypeDef  *ADC_Instance){
		/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
	__HAL_RCC_ADC2_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();
  hadc->Instance = ADC_Instance;
  hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc->Init.Resolution = ADC_RESOLUTION_12B;
  hadc->Init.ScanConvMode = DISABLE;
  hadc->Init.ContinuousConvMode = DISABLE;
  hadc->Init.DiscontinuousConvMode = DISABLE;
  hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc->Init.NbrOfConversion = 1;
  hadc->Init.DMAContinuousRequests = DISABLE;
  hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
			if (HAL_ADC_Init(hadc) != HAL_OK){
				return -1;}
			
	return 0;
	}
   
	// adquirir el valor de la conversion 
double ADC_getVoltage(ADC_HandleTypeDef *hadc, uint32_t Channel)
	{
		ADC_ChannelConfTypeDef sConfig = {0};
		HAL_StatusTypeDef status;

		uint32_t raw = 0;
		double voltage = 0;
		 /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = Channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
  {
    return -1;
  }
		
		HAL_ADC_Start(hadc);
		
		do{
			status = HAL_ADC_PollForConversion(hadc, 0);} //This funtions uses the HAL_GetTick(), then it only can be executed wehn the OS is running
		while(status != HAL_OK); // CUIDADO, creo que es una funcion bloqueante
		
		raw = HAL_ADC_GetValue(hadc);
		
		voltage = raw*POT_REF/RESOLUTION_12B; 
		
		return voltage;

}


void Th_pot1_pot2 (void *argument) {
	
 static ADC_HandleTypeDef hadc1 = {0}; // POTENCIOMETRO 1, PA3 ------> ADC123_IN3,  CHANNEL 3
 static ADC_HandleTypeDef hadc2 = {0}; // POTENCIOMETRO 2, PC0 ------> ADC123_IN10, CHANNEL 10
 static obj_pot msg;
 
  //Init_MsgQueue_pot(); se ejecuta en Init_Thread_pot
 
  ADC1_pins_F429ZI_config(&hadc1, &hadc2);
  double voltage1, voltage2;
	double valorAnterior1=0, valorAnterior2=0, valorActual1, valorActual2;
	double dif1, dif2; // diferencia entre el valor enterior y el vlaor actual para valorar si debe de enviarse o no
		
  ADC_Init_Single_Conversion(&hadc1,ADC2);
  ADC_Init_Single_Conversion(&hadc2,ADC1);
 
 tim_temp_pot = osTimerNew(tim_temp_Callback, osTimerPeriodic, (void*)0, NULL);
 osTimerStart(tim_temp_pot, 1000U);
 
  while (1) {
    // Leer valores de los potenciómetros
		
		
			osThreadFlagsWait(FLAG_TEMP, osFlagsWaitAny, osWaitForever);
			
			voltage1= ADC_getVoltage(&hadc1, 3); // CHANNEL 3 
			voltage2= ADC_getVoltage(&hadc2, 10); // CHANNEL 10 
			
			// Actualizar el mensaje con los nuevos valores, se requieren valores de 5 a 30 y el voltage1/2 estan sobre 0 a 3.3
		
			valorActual1 = (round(((voltage1/3.3)*25+5)*10))/10;
			valorActual2 = (round(((voltage2/3.3)*25+5)*10))/10;
			
			dif1 = fabs(valorActual1-valorAnterior1);
			dif2 = fabs(valorActual2-valorAnterior2);
			
			/* Para que no se sature la cola con valores iguales, estoy comparando en cada lectura el valor
			leido con el valor acual, de manera que solo se guarda en la cola los valores nuevos, ademas, 
			dado que varia mucho hasta los decimales (posiblemente por la calidad de mis cables) los he pasado al 
			final como int, por la misma razon, para que no sature la cola de valores repetidos 
			*/

			if(dif1>=0.1 || dif2>=0.1){ // dado que es muy sensible, para no saturar la cola que solo lo envía por la cola si 
																	// tiene una diferencia de valor de 0.2
			
				valorAnterior1=valorActual1;
				valorAnterior2=valorActual2;
				msg.pot1=valorActual1;
				msg.pot2=valorActual2;
		
			// Enviar el mensaje a la cola
			osMessageQueuePut(queue_pot, &msg, 0U, 0U);
			}

  }
}

// este timer salta cada 100ms (sustitucion del delay)
static void tim_temp_Callback(void* argument){ 
	osThreadFlagsSet(id_pot_th, FLAG_TEMP);
}
