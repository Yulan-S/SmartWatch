#include "buzzer.h"
#include "delay.h"

uint8_t flag_bee;

void Buzzer_init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB�˿�ʱ��
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	//PA6����Ϊ�������,SCL
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOA
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�ߵ�ƽ ������������
	Buzzer_Init_on();
	
}

void Buzzer_Init_on()  //��ʼ������ʹ�ã������ʼ���ɹ�������죬����200ms
{
	GPIO_SetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�͵�ƽ ����������
	delay_ms(1000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�ߵ�ƽ ������������	
}

void Buzzer_Error_on()  //�����ض����󣬷��������죬����2s
{

		GPIO_SetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�͵�ƽ ����������

}
void Buzzer_Cmp_f(float A,float B)
{	
	if(A > B)
	{
		Buzzer_Error_on();
	}
	else 
		GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�ߵ�ƽ ������������		
}


void Buzzer_Cmp(int32_t A,int32_t B)
{	
	if(A > B)
	{
		Buzzer_Error_on();
	}
	else 
		GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�ߵ�ƽ ������������		
	if(B == 0)
	{
				GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12����Ϊ�ߵ�ƽ ������������	
	}
}

//void Buzzer_Smaller(int32_t A,int32_t B)
//{	
//	if(A < B)
//	{
//		Buzzer_Error_on();
//	}
//}

