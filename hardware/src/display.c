#include "display.h"
#include "delay.h"
#include "ds1302.h"
#include "OLED_I2C.h"
#include "ds18b20.h"
#include "usart1.h"
#include "atgm336h.h"
#include "max30102.h"
#include "main.h"
#include "buzzer.h"
#include "onenet.h"
#include "esp8266.h"


extern unsigned char setn;
extern unsigned char  p_r;
extern short temperature;
extern float longitude_sum, latitude_sum;
extern unsigned char setm;
extern _SaveData Save_Data;
extern int32_t hrAvg;//心率
extern int32_t spo2Avg;//血氧浓度
	unsigned char *dataPtr = NULL;
	unsigned short timeCount = 0;	//发送间隔变量

int32_t hrMax = 100;  //心率阈值
int32_t spo2Min = 90;//血氧阈值
short temMax = 300;//血氧阈值

float Temperature;


void Display_GPS(void)
{
	OLED_ShowStr(0,1,2,"ATGM336H Init");  //
	OLED_ShowStr(0,4,2,"Waiting for GPS");  //		
	ATGM336_Init();	

	OLED_ShowStr(108,6,2,"OK");	
	delay_ms(1000);
	OLED_CLS();//清屏	
	Read_Data();
}
		int time = 0;
void Display_Items(void)  //第二界面，选择菜单
{

		if(time == 10)
		{
		OLED_CLS();//清屏
				time = 0;
		}
	
		OLED_ShowStr(0,1,1,"1.Heart Rate");   
		OLED_ShowStr(0,2,1,"2.Blood oxygen");
		OLED_ShowStr(0,3,1,"3.Body Temperature");   	
		OLED_ShowStr(0,4,1,"4.Position(GPS)"); 
		OLED_ShowStr(0,5,1,"5.Wifi State");	
		OLED_ShowStr(0,6,1,"6.Net State"); 	
		OLED_ShowChar(110,setm,'<',1,0);
		OLED_ShowChar(110,7,setm+'0',1,0);	
		delay_ms(10);	
			time++;
}

void DisplayWifi_State(void)  //显示WIFI
{
	int i,x=6;
	for(i=0;i<2;i++)OLED_ShowCN(i*16,0,i+31,0);  //网络
	for(i=0;i<3;i++)OLED_ShowCN(32+i*16,0,i+24,0);	 //初始化
	OLED_ShowStr(82,1,1,"success");
	delay_ms(10);
	OLED_ShowStr(0,3,1,"Wifi ID:");
	OLED_ShowStr(0,5,1,"Wifi Key:");	

	OLED_ShowStr(40,4,1,"Mate 50 Pro"); 	
	OLED_ShowStr(40,7,1,"12345678"); 	//同main.h中内容
}

void DisplayOneNet(void)  //显示平台信息
{
	int i,x=6;
//	OLED_CLS();//清屏
	for(i=0;i<2;i++)OLED_ShowCN(i*16,0,i+33,0);  	//平台
	for(i=0;i<3;i++)OLED_ShowCN(32+i*16,0,i+24,0);	 //初始化
	OLED_ShowStr(82,1,1,"success");
	delay_ms(10);
			OLED_ShowStr(0,3,1,"Send Data to OneNet");	
		temperature=DS18B20_Get_Temp();//温度获取
		Temperature = temperature/10; 
		GetHeartRateSpO2();  //心率血氧获取
		Read_Data();  //GPS获取
		
		if(++timeCount >= 10)									//发送间隔0.5s
		{		
			OneNet_SendData();									//发送数据	

			timeCount = 0;
		}
		
		dataPtr = ESP8266_GetIPD(0);
		if(dataPtr != NULL)
		{

			OneNet_RevPro(dataPtr);
		}
			
}


