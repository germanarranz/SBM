#include "lcd.h"
#include "Arial12x12.h"
 
/*
MODULO LCD: 

- Módulo encargado de representación de información por el LCD conectado al bus SPI 
- Thread leyendo de cola de mensajes con la información que debe representarse en el LCD y la línea en que debe representarse 
- Simpemnetos hemos de aplicar todo lo que hemos aprendido del LCD y del SPI, para crear un módulo que pueda leer mensajes de una cola y representarlos
en el LCD. Tambien simplificaremos el codigo de manejo del LCD

*/


osMessageQueueId_t queue_lcd;            //Identificador de la cola del LCD
osThreadId_t tid_Thread_lcd;             //Thread encargado de gestionar el LCD
osMessageQueueId_t id_Queue_lcd(void);   //Funcion para que se pueda referir al la cola del LCD desde otros módulos del proyecto (devulelve queue_lcd y se define en el .h también)
obj_lcd msg_lcd;                         //Mensaje que se le pasara a la cola (se detalla en el .h)
osTimerId_t tim_reset;                   //Timer del reset del LCD (eliminamos el timer físico anterior y lo sustituimos por uno virtual)

extern ARM_DRIVER_SPI Driver_SPI1;       //Definimos la estructura para moder trabajar con CMSIS driver, en este caso con la parte del LCD
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

unsigned char buffer[512];               //Array en el que iran todos los valores en display del LCD
static uint16_t positionL1, positionL2;  //Variables para manejar los offset en fichero Arial12x12.h

void Thread_lcd (void *argument);        //Declaracion de la funcionalidad principal del hilo del LCD
static void Thread_test(void* argument); //Hilo de prueba que mete valores a la cola

static void LCD_init(void);                             //Inicialización del LCD
static void LCD_reset(void);                            //Reset del LCD
static void LCD_wr_data(unsigned char data);            //Escribe los datos que queremos representar en el LCD
static void LCD_wr_cmd(unsigned char cmd);              //Escribe los comandos de configuración en el LCD
static void LCD_update(void);                           //La funcion que se encarga de asignar los valores que queremos escribir al buffer de salida
static void symbolToLocalBuffer_L1(uint8_t symbol);     //Se encarga de la linea 1
static void symbolToLocalBuffer_L2(uint8_t symbol);     //Se encarga de la linea 2
static void dataToBuffer(obj_lcd msg);                  //Trabaja con los strings que le podamos pasar y los pasa
static void tim_reset_Callback(void* argument);         //Callback del timer de reset del LCD


//Callback del timer

static void tim_reset_Callback(void* argument){
	
	//Activa la flag RESET_FLAG
	
	osThreadFlagsSet(tid_Thread_lcd, RESET_FLAG);
	
}

//Funcion para acceder a la cola del LCD desde otras partes del código

osMessageQueueId_t id_Queue_lcd(void){
	
	//Devuelve el identificador de la cola
	
	return queue_lcd;
}

//Creamos la cola donde van a ir todos los mensajes del LCD

int Init_queue_lcd(void){
	/*
	MUY IMPORTANTE: 
	Creamos la cola, para ello invocamos a la funcion osMessageQueueNew y le pasamos los siguientes parametros:
	1. El numero maximo de mensajes que queremos en la cola, por ejemplo 16 (completamente arbitrario y elegido en base a que no creemos que haya mas de 16 mensajes en esta cola)
	2. El tamaño maximo de datos de cada mensaje, en nuestro caso será el tamaño de la estructura de datos
	3. NULL porque queremos los valores por defecto
	*/
	
	queue_lcd = osMessageQueueNew(16, sizeof(obj_lcd), NULL);
	
	//Gestion de errores

	
	if (queue_lcd == NULL) {
		return (-1);
	}
	return (0);
}

//Creamos el hilo que gestionara el LCD

int Init_Thread_lcd (void) {
 
  tid_Thread_lcd = osThreadNew(Thread_lcd, NULL, NULL);
	
	//También instanciamos el timer que reseteara el LCD
	tim_reset = osTimerNew(tim_reset_Callback, osTimerOnce, (void*)0, NULL);
  if (tid_Thread_lcd == NULL) {
    return(-1);
  }
	
	//Si no hay errores Inicializamos la cola
  return(Init_queue_lcd());
}
 


