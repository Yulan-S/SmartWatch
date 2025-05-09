//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//�����豸����
#include "esp8266.h"

//Ӳ������
#include "delay.h"
#include "usart2.h"
#include "OLED_I2C.h"
#include "main.h"


//C��
#include <string.h>
#include <stdio.h>

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
uint8_t Wifi_state = 0;

/*-------------------------------------------------*/
/*��������WiFi��λ                                 */
/*��  ����timeout����ʱʱ�䣨100ms�ı�����         */
/*����ֵ��0����ȷ   ����������                     */
/*-------------------------------------------------*/
//char WiFi_Reset(int timeout)
//{
//	RESET_IO(0);                                    //��λIO���͵�ƽ
//	delay_ms(500);                                  //��ʱ500ms
//	RESET_IO(1);                                    //��λIO���ߵ�ƽ	
//	while(timeout--){                               //�ȴ���ʱʱ�䵽0
//		delay_ms(100);                              //��ʱ50ms
//		if(ESP8266_WaitRecive() == REV_OK)             //������յ�ready��ʾ��λ�ɹ�
//			break;       						    //��������whileѭ��
//	}
//	if(timeout<=0)return 1;                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�ready������1
//	else return 0;		         				    //��֮����ʾ��ȷ��˵���յ�ready��ͨ��break��������while
//}


/*
��������WiFi��λ                                 
��  ������        
����ֵ����

*/
void Wifi_Init(void)
{
	Usart2_Init(115200);
	ESP8266_GPIO_Init();
	ESP8266_Init();					//��ʼ��ESP8266
	delay_ms(1000);
	Wifi_state = 1;
	OLED_CLS();//����
		
}



//==========================================================
//	�������ƣ�	ESP8266_Clear
//
//	�������ܣ�	��ջ���
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	�������ƣ�	ESP8266_WaitRecive
//
//	�������ܣ�	�ȴ��������
//
//	��ڲ�����	��
//
//	���ز�����	REV_OK-�������		REV_WAIT-���ճ�ʱδ���
//
//	˵����		ѭ�����ü���Ƿ�������
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//������ռ���Ϊ0 ��˵��û�д��ڽ��������У�����ֱ����������������
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//�����һ�ε�ֵ�������ͬ����˵���������
	{
		esp8266_cnt = 0;							//��0���ռ���
			
		return REV_OK;								//���ؽ�����ɱ�־
	}
		
	esp8266_cntPre = esp8266_cnt;					//��Ϊ��ͬ
	
	return REV_WAIT;								//���ؽ���δ��ɱ�־

}

//==========================================================
//	�������ƣ�	ESP8266_SendCmd
//
//	�������ܣ�	��������
//
//	��ڲ�����	cmd������
//				res����Ҫ���ķ���ָ��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res,short timeout)
{
	uint8_t flag =0; 
	int i;
	Usart_SendString(cmd);		//USART2����ָ��
	while(timeout--)					//�ж��Ƿ�ʱ
	{
		if(ESP8266_WaitRecive() == REV_OK)							//����յ�����
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//����������ؼ���
			{
				ESP8266_Clear();									//��ջ���		
				break;   //��������whileѭ��
			}
		}
		if(flag == 0)		//��̬OLED ��ӡ������
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
	if(timeout<=0)return 1;                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�ready������1
	else return 0;		         				    //��֮����ʾ��ȷ��˵���յ���Ӧ�ַ���ͨ��break��������while
}

/*-------------------------------------------------*/
/*��������WiFi����·����ָ��                       */
/*��  ����timeout����ʱʱ�䣨1s�ı�����            */
/*����ֵ��0����ȷ   ����������                     */
/*-------------------------------------------------*/
//char WiFi_JoinAP(int timeout)
//{		
//	uint8_t flag =0; 
//	int i;
//	Usart_SendString("AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PASS);	
//	while(timeout--){                               //�ȴ���ʱʱ�䵽0
//		if(ESP8266_WaitRecive() == REV_OK)							//����յ�����
//		{
//			if(strstr((const char *)esp8266_buf, "GOT IP") != NULL)		//����������ؼ���
//			{
//				ESP8266_Clear();									//��ջ���		
//				break;   //��������whileѭ��
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
//		}                  //����������ڵĳ�ʱʱ��
//	}
//	if(timeout<=0)return 1;                         //���timeout<=0��˵����ʱʱ�䵽�ˣ�Ҳû���յ�WIFI GOT IP������1
//	return 0;                                       //��ȷ������0
//}


