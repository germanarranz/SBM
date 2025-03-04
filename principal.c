#include "principal.h"

static osThreadId_t tid_Thread_main;
static void Thread_Main(void* argumemnt);
static void agregarMedida(buf_medidas* buf);
static void procesarComandosRS232(void);
static void clean_buffer(void);

extern uint8_t hora;                                  
extern uint8_t min;
extern uint8_t seg;

t_estado estado;
obj_lcd msg_lcd_main;
obj_joystick msg_joy_main;
obj_temp msg_temp_main;
obj_pwm msg_pwm_main;
obj_rgb msg_rgb_main;
obj_pot msg_pot_main;
obj_com msg_comRX_main;
obj_com msg_comTX_main;
float temp_ref;
buf_medidas buffer_medidas;
osTimerId_t tim_disp; 

static void tim_disp_Callback(void* argument){
	
	osThreadFlagsSet(tid_Thread_main, FLAG_DISP);
	
}

int Init_Thread_principal(void){
  tid_Thread_main = osThreadNew(Thread_Main, NULL, NULL);
  if (tid_Thread_main == NULL){
    return -1;
    
  }else{
    
    return(Init_Thread_clock() |
    Init_Thread_Joystick() |
    Init_Thread_lcd() |
    Init_Thread_pot() |
    Init_Thread_temp() |
    Init_Thread_rgb() |
    Init_Th_pwm() |
		Init_Thread_COM());
  }
  
  
}