void DisplayGPS(void)
{
	int i,x;
	Read_Data();
	for(i=0;i<2;i++)OLED_ShowCN(8,i*4,35,0);  	//经
	for(i=2;i<7;i=i+4)OLED_ShowCN(8,i,36,0);  	//纬	
	for(i=0;i<8;i=i+2)OLED_ShowCN(24,i,37,0);	//度
	for(i=0;i<8;i=i+2)OLED_ShowChar(40,i,':',2,0);	//:
	x= 6; //longitude
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[0],2,0);  
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[1],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[2],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[3],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[4],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[5],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[6],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[7],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[8],2,0);
	OLED_ShowChar((x++)*8,0,Save_Data.longitude[9],2,0);
	
	x= 6; //longitude_sum	
	OLED_ShowChar((x++)*8,4,(int)longitude_sum/100+'0',2,0);  
	OLED_ShowChar((x++)*8,4,(int)longitude_sum%100/10+'0',2,0);
	OLED_ShowChar((x++)*8,4,(int)longitude_sum%10+'0',2,0);
	OLED_ShowChar((x++)*8,4,'.',2,0);
	OLED_ShowChar((x++)*8,4,(int)(longitude_sum*10)%10+'0',2,0);
	OLED_ShowChar((x++)*8,4,(int)(longitude_sum*100)%10+'0',2,0);
	OLED_ShowChar((x++)*8,4,(int)(longitude_sum*1000)%10+'0',2,0);
	OLED_ShowChar((x++)*8,4,(int)(longitude_sum*10000)%10+'0',2,0);
	OLED_ShowChar((x++)*8,4,(int)(longitude_sum*100000)%10+'0',2,0);	
	
	x= 6; //Save_Data.latitude
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[0],2,0);  
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[1],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[2],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[3],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[4],2,0);	
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[5],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[6],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[7],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[8],2,0);
	OLED_ShowChar((x++)*8,2,Save_Data.latitude[9],2,0);
//	OLED_ShowChar((x++)*8,2,(unsigned char )Save_Data.latitude*100000%10+'0',2,0);
	
	x= 6; //longitude_sum	
	OLED_ShowChar((x++)*8,6,(int)latitude_sum/100+'0',2,0);  
	OLED_ShowChar((x++)*8,6,(int)latitude_sum%100/10+'0',2,0);
	OLED_ShowChar((x++)*8,6,(int)latitude_sum%10+'0',2,0);
	OLED_ShowChar((x++)*8,6,'.',2,0);
	OLED_ShowChar((x++)*8,6,(int)(latitude_sum*10)%10+'0',2,0);
	OLED_ShowChar((x++)*8,6,(int)(latitude_sum*100)%10+'0',2,0);
	OLED_ShowChar((x++)*8,6,(int)(latitude_sum*1000)%10+'0',2,0);
	OLED_ShowChar((x++)*8,6,(int)(latitude_sum*10000)%10+'0',2,0);
//	OLED_ShowChar((x++)*8,6,(int)(latitude_sum*100000)%10+'0',2,0);	

}

void DisplayTime(void)//显示时间函数
{
	  unsigned char i=0,x=0;
	  u16 nian_temp;
	
	  if(setn==0)DS1302_DateRead(&SysDate);//读时间
	  nian_temp=2000+SysDate.year;
		if((nian_temp%400==0)||((nian_temp%100!=0)&&(nian_temp%4==0)))  //判断是否为闰年
				p_r=1;
		else
				p_r=0;
	
	  OLED_ShowChar((x++)*8,2,'2',2,setn+1-1);
	  OLED_ShowChar((x++)*8,2,'0',2,setn+1-1);
	  OLED_ShowChar((x++)*8,2,SysDate.year/10+'0',2,setn+1-1);
	  OLED_ShowChar((x++)*8,2,SysDate.year%10+'0',2,setn+1-1);
	  OLED_ShowChar((x++)*8,2,'-',2,0);
	  OLED_ShowChar((x++)*8,2,SysDate.mon/10+'0',2,setn+1-2);
	  OLED_ShowChar((x++)*8,2,SysDate.mon%10+'0',2,setn+1-2);
	  OLED_ShowChar((x++)*8,2,'-',2,0);
	  OLED_ShowChar((x++)*8,2,SysDate.day/10+'0',2,setn+1-3);
	  OLED_ShowChar((x++)*8,2,SysDate.day%10+'0',2,setn+1-3);
	  
	  OLED_ShowCN(i*16+88,2,0,setn+1-4);//测试显示中文：周
	  switch(SysDate.week)
    {
    case 1: 
			  OLED_ShowCN(i*16+104,2,1,setn+1-4);//测试显示中文：一
        break;

    case 2: 
			  OLED_ShowCN(i*16+104,2,2,setn+1-4);//测试显示中文：二
        break;

    case 3: 
			  OLED_ShowCN(i*16+104,2,3,setn+1-4);//测试显示中文：三
        break;

    case 4: 
			  OLED_ShowCN(i*16+104,2,4,setn+1-4);//测试显示中文：四
        break;

    case 5: 
			  OLED_ShowCN(i*16+104,2,i+5,setn+1-4);//测试显示中文：五
        break;

    case 6: 
			  OLED_ShowCN(i*16+104,2,6,setn+1-4);//测试显示中文：六
        break;

    case 7: 
			  OLED_ShowCN(i*16+104,2,7,setn+1-4);//测试显示中文：日
        break;
    }
    x=4;
	  OLED_ShowChar((x++)*8,6,SysDate.hour/10+'0',1,setn+1-5);
	  OLED_ShowChar((x++)*8,6,SysDate.hour%10+'0',1,setn+1-5);
	  OLED_ShowChar((x++)*8,6,':',1,0);
	  OLED_ShowChar((x++)*8,6,SysDate.min/10+'0',1,setn+1-6);
	  OLED_ShowChar((x++)*8,6,SysDate.min%10+'0',1,setn+1-6);
	  OLED_ShowChar((x++)*8,6,':',1,0);
	  OLED_ShowChar((x++)*8,6,SysDate.sec/10+'0',1,setn+1-7);
	  OLED_ShowChar((x++)*8,6,SysDate.sec%10+'0',1,setn+1-7);
}