//==========================================================
//	�������ƣ�	ESP8266_SendData
//
//	�������ܣ�	��������
//
//	��ڲ�����	data������
//				len������
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{
	char cmdBuf[32];
	ESP8266_Clear();														//��ս��ջ���
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		// ��ʽ���ַ������洢���ַ�����
	if(!ESP8266_SendCmd(cmdBuf, ">",100))				//�յ���>��ʱ���Է�������
		Wifi_printf( data,len);										//��������
}

//==========================================================
//	�������ƣ�	ESP8266_GetIPD
//
//	�������ܣ�	��ȡƽ̨���ص�����
//
//	��ڲ�����	�ȴ���ʱ��(����10ms)
//
//	���ز�����	ƽ̨���ص�ԭʼ����
//
//	˵����		��ͬ�����豸���صĸ�ʽ��ͬ����Ҫȥ����
//				��ESP8266�ķ��ظ�ʽΪ	"+IPD,x:yyy"	x�������ݳ��ȣ�yyy����������
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//����������
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//������IPD��ͷ
			if(ptrIPD == NULL)											//���û�ҵ���������IPDͷ���ӳ٣�������Ҫ�ȴ�һ�ᣬ�����ᳬ���趨��ʱ��
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//�ҵ�':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		delay_ms(5);													//��ʱ�ȴ�
	} while(timeOut--);
	
	return NULL;														//��ʱ��δ�ҵ������ؿ�ָ��

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
void ESP8266_Rst(void)
{	
	delay_ms(2000);
	OLED_CLS();//����
	ESP8266_Init();  //����ִ�г�ʼ��
}


//==========================================================
//	�������ƣ�	ESP8266_Init
//
//	�������ܣ�	��ʼ��ESP8266
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//ESP8266��λ����
	GPIO_Initure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Initure.GPIO_Pin = GPIO_Pin_4;					//GPIOA4-��λ
	GPIO_Initure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Initure);
	
		//ESP8266ʹ������
	GPIO_Initure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Initure.GPIO_Pin = GPIO_Pin_5;					//GPIOA5-ʹ��
	GPIO_Initure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Initure);
	
	GPIO_SetBits(GPIOA,GPIO_Pin_5);//ʹ��ģ��

}



void ESP8266_Init(void)
{
	int i;
	/*��һ����ģ�鸴λ*/
	OLED_ShowStr(0,0,1,"1.RESET");    
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);
	delay_ms(250);
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
	delay_ms(100);
	OLED_ShowStr(110,0,1,"OK");		
	
	ESP8266_Clear();
	
	OLED_ShowStr(0,1,1,"2.AT");    //��һ�� 
	if(ESP8266_SendCmd("AT\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();
	}
	else
	OLED_ShowStr(110,1,1,"OK");			
	delay_ms(500);
	
	OLED_ShowStr(0,2,1,"3.AT+CWMODE=1");  // �ڶ���
	if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();	
	}
	OLED_ShowStr(110,2,1,"OK");			
	delay_ms(500);
	
	OLED_ShowStr(0,3,1,"4.AT+CWDHCP=1");  // ������	
	if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();	
	}
	OLED_ShowStr(110,3,1,"OK");	
	delay_ms(500);
	
	OLED_ShowStr(0,4,1,"5. CWJAP");  // ���Ĳ�		
	if(ESP8266_SendCmd(ESP8266_WIFI_INFO,"GOT IP",100))
	{
		for(i=0;i<2;i++)OLED_ShowCN(20+i*16,5,i+27,0);
		for(i=0;i<2;i++)OLED_ShowCN(60+i*16,5,i+29,0);			
		ESP8266_Rst();		
	}
	OLED_ShowStr(108,4,1,"OK");		
	delay_ms(500);
	
	OLED_ShowStr(0,5,1,"6. Wifi Init OK");  // ���Ĳ�	
	
}

//==========================================================
//	�������ƣ�	USART2_IRQHandler
//
//	�������ܣ�	����2�շ��ж�
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //�����ж�
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //��ֹ���ڱ�ˢ��
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);  //����жϷ�����
	}

}