static void Thread_Main(void* argument){
	estado = REP;
	temp_ref = 25.0;
  tim_disp = osTimerNew(tim_disp_Callback, osTimerPeriodic, (void*)0, NULL);

	
  while(1){

		
    switch(estado){
      
      case REP:
        msg_rgb_main.color = OFF;
				osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
			  sprintf(msg_lcd_main.l_1, "      SBM 2023");
				sprintf(msg_lcd_main.l_2, "       %.2u:%.2u:%.2u ", hora, min, seg);
				osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);

			
				if((osMessageQueueGet(id_Queue_joystick(), &msg_joy_main, NULL, 100U) == osOK)){
					if((msg_joy_main.dir == MIDDLE) && (msg_joy_main.tipoP == 1)){
						estado = ACTIVO;
						
					}
					
					
				}
					
      break;
      
      case ACTIVO:
				sprintf(msg_lcd_main.l_1, "   ACT---%.2u:%.2u:%.2u---", hora, min, seg);
        if((osMessageQueueGet(id_Queue_temp(), &msg_temp_main, NULL, osWaitForever) == osOK)){
					if(temp_ref - msg_temp_main.temp >= 5){
							msg_rgb_main.color = RED;
							msg_pwm_main.pow = HIGH;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:100%%", msg_temp_main.temp, temp_ref);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if((temp_ref - msg_temp_main.temp <= 5) &&(temp_ref - msg_temp_main.temp > 0)){
							msg_rgb_main.color = YELLOW;
							msg_pwm_main.pow = MED_HIGH;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:80%%", msg_temp_main.temp, temp_ref);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if((temp_ref - msg_temp_main.temp > -5)&&(temp_ref - msg_temp_main.temp < 0)){
							msg_rgb_main.color = CIAN;
							msg_pwm_main.pow = MED_LOW;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:40%%", msg_temp_main.temp, temp_ref);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if(temp_ref - msg_temp_main.temp <= -5){
							msg_rgb_main.color = BLUE;
							msg_pwm_main.pow = LOW;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:0%%", msg_temp_main.temp, temp_ref);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}
          agregarMedida(&buffer_medidas);
				}
        if((osMessageQueueGet(id_Queue_joystick(), &msg_joy_main, NULL, 100U) == osOK)){
					if((msg_joy_main.dir == MIDDLE) && (msg_joy_main.tipoP == 1)){
						estado = TEST;
					}
				}
      break;
      
      case TEST:
				sprintf(msg_lcd_main.l_1, "   TEST---%.2u:%.2u:%.2u---", hora, min, seg);
				osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
				if((osMessageQueueGet(id_queue_pot(), &msg_pot_main, NULL, 10U) == osOK)){
					if(msg_pot_main.pot2 - msg_pot_main.pot1 >= 5.0){
							msg_rgb_main.color = RED;
							msg_pwm_main.pow = HIGH;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:100%%", msg_pot_main.pot1, msg_pot_main.pot2);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if((msg_pot_main.pot2 - msg_pot_main.pot1 >= 0.0) && (msg_pot_main.pot2 - msg_pot_main.pot1 < 5.0)){
							msg_rgb_main.color = YELLOW;
							msg_pwm_main.pow = MED_HIGH;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:80%%", msg_pot_main.pot1, msg_pot_main.pot2);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if((msg_pot_main.pot2 - msg_pot_main.pot1 > -5.0)&&(msg_pot_main.pot2 - msg_pot_main.pot1< 0.0)){
							msg_rgb_main.color = CIAN;
							msg_pwm_main.pow = MED_LOW;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:40%%", msg_pot_main.pot1, msg_pot_main.pot2);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}else if(msg_pot_main.pot2 - msg_pot_main.pot1 <= -5.0){
							msg_rgb_main.color = BLUE;
							msg_pwm_main.pow = LOW;
						sprintf(msg_lcd_main.l_2, " Tm:%.1f-Tr:%.1f-D:0%%", msg_pot_main.pot1, msg_pot_main.pot2);
						osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
						osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
						osMessageQueuePut(id_Queue_pwm(), &msg_pwm_main, NULL, 0U);
					}
					
				}
				
				 if((osMessageQueueGet(id_Queue_joystick(), &msg_joy_main, NULL, 100U) == osOK)){
					if((msg_joy_main.dir == MIDDLE) && (msg_joy_main.tipoP == 1)){
						estado = DEBUG;
					}
				}
        
      break;
      
      case DEBUG:
        osTimerStart(tim_disp, 500U);
				msg_rgb_main.color = OFF;
        osMessageQueuePut(id_Queue_rgb(), &msg_rgb_main, NULL, 0U);
				sprintf(msg_lcd_main.l_1,"      ---P&D---");
				osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
				
				static t_dep dep;
				static bool display = false;
				
				uint8_t hor_u_aux = hora % 10;
				uint8_t hor_d_aux = hora / 10;
				uint8_t min_u_aux = min % 10;
				uint8_t min_d_aux = min / 10;
				uint8_t seg_u_aux = seg % 10;
				uint8_t seg_d_aux = seg / 10;
				uint8_t temp_d_aux = (uint8_t)(temp_ref/10);
				uint8_t temp_u_aux = (uint8_t)temp_ref%10;
				uint8_t temp_dec_aux = ((uint8_t)(temp_ref*10))%10;
				
				while(estado == DEBUG){
            procesarComandosRS232();
					 if((osMessageQueueGet(id_Queue_joystick(), &msg_joy_main, NULL, 100U) == osOK)){
							if(msg_joy_main.tipoP == 0){
								switch(msg_joy_main.dir){
									
									case RIGHT:
										if(dep < TEMP_DEC){
											dep++;
										}
										break;
									case LEFT:
										if(dep > HOR_D){
											dep--;
										}
										break;
									case MIDDLE:
                      if ((hor_d_aux * 10 + hor_u_aux) > 23) {
                        hor_d_aux = 0;
                        hor_u_aux = 0;
                      }
                      poner_hora(((hor_d_aux*10)+hor_u_aux), ((min_d_aux*10)+min_u_aux), ((seg_d_aux*10)+seg_u_aux));
											temp_ref = (float)temp_d_aux * 10.0f + (float)temp_u_aux + (float)temp_dec_aux / 10.0f;
                      if(temp_ref > 30.0){
                         temp_ref = 30.0;
                      }else if(temp_ref < 5.0){
                         temp_ref = 5.0;
                      }
										break;
									
									case UP:
										switch(dep){
											case HOR_D:
												if(hor_d_aux < 2){
													hor_d_aux++;
												}else{
													hor_d_aux = 0;
												}
											break;
											case HOR_S:
												if((hor_d_aux < 2) || (hor_d_aux == 2 && hor_u_aux <3)){
													if (hor_u_aux < 9) {
                            hor_u_aux++;
                            } else {
                              hor_u_aux = 0;
                            }
                        } else {
                          hor_u_aux = 0;
                           }
												break;
											case MIN_D:
												if(min_d_aux < 5){
													min_d_aux++;
												}else{
													min_d_aux = 0;
												}
												break;
											case MIN_S:
												if((min_u_aux < 9) && (min_d_aux < 6)){
													min_u_aux++;
												}else{
													min_u_aux = 0;
												}
											break;
												case SEG_D:
												if(seg_d_aux < 5){
													seg_d_aux++;
												}else{
													seg_d_aux = 0;
												}
											break;
												case SEG_S:
												if((seg_u_aux < 9) && (seg_d_aux < 5)){
													seg_u_aux++;
												}else{
													seg_u_aux = 0;
												}
											break;
											case TEMP_D:
												if(temp_d_aux < 3){
													temp_d_aux++;
												}else{
													temp_d_aux = 0;
												}
											break;
											case TEMP_U:
												if((temp_u_aux < 9) && (temp_d_aux < 3)){
													temp_u_aux++;
												}else{
													temp_u_aux = 0;
												}
											break;
												case TEMP_DEC:
												if(temp_dec_aux < 9){
													temp_dec_aux++;
												}else{
													temp_dec_aux = 0;
												}
											break;
												
										}
									break;
										
										case DOWN:
										switch(dep){
											case HOR_D:
												if(hor_d_aux > 0){
													hor_d_aux--;
												}else{
													hor_d_aux = 2;
												}
											break;
											case HOR_S:
												if(hor_u_aux > 0){
													hor_u_aux--;
												}else{
													hor_u_aux = 9;
												}
												break;
											case MIN_D:
												if(min_d_aux > 0){
													min_d_aux--;
												}else{
													min_d_aux = 5;
												}
												break;
											case MIN_S:
												if(min_u_aux > 0){
													min_u_aux--;
												}else{
													min_u_aux = 9;
												}
											break;
												case SEG_D:
												if(seg_d_aux > 0){
													seg_d_aux--;
												}else{
													seg_d_aux = 5;
												}
											break;
												case SEG_S:
												if(seg_u_aux > 0){
													seg_u_aux--;
												}else{
													seg_u_aux = 9;
												}
											break;
											case TEMP_D:
												if(temp_d_aux > 0){
													temp_d_aux--;
												}else{
													temp_d_aux = 3;
												}
											break;
											case TEMP_U:
												if(temp_u_aux > 0){
													temp_u_aux--;
												}else{
													temp_u_aux = 9;
												}
											break;
												case TEMP_DEC:
												if(temp_dec_aux > 0){
													temp_dec_aux--;
												}else{
													temp_dec_aux = 9;
												}
											break;
										}
									break;
								}
								
							}else{
                estado = REP;
              }
					 }
					 
					 if(display){
						 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:%d%d ---Tr: %d%d.%d", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
						 display = false;
					 }else{
						 switch(dep){
							 case HOR_D:
								 sprintf(msg_lcd_main.l_2," H: _%d:%d%d:%d%d ---Tr: %d%d.%d", hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case HOR_S:
								 sprintf(msg_lcd_main.l_2," H: %d_:%d%d:%d%d ---Tr: %d%d.%d", hor_d_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case MIN_D:
								 sprintf(msg_lcd_main.l_2," H: %d%d:_%d:%d%d ---Tr: %d%d.%d", hor_d_aux, hor_u_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case MIN_S:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d_:%d%d ---Tr: %d%d.%d", hor_d_aux, hor_u_aux, min_d_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case SEG_D:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:_%d ---Tr: %d%d.%d", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_u_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case SEG_S:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:%d_ ---Tr: %d%d.%d", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, temp_d_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case TEMP_D:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:%d%d ---Tr: _%d.%d", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_u_aux, temp_dec_aux);
								 break;
							 case TEMP_U:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:%d%d ---Tr: %d_.%d", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_dec_aux);
								 break;
							 case TEMP_DEC:
								 sprintf(msg_lcd_main.l_2," H: %d%d:%d%d:%d%d ---Tr: %d%d._", hor_d_aux, hor_u_aux, min_d_aux, min_u_aux, seg_d_aux, seg_u_aux, temp_d_aux, temp_u_aux);
								 break;
						 }
						 display = true;
					 }
					osMessageQueuePut(id_Queue_lcd(), &msg_lcd_main, NULL, 0U);
					osThreadFlagsWait(FLAG_DISP, osFlagsWaitAny, osWaitForever);
			}
				

				
				
					break;
			
				
			
      
      
    }
  
}
	
}
  
static void agregarMedida(buf_medidas* buf){
  static int dc_aux;
  
  switch(msg_pwm_main.pow){
    
    case LOW:
      dc_aux = 0;
    break;
    
    case MED_LOW:
      dc_aux = 40;
    break;
    
    case MED_HIGH:
      dc_aux = 80;
    break;
    
    case HIGH:
      dc_aux = 100;
    break;
    
  }
  
  if(buf->cnt == 10){
    buf->ini = (buf->ini +1)%10;
    buf->cnt--;
  }
  
  sprintf(buf->medidas[buf->fin].mesure, "%.2u:%.2u:%.2u--Tm:%.1f--Tr:%.1f--D:%d%%\n\r", hora, min, seg, msg_temp_main.temp, temp_ref, dc_aux);
  buf->fin = (buf->fin + 1)%10;
  buf->cnt++;
}

static void clean_buffer(void){
  for(int i = 0; i < buffer_medidas.cnt; i++){
    for(int j = 0; j < 36; j++){
    buffer_medidas.medidas[i].mesure[j] = 0x00;
  }
}
}


static void procesarComandosRS232(){

        // Verificar si hay comandos nuevos del PC
    if (osMessageQueueGet(id_Queue_PC_SIS(), &msg_comRX_main, NULL, 100U) == osOK) {

     uint8_t h,m,s;
     float temp_ref_aux;
     uint8_t cnt_aux = 0;

                // Procesar el comando recibido
        switch (msg_comRX_main.CMD){
                    case HORA:
                    if (sscanf(msg_comRX_main.payload, "%hhu:%hhu:%hhu", &h, &m, &s) == 3) {
                        if (h < 24 && m < 60 && s < 60) {
                            poner_hora(h,m,s);
                        }
                     msg_comTX_main.SOH_type = SOH;
                     msg_comTX_main.CMD = PUESTA;
                     msg_comTX_main.LEN = 0x0C;
                     sprintf(msg_comTX_main.payload, "%d:%d:%d\n\r",h,m,s);
                     msg_comTX_main.EOT_type = EOT;
                     osMessageQueuePut(id_Queue_SIS_PC(), &msg_comTX_main, NULL, 0U);
                     
                    break;

                    case TEMPERATURA:
                      if (sscanf(msg_comRX_main.payload, "%f", &temp_ref_aux) == 1) {
                        if (temp_ref_aux >= 5.0 && temp_ref_aux <= 30.0) {
                            temp_ref = temp_ref_aux;
                        }
                     msg_comTX_main.SOH_type = SOH;
                     msg_comTX_main.CMD = TEM_REF;
                     msg_comTX_main.LEN = 0x08;
                     sprintf(msg_comTX_main.payload, "%.1f\n\r",temp_ref);
                     msg_comTX_main.EOT_type = EOT;
                     osMessageQueuePut(id_Queue_SIS_PC(), &msg_comTX_main, NULL, 0U);
                    

                    break;

                    case ALL:
                     
                     msg_comTX_main.SOH_type = SOH;
                     msg_comTX_main.CMD = MEDIDA;
                     msg_comTX_main.LEN = 0x25;
                     while(cnt_aux < buffer_medidas.cnt){
                      memcpy(msg_comTX_main.payload, buffer_medidas.medidas[cnt_aux].mesure, 36);
                      cnt_aux++;
                      msg_comTX_main.EOT_type = EOT;
                      osMessageQueuePut(id_Queue_SIS_PC(), &msg_comTX_main, NULL, 0U);
                     }
                      
                    

                    break;

                    case CLEAN:
                     msg_comTX_main.SOH_type = SOH;
                     msg_comTX_main.CMD = CLEAN_DONE;
                     msg_comTX_main.LEN = 0x04;
                     clean_buffer();
                     msg_comTX_main.EOT_type = EOT;
                     
                    

                    break;


        // Enviar respuesta al PC
        // prepararYEnviarRespuesta(msg_PC_SIS.cmd);
            }

    }
    }
}



}

