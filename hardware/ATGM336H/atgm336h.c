#include "atgm336h.h"
#include "OLED_I2C.h"
#include "usart1.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "key.h"

extern _SaveData Save_Data;
uint8_t GPS_state = 0;

void errorLog(uint8_t num)
{
		int i;
		for(i=0;i<5;i++)OLED_ShowCN(i*16,0,i+22,0);  //硬件初始化
	
}

void GPS_Init(void)
{
	OLED_ShowStr(0,0,2,"ATGM336H Init");
 	OLED_ShowStr(0,4,2,"Waiting for GPS"); 
	ATGM336_Init();
 	OLED_ShowStr(110,7,1,"OK"); 
		OLED_CLS();//清屏

}

void ATGM336_Baisc_Init(void)
{
	  GPIO_InitTypeDef GPIO_InitStructure;     //定义一个设置GPIO功能的变量
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);   //使能GPIOA时钟
	
		uart1_Init(9600);  //串口初始化
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;              //准备设置PA11
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      //IO速率50M
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;	      		//下拉输入
		GPIO_Init(GPIOA, &GPIO_InitStructure);                 //设置PA11
}


void ATGM336_Init(void)
{
	int i; 
	uart1_Init(9600);
		bool Point_flag;
		while(!GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))
	{
					if(Point_flag == 0){
			for(i=0;i<11;i++)
		{
				for(i=0;i<11;i++)OLED_ShowChar(0+(i*4),7,' ',1,0);delay_ms(10);
				Point_flag = 1;
		}
		}
		else
		{
			for(i=0;i<11;i++){OLED_ShowChar(0+(i*4),7,' ',1,0);}
			Point_flag = 0;		
			delay_ms(10);			
		}
		if(setc == 0)
			break;

	}
			if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))
		{
				GPS_state	= 1;
		}
			for(i=0;i<9;i++){OLED_ShowChar(0+(i*4),7,' ',1,0);}	
}


// 获取GPS定位信息
void GetsGpsBuffer()
{
	char *subString;
	char *subStringNext;
	char i = 0;
	char usefullBuffer[2];
	if (Save_Data.isGetData)
	{
		Save_Data.isGetData = false;
		//截取数据帧前六部分    							 |对地航速 对地航向  日期
		//$GNRMC,112536.000,A,2322.75023,N,11326.28605,E,|  0.00,   0.00,  100722,,,A*78		
		for (i = 0; i <= 6; i++)
		{
			if (i == 0)
			{
				if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
					errorLog(1); // 解析错误
			}
			else
			{
				subString++;
				if ((subStringNext = strstr(subString, ",")) != NULL)
				{
					switch (i)
					{
					case 1:memcpy(Save_Data.UTCTime, subString, subStringNext - subString);break; // 获取UTC时间
					case 2:memcpy(usefullBuffer, subString, subStringNext - subString);break; // 获取UTC时间
					case 3:memcpy(Save_Data.latitude, subString, subStringNext - subString);break; // 获取纬度信息
					case 4:memcpy(Save_Data.N_S, subString, subStringNext - subString);break; // 获取N/S
					case 5:memcpy(Save_Data.longitude, subString, subStringNext - subString);break; // 获取经度信息
					case 6:memcpy(Save_Data.E_W, subString, subStringNext - subString);break; // 获取E/W
 
					default:
						break;
					}
					subString = subStringNext;
					Save_Data.isParseData = true;
					if (usefullBuffer[0] == 'A')
						Save_Data.isUsefull = true;
					else if (usefullBuffer[0] == 'V')
						Save_Data.isUsefull = false;
				}
				else
				{
					errorLog(2); // 解析错误
				}
			}
		}
	}
}
float longitude_sum, latitude_sum;
uint8_t longitude_int, latitude_int;
void ParseGpsBuffer()
{
	// 转化为数字
	longitude_sum = atof(Save_Data.longitude);
	latitude_sum = atof(Save_Data.latitude);
	// printf("维度 = %.5f %.5f\r\n",longitude_sum,latitude_sum);
	// 整数
	longitude_int = longitude_sum / 100;
	latitude_int = latitude_sum / 100;
 
	// 转化为经纬度
	longitude_sum = longitude_int + ((longitude_sum / 100 - longitude_int) * 100) / 60;
	latitude_sum = latitude_int + ((latitude_sum / 100 - latitude_int) * 100) / 60;
 
	if (Save_Data.isParseData)
	{
		Save_Data.isParseData = false;
 
		// printf("Save_Data.UTCTime = %s\r\n", Save_Data.UTCTime);
		if (Save_Data.isUsefull)
		{
			Save_Data.isUsefull = false;
			
			
			
// printf("经度 = %s\r\n",Save_Data.longitude);
// printf("经度 = %.5f\r\n", longitude_sum);
// printf("维度 = %s\r\n",Save_Data.latitude);
// printf("纬度 = %.5f\r\n", latitude_sum);
		}
		else
		{
			// printf("GPS DATA is not usefull!\r\n");
		}
	}
}
// 获取数据参数
void Read_Data(void)
{
	// 经纬度获取
	GetsGpsBuffer();
	// 解析经纬度
	ParseGpsBuffer();
 
}
 