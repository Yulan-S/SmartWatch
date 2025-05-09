#ifndef __usart1_H
#define __usart1_H	

#include "stm32f10x.h"
#include "stdio.h"     
#include "stdarg.h"		
#include "string.h"     //������Ҫ��ͷ�ļ�

#define USART1_RXBUFF_SIZE   48 

//#define USART1_RX_ENABLE     0      //�Ƿ������չ���  1������  0���ر�
//#define USART1_TXBUFF_SIZE   256    //���崮��1 ���ͻ�������С 256�ֽ�

#define USART1_REC_LEN  			200  	//�����������ֽ��� 200
#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����



#define false 0
#define true 1
 
//�������鳤��
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 

typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;		//�Ƿ��ȡ��GPS����
	char isParseData;	//�Ƿ�������
	char UTCTime[UTCTime_Length];		//UTCʱ��
	char latitude[latitude_Length];		//γ��
	char N_S[N_S_Length];		//N/S
	char longitude[longitude_Length];		//����
	char E_W[E_W_Length];		//E/W
	char isUsefull;		//��λ��Ϣ�Ƿ���Ч
} _SaveData;



extern unsigned int RxCounter;          //�ⲿ�����������ļ����Ե��øñ���
extern char Usart1RecBuf[USART1_RXBUFF_SIZE]; //�ⲿ�����������ļ����Ե��øñ���

void uart1_Init(u32 bound);
void uart1_SendStr(char* fmt,...);
void uart1_send(unsigned char *bufs,unsigned char len);
		 				    
#endif

