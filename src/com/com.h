#ifndef COM_H
#define COM_H

#include "cmsis_os2.h"   
#include "Driver_USART.h"

typedef struct {
    uint8_t SOH_type;
    uint8_t CMD;
    uint8_t LEN;
    char payload[50]; // Asumiendo un tamaño máximo para simplificar
    uint8_t payOK;
    uint8_t EOT_type;
} obj_com;

#define FLAG_USART 0x02

int Init_Thread_COM (void);
osMessageQueueId_t id_Queue_PC_SIS(void);
osMessageQueueId_t id_Queue_SIS_PC(void);

// estructura de trama
#define SOH 0x01
#define EOT 0xFE

// comandos del PC al sistema
#define HORA 0x20                 // establece la hora
#define TEMPERATURA 0x25    // establede el vlaor de temperatura de referencia
#define ALL 0x55                     // todas las medidas
#define CLEAN 0x60                 // borrar todas las medidas

// comandos del sistema al PC
#define PUESTA 0xDF             // puesta en hora
#define TEM_REF 0xDA            // ?????
#define MEDIDA 0xAF                // devuelve el valor de la medida almacenada
#define CLEAN_DONE 0x9F                // borra todas las medidas

#endif