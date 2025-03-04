#include "com.h"

extern ARM_DRIVER_USART Driver_USART3;
static ARM_DRIVER_USART *USARTdrv = &Driver_USART3;

osThreadId_t tid_Thread_RX;
osThreadId_t tid_Thread_TX;

osMessageQueueId_t queue_SIS_PC;
osMessageQueueId_t queue_PC_SIS;

obj_com msg_com_RX;
obj_com msg_com_TX;

/*---------FUNCIONES------------*/
static void Init_USART(void);
static void CallBack_USART3(uint32_t event);
void Thread_RX (void *argument); 
void Thread_TX (void *argument); 
static int Init_queue_COM(void);

static void Init_USART(void){
		USARTdrv->Initialize(CallBack_USART3); 
	USARTdrv->PowerControl(ARM_POWER_FULL);
	USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                    ARM_USART_DATA_BITS_8 |
                    ARM_USART_PARITY_NONE |
                    ARM_USART_STOP_BITS_1 |
                    ARM_USART_FLOW_CONTROL_NONE, 115200);
	
/* Enable Receiver and Transmitter lines */
	USARTdrv->Control (ARM_USART_CONTROL_TX, 1);
  USARTdrv->Control (ARM_USART_CONTROL_RX, 1);
}
	
static void CallBack_USART3(uint32_t event){
	  uint32_t mask;
  mask = 
         ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_TX_COMPLETE       ;
  
	if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) 
    /* Success: Wakeup Thread */
		osThreadFlagsSet(tid_Thread_RX,FLAG_USART);
    
	if (event &  mask)
			osThreadFlagsSet(tid_Thread_TX,FLAG_USART);
	
}
	
osMessageQueueId_t id_Queue_PC_SIS(void){
	return queue_PC_SIS;
}

osMessageQueueId_t id_Queue_SIS_PC(void){
	return queue_SIS_PC;
}

int Init_Thread_COM (void){
  Init_USART();

tid_Thread_RX = osThreadNew(Thread_RX, NULL, NULL);
tid_Thread_TX = osThreadNew(Thread_TX, NULL, NULL);
  if (tid_Thread_RX == NULL || tid_Thread_RX == NULL) {
    return(-1);
  }
	
  return(Init_queue_COM());

}

void Thread_RX(void* argument){
    uint8_t byte;
    static uint16_t cnt = 0;
  msg_com_RX.CMD=0;
  msg_com_RX.EOT_type=0;
  msg_com_RX.LEN=0;
  msg_com_RX.SOH_type = 0;
  msg_com_RX.payOK=0;
	while(1){

		USARTdrv->Receive(&byte, 1);
		osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
    if(byte==SOH){
        if(msg_com_RX.SOH_type == 0 && msg_com_RX.EOT_type == 0 && msg_com_RX.LEN ==0 && msg_com_RX.CMD ==0 && msg_com_RX.payOK==0){
          msg_com_RX.SOH_type= SOH;    
        } 
    }
    else if(msg_com_RX.SOH_type !=0  && msg_com_RX.EOT_type == 0 && msg_com_RX.LEN ==0 && msg_com_RX.CMD ==0 && msg_com_RX.payOK==0){
          msg_com_RX.CMD= byte;
      }
    else if(msg_com_RX.SOH_type !=0  && msg_com_RX.EOT_type == 0 && msg_com_RX.LEN ==0 && msg_com_RX.CMD !=0 && msg_com_RX.payOK==0){
          msg_com_RX.LEN= byte;
      }
    else if(msg_com_RX.SOH_type !=0  && msg_com_RX.EOT_type == 0 && msg_com_RX.LEN !=0 && msg_com_RX.CMD !=0 && msg_com_RX.payOK==0){
         if(byte!=EOT){
            msg_com_RX.payload[cnt] = byte;
            cnt++;
          }else {
            msg_com_RX.payOK=1;
            cnt=0;
            msg_com_RX.EOT_type= EOT;
            osMessageQueuePut(id_Queue_PC_SIS(), &msg_com_RX, NULL, 0U);
            for(int i = 0; i <msg_com_RX.LEN-3; i++){
            msg_com_RX.payload[i] = 0x00;
           }
            msg_com_RX.CMD=0;
            msg_com_RX.EOT_type=0;
            msg_com_RX.LEN=0;
            msg_com_RX.SOH_type = 0;
            msg_com_RX.payOK=0;
              
         }
      }
	}
}

void Thread_TX(void* argument){
  while(1){
    if((osMessageQueueGet(id_Queue_SIS_PC(), &msg_com_TX, NULL, 100U) == osOK)){ // trama nueva
      uint8_t cnt=0;
      USARTdrv->Send(&msg_com_TX.SOH_type, 1);
      osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
      USARTdrv->Send(&msg_com_TX.CMD, 1);
      osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
      USARTdrv->Send(&msg_com_TX.LEN, 1);
      osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
      USARTdrv->Send(&msg_com_TX.LEN, 1);
      osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
      while(cnt<=(msg_com_TX.LEN-3)){
         USARTdrv->Send(&msg_com_TX.payload[cnt], 1);
         osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
         cnt++;
      }
      USARTdrv->Send(&msg_com_TX.EOT_type, 1);
      osThreadFlagsWait(FLAG_USART, osFlagsWaitAny, osWaitForever);
    
  }

}


}
static int Init_queue_COM(void){
	
	queue_PC_SIS = osMessageQueueNew(16, sizeof(obj_com), NULL);
  queue_SIS_PC = osMessageQueueNew(16, sizeof(obj_com), NULL);
	if(queue_PC_SIS == NULL || queue_SIS_PC == NULL){
		return(-1);
	}else{
		return (0);
	}
	
}






















