//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//�����豸
#include "esp8266.h"

//Э���ļ�
#include "onenet.h"
#include "mqttkit.h"

//�㷨
#include "base64.h"
#include "hmac_sha1.h"

//Ӳ������
//#include "usart1.h"
#include "delay.h"
#include "OLED_I2C.h"
#include "esp8266.h"

//C��
#include <string.h>
#include <stdio.h>
#include "main.h"

char devid[16];

char key[48];


extern unsigned char esp8266_buf[512];
extern u8 temperature,humidity;
uint8_t OneNet_state = 0;
extern int32_t hrAvg;//����
extern int32_t spo2Avg;//Ѫ��Ũ��
extern float longitude_sum, latitude_sum;
extern float Temperature;


/************************************************************
*	�������ƣ�	OTA_Authorization
*
*	�������ܣ�	����Authorization
*
*	��ڲ�����	ver��������汾�ţ����ڸ�ʽ��Ŀǰ��֧�ָ�ʽ"2018-10-31"
*				res����Ʒid
*				et������ʱ�䣬UTC��ֵ
*				access_key��������Կ
*				dev_name���豸��
*				authorization_buf������token��ָ��
*				authorization_buf_len������������(�ֽ�)
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		��ǰ��֧��sha1
************************************************************/

void OneNet_Init(void)
{
	int i;
	OLED_ShowStr(0,3,1,"Connect MQTTs Server");
	if(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);	
		NetWork_Rst();
	}
	OLED_ShowStr(110,6,2,"OK");
}
//==========================================================
//	�������ƣ�	ESP8266_Rst
//
//	�������ܣ�	��ʼ��ESP8266�����У�������ִ�����Ҫ�ú������и�λ
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================

