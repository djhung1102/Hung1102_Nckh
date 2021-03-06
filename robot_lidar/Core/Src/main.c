#include "main.h"
#include "stdio.h"
#include <stdint.h>
#include <math.h> 
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>


TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim9;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM9_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
void delay_us(uint16_t us);

#ifdef __GNUC__
     #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
     #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart2,(uint8_t *)&ch,1,0xFFFF);  // USB PC
	return ch;
}

cJSON *str_json, *str_Den1 , *str_X1  , *str_X2;

uint8_t  rx_index1, rx_data1;
char rx_buffer1[100];

uint8_t  rx_index2, rx_data2;
char rx_buffer2[500];

char ResponseRX[500];

uint32_t  rx_indexResponse;

uint8_t ErrorCode = 0;

int ConfigAT = 0;
long last = 0;

uint8_t CheckConnect = 1;

unsigned int NhietDo = 0;
unsigned int Den1 = 0;
unsigned int X1 = 0;
unsigned int X2 = 0;
unsigned int Tong = 0;
unsigned int distance;
unsigned int distance1;
unsigned int distance2;

const float speedOfSound = 0.0343/2;
uint32_t time = 0;

float Ki;
float Kp;
float Kd;
float sp=600;
float Err_now,Err_sum,Err_last,Err_del;
float u=0;
volatile int encoder_cnt=0,cnt_pre=0,rase=0, x;


char *mqtt_server = "ngoinhaiot.com";
char *mqtt_port = "1111";
char *mqtt_user = "papyboi123";
char *mqtt_pass = "FA3436ECEFC84694";
char *mqtt_sub = "papyboi123/B"; // nhan du lieu
char *mqtt_pub = "papyboi123/A"; // gui du lieu


void SettingESP(void);
void dieukhienmotor(void);
void clearbuffer_UART_ESP(void);		// doc bo dem UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);		// nhan du lieu UART
void Send_AT_Commands_Setting(char *AT_Commands, char *DataResponse, uint32_t timesend , uint32_t setting);			// setting tap lenh AT bang STM cho ESP
void Received_AT_Commands_ESP(void);		// ham nhan du lieu cua setting ESP
void Received_AT_Commands_ESP_MessagerMQTT(void);		// ham doc du lieu mqtt
void clearResponse(void);		//xoa du lieu 
void ConnectMQTT(char *server , char *port , char *user , char *pass , char *sub , char *pub);  // ket noi MQTT
void Send_AT_Commands_ConnectMQTT(char *AT_Commands, char *DataResponse , uint32_t timeout , uint32_t setting , uint32_t count);
void Send_AT_Commands_SendMessager(char *AT_Commands, char *DataResponse , uint32_t timeout , uint32_t setting , uint32_t count);
void chuongtrinhcambien(void);	
void chuongtrinhcambien1(void);	
void chuongtrinhcambien2(void);	
void SendData(char *pub , unsigned int distance , unsigned int rase);
void ParseJson(char *DataMQTT);
void Send_AT_Commands_CheckConnectMQTT(char *AT_Commands, char *DataResponse , uint32_t timeout , uint32_t setting , uint32_t count);
void SendMQTT(void);		// gui du lieu len server 1s 1 lan
void PID_setup(void)
{
Ki=0.0001;
Kp=0.0601;
Kd=0.0000;
 
 Err_now = sp - rase ;
 Err_sum = Err_now + Err_last ;
 Err_del = Err_now - Err_last ;
 Err_last = Err_now ;

 u = u+ Kp*Err_now + Ki*Err_sum + Kd*Err_del ;    // do rong xung 
 if ( u > 199 ) {

 u = 199;}
 else if ( u< 0 ) u= 0 ;
}

