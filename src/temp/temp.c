/*
Bueno, primer modulo que no tenemos ni idea de como hacerlo, asi que primero nos tendremos que empollar el protocolo I2C, el CMSIS driver para el I2C y como funciona el sensor de temperatura LM75B. 
Empezaremos por el protocolo I2C

Protocolo I2C:

	I2C es un protocolo de comunicación sincrono, es decir que hay un reloj que regula la trasmision de bits de un dispositivo a otro. Este bus cuenta con dos canales, SDA(Serial DAta Line) 
	y SCL(Serial CLock Line), siendo el primero para los datos y el otro para el reloj, a parte de una referencia comun de masa para ambos canales. 
	
	La velocdad de transmision es configurable y puede ser desde 100Kbits/s hasta 3,4 Mbits/s. Como el SPI se configura mediante una estructura Maestro-Esclavo, pero al contrario que el SPI, 
	el I2C soporta multiples Maestros.
	
	Comunicación serie, utilizando un conductor para manejar el timming (SCL) (pulsos de reloj) y otro para intercambiar datos (SDA), que transportan información entre los dispositivos conectados 
	al bus.

	Las líneas SDA (Serial Data) y SCL (Serial Clock) están conectadas a la fuente de alimentación a través de las resistencias de pull-up. Cuando el bus está libre, ambas líneas están en nivel alto.
	
	Hablemos ahora de como se efectua la transmision de datos, por cada pulsao de reloj se transmitira un bit, y los datos solo pueden conmutar cuando el reloj esta a nivel bajo, partiendo de esta base 
	vamos a crear una trama de bits, el primer bit será un bit de start para confirmar que empieza la trama de datos, los siete siguientes serán la direccion del escalvo en el caso de que estemos 
	utilizando varios esclavos y el octavo si se va a leer o escribir. A continuación enviaremos tramas de 8 bits que serán los datos que queramos leer o escribir, el ultimo bit será un bit de stop. 
	Cabe destacar que cada vez que se reciba o envie una trama de 8 bits, el maestro o el esclavo (dependiendio si es W/R) enviara un ACK confirmando la operaion de lectura o escritura.
	
	En nuestra placa el SDA será el PB9 y el SCK el PB8
*/

/*

CMSIS Driver I2C:
	
	Tras ojear la API de CMSIS Driver , hemos de decir que la manera de manejar el bus I2C guarda bastantes similitudes con la manera en la que manejamos el bus SPI. En primer lugar hemos 
	de declarar una estructura del tipo ARM_DRIVER_I2C, a la cual accederemos mediante un puntero.
	
	Lo primero que hemos de hacer es configurar el bus I2C con las opciones que creamos necesarias para nuestra aplicacion para ello, este procedimineto guarda bastante relación con el SPI, 
	ya que las variables que hay dentro de una estructura de datos ARM_DRIVER_I2C son bastante paredidas a las del I2C (Initialize, Control, PowerControl, etc...). Para inicializar el bus I2C 
	crearemos una estructura de datos que acrualice las variables Initialize, PowerControl y control, que son las que nos interesan. A Initialize le pasaremos la funcion de Callback de eventos 
	del I2C, que será una funcioón que active una flag cuando ocurra un evento en el bus I2C. Para el Power control le pasaremos el Parametro ARM_POWER_FULL ya que queremos operar al maximo 
	rendimiento y  por ultimo accederemos tres veces a la variable Control. A la variable Control se le pueden pasar 4 tipos diferentes de parametros: ARM_I2C_OWN_ADDRESS (Para definir 
	la direccion del esclavo) ARM_I2C_BUS_SPEED (Para la velocidad del bus) ARM_I2C_BUS_CLEAR (Para limpiar el bus enviando nueve pulsos de reloj) y ARM_I2C_ABORT_TRANSFER (Para abortar 
	la trasnmision de datos). En nuestro caso solo utilizaremos el ARM_I2C_BUS_CLEAR, ARM_I2C_BUS_SPEED y el ARM_I2C_OWN_ADDRESS.
	
	
	Una vez hayamos inicializado nuestro bus I2C es hora de leer datso del sensor de temperatura, para ello utilizaremos las funciones MasterTransmit y MasterRecive:


	int32_t ARM_I2C_MasterTransmit	(	uint32_t 	addr,
	const uint8_t * 	data,
	uint32_t 	num,
	bool 	xfer_pending)

	addr = Direccion del Esclavo
	data = Puntero a los datos que queremos transmitir al esclavo
	num = Numero de Bytes a transmitir
	xfer_pending = Si la transimion esta pendiente de finalizar, no se generará el bit de stop si esta a true

	
	int32_t ARM_I2C_MasterReceive	(	uint32_t 	addr,
	uint8_t * 	data,
	uint32_t 	num,
	bool 	xfer_pending)	

	addr = Direccion del Esclavo
	data = Puntero a los datos que queremos recibir del esclavo
	num = Numero de Bytes a recibir
	xfer_pending = Si la transimion esta pendiente de finalizar, no se generará el bit de stop si esta a true

	Todo esto combinado con la Callback del I2C nos permite empezar a recibir datos del sensor de temperatura, ¿Pero como estan codificados esos datos?
	
*/