void DisplayTemperature(void)//显示温度函数
{
		int i;
		temperature=DS18B20_Get_Temp();
	  unsigned char x=10;//显示的第几个字符
		for(i=0;i<2;i++)OLED_ShowCN(i*16,1,i+20,0);//测试显示中文：温度			
		OLED_ShowChar((x++)*8,1,temperature/100+'0',2,0);
		OLED_ShowChar((x++)*8,1,temperature%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,1,'.',2,0);
		OLED_ShowChar((x++)*8,1,temperature%10+'0',2,0);
		OLED_ShowChar((x++)*8,1,'C',2,0);
		x=10;
		for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+38,0);//阈值温	
		OLED_ShowChar((x++)*8,4,temMax/100+'0',2,0);
		OLED_ShowChar((x++)*8,4,temMax%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,4,'.',2,0);
		OLED_ShowChar((x++)*8,4,temMax%10+'0',2,0);	
		OLED_ShowChar((x++)*8,4,'C',2,0);
		Buzzer_Cmp(temperature,temMax);	  //当前值与阈值对比
}

void DisplayHealth_Rate(void)//显示心率
{
		int i;
		GetHeartRateSpO2();
	  unsigned char x=10;//显示的第几个字符
		for(i=0;i<2;i++)OLED_ShowCN(i*16,1,i+16,0);//心率	
		OLED_ShowChar((x++)*8,1,hrAvg%1000/100+'0',2,0);
		OLED_ShowChar((x++)*8,1,hrAvg%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,1,hrAvg%10+'0',2,0);
		OLED_ShowStr((x++)*8,1,2,"bmp");
		x=10;//显示的第几个字符
		for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+38,0);//阈值		
		OLED_ShowChar((x++)*8,4,hrMax%1000/100+'0',2,0);
		OLED_ShowChar((x++)*8,4,hrMax%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,4,hrMax%10+'0',2,0);
		OLED_ShowStr((x++)*8,4,2,"bmp");	
		Buzzer_Cmp(hrAvg,hrMax);  //当前值与阈值对比

}

void Display_Blood_oxygen(void)//显示血氧
{
		int i;
	
	  unsigned char x=8;//显示的第几个字符
		GetHeartRateSpO2();
		for(i=0;i<2;i++)OLED_ShowCN(i*16,1,i+18,0);//测试显示中文：血氧			
		OLED_ShowChar((x++)*8,1,spo2Avg%1000/100+'0',2,0);
		OLED_ShowChar((x++)*8,1,spo2Avg%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,1,spo2Avg%10+'0',2,0);
		OLED_ShowStr((x++)*8,1,2,"ml/dl");
		x=8;
		for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+38,0);//阈值		
		OLED_ShowChar((x++)*8,4,spo2Min%1000/100+'0',2,0);
		OLED_ShowChar((x++)*8,4,spo2Min%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,4,spo2Min%10+'0',2,0);	
		OLED_ShowStr((x++)*8,4,2,"ml/dl");

		Buzzer_Cmp(spo2Min,spo2Avg);	//当前值与阈值对比

}