int main(void)
{

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_TIM4_Init();
	MX_TIM6_Init();
  MX_TIM9_Init();
	MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
	HAL_TIM_Base_Start(&htim9);
	
	printf("Setup UART\r\n");
	HAL_UART_Receive_IT(&huart2, &rx_data1, 1);
	HAL_UART_Receive_IT(&huart6, &rx_data2, 1);
	HAL_UART_Receive_IT(&huart1, &rx_data2, 1);
	
	HAL_Delay(1000);
	printf("Setting ESP8266\r\n");
	
	SettingESP();	
	HAL_Delay(1000);
	
	printf("Setup OK\r\n");
	printf("========================================================\r\n");
	
	ConnectMQTT(mqtt_server, mqtt_port, mqtt_user, mqtt_pass, mqtt_sub, mqtt_pub);
	
	printf("========================================================\r\n");
	
	last = HAL_GetTick();
	
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1|TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&htim6);
	
	unsigned char first_msg[] = "\r\n--- Raspberry / STM32 Communication ---\r\n";
  HAL_UART_Transmit(&huart2, first_msg, sizeof(first_msg), 1000);
	//left
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	//right
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);

  while (1)
  {
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) 
			{
				HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
				do {
    		HAL_Delay(200);
    	} while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
				}
		dieukhienmotor();
		chuongtrinhcambien();	
		chuongtrinhcambien1();	
		chuongtrinhcambien2();	
		SendMQTT();
		if(distance>0)
		{
			if(distance<=45 && distance1<=45 && distance2<=45)
			{
					//left
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
					//right
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
			}
			else if(distance<=45 && distance1>45 && distance2>45)
			{
					//left
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
					//right
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
			}
			else if(distance<=45 && distance1>distance2)
			{
					//left
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
					//right
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
			}
			else if(distance<=45 && distance1>=45 && distance2>45)
			{
					//left
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
					//right
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
			}
		}
  }

}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	unsigned char init[] = "TR";
	unsigned char start[] = "AA";
	unsigned char end[] = "ZZ";
	int data = 0x555555;

	uint8_t bytesDataLen = sizeof(data);
	uint8_t filter = 0xFF;
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
	HAL_UART_Transmit(&huart1, init, sizeof(init), 100);
	// Start Transmission
	HAL_UART_Transmit(&huart1, start, sizeof(start), 1000);
	/*for (int i = (bytesDataLen - 1) ; i >= 0 ; i--) {
		char buffer[8];
		data = data & filter;
		sprintf(buffer, "%04d\n", data);
		HAL_UART_Transmit(&huart2, (uint8_t*)buffer, sizeof(buffer), 1000);
		filter = filter >> 8;
	}*/

	data &= filter;
	char buffer[10];
	sprintf(buffer, "%04d", data);
	HAL_UART_Transmit(&huart1, (uint8_t*)buffer, 15, 100);

	// End Transmission
	HAL_UART_Transmit(&huart1, end, sizeof(end), 1000);
	HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 1000);

}

void SendMQTT(void)
{
	if(HAL_GetTick() - last >= 1000)
	{
		if(CheckConnect == 1)
		{
			SendData(mqtt_pub, distance, rase);
		}
		last = HAL_GetTick();
	}
}

void chuongtrinhcambien(void)
{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		delay_us(3);
		
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
		delay_us(10);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_RESET);
		
		time = 0;
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET)
		{
			time++;
			delay_us(2);
		};
		distance = (time + 0.0f)*2.8*speedOfSound;
		HAL_Delay(1000);
}

void chuongtrinhcambien1(void)
{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		delay_us(3);
		
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		delay_us(10);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET);
		
		time = 0;
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_SET)
		{
			time++;
			delay_us(2);
		};
		distance1 = (time + 0.0f)*2.8*speedOfSound;
		HAL_Delay(1000);
}
void chuongtrinhcambien2(void)
{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
		delay_us(3);
		
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
		delay_us(10);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_13) == GPIO_PIN_RESET);
		
		time = 0;
		
		while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_13) == GPIO_PIN_SET)
		{
			time++;
			delay_us(2);
		};
		distance2 = (time + 0.0f)*2.8*speedOfSound;
		HAL_Delay(1000);
}

