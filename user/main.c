#include "stm32f10x.h"
#include "onenet.h"
#include "esp8266.h"
#include "sys.h"
#include "delay.h"
#include "usart1.h"
#include "usart2.h"
#include "OLED_I2C.h"
#include "ds18b20.h"
#include "display.h"
#include "ds1302.h"
#include "max30102.h"
#include <string.h>
#include "atgm336h.h"
#include "key.h"
#include "main.h"
#include "buzzer.h"


u8 temperature=0,humidity=0;
unsigned char  p_r=0;		 						//平年/润年  =0表示平年，=1表示润年
unsigned char setn;
unsigned char setm=1;//坐标初值在1
unsigned char setc=0;//OK初值在主界面

	
extern _SaveData Save_Data;

void Hardware_Init(void);
void Init_Statute(void);

int main(void)
{
		int timeout; 

	

	uint32_t i,x;
	Hardware_Init();

	OLED_CLS();//清屏	
	
	Init_Statute();
	
	delay_ms(1000);	
	OLED_CLS();//清屏
	DisplayTime();  //进入主界面
	while(1)
	{
		KeySetting_Contrl();
		
	}
}

void Hardware_Init(void)
{
	int i;
	Delay_Init();									//systick初始化
	I2C1_Configuration(); //IIC初始化	
	OLED_Init();	    //OLED初始化
	KEY_Init();  //按键初始化 
	delay_ms(100);	
	OLED_CLS();//清屏
	OLED_ShowStr(0,0,1,"OLED");  //打印OLED状态
	for(i=0;i<7;i++){OLED_ShowChar((52+i*8),0,'.',1,0);delay_ms(5);}
	OLED_ShowStr(110,0,1,"OK");
	
	OLED_ShowStr(0,2,1,"DS18B20");  //	

	DS18B20_Init();  //温湿度传感器初始化
	for(i=0;i<7;i++){OLED_ShowChar((52+i*8),2,'.',1,0);delay_ms(5);}
	OLED_ShowStr(110,2,1,"OK");	
	delay_ms(100);
	
	OLED_ShowStr(0,4,1,"DS1302");  //	
	DS1302_Init(&SysDate);  //时钟初始化
	delay_ms(100); 
	DS1302_DateRead(&SysDate);//读时间	
	for(i=0;i<7;i++){OLED_ShowChar((52+i*8),4,'.',1,0);delay_ms(5);}
	OLED_ShowStr(110,4,1,"OK");
	
	delay_ms(100);
	OLED_ShowStr(0,6,1,"MAX30102");  //心率血氧初始化
	Init_MAX30102();//心率血氧初始化	
	for(i=0;i<7;i++){OLED_ShowChar((52+i*8),6,'.',1,0);delay_ms(5);}
	OLED_ShowStr(108,6,1,"OK");
	Buzzer_init();
	
//	delay_ms(100);
//	ATGM336_Baisc_Init();
//	OLED_ShowStr(0,4,1,"ATGM336");  //
//	for(i=0;i<7;i++){OLED_ShowChar((52+i*8),3,'.',1,0);delay_ms(5);}
//	OLED_ShowStr(108,3,1,"OK");	

}

void Init_Statute(void)
{
	int i;
	for(i=0;i<5;i++)OLED_ShowCN(i*16,3,i+22,0);  //硬件初始化
	OLED_ShowStr(110,3,3,"OK");
	delay_ms(1000);	
	
}


