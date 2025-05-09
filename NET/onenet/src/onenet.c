//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "esp8266.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//算法
#include "base64.h"
#include "hmac_sha1.h"

//硬件驱动
//#include "usart1.h"
#include "delay.h"
#include "OLED_I2C.h"
#include "esp8266.h"

//C库
#include <string.h>
#include <stdio.h>
#include "main.h"

char devid[16];

char key[48];


extern unsigned char esp8266_buf[512];
extern u8 temperature,humidity;
uint8_t OneNet_state = 0;
extern int32_t hrAvg;//心率
extern int32_t spo2Avg;//血氧浓度
extern float longitude_sum, latitude_sum;
extern float Temperature;


/************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
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
//	函数名称：	ESP8266_Rst
//
//	函数功能：	初始化ESP8266过程中，如果出现错误，需要该函数进行复位
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================

void NetWork_Rst(void)
{	
	delay_ms(2000);
	OLED_CLS();//清屏
	OneNet_Init();  //重新执行初始化
}
/************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
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
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[64];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key

//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "version=%s%d\n%s\nproducts/%s\n%s", ver,et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "version=%s%d\n%s\nproducts/%s/devices/%s\n%s",ver,et, METHOD, res, dev_name, ver);
	//UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
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
	OLED_CLS();//清屏
	OneNet_Link();  //重新执行初始化
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
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

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
//			OLED_CLS();//清屏
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:OLED_ShowStr(0,4,1,"Connected Success"); status = 0;break;
					case 1:OLED_ShowStr(0,4,1,"Connected Fail:"); OLED_ShowStr(0,5,1,"Protocol error");break;  						//连接失败，协议错误
					case 2:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Illegal client ID");break;  				//连接失败，非法的clientid
					case 3:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Server failure"); break;							//连接失败：服务器失败);
					case 4:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Incorrect username or password");break;  //连接失败：用户名或密码错误
					case 5:OLED_ShowStr(0,4,1,"Connected Fail:");	OLED_ShowStr(0,5,1,"Illegal link");break;	//连接失败：非法链接(比如token非法)\r\n");
					
//					default:UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
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
	sprintf(text, "\"Heart_Rate\":[{\"v\":%d}],", hrAvg);	//心率
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Blood_Oxygen\":[{\"v\":%d}],", spo2Avg);	  //心率
	strcat(buf, text);
//	

	memset(text, 0, sizeof(text));
	sprintf(text, "\"Longitude\":[{\"v\":%5f}],", longitude_sum);	//经度
	
	strcat(buf, text);	
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Latitude\":[{\"v\":%5f}]",latitude_sum);  //纬度
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
//	sprintf(text, "\"Heart_Rate\":[{\"v\":%d}]",hrAvg);  //心率
//	strcat(buf, text);	                        
////	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Blood_Oxygen\":[{\"v\":%d}]",spo2Avg);  //血氧
	strcat(buf, text);	

	memset(text, 0, sizeof(text));
	sprintf(text, "\"Longitude\":[{\"v\":%5f}]",longitude_sum);  //经度
	strcat(buf, text);	
	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "\"Latitude\":[{\"v\":%5f}]",latitude_sum);  //纬度
//	strcat(buf, text);	
	
	strcat(buf, "}}");
	
	return strlen(buf);

}



//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
	
	char buf[500];
	
	short body_len = 0, i = 0;
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf1(buf);																	//获取当前需要发送的数据流的总长度
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
			MQTT_DeleteBuffer(&mqttPacket);															//删包

		}
		else
		{
			i++;
		}
	}
//	body_len = OneNet_FillBuf2(buf);																	//获取当前需要发送的数据流的总长度
	
//	if(body_len)
//	{
//		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
//		{
//			for(; i < body_len; i++)
//				mqttPacket._data[mqttPacket._len++] = buf[i];
//			
//			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
//			MQTT_DeleteBuffer(&mqttPacket);															//删包

//		}
//		else
//		{
//			i++;
//		}
//	}
	
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
//	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/#", PROID, DEVICE_NAME);
	
//	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
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
		case MQTT_PKT_PUBLISH:																//接收的Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				
//				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//																	cmdid_topic, topic_len, req_payload, req_len);
				
				data_ptr = strstr(cmdid_topic, "request/");									//查找cmdid
				if(data_ptr)
				{
					char topic_buf[80], cmdid[40];
					
					data_ptr = strchr(data_ptr, '/');
					data_ptr++;
					
					memcpy(cmdid, data_ptr, 36);											//复制cmdid
					cmdid[36] = 0;
					
					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/response/%s",
															PROID, DEVICE_NAME, cmdid);
					OneNET_Publish(topic_buf, "ojbk");										//回复命令
				}
			}
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
							OLED_ShowStr(0,5,1,"Mqtt Publish Send OK");
//				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		case MQTT_PKT_SUBACK:																//发送Subscribe消息的Ack
		
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
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, ':');					//搜索':'

	if(dataPtr != NULL)					//如果找到了
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//判断是否是下发的命令控制数据
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//转为数值形式
		if(strstr((char *)req_payload, "redled"))		//搜索"redled"
		{
			if(num == 1)								//控制数据如果为1，代表开
			{
//				UsartPrintf(USART_DEBUG, "GOOOFFFF\r\n");
			}
			else if(num == 0)							//控制数据如果为0，代表关
			{
//				UsartPrintf(USART_DEBUG, "Tips:	ASDASDAD\r\n");
			}
		}
														//下同

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
	OLED_CLS();//清屏		
///	
	OneNet_Link();		//接入OneNET
	delay_ms(1000);
		OneNET_Subscribe();
	OLED_CLS();//清屏
	OneNet_state = 1;

}