/*
Esta es la funcionalidad del hilo, y no podría ser mas simple. Simplemente tenemos que inicializar el LCD y estar constantemente sacando mensajes de la cola, si no ha habido errores en este proceso
imprimimos en el LCD el mensaje que hayamos obtenido
*/


void Thread_lcd (void *argument) {
  LCD_init();
  
  while (1) {
    if(osMessageQueueGet(id_Queue_lcd(), &msg_lcd, NULL, osWaitForever) == osOK){
      dataToBuffer(msg_lcd);
    }
                    
  }
}

//Reset del LCD

void LCD_reset(void){
	
	static GPIO_InitTypeDef GPIO_InitStruct;
	
	SPIdrv->Initialize(NULL);
  SPIdrv->PowerControl(ARM_POWER_FULL);
  SPIdrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 20000000);
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	
	 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;         //Lo ponemos en output digital
	GPIO_InitStruct.Pull = GPIO_PULLUP;                  //Lo ponemos en pull-up
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;        //Lo ponemos a alta velocidad
	GPIO_InitStruct.Pin = GPIO_PIN_13;                   //F13 A0
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
	
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;          //Lo ponemos en output digital
	GPIO_InitStruct.Pull = GPIO_PULLUP;                  //Lo ponemos en pull-up
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   //Lo ponemos a alta velocidad
	GPIO_InitStruct.Pin = GPIO_PIN_6;                    //A6 Reset
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;          //Lo ponemos en output digital
	GPIO_InitStruct.Pull = GPIO_PULLUP;                  //Lo ponemos en pull-up
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;        //Lo ponemos a alta velocidad
	GPIO_InitStruct.Pin = GPIO_PIN_14;                   //D14 Chip select
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	
	//En vez de utilizar un timer físico lo hemos sustituido por timers virtuales
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	osTimerStart(tim_reset, 1U);
	osThreadFlagsWait(RESET_FLAG, osFlagsWaitAny, osWaitForever);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	osTimerStart(tim_reset, 1U);
	osThreadFlagsWait(RESET_FLAG, osFlagsWaitAny, osWaitForever);

}
/*
Esta función hace que el LCD pueda leer datos, y se invocara cuando se quiera cambiar el display del LCD
*/


static void LCD_wr_data(unsigned char data){
	
	/*
	En primer lugar definimos una variable tipo ARM_SPI_STATUS para almacenar el estado del driver de CMSIS
	*/

  static ARM_SPI_STATUS estado;
	
	/*
	Activamos el CS (es activo a nivel bajo) para que podamos manejar el LCD
	*/
	
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	
	/*
	Habilitamos el A0 a nivel alto para que el LCD sepa que todo lo que viene apartir de ahora son bits de datos
	*/
	
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
	
	/*
	Enviamos al SPI los datos que nos han pasado como parametro de la función, para ello hay que enviar un puntero apuntando a los datos, y el tamaño de los datos
	*/
	
  SPIdrv->Send(&data, sizeof(data));
	
	/*
	Ahora vamos a crear una transferencia de datos bloqueante, que esta siempre comprobando el el esstado del driver, mientras este busy (enviando datos) no hará nada (estare constantemente buscando del estado
	del driver CMSIS), cuando este cambie (haya terminado de mandar datos) saldra del bucle y volvera a poner el CS a nivel alto (desactivara la comunicacion con el LCD)
	*/
	
  do{

    estado = SPIdrv->GetStatus();


  }while(estado.busy);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);

}

