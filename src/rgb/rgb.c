#include "rgb.h"

static osThreadId_t tid_Thread_rgb;
static osMessageQueueId_t queue_rgb;
static osThreadId_t tid_Thread_test_rgb;
void Init_rgb(void);
void rgb(obj_rgb msg);

obj_rgb msg;

void Thread_rgb (void *argument);
void Thread_rgb_test(void* argument);

osMessageQueueId_t id_Queue_rgb(void){
	return queue_rgb;
}


int Init_queue_rgb(void){
  queue_rgb = osMessageQueueNew(8, sizeof(obj_rgb), NULL);
  if(queue_rgb == NULL)
  {
    return(-1);
  }
  return(0);
}


int Init_Thread_rgb (void) {
 
  tid_Thread_rgb = osThreadNew(Thread_rgb, NULL, NULL);
  if (tid_Thread_rgb == NULL) {
    return(-1);
  }
 
  return(Init_queue_rgb());
}

void Thread_rgb (void *argument) {
  Init_rgb();
  while (1) {
		if(osMessageQueueGet(queue_rgb, &msg, NULL, osWaitForever) == osOK){
			rgb(msg);
		}
  }
}

void rgb(obj_rgb msg){
	
	switch (msg.color){
		
		case RED:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);  //R
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);//G
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);//B
		break;
		
		case YELLOW:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET); //R
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);//G
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);//B
		break;
		
		case CIAN:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);//R
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); //G
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET); //B
		break;
		
		case BLUE:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET); //R
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);//G
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET); //B
		break;
		
		case OFF:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET); //R
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET); //G
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET); //B
		break;
		
	}
	
}

void Init_rgb(void){
  
   static GPIO_InitTypeDef GPIO_InitStruct;
  
  __HAL_RCC_GPIOD_CLK_ENABLE();
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
  
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
  
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
  
  
}




