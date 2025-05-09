//单片机头文件
#include "stm32f10x.h"

//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart2.h"
#include "OLED_I2C.h"
#include "main.h"


//C库
#include <string.h>
#include <stdio.h>

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
uint8_t Wifi_state = 0;

/*-------------------------------------------------*/
/*函数名：WiFi复位                                 */
/*参  数：timeout：超时时间（100ms的倍数）         */
/*返回值：0：正确   其他：错误                     */
/*-------------------------------------------------*/
//char WiFi_Reset(int timeout)
//{
//	RESET_IO(0);                                    //复位IO拉低电平
//	delay_ms(500);                                  //延时500ms
//	RESET_IO(1);                                    //复位IO拉高电平	
//	while(timeout--){                               //等待超时时间到0
//		delay_ms(100);                              //延时50ms
//		if(ESP8266_WaitRecive() == REV_OK)             //如果接收到ready表示复位成功
//			break;       						    //主动跳出while循环
//	}
//	if(timeout<=0)return 1;                         //如果timeout<=0，说明超时时间到了，也没能收到ready，返回1
//	else return 0;		         				    //反之，表示正确，说明收到ready，通过break主动跳出while
//}


/*
函数名：WiFi复位                                 
参  数：空        
返回值：空

*/
void Wifi_Init(void)
{
	Usart2_Init(115200);
	ESP8266_GPIO_Init();
	ESP8266_Init();					//初始化ESP8266
	delay_ms(1000);
	Wifi_state = 1;
	OLED_CLS();//清屏
		
}



//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res,short timeout)
{
	uint8_t flag =0; 
	int i;
	Usart_SendString(cmd);		//USART2发送指令
	while(timeout--)					//判断是否超时
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存		
				break;   //主动跳出while循环
			}
		}
		if(flag == 0)		//动态OLED 打印进度条
		{
			for(i=0;i<15;i++){OLED_ShowChar((i*8),7,'.',1,0);delay_ms(10);}
			flag = 1;
		}
		else
		{
			for(i=0;i<15;i++){OLED_ShowChar((i*8),7,' ',1,0);}
			flag = 0;		
			delay_ms(10);			
		}
	}
	if(timeout<=0)return 1;                         //如果timeout<=0，说明超时时间到了，也没能收到ready，返回1
	else return 0;		         				    //反之，表示正确，说明收到相应字符，通过break主动跳出while
}

/*-------------------------------------------------*/
/*函数名：WiFi加入路由器指令                       */
/*参  数：timeout：超时时间（1s的倍数）            */
/*返回值：0：正确   其他：错误                     */
/*-------------------------------------------------*/
//char WiFi_JoinAP(int timeout)
//{		
//	uint8_t flag =0; 
//	int i;
//	Usart_SendString("AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PASS);	
//	while(timeout--){                               //等待超时时间到0
//		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
//		{
//			if(strstr((const char *)esp8266_buf, "GOT IP") != NULL)		//如果检索到关键词
//			{
//				ESP8266_Clear();									//清空缓存		
//				break;   //主动跳出while循环
//			}
//		}
//		if(flag == 0)
//		{
//			for(i=0;i<15;i++){OLED_ShowChar((i*8),7,'.',1,0);delay_ms(10);}
//			flag = 1;
//		}
//		else
//		{
//			for(i=0;i<15;i++){OLED_ShowChar((i*8),7,' ',1,0);}
//			flag = 0;		
//			delay_ms(10);			
//		}                  //串口输出现在的超时时间
//	}
//	if(timeout<=0)return 1;                         //如果timeout<=0，说明超时时间到了，也没能收到WIFI GOT IP，返回1
//	return 0;                                       //正确，返回0
//}


//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{
	char cmdBuf[32];
	ESP8266_Clear();														//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		// 格式化字符串并存储到字符数组
	if(!ESP8266_SendCmd(cmdBuf, ">",100))				//收到‘>’时可以发送数据
		Wifi_printf( data,len);										//发送数据
}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		delay_ms(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

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
void ESP8266_Rst(void)
{	
	delay_ms(2000);
	OLED_CLS();//清屏
	ESP8266_Init();  //重新执行初始化
}


//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//ESP8266复位引脚
	GPIO_Initure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Initure.GPIO_Pin = GPIO_Pin_4;					//GPIOA4-复位
	GPIO_Initure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Initure);
	
		//ESP8266使能引脚
	GPIO_Initure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Initure.GPIO_Pin = GPIO_Pin_5;					//GPIOA5-使能
	GPIO_Initure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Initure);
	
	GPIO_SetBits(GPIOA,GPIO_Pin_5);//使能模块

}



void ESP8266_Init(void)
{
	int i;
	/*第一步，模块复位*/
	OLED_ShowStr(0,0,1,"1.RESET");    
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);
	delay_ms(250);
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
	delay_ms(100);
	OLED_ShowStr(110,0,1,"OK");		
	
	ESP8266_Clear();
	
	OLED_ShowStr(0,1,1,"2.AT");    //第一步 
	if(ESP8266_SendCmd("AT\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();
	}
	else
	OLED_ShowStr(110,1,1,"OK");			
	delay_ms(500);
	
	OLED_ShowStr(0,2,1,"3.AT+CWMODE=1");  // 第二步
	if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();	
	}
	OLED_ShowStr(110,2,1,"OK");			
	delay_ms(500);
	
	OLED_ShowStr(0,3,1,"4.AT+CWDHCP=1");  // 第三步	
	if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();	
	}
	OLED_ShowStr(110,3,1,"OK");	
	delay_ms(500);
	
	OLED_ShowStr(0,4,1,"5. CWJAP");  // 第四步		
	if(ESP8266_SendCmd(ESP8266_WIFI_INFO,"GOT IP",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();		
	}
	OLED_ShowStr(108,4,1,"OK");		
	delay_ms(500);
	
	OLED_ShowStr(0,5,1,"6. Wifi Init OK");  // 第四步	
	
}

//==========================================================
//	函数名称：	USART2_IRQHandler
//
//	函数功能：	串口2收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收中断
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //防止串口被刷爆
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);  //清除中断服务函数
	}

}
