#include "buzzer.h"
#include "delay.h"

uint8_t flag_bee;

void Buzzer_init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	//PA6配置为推挽输出,SCL
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOA
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响
	Buzzer_Init_on();
	
}

void Buzzer_Init_on()  //初始化结束使用，如果初始化成功，则会响，持续200ms
{
	GPIO_SetBits(GPIOB,GPIO_Pin_5); //PA12配置为低电平 蜂鸣器会响
	delay_ms(1000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响	
}

void Buzzer_Error_on()  //出现特定错误，蜂鸣器会响，持续2s
{

		GPIO_SetBits(GPIOB,GPIO_Pin_5); //PA12配置为低电平 蜂鸣器会响

}
void Buzzer_Cmp_f(float A,float B)
{	
	if(A > B)
	{
		Buzzer_Error_on();
	}
	else 
		GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响		
}


void Buzzer_Cmp(int32_t A,int32_t B)
{	
	if(A > B)
	{
		Buzzer_Error_on();
	}
	else 
		GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响		
	if(B == 0)
	{
				GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响	
	}
}

//void Buzzer_Smaller(int32_t A,int32_t B)
//{	
//	if(A < B)
//	{
//		Buzzer_Error_on();
//	}
//}