/*

Sensor de Temperatura LM75B:

	Vamos a ir directamente a como manejar este sensor. En primer lugar hemos de asignarle una direcccion de esclavo, esta dirección viene dada por los cuatro primeros bits "1001" y los tres 
	siguientes son configurables por el usuario, en nuestro caso los pondremos todos a uno => 1001000 => 0x48
	
	Para inicializar la lectura desde el master le enviamos la direccion del esclavo seguido de un byte a 0 (asi sabrá que entramos en el registro de temperatura), esperamos a al ACK 
	y emepezamos a leer. 

	Entramos en la chicha, el LM75B lee la temperatura de uan manera un tanto peculiar: Cada lectura del registro de temperatura contiene dos bytes, pero solo contiene información relevante 
	el primero y los tres primeros bits del segundo. Estos bits estan codificados en C2, por lo que mucho ojo al decodificarlo con el bit mas importante si esta a 1 o a 0.
	
	A partir de aqui podemos seguir las instrucciones del fabricante (y un poco de logica en C) para convertir el valor que hemos recogido del sensor y pasarlo a celsius. En primer lugar 
	almacenaremos el dato en un array de dos bytes, sumaremos esos bytes y truncaremos los 5 bits menos relevantes. Una vez tenemos el dato con 11 bits tal cual lo queremos 
	lo multiplicaremos por 0.125 tal y como dice el fabricante para convertirlo a ºC


Vamos a implemnetarlo!!!
*/


#include "temp.h"
 
osThreadId_t tid_Thread_temp;     //Identificador del Thread 
osMessageQueueId_t queue_temp;    //Identificador de la cola 
osTimerId_t tim_temp;             //Temporizador para medir la temperatura cada segundo
obj_temp msg_temp;                //Esta será la estructura donde se guarda la temperatura y se mete a la cola

/*
Estructura para poder manejar el I2C y su correspondiente puntero para acceder a ella 
*/

extern ARM_DRIVER_I2C Driver_I2C1;
ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C1;

void Thread_temp (void *argument);              //Inicializador del hilo
static void i2c_init(void);                     //Inicializador del I2C
static void tim_i2c_Callback(void* argument);   //Callback del I2C (Se accederá a ella cada vez que se produzca una trasmision por el bus I2C)

/*
Callback del I2C: Activara la Flag I2C_FLAG cada vez que se produzca una transmision de datos por el I2C
*/

static void i2c_Callback(uint32_t event){
	osThreadFlagsSet(tid_Thread_temp, I2C_FLAG);
}

/*
Como en todos los modulos definimos una funcion que nos devuelva el identificador de la cola para que pueda ser accesible desde el main
*/

osMessageQueueId_t id_Queue_temp(void){
	
	return queue_temp;
}

/*
Esta es la callback del timer que nos permite leer la temperatura cada segundo, activara la flag TIM_FLAG
*/

static void tim_i2c_Callback(void* argument){
	
	osThreadFlagsSet(tid_Thread_temp, TIM_FLAG);
	
}

/*
Inicializamos la cola de la temperatura, como siempre hay que asignarle un tamaño, al ser una cola bastante concurrida (lee cada segundo) le asignaremos 16 de tamaño, el tamaño de cada objeto
será el de la estructura en el que guardemos la temperatura
*/

static int Init_queue_temp(void){
	
	queue_temp = osMessageQueueNew(16, sizeof(obj_temp), NULL);
	if(queue_temp == NULL){
		return(-1);
	}else{
		return (0);
	}
	
}

/*
Inicializamos el hilo, si no hay fallos inicializa la cola de la temperatura
*/
 
int Init_Thread_temp (void) {
 
  tid_Thread_temp = osThreadNew(Thread_temp, NULL, NULL);
  if (tid_Thread_temp == NULL) {
    return(-1);
  }
 
  return(Init_queue_temp());
}

/*
Empieza lo bueno, en primer lugar vamos a definirnos una función que nos permitra inicializar el bus I2C, para ello tiocaremos los siguientes parametros:
*/