void dieukhienmotor(void)
{
		sp=600;
		HAL_Delay(100);
		encoder_cnt = __HAL_TIM_GET_COUNTER(&htim2);
		rase=((encoder_cnt-cnt_pre)*60)/44;

		PID_setup();
		cnt_pre=encoder_cnt;
		__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_1,u);
}

// setting tap lenh AT cho ESP bang code STM
void Send_AT_Commands_Setting(char *AT_Commands, char *DataResponse, uint32_t timesend , uint32_t setting)
{
	last = HAL_GetTick();
	ConfigAT = setting;
	char SendDataAT[50];
	for(int i=0; i<=50; i++)
	{
		SendDataAT[i] = 0;
	}
	// dua data lenh AT vao mang SendDataAT[]
	snprintf(SendDataAT, sizeof(SendDataAT), "%s\r\n", AT_Commands);
	// send ESP qua UART6
	HAL_UART_Transmit(&huart6, (uint8_t *)&SendDataAT, strlen(SendDataAT), 1000);
	printf("Send AT Command Setting: %s\r\n", SendDataAT);	
	// doi phan hoi tu ESP, neu doi lau qua thi goi lenh tiep ( phan hoi tu ham ngat UART6)
	last = HAL_GetTick();
	while(1)
	{
		// qua 5s thi gui lai lenh cu den khi nao phan hoi OK thi thoi
		if(HAL_GetTick() - last >= timesend)
		{
			HAL_UART_Transmit(&huart6, (uint8_t *)&SendDataAT, strlen(SendDataAT), 1000);
			printf("Send AT Commands Setting TimeSend: %s\r\n", SendDataAT);
			last = HAL_GetTick();
		}
		if(strstr(rx_buffer2,DataResponse) != NULL)
		{
			//printf("Data Buffer2: %s\r\n",rx_buffer2);
			printf("Reponse Setting: %s\r\n",DataResponse);
			clearbuffer_UART_ESP();
			break;		
		}
	}
}

void SettingESP(void)
{
	Send_AT_Commands_Setting("AT+RST\r\n", "OK", 10000, 0);
	HAL_Delay(3000);
	
	Send_AT_Commands_Setting("AT\r\n", "OK", 2000, 0);
	HAL_Delay(3000);
	
	Send_AT_Commands_Setting("ATE0\r\n", "OK", 2000, 0);
	HAL_Delay(3000);
	
	Send_AT_Commands_Setting("AT+CWMODE=1,1\r\n", "OK", 2000, 0);
	HAL_Delay(3000);
	
	Send_AT_Commands_Setting("AT+CWJAP=\"Manh Hung\",\"11022000\"\r\n", "WIFI CONNECTED", 10000, 0);
	HAL_Delay(3000);
	
	Send_AT_Commands_Setting("AT+CIPMUX=0\r\n", "OK", 2000 , 0);
	HAL_Delay(3000);
	
	ErrorCode = 0;
}

void clearbuffer_UART_ESP(void)			// doc bo dem UART
{
	for(int i = 0 ; i < 500 ; i++)
	{
		rx_buffer2[i] = 0;
	}
	rx_index2 = 0;	
}

void clearResponse(void)			// xoa du lieu 
{
	for(int i = 0 ; i < 500; i++)
	{
		ResponseRX[i] = 0;
	}
	rx_indexResponse = 0;
}

