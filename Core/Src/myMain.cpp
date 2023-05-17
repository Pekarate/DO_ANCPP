/*
 * myMain.cpp
 *
 *  Created on: Apr 16, 2023
 *      Author: ADMIN
 */
#include "stdio.h"
#include "MAX30100.hpp"
#include "string.h"
#include "sim7600.hpp"
MAX30100* pulseOxymeter;
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;
typedef enum{
	SIM_INIT =0,
	SIM_READ_GPS,
	SIM_REQUEST_HTTP,
	SIM_SEND_SMS,
	SIM_DONE,
	SIM_IDLE
}SIM_STATE;


#define NUM_SAMPLE 10
uint8_t heart_beat[NUM_SAMPLE];
uint8_t  heart_beatcnt=0;
uint32_t adder_time;
uint8_t result_heartbeat = 0;
uint16_t BatteryADC[100] = {0};
uint8_t  Battery = 0;
uint8_t sleep_enable = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
//	printf("GPIO %d LOW\r\n",GPIO_Pin);
	printf("weakup by GPIO\r\n");
	HAL_PWR_DisableSleepOnExit ();
}

void enter_sleep_mode(){
	if(!sleep_enable){
		return;
	}
	printf("ENTER SLEEP MODE\r\n");
	HAL_TIM_Base_Stop_IT(&htim3);
	SIM7600_TURN_OFF();
	HAL_Delay(100);

	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	__HAL_TIM_SET_COUNTER(&htim2,0);
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_SuspendTick();

	HAL_PWR_EnableSleepOnExit ();

	//	  Enter Sleep Mode , wake up is done once User push-button is pressed
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
	HAL_Delay(100);
	printf("EXIT SLEEP MODE\r\n");
	HAL_NVIC_SystemReset();
}
void add_heart_beat(uint8_t value)
{
	if(heart_beatcnt ==0)
	{
		heart_beat[heart_beatcnt] = value;
		adder_time = HAL_GetTick() +1500;
		heart_beatcnt++;
	}
	else
	{
		if((abs(value - heart_beat[heart_beatcnt-1]) >3) || (HAL_GetTick()>adder_time))
		{
			memset(heart_beat,0,NUM_SAMPLE);
			heart_beatcnt = 0;
			heart_beat[heart_beatcnt] = value;
			adder_time = HAL_GetTick() +1500;
			heart_beatcnt++;
			return;
		}
		else
		{
			if(heart_beatcnt == NUM_SAMPLE)
			{
				for(int i =0;i<NUM_SAMPLE-1;i++)
				{
					heart_beat[i] = heart_beat[i+1];
				}
				heart_beat[4] = value;
				heart_beatcnt = NUM_SAMPLE;
				uint16_t total =0;
				for(int i =0;i<NUM_SAMPLE;i++)
				{
					total+=heart_beat[i];
				}
				result_heartbeat = total/NUM_SAMPLE;
			}
			else
			{
				heart_beat[heart_beatcnt] = value;
				heart_beatcnt++;
			}
		}
	}
	adder_time = HAL_GetTick() +1500;

}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance == hadc1.Instance)
	{
		uint32_t total = 0;
		for (int i = 0;i<100;i++){
			total += BatteryADC[i];
		}
		total = total / 100;
		float voltage  = 3.3f;
		voltage = (voltage *total/4096);
//		printf("bat : %d\n",(int)(voltage *100));
		voltage=voltage*2.078f;
		Battery = (voltage - 3.7f)*100/(4.2f-3.7f);
		if(Battery > 100)
			Battery = 100;
	}
}
void init()
{
	HAL_Delay(100);
	printf("helloworld\r\n");
	HAL_NVIC_DisableIRQ(EXTI0_IRQn);
//	uint8_t total = 0;
//	for(uint8_t i =0;i<100;i++)
//	{
//		total+=HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
//		HAL_Delay(10);
//	}
//	if(total >20)
//	{
//		sleep_enable = 1;
//		enter_sleep_mode();
//	}
	SIM7600_TURN_ON();
//	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	pulseOxymeter = new MAX30100( DEFAULT_OPERATING_MODE, DEFAULT_SAMPLING_RATE, DEFAULT_LED_PULSE_WIDTH, DEFAULT_IR_LED_CURRENT, true, true );
	pulseOxymeter->resetFIFO();
	HAL_TIM_Base_Start_IT(&htim3);

}

