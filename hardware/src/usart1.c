#include <stm32f10x.h>
#include <stdio.h>
#include "usart1.h"

//////////////////////////////////////////////////////////////////
_SaveData Save_Data;
unsigned int DataIndex = 0;


#if USART1_TX_ENABLE

void uart1_Init(u32 bound)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;     //定义一个设置GPIO功能的变量
	USART_InitTypeDef USART_InitStructure;   //定义一个设置串口功能的变量
#if USART1_RX_ENABLE                         //如果使能接收功能
	NVIC_InitTypeDef NVIC_InitStructure;     //如果使能接收功能，定义一个设置中断的变量
#endif

#if USART1_RX_ENABLE                                 //如果使能接收功能
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //设置中断向量分组：第2组 抢先优先级：0 1 2 3 子优先级：0 1 2 3
#endif	
      
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);  //使能串口1时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);   //使能GPIOA时钟
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;              //准备设置PA9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      //IO速率50M
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	       //复用推挽输出，用于串口1的发送
    GPIO_Init(GPIOA, &GPIO_InitStructure);                 //设置PA9
   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;             //准备设置PA10 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //浮空输入，用于串口1的接收
    GPIO_Init(GPIOA, &GPIO_InitStructure);                 //设置PA10
	
	USART_InitStructure.USART_BaudRate = bound;                                    //波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                    //8个数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                         //1个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;                            //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
#if USART1_RX_ENABLE               												   //如果使能接收模式
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	               //收发模式
#else                                                                              //如果不使能接收模式
	USART_InitStructure.USART_Mode = USART_Mode_Tx ;	                           //只发模式
#endif        
    USART_Init(USART1, &USART_InitStructure);                                      //设置串口1	

#if USART1_RX_ENABLE  	         					        //如果使能接收模式
	USART_ClearFlag(USART1, USART_FLAG_RXNE);	            //清除接收标志位
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);          //开启接收中断
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;       //设置串口1中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0; //抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//中断通道使能
	NVIC_Init(&NVIC_InitStructure);	                        //设置串口1中断
#endif  

	USART_Cmd(USART1, ENABLE);                              //使能串口1
}

/*-------------------------------------------------*/
/*函数名：串口1 printf函数                         */
/*参  数：char* fmt,...  格式化输出字符串和参数    */
/*返回值：无                                       */
/*-------------------------------------------------*/

__align(8) char Usart1_TxBuff[USART1_TXBUFF_SIZE]; 

void uart1_SendStr(char* fmt,...)//串口1打印数据
{
	unsigned int i,length;
	
	va_list ap;
	va_start(ap,fmt);
	vsprintf(Usart1_TxBuff,fmt,ap);
	va_end(ap);	
	
	length=strlen((const char*)Usart1_TxBuff);		
	while((USART1->SR&0X40)==0);
	for(i = 0;i < length;i ++)
	{			
		USART1->DR = Usart1_TxBuff[i];
		while((USART1->SR&0X40)==0);	
	}	
}
#endif

#define USART1_RX_ENABLE 1 

#if  USART1_RX_ENABLE                   //如果使能接收功能
char Usart1_RxCompleted = 0;            //定义一个变量 0：表示接收未完成 1：表示接收完成 
unsigned int Usart1_RxCounter = 0;      //定义一个变量，记录串口1总共接收了多少字节的数据
char Usart1_RxBuff[USART1_RXBUFF_SIZE]; //定义一个数组，用于保存串口1接收到的数据   	

char USART_RX1_BUF[USART1_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.

//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART1_RX_STA=0;       //接收状态标记	



void CLR_Buf(void)                           // 串口缓存清理
{
	memset(USART_RX1_BUF, 0, USART1_REC_LEN);      //清空
  DataIndex = 0;                    
}


void uart1_Init(u32 bound)
{
    //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;     //定义一个设置GPIO功能的变量
	USART_InitTypeDef USART_InitStructure;   //定义一个设置串口功能的变量                        //如果使能接收功能
	NVIC_InitTypeDef NVIC_InitStructure;     //如果使能接收功能，定义一个设置中断的变量
                             //如果使能接收功能
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //设置中断向量分组：第2组 抢先优先级：0 1 2 3 子优先级：0 1 2 3
      
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);  //使能串口1时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);   //使能GPIOA时钟
	

 	USART_DeInit(USART1);  //复位串口1
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;              //准备设置PA9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      //IO速率50M
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	       //复用推挽输出，用于串口1的发送
	GPIO_Init(GPIOA, &GPIO_InitStructure);                 //设置PA9
   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;             //准备设置PA10 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //浮空输入，用于串口1的接收
	GPIO_Init(GPIOA, &GPIO_InitStructure);                 //设置PA10
	
	USART_InitStructure.USART_BaudRate = bound;                                    //波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                    //8个数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                         //1个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;                            //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制            												   //如果使能接收模式
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	               //收发模式
      
  USART_Init(USART1, &USART_InitStructure);                                      //设置串口1	

	USART_ClearFlag(USART1, USART_FLAG_RXNE);	            //清除接收标志位
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);          //开启接收中断
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;       //设置串口1中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2; //抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//中断通道使能
	NVIC_Init(&NVIC_InitStructure);	                        //设置串口1中断


	USART_Cmd(USART1, ENABLE);                              //使能串口1
	
	CLR_Buf();//清空缓存
}


void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) 
	{
		Res =USART_ReceiveData(USART1);//(USART1->DR);	//读取接收到的数据
	
	if(Res == '$')
	{
		DataIndex = 0;	
	}
		
 
	  USART_RX1_BUF[DataIndex++] = Res;
 
	if(USART_RX1_BUF[0] == '$' && USART_RX1_BUF[4] == 'M' && USART_RX1_BUF[5] == 'C')			//确定是否收到"GPRMC/GNRMC"这一帧数据
	{
		if(Res == '\n')									   
		{
			memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);      //清空
			memcpy(Save_Data.GPS_Buffer, USART_RX1_BUF, DataIndex); 	//保存数据
			Save_Data.isGetData = true;
			DataIndex = 0;
			memset(USART_RX1_BUF, 0, USART1_REC_LEN);      //清空				
		}	
				
	}
	
	if(DataIndex >= USART1_REC_LEN)
	{
		DataIndex = USART1_REC_LEN;
	}	  		 
   } 
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
}







#endif