void Send_AT_Commands_ConnectMQTT(char *AT_Commands, char *DataResponse , uint32_t timeout , uint32_t setting , uint32_t count)
{
	clearbuffer_UART_ESP();
	last = HAL_GetTick();
	uint32_t Size = 300;
	uint32_t Count = 0;
	ConfigAT = setting;
	char DataHTTP[Size];
	for(int i = 0 ; i < Size; i++)
	{
		DataHTTP[i] = 0;
	}
	// dua data lenh AT_Commands vao mang  SendHTTP
	snprintf(DataHTTP, sizeof(DataHTTP),"%s", AT_Commands);
	HAL_UART_Transmit(&huart6,(uint8_t *)&DataHTTP,strlen(DataHTTP),1000);
	printf("Send AT-Commands Data: %s\r\n", DataHTTP);
	last = HAL_GetTick();
	while(1)
	{
		//chay ham ngat uart
		if(HAL_GetTick() - last >= timeout)
		{
			Count++;
			HAL_UART_Transmit(&huart6,(uint8_t *)&DataHTTP,strlen(DataHTTP),1000);
			printf("Send AT-Commands Data TimeOut: %s\r\n", DataHTTP);
			last = HAL_GetTick();
		}
		if(strstr(rx_buffer2,DataResponse) != NULL)
		{
			//printf("Reponse DataBlynk: %s\r\n",DataResponse);
			printf("MQTT Connect OK\r\n");
			clearbuffer_UART_ESP();
			ErrorCode = 1;
			CheckConnect = 1;
			last = HAL_GetTick();
			
			break;
		}
		if(Count >= count)
		{
			printf("MQTT Connect ERROR\r\n"); // gui lenh setting lai
			ErrorCode = 0;
			CheckConnect = 0;
			clearbuffer_UART_ESP();
			last = HAL_GetTick();
			break;
		}
	}
}

void ConnectMQTT(char *server , char *port , char *user , char *pass , char *sub , char *pub)   // ket noi server MQTT
{
	uint32_t id = 0;
	id = rand()%100;
	char clientid[100];
	char MathRandom[100];
	char MQTTUSERCFG[100];
	char MQTTCONN[100];
	char MQTTSUB[100];
	
	for(int i = 0 ; i < 100; i++)
	{
		clientid[i] = 0;
		MathRandom[i] = 0;
		MQTTUSERCFG[i] = 0;
		MQTTCONN[i] = 0;
		MQTTSUB[i] = 0;
	}
	sprintf(MathRandom, "%d", id);
	
	strcat(clientid, "ESP"); 
	strcat (clientid, MathRandom);

	//AT+MQTTUSERCFG=0,1,"ESP8266","toannv10291","toannv10291",0,0,""$0D$0A => OK
	strcat(MQTTUSERCFG, "AT+MQTTUSERCFG=0,1,\"");
	strcat(MQTTUSERCFG,clientid);
	strcat(MQTTUSERCFG,"\",\"");
	strcat(MQTTUSERCFG,user);
	strcat(MQTTUSERCFG,"\",\"");
	strcat(MQTTUSERCFG,pass);
	strcat(MQTTUSERCFG,"\",0,0,");
	strcat(MQTTUSERCFG,"\"\"");
	strcat(MQTTUSERCFG,"\r\n");
	//printf("MQTTUSERCFG: %s",MQTTUSERCFG);
	
	//AT+MQTTCONN=0,"mqtt.ngoinhaiot.com",1111,1$0D$0A
	strcat(MQTTCONN, "AT+MQTTCONN=0,\"");
	strcat(MQTTCONN, server);
	strcat(MQTTCONN, "\",");
	strcat(MQTTCONN, port);
	strcat(MQTTCONN, ",1\r\n");
	//printf("MQTTCONN: %s",MQTTCONN);
	
	//AT+MQTTSUB=0,"toannv10291/quat",0$0D$0A => OK
	strcat(MQTTSUB, "AT+MQTTSUB=0,\"");
	strcat(MQTTSUB, sub);
	strcat(MQTTSUB, "\",0\r\n");
	//printf("MQTTSUB: %s",MQTTSUB);
	
	Send_AT_Commands_ConnectMQTT(MQTTUSERCFG, "OK" , 5000 , 0 , 5);
	HAL_Delay(1000);
	clearbuffer_UART_ESP();
	
	Send_AT_Commands_ConnectMQTT(MQTTCONN, "+MQTTCONNECTED" , 5000 , 0 , 5);
	HAL_Delay(1000);
	clearbuffer_UART_ESP();
	
	
	Send_AT_Commands_ConnectMQTT(MQTTSUB, "OK" , 5000 , 0 , 5);
	HAL_Delay(1000);
	clearbuffer_UART_ESP();
	
	ConfigAT = 1;
	ErrorCode = 1;
}

