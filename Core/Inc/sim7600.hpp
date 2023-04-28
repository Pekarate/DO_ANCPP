/*
 * sim7600.h
 *
 *  Created on: Apr 13, 2023
 *      Author: ADMIN
 */

#ifndef INC_SIM7600_HPP_
#define INC_SIM7600_HPP_
#include "stdint.h"
int At_Command(char *cmd ,char *RSP1,uint32_t timeout);
int AT_Sms_Send(char* input_number,char* msg);

void SIM7600_TURN_ON();
void SIM7600_TURN_OFF();
void Sim7600_init();
int AT_SIM7600_HTTP_Get(char * request_url,char *rsp,uint16_t *sizess);
int At_Command_response(char *cmd ,char *RSP1,char* rsp,int *len,uint32_t timeout);
int  SIM_7600_read_GNSS(char *Location);
#endif /* INC_SIM7600_HPP_ */