SIM_STATE simstate = SIM_INIT;
char location[100] = {0};
uint32_t Baterry_check_time = 0;
uint32_t pulseDetected = 0;
uint32_t timepulseDetected = HAL_GetTick() +5000;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) // 20ms
{
	if(htim->Instance == htim3.Instance){
		  pulseoxymeter_t result = pulseOxymeter->update();
		  if( result.pulseDetected == true )
		  {
				pulseDetected++;
				printf("BEAT: %d\r\n",(int)result.heartBPM);
				if((int)result.heartBPM < 200)
				{

					add_heart_beat((int)result.heartBPM);
					if(result_heartbeat)
					{
						printf("OK: result_heartbeat: %d\r\n",result_heartbeat);
//						HAL_TIM_Base_Stop_IT(&htim3);
					}
				}
	      }
	}else if(htim->Instance == htim2.Instance){
#define MINUTES 1
		static int countinterrup = 0;
		if(countinterrup == (MINUTES+1)) // 1p
		{
			printf("weakup from timer : %dp",MINUTES);
			HAL_PWR_DisableSleepOnExit ();
		}
		countinterrup++;
	}

}
void loop()
{
	static int http_try = 0;
	if(HAL_GetTick() > Baterry_check_time)
	{
//		printf("battety check\r\n");
//		if(result_heartbeat == 0)
//		{
//			printf("reading heartbeat\r\n");
//		}
		HAL_ADC_Start_DMA(&hadc1, (uint32_t *)BatteryADC, 100);
		Baterry_check_time = HAL_GetTick() +2000;
	}
	if(HAL_GetTick() > timepulseDetected)
	{
		timepulseDetected = HAL_GetTick() +5000;
		if(!pulseDetected){
			  delete pulseOxymeter;
			  pulseOxymeter = new MAX30100( DEFAULT_OPERATING_MODE, DEFAULT_SAMPLING_RATE, DEFAULT_LED_PULSE_WIDTH, DEFAULT_IR_LED_CURRENT, true, true );
			  pulseOxymeter->resetFIFO();
		}
		pulseDetected=0;
	}
	switch (simstate) {
		case SIM_INIT:
			 if(At_Command((char*)"AT\r\n",(char*)"OK\r\n", 2000)==1){
				 Sim7600_init();
				 simstate = SIM_READ_GPS;
//				 simstate = SIM_REQUEST_HTTP;
			  }
			break;
		case SIM_READ_GPS:
			if(SIM_7600_read_GNSS(location))
			{
				http_try = 0;
				printf("OK: location: %s\r\n",location);
				simstate = SIM_REQUEST_HTTP;
			}else
			{
				HAL_Delay(5000);
				At_Command((char*)"AT\r\n",(char*)"OK\r\n", 2000);
			}
			break;
		case SIM_REQUEST_HTTP:
			{
				if(result_heartbeat == 0 && (HAL_GetTick() < 50000))
				{
					printf("wating heart beat \r\n");
					HAL_Delay(1000);
					break;
				}

				char request[200];
				if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))
				{
					if(result_heartbeat == 0)
					{
						snprintf(request,200,"token=00001&location=%s&heart_rate=%d&water_state=1&bat_cap=%d",location,result_heartbeat,Battery);

					}else
					{
						snprintf(request,200,"token=00001&location=%s&heart_rate=%d&water_state=2&bat_cap=%d",location,result_heartbeat,Battery);
					}
				}else
				{
					if(result_heartbeat <= 100)
					{
						snprintf(request,200,"token=00001&location=%s&heart_rate=%d&water_state=3&bat_cap=%d",location,result_heartbeat,Battery);

					}else
					{
						snprintf(request,200,"token=00001&location=%s&heart_rate=%d&water_state=4&bat_cap=%d",location,result_heartbeat,Battery);
					}
				}
				int res= AT_SIM7600_HTTP_Get(request,NULL,NULL);
				printf("HTTP STATUS CODE: %d\r\n",res);
				if((res== 200) || (http_try ==1)){
					if(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))
					{
						simstate = SIM_SEND_SMS;
					}
					else{
						simstate = SIM_DONE;
						sleep_enable = 1;
					}
				}
				http_try ++;
			}
			break;
		case SIM_SEND_SMS:
			{
				static int smscnt[2] ={0,0};
					char request[200];
					if((result_heartbeat < 100) && (smscnt[0] < 2)){  // nhijp tim <100
						sprintf(request,"EMERGENCY MyLocation: %s",location);
						AT_Sms_Send((char *)"+84961548396",request);
						smscnt[0] ++;
					}
					if((result_heartbeat > 100) && (smscnt[1] < 2)){
						sprintf(request,"high heart rate I need help urgently,My Location: %s",location);
						AT_Sms_Send((char *)"+84961548396",request);
						smscnt[1] ++;
					}

				simstate = SIM_READ_GPS;
			}
			break;
		default:
			break;
	}
//	if(HAL_GetTick()>50000 && (result_heartbeat == 0))
//	{
//		sleep_enable = 1;
//	}
	enter_sleep_mode();
}
extern "C"
{
    void initC()
    {
    	init();
    }
}
extern "C"
{
    void loopC()
    {
    	loop();
    }
}