static void LCD_wr_cmd(unsigned char cmd){
	
	/*
	En primer lugar definimos una variable tipo ARM_SPI_STATUS para almacenar el estado del driver de CMSIS
	*/

  static ARM_SPI_STATUS estado;
	
	/*
	Activamos el CS (es activo a nivel bajo) para que podamos manejar el LCD
	*/
	
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	
	/*
	Habilitamos el A0 a nivel bajo para que el LCD sepa que todo lo que viene apartir de ahora son bits de cmd (configuración)
	*/
	
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_RESET);
	
	/*
	Enviamos al SPI los cmd que nos han pasado como parametro de la función, para ello hay que enviar un puntero apuntando a los datos, y el tamaño de los datos
	*/
	
  SPIdrv->Send(&cmd, sizeof(cmd));
	
	/*
	Ahora vamos a crear una transferencia de datos bloqueante, que esta siempre comprobando el el esstado del driver, mientras este busy (enviando datos) no hará nada (estare constantemente buscando del estado
	del driver CMSIS), cuando este cambie (haya terminado de mandar datos) saldra del bucle y volvera a poner el CS a nivel alto (desactivara la comunicacion con el LCD)
	*/
	
  do{

    estado = SPIdrv->GetStatus();


  }while(estado.busy);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);

}

/*
Esta función le pasa al LCD unos parametros predefinidos para su correcta configuración. El significado de cada uno se detalla a continuación
*/

void LCD_init(void){ 
	LCD_reset();       //Hemos implemntado en la propia fncion de inicialización, asi no tenemos que llamar a dos funciones
  LCD_wr_cmd(0xAE);  //Display OFF
  LCD_wr_cmd(0xA2);  //Fija el valor de la relación de la tensión de polarización del LCD a 1/9 
  LCD_wr_cmd(0xA0);  //El direccionamiento de la RAM de datos del display es la normal
  LCD_wr_cmd(0xC8);  //El scan en las salidas COM es el normal
  LCD_wr_cmd(0x22);  //Fija la relación de resistencias interna a 2
  LCD_wr_cmd(0x2F);  //Power on
  LCD_wr_cmd(0x40);  //Display empieza en la línea 0
  LCD_wr_cmd(0xAF);  //Display ON
  LCD_wr_cmd(0x81);  //Contraste 
  LCD_wr_cmd(0x12);  //Valor Contraste (a eleccion del usuario)
  LCD_wr_cmd(0xA4);  //Display all points normal
  LCD_wr_cmd(0xA6);  //LCD Display normal
  
}

/*
Esta fnción recorre todo el array buffer y se lo va pasando a la función LCD_wr_data para que lo escriba en el LCD 
*/

void LCD_update(void){
	
  static int i;             //Variable para recorrer todo el array  

	/*
	Ahora vamo a meter todo lo que sería propio de la linea dos a la linea uno
	(todo lo que seria de la pagina 2 a la pagina 1)
	*/
	
	
	/*Primero hay que inicializar los bit a 0*/	

  LCD_wr_cmd(0x00);         //4 bits de la parte baja de la dirección a 0
  LCD_wr_cmd(0x10);         //4 bits de la parte alta de la dirección a 0
  LCD_wr_cmd(0xB0);         //Pagina 0

  for(i=0;i<128;i++){       //Recorremos toda esa pagina

    LCD_wr_data(buffer[i]);

  }
	


  LCD_wr_cmd(0x00);
  LCD_wr_cmd(0x10);
  LCD_wr_cmd(0xB1);         //Pagina 1

  for(i=128;i<256;i++){

    LCD_wr_data(buffer[i]);
  }
	


  LCD_wr_cmd(0x00);
  LCD_wr_cmd(0x10);
  LCD_wr_cmd(0xB2);        //Pagina 2

  for(i=256;i<384;i++){

    LCD_wr_data(buffer[i]);
  }

  LCD_wr_cmd(0x00);
  LCD_wr_cmd(0x10);
  LCD_wr_cmd(0xB3);        //Pagina 3

  for(i=384;i<512;i++){

    LCD_wr_data(buffer[i]);
  }

}

/*Esta es la funcion en concreto, recibe un caracter en uint8_t y toma los 
valores del fichero Arial12x12.h y los carga en el buffer*/