void NetWork_Rst(void)
{	
	delay_ms(2000);
	OLED_CLS();//����
	OneNet_Init();  //����ִ�г�ʼ��
}
/************************************************************
*	�������ƣ�	OTA_UrlEncode
*
*	�������ܣ�	sign��Ҫ����URL����
*
*	��ڲ�����	sign�����ܽ��
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		+			%2B
*				�ո�		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	�������ƣ�	OTA_Authorization
*
*	�������ܣ�	����Authorization
*
*	��ڲ�����	ver��������汾�ţ����ڸ�ʽ��Ŀǰ��֧�ָ�ʽ"2018-10-31"
*				res����Ʒid
*				et������ʱ�䣬UTC��ֵ
*				access_key��������Կ
*				dev_name���豸��
*				authorization_buf������token��ָ��
*				authorization_buf_len������������(�ֽ�)
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		��ǰ��֧��sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//����ǩ����Base64������ �� URL������
	char hmac_sha1_buf[64];							//����ǩ��
	char access_key_base64[64];						//����access_key��Base64������
	char string_for_signature[72];					//����string_for_signature������Ǽ��ܵ�key

//----------------------------------------------------�����Ϸ���--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------��access_key����Base64����----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------����string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "version=%s%d\n%s\nproducts/%s\n%s", ver,et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "version=%s%d\n%s\nproducts/%s/devices/%s\n%s",ver,et, METHOD, res, dev_name, ver);
	//UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------����-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------�����ܽ������Base64����------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------��Base64����������URL����---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------����Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "&res=products%%2F%s&et=%d&method=%s&sign=%s",  res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s",  res, dev_name, et, METHOD, sign_buf);
	//UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

void OneNet_DevLink_Rst(void)
{	
	delay_ms(2000);
	OLED_CLS();//����
	OneNet_Link();  //����ִ�г�ʼ��
}

void OneNet_Link(void)
{
	int i;

	if(OneNet_DevLink())
	{
		for(i=0;i<2;i++)OLED_ShowCN(40+i*16,5,i+29,0);	
		OneNet_DevLink_Rst();
	}
}

//==========================================================
//	�������ƣ�	OneNet_DevLink
//
//	�������ܣ�	��onenet��������
//
//	��ڲ�����	��
//
//	���ز�����	1-�ɹ�	0-ʧ��
//
//	˵����		��onenetƽ̨��������
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//Э���

	unsigned char *dataPtr;
	char authorization_buf[160];
	
	_Bool status = 1;
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);
	snprintf(authorization_buf,sizeof(authorization_buf),
		"version=2018-10-31&res=products%%2FrPt76ojBtN%%2Fdevices%%2FWatch01&et=1956499200&method=sha1&sign=dbs%%2FFaTPkp%%2Fyweikgd6jUmjQgKo%%3D");
	
 			OLED_ShowStr(0,0,1,"OneNET_DevLink");
			OLED_ShowStr(0,1,1,"NAME:Watch01");	
			OLED_ShowStr(0,2,1,"PROID:rPt76ojBtN");	
//			delay_ms(1000);
//			OLED_CLS();//����
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ϴ�ƽ̨
		
		dataPtr = ESP8266_GetIPD(250);									//�ȴ�ƽ̨��Ӧ
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:OLED_ShowStr(0,4,1,"Connected Success"); status = 0;break;
					case 1:OLED_ShowStr(0,4,1,"Connected Fail:"); OLED_ShowStr(0,5,1,"Protocol error");break;  						//����ʧ�ܣ�Э�����
					case 2:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Illegal client ID");break;  				//����ʧ�ܣ��Ƿ���clientid
					case 3:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Server failure"); break;							//����ʧ�ܣ�������ʧ��);
					case 4:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Incorrect username or password");break;  //����ʧ�ܣ��û������������
					case 5:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Illegal link");break;	//����ʧ�ܣ��Ƿ�����(����token�Ƿ�)\r\n");
					
//					default:UsartPrintf(USART_DEBUG, "ERR:	����ʧ�ܣ�δ֪����\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//ɾ��
	}
	else
		
//		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}

unsigned char OneNet_FillBuf1(char *buf)
{
	
	char text[200]= " " ;
	
	memset(text, 0, sizeof(text));

	strcpy(buf, "{\"id\":123,\"dp\":{");
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Temperature\":[{\"v\":%1f}],", Temperature);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Heart_Rate\":[{\"v\":%d}],", hrAvg);	//����
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Blood_Oxygen\":[{\"v\":%d}],", spo2Avg);	  //����
	strcat(buf, text);
//	

	memset(text, 0, sizeof(text));
	sprintf(text, "\"Longitude\":[{\"v\":%5f}],", longitude_sum);	//����
	
	strcat(buf, text);	
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Latitude\":[{\"v\":%5f}]",latitude_sum);  //γ��
	strcat(buf, text);	
	
	strcat(buf, "}}");
	
	return strlen(buf);

}

unsigned char OneNet_FillBuf2(char *buf)
{
	
	char text[500]= " " ;
	
	memset(text, 0, sizeof(text));
//	sprintf(text,"{\"id\":123,\"params\":{\"Temperature\":{\"value\":%d,},\"Heart_Rate\":{\"value\":%d},\"Blood_Oxygen\":[{\"value\":%d}],\"Longitude\":{\"value\":%5f},\"Latitude\":{\"v\":%5f}}}"
//		,temperature/10,hrAvg,spo2Avg,longitude_sum,latitude_sum);
//	strcat(buf, text);
	strcpy(buf, "{\"id\":123,\"dp\":{");
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Temperature\":[{\"v\":%d}],", temperature/10);
	strcat(buf, text);
//	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "\"Humidity\":[{\"v\":%d}]",humidity);
//	strcat(buf, text);
	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "\"Heart_Rate\":[{\"v\":%d}]",hrAvg);  //����
//	strcat(buf, text);	                        
////	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Blood_Oxygen\":[{\"v\":%d}]",spo2Avg);  //Ѫ��
	strcat(buf, text);	

	memset(text, 0, sizeof(text));
	sprintf(text, "\"Longitude\":[{\"v\":%5f}]",longitude_sum);  //����
	strcat(buf, text);	
	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "\"Latitude\":[{\"v\":%5f}]",latitude_sum);  //γ��
//	strcat(buf, text);	
	
	strcat(buf, "}}");
	
	return strlen(buf);

}



//==========================================================
//	�������ƣ�	OneNet_SendData
//
//	�������ܣ�	�ϴ����ݵ�ƽ̨
//
//	��ڲ�����	type���������ݵĸ�ʽ
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//Э���
	
	char buf[500];
	
	short body_len = 0, i = 0;
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf1(buf);																	//��ȡ��ǰ��Ҫ���͵����������ܳ���
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��

		}
		else
		{
			i++;
		}
	}
//	body_len = OneNet_FillBuf2(buf);																	//��ȡ��ǰ��Ҫ���͵����������ܳ���
	
//	if(body_len)
//	{
//		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
//		{
//			for(; i < body_len; i++)
//				mqttPacket._data[mqttPacket._len++] = buf[i];
//			
//			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
//			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��

//		}
//		else
//		{
//			i++;
//		}
//	}
	
}

//==========================================================
//	�������ƣ�	OneNET_Publish
//
//	�������ܣ�	������Ϣ
//
//	��ڲ�����	topic������������
//				msg����Ϣ����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���
	
//	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������
		
		MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
	}

}

//==========================================================
//	�������ƣ�	OneNET_Subscribe
//
//	�������ܣ�	����
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/#", PROID, DEVICE_NAME);
	
//	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������
		
		MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
	}

}

//==========================================================
//	�������ƣ�	OneNet_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//���յ�Publish��Ϣ
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				
//				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//																	cmdid_topic, topic_len, req_payload, req_len);
				
				data_ptr = strstr(cmdid_topic, "request/");									//����cmdid
				if(data_ptr)
				{
					char topic_buf[80], cmdid[40];
					
					data_ptr = strchr(data_ptr, '/');
					data_ptr++;
					
					memcpy(cmdid, data_ptr, 36);											//����cmdid
					cmdid[36] = 0;
					
					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/response/%s",
															PROID, DEVICE_NAME, cmdid);
					OneNET_Publish(topic_buf, "ojbk");										//�ظ�����
				}
			}
			
		case MQTT_PKT_PUBACK:														//����Publish��Ϣ��ƽ̨�ظ���Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
							OLED_ShowStr(0,5,1,"Mqtt Publish Send OK");
//				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		case MQTT_PKT_SUBACK:																//����Subscribe��Ϣ��Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
			{
									topic_len++;
			OLED_ShowStr(0,5,1,"Mqtt Subscribe OK");		
			}

			else
			{
				topic_len++;
				OLED_ShowStr(0,5,1,"Mqtt Subscribe err");			
			}

		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//��ջ���
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, ':');					//����':'

	if(dataPtr != NULL)					//����ҵ���
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//�ж��Ƿ����·��������������
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//תΪ��ֵ��ʽ
		if(strstr((char *)req_payload, "redled"))		//����"redled"
		{
			if(num == 1)								//�����������Ϊ1������
			{
//				UsartPrintf(USART_DEBUG, "GOOOFFFF\r\n");
			}
			else if(num == 0)							//�����������Ϊ0�������
			{
//				UsartPrintf(USART_DEBUG, "Tips:	ASDASDAD\r\n");
			}
		}
														//��ͬ

	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}


void OneNet_Config(void)
{
	OneNet_Init();
	delay_ms(1000);
	OLED_CLS();//����		
///	
	OneNet_Link();		//����OneNET
	delay_ms(1000);
		OneNET_Subscribe();
	OLED_CLS();//����
	OneNet_state = 1;

}
