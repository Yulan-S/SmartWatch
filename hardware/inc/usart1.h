#ifndef __usart1_H
#define __usart1_H	

#include "stm32f10x.h"
#include "stdio.h"     
#include "stdarg.h"		
#include "string.h"     //包含需要的头文件

#define USART1_RXBUFF_SIZE   48 

//#define USART1_RX_ENABLE     0      //是否开启接收功能  1：开启  0：关闭
//#define USART1_TXBUFF_SIZE   256    //定义串口1 发送缓冲区大小 256字节

#define USART1_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收



#define false 0
#define true 1
 
//定义数组长度
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 

typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;		//是否获取到GPS数据
	char isParseData;	//是否解析完成
	char UTCTime[UTCTime_Length];		//UTC时间
	char latitude[latitude_Length];		//纬度
	char N_S[N_S_Length];		//N/S
	char longitude[longitude_Length];		//经度
	char E_W[E_W_Length];		//E/W
	char isUsefull;		//定位信息是否有效
} _SaveData;



extern unsigned int RxCounter;          //外部声明，其他文件可以调用该变量
extern char Usart1RecBuf[USART1_RXBUFF_SIZE]; //外部声明，其他文件可以调用该变量

void uart1_Init(u32 bound);
void uart1_SendStr(char* fmt,...);
void uart1_send(unsigned char *bufs,unsigned char len);
		 				    
#endif