static void LCD_symbolToLocalBuffer_L1(uint8_t symbol){ 
	
	/*
	En primer lugar declaramos variables auxiliares que nos ayudaran a almacenar
	valores mientras se recorre cada linea del fichero Arial12x12.h
	*/
	
  uint8_t i, value1, value2;
  uint16_t offset = 0;
	
	
	/*
	Luego debemos localizar la linea del array del fichero Arial12x12.h para poder
	recorrerla y pasar los valores a buffer, para ello declaramos una variable
	que llamaremos offset y que se "saltara" lineas dependiendo de que caracter hayamos
	seleccionado. El propio offset tien un offset que es el primer caracter ASCII, siendo
	este el ' '. Se multiplica por 25 porque cada carcter tiene 25 bytes
	*/
  
  offset = 25*(symbol - ' ');
	
	/*
	El fichero Arial12x12.h esta distribuido tal que los caracteres con numero impar 
	se corresponden con los de la primera pagina y los valores con numero par se 
	corresponden con los de la segunda página, de ahi viene los offsets de 1 y 2, para saltarse
	el primer byte de cada caracter y luego para ir de imapar en par.
	*/
  
  for(i=0;i<12;i++){
		
    value1 = Arial12x12[offset+i*2+1];
    value2 = Arial12x12[offset+i*2+2];
		
		/*Por ultimo escribimos los valores en el buffer el impar ira a la pagina 1 
		y el par a la linea 2*/
    
    buffer[i+positionL1] = value1; // Pagina 0
    buffer[i+128+positionL1] = value2; // Pagina 1
  }
	
	/*
	Creamos una variable que lee el primer valor de cada caracter del fichero Arial, este valor indica la anchura del caracter, asi que si vamos incrementando esa variable con el valor de la anchura
	de los diferentes caracteres. Despues se la sumamos al offset del buffer y asi el texto se va ajustando a la anchura de cada caracter.
	*/
	
  positionL1 += Arial12x12[offset];

}

static void LCD_symbolToLocalBuffer_L2(uint8_t symbol){
	
	/*
	Misma funcion que le anterior, solo que ahora se modifican los offset en buffer para que en vez de escribir en la primera linea escriba en la segunda (paginas 2 y 3)
	*/
	
  uint8_t i, value1, value2;
  uint16_t offset = 0;

  
  offset = 25*(symbol - ' ');
  
  for(i=0; i<12; i++){
    value1 = Arial12x12[offset+i*2+1];
    value2 = Arial12x12[offset+i*2+2];
    
    buffer[i+256+positionL2] = value1; // Pagina 2
    buffer[i+384+positionL2] = value2; // Pagina 3
  }
  positionL2 += Arial12x12[offset];

}

static void LCD_symbolToLocalBuffer(uint8_t line, uint8_t symbol){
	
	/*
	Modificamos la version anterior de la funcion para que ahora reciba también la linea donde se quiere 
	escribir como primer parametro. Dependiendo si es 1 o 2 llamara a una funcion de escritura o a otra
	*/
	
  if (line == 1){
    LCD_symbolToLocalBuffer_L1(symbol);
  }
	
  else if(line == 2){
    LCD_symbolToLocalBuffer_L2(symbol);
  }
	
}

/*

Hemos cambiado esta funcion respecto a las prácticas anteriores, ahora en vez de recibir dos arrays de caracteres, recibe un dato de tipo obj_lcd
y se encarga de ponerlo en la linea correspondiente.

MUY IMPORTANTE: Si solo queremos escribir en una lina, la otra hay que mandarla vacía.
*/

static void dataToBuffer(obj_lcd msg){
	static int i, j;
  positionL1 = 0;
  positionL2 = 0;
	
	//Lee el dato de la primera linea y lo pone en el buffer
	
	for(i = 0; i < strlen(msg.l_1); i++){
		
		LCD_symbolToLocalBuffer(1, msg.l_1[i]);
	}
	
	//Ponemos a cero el buffer
	
	for(i = positionL1; i < 128; i++){
		
		buffer[i] = 0x00;
		buffer[i+128] = 0x00;
		
	}	
	
	//Lee el dato de la segunda linea y lo pone en el buffer
	
	for(j = 0; j < strlen(msg.l_2); j++){
		
		LCD_symbolToLocalBuffer(2, msg.l_2[j]);
	}
	
	//Pone a 0 la segunda linea
	
	for(j = positionL2; j < 128; j++){
		
		buffer[j+256] = 0x00;
		buffer[j+384] = 0x00;
		
	}
	
	//Actualizamos el LCD direcatamente aqui, asi mo tenemos que llamarla fuera de aqui.
  LCD_update();
 
}