void Send_AT_Commands_SendMessager(char *AT_Commands, char *DataResponse , uint32_t timeout , uint32_t setting , uint32_t count)
{
	clearbuffer_UART_ESP();
	last = HAL_GetTick();
	uint32_t Size = 300;
	uint32_t Count = 0;
	ConfigAT = setting;
	char DataHTTP[Size];
	for(int i = 0 ; i < Size; i++)
	{
		DataHTTP[i] = 0;
	}
	// dua data lenh AT_Commands vao mang  SendHTTP
	snprintf(DataHTTP, sizeof(DataHTTP),"%s", AT_Commands);
	HAL_UART_Transmit(&huart6,(uint8_t *)&DataHTTP,strlen(DataHTTP),1000);
	printf("Send AT-Commands Data: %s\r\n", DataHTTP);
	last = HAL_GetTick();
	while(1)
	{
		//chay ham ngat uart
		if(HAL_GetTick() - last >= timeout)
		{
			Count++;
			HAL_UART_Transmit(&huart6,(uint8_t *)&DataHTTP,strlen(DataHTTP),1000);
			printf("Send AT-Commands Send Data MQTT: %s\r\n", DataHTTP);
			last = HAL_GetTick();
		}
		if(strstr(rx_buffer2,DataResponse) != NULL)
		{
			//printf("Reponse DataBlynk: %s\r\n",DataResponse);
			printf("SEND MQTT OK\r\n");
			clearbuffer_UART_ESP();
			ErrorCode = 0;
			CheckConnect = 1;
			last = HAL_GetTick();
			break;
		}
		if(Count >= count)
		{
			printf("SEND MQTT ERROR\r\n"); // gui lenh setting lai
			ErrorCode = 1;
			clearbuffer_UART_ESP();
			last = HAL_GetTick();
			break;
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)		// nhan du lieu UART
{
	// che do setting ConfigAT = 0
	if(ConfigAT == 0)
	{
		if(huart -> Instance == USART6)
		{
			Received_AT_Commands_ESP();
			HAL_UART_Receive_IT(&huart6,&rx_data2,1);
		}
	}
	else if(ConfigAT == 1)
	{
		if(huart -> Instance == USART6)
		{
			Received_AT_Commands_ESP_MessagerMQTT();
			HAL_UART_Receive_IT(&huart6,&rx_data2,1);
		}
	}
}
void Received_AT_Commands_ESP(void)				// nhan du lieu setting cua ESP
{
	rx_buffer2[rx_index2++] = rx_data2;
}

void Received_AT_Commands_ESP_MessagerMQTT(void)		// nhan du lieu cua MQTT
{
	
		if(rx_data2 != '\n')
		{
			ResponseRX[rx_indexResponse++] = rx_data2;
		}
		else
		{
			ResponseRX[rx_indexResponse++] = rx_data2;
			rx_indexResponse = 0;	
			printf("Data MQTT:%s\r\n",ResponseRX);	
			
			//CheckConnect
			if(strstr(ResponseRX,"MQTTCONNECTED") != NULL) 
			{
				printf("Connect MQTT\r\n");	
				CheckConnect = 1;
				last = HAL_GetTick();
			}
			else if(strstr(ResponseRX,"MQTTDISCONNECTED") != NULL)
			{
				printf("Not Connect MQTT\r\n");
				CheckConnect = 0;
				last = HAL_GetTick();
			}
			else if(strstr(ResponseRX,"+MQTTSUBRECV") != NULL)
			{
				char *DataMQTT;
			//+MQTTSUBRECV:0,"toannv10291/quat",9,{"A":"1"}\n
				DataMQTT = strtok(ResponseRX,",");
				DataMQTT = strtok(NULL,",");
				DataMQTT = strtok(NULL,",");
				DataMQTT = strtok(NULL,"\n");		
				printf("DATA MQTT: %s\r\n",DataMQTT);		
				
				ParseJson(DataMQTT);
				
				last = HAL_GetTick();
			}		
			last = HAL_GetTick();
			clearResponse();				
		}
}

void SendData(char *pub , unsigned int distance , unsigned int rase)
{
	//AT+MQTTPUBRAW=0,"toannv10291/maylanh",5,0,1$0D$0A
	/*
	tra ve 
	OK\r\n\r\n>
	=> send data => ABCD$0D$0A => tra ve +MQTTPUB:OK
	*/
	
	char MQTTPUBRAW[100];
	char JSON[100];
	char Str_distance[100];
	char Str_rase[100];
	char Length[100];

	
	for(int i = 0 ; i < 100; i++)
	{
		MQTTPUBRAW[i] = 0;
		JSON[i] = 0;
		Str_distance[i] = 0;
		Str_rase[i] = 0;
		Length[i] = 0;
	}
	sprintf(Str_distance, "%d", distance);
	sprintf(Str_rase, "%d", NhietDo);
	
	strcat(JSON,"{\"KC\":\"");
	strcat(JSON,Str_distance);
	strcat(JSON,"\",");
	
	strcat(JSON,"\"rase\":\"");
	strcat(JSON,Str_rase);
	strcat(JSON,"\r\n");
	
	printf("DataJson: %s\n", JSON);
	
	int len = 0;
	len = strlen(JSON);
	sprintf(Length, "%d", len);
	
	////AT+MQTTPUBRAW=0,"toannv10291/maylanh",5,0,1$0D$0A
	strcat(MQTTPUBRAW,"AT+MQTTPUBRAW=0,\"");
	strcat(MQTTPUBRAW,pub);
	strcat(MQTTPUBRAW,"\",");
	strcat(MQTTPUBRAW,Length);
	strcat(MQTTPUBRAW,",0,1\r\n");
	printf("MQTTPUBRAW: %s\n",MQTTPUBRAW);
	
	//{"KC":"","ND":"","Den1":"","X1":"","X2":"","Tong":""}

  // cho nay check connect mqtt nua
	Send_AT_Commands_SendMessager(MQTTPUBRAW, "OK\r\n\r\n>" , 5000 , 0 , 3);
	
	clearbuffer_UART_ESP();
	
	if(ErrorCode == 0)
	{
		Send_AT_Commands_SendMessager(JSON, "+MQTTPUB:OK" , 5000 , 0 , 5);
		clearbuffer_UART_ESP();
	}

	ConfigAT = 1;	
}

void ParseJson(char *DataMQTT)
{
	str_json = cJSON_Parse(DataMQTT);
	if (!str_json)
  {
			printf("JSON error\r\n"); 
			return;
  }
	else
	{
		printf("JSON OKE\r\n"); 

/*		str_Den1 = cJSON_GetObjectItem(str_json, "Den1"); // kiem tra ten Den1 co trong tap du lieu JSON k va g?n cho bien srt_Den1

				
				if(strstr(str_Den1->valuestring,"0") != NULL)
				{
					printf("OFF 1\r\n");
					Den1 = 0;
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_RESET);
				}
				else if(strstr(str_Den1->valuestring,"1") != NULL)
				{
					printf("ON 1\r\n");
					Den1 = 1;
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_SET);
				}
   //   }

			
			str_X1 = cJSON_GetObjectItem(str_json, "X1"); //Get information about the value corresponding to the name key
     
			if (str_X1->type == cJSON_String)
      {
				printf("X1:%s \r\n", str_X1->valuestring);
				X1 = atoi(str_X1->valuestring);
			
      }
			
			str_X2 = cJSON_GetObjectItem(str_json, "X2"); //Get information about the value corresponding to the name key
      if (str_X2->type == cJSON_String)
      {
				printf("X2:%s \r\n", str_X2->valuestring);
				X2 = atoi(str_X2->valuestring);
			
      }
		
		cJSON_Delete(str_json);     
		*/
	}
	
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 50;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 199;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 100;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 100-1;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 0xffff-1;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 100;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 64000 - 1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE9 PE10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}
void delay_us(uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim9, 0);
	while(__HAL_TIM_GET_COUNTER(&htim9) < us);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