static void i2c_init(void){
	
	/*
	La función que será llamada cada vez que se produzca una transmisión de datos a través del I2C, nos ayudara a temporizar la lectura de datos del sensor
	*/
	
	I2Cdrv ->Initialize(i2c_Callback);    

	/*
	Configuarremos su modo de comsumo al maximo, asi podrá rendir al máximo
	*/
	
	I2Cdrv ->PowerControl(ARM_POWER_FULL);
	
	/*
	Configuramos la velocidad del bus en nuestro caso ARM_I2C_BUS_SPEED_STANDARD
	*/
                         
	I2Cdrv ->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
	
	/*
	Limpiamos el bus I2C de posibles comunicaciones que se hubieran quedado a medias (como hay que pasarle un argumento, le pasamos un 0)
	*/
	
	I2Cdrv ->Control(ARM_I2C_BUS_CLEAR, 0);
	
	/*
	Definimos la dirección del esclavo 
	*/
	
	I2Cdrv ->Control(ARM_I2C_OWN_ADDRESS, 0x48);

}
 
/*

Lo mas importante como leemos la temperatura del LM75B, este sensor nos entrega la temperatura mediante dos bytes de 8 bits cada uno, pero los 5 ultimos digitos del ultimo byte son 
insgnificantes para nuestro cálculo. 

En primer lugar vamos a explicar el prodecidimiento para leer temperatura. Lo primero será que el Master le pase al sensor de que registro quiere leer, en nuestro caso del registro
temp cuya direccion en 0x00, para acceder a el utilizaremos la funcion MasterTransmit, a la cual hay que pasarle los siguientes argumentos:

int32_t ARM_I2C_MasterTransmit	(	uint32_t 	addr,
	const uint8_t * 	data,
	uint32_t 	num,
	bool 	xfer_pending)

	addr = Direccion del Esclavo
	data = Puntero a los datos que queremos transmitir al esclavo
	num = Numero de Bytes a transmitir
	xfer_pending = Si la transimion esta pendiente de finalizar, no se generará el bit de stop si esta a true

En primer lugar le pasaremos la dirección del esclavo la cual hemos decidido que será 0x48 (1001000)

A continuación hay que pasarle un puntero a una variable de 8 bits (aunque el registro tenga dos bytes, el primero siempre esta a 0) con el registro que queremos acceder, en nuestro caso
el de temperatura (0).

El siguiente parametro será el numero de bytes que queremos leer, en nuestro caso 2, recordar que queremos acceder a 0x00.

Por ultimo, hay que pasarle true porque no queremos finalizar la transmisión ya que ahora vendrán los datos

Tras esto esperamos a que se activle I2C_Flag, que nos indicara que finalizado la transmisión 

Para los datos vamos a declarar un array de 2 datos de 8 bits donde almacenaremos cada bit, despues de eso desplazaremos el primer dato 8 bits a la izq (recordemos que es el MSB y tiene que actiuar como tal)
lo sumaremos al LSB y desplazaremos 5 a la dcha para eliminar los 5 bits menos significativos (son irrelevantes).

temp[0] = 0101 0101
temp[1] = 1010 1010

temp[0] << 8 = 0101 0101 0000 0000
(temp[0] << 8) + temp[1] = 0101 0101 1010 1010
((temp[0] << 8) + temp[1]) >> 5 = 0010 1010 1101 (685)
685*0.125 = 85.625 ºC

Una vez hecho esto metemos el valor en cola y esperamos un segundo para repetir el proceso

*/

void Thread_temp (void *argument) {
	
	//Variable para acceder al registro de temperatura
	static uint8_t reg_temp = 0;
	
	//Array para guardar los datos de temperatura
	static uint8_t temp[2];
	
	//Inicialización del I2C
	i2c_init();
	
	//Instanciamos el timer que nos permtirá recoger la temperatura cada segundo
	tim_temp = osTimerNew(tim_i2c_Callback, osTimerPeriodic, (void*)0, NULL);
	
	//Iniciar el timer
	osTimerStart(tim_temp, 1000U);
	
  while (1) {
		
		//Eseperar al timer de 1s
	  osThreadFlagsWait(TIM_FLAG, osFlagsWaitAny, osWaitForever);
		
		//Transitir que queremos leer el registro de temperatura
		I2Cdrv ->MasterTransmit(0x48, &reg_temp, 1, true);
		
		//Esperar a la Flag del I2C
		osThreadFlagsWait(I2C_FLAG, osFlagsWaitAll, osWaitForever);
		
		//Recoger la temperatura en 2 bytes y acabar la transmisión 
		I2Cdrv ->MasterReceive(0x48, temp, 2, false);
		
		//Esperar a la Flag del I2C
		osThreadFlagsWait(I2C_FLAG, osFlagsWaitAll, osWaitForever); 
		
		//Hacemos los calculos que hemos descrito en la introducción
		msg_temp.temp = (((temp[0] << 8) + temp[1]) >> 5)* 0.125;
		
		//Lo ponemos en cola 
		osMessageQueuePut(queue_temp, &msg_temp, 0U, 0U);

  }
}
