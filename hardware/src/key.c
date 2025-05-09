#include "key.h"
#include "delay.h"
#include "display.h"
#include "OLED_I2C.h"
#include "esp8266.h"
#include "atgm336h.h"
#include "onenet.h"

//按键初始化函数 
//PB12、13、14、15设置成输入

unsigned char keynum = 0;
extern uint8_t Wifi_state;	
extern uint8_t GPS_state;
extern uint8_t OneNet_state;
extern short temperature;
extern int32_t hrAvg;//心率
extern int32_t spo2Avg;//血氧浓度

/*


*/

void KEY_Init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
    
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);//使能PORTB时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//关闭jtag，使能SWD，可以用SWD模式调试
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	 GPIO_PinRemapConfig(GPIO_Remap_MISC, ENABLE);

	AFIO->EXTICR[1] |= 0x0001;  //PB4
	AFIO->EXTICR[0] |= 0x0000;	//PA0
	
	home_int();
	ok_int();
} 
//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//注意此函数有响应优先级,KEY0>KEY1
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	delay_ms(10);//去抖动 
//	if(key_up&&(KEY0==0||KEY1==0||KEY2==0||KEY3==0))
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0))
	{
		delay_ms(10);//去抖动 
		key_up=0;
		if     (KEY0==0)return KEY_SETING;
		else if(KEY1==0)return KEY_ADD;
		else if(KEY2==0)return KEY_CUT;
//		else if(KEY3==0)return KEY_CONFIRM;
//		else if(KEY4==0)return KEY_HOME;
	}else if(KEY0==1&&KEY1==1&&KEY2==1)key_up=1; 
//	}else if(KEY0==1&&KEY1==1&&KEY2==1&&KEY3==1)key_up=1; 	     
	return 0;// 无按键按下
		
}

void KeySettings_Time(void)//按键设置函数
{
//	  unsigned char keynum = 0;
//	
//	  keynum = KEY_Scan(0);//获取按键值
		if(keynum==KEY_SETING)//设置
		{
				setn++;
				if(setn > 7)setn=0;
		}
		if(keynum==KEY_ADD)//加
		{
				if(setn == 1)//设置年
				{
						SysDate.year ++;
						if(SysDate.year == 100)SysDate.year=0;
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 2)//设置月
				{
						SysDate.mon ++;
						if(SysDate.mon == 13)SysDate.mon = 1;
						if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))
						{
								if(SysDate.day>30)SysDate.day=1;
						}
						else
						{
								if(SysDate.mon==2)
								{
										if(p_r==1)
										{
												if(SysDate.day>29)
														SysDate.day=1;
										}
										else
										{
												if(SysDate.day>28)
														SysDate.day=1;
										}
								}
						}
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 3)//设置日
				{
						SysDate.day ++;
						if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))//大月
						{
								if(SysDate.day==32)//最大31天
										SysDate.day=1;//从1开始
						}
						else
						{
								if(SysDate.mon==2)//二月份
								{
										if(p_r==1)//闰年
										{
												if(SysDate.day==30)//最大29天
														SysDate.day=1;
										}
										else
										{
												if(SysDate.day==29)//最大28天
														SysDate.day=1;
										}
								}
								else
								{
										if(SysDate.day==31)//最大30天
												SysDate.day=1;
								}
						}
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 4)//设置星期
				{
						SysDate.week ++;
						if(SysDate.week == 8)SysDate.week=1;
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 5)//设置时
				{
						SysDate.hour ++;
						if(SysDate.hour == 24)SysDate.hour=0;
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 6)//设置分
				{
						SysDate.min ++;
						if(SysDate.min == 60)SysDate.min=0;
						DS1302_DateSet(&SysDate);//设置时间
				}
				if(setn == 7)//设置秒
				{
						SysDate.sec ++;
						if(SysDate.sec == 60)SysDate.sec=0;
						DS1302_DateSet(&SysDate);//设置时间
				}
		}
		if(keynum==KEY_CUT)//减
		{
				if(setn == 1)//设置年
				{
						if(SysDate.year == 0)SysDate.year=100;
						SysDate.year --;
						DS1302_DateSet(&SysDate);
				}
				if(setn == 2)//设置月
				{
						if(SysDate.mon == 1)SysDate.mon=13;
						SysDate.mon --;
						if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))
						{
								if(SysDate.day>30)
										SysDate.day=1;
						}
						else
						{
								if(SysDate.mon==2)
								{
										if(p_r==1)
										{
												if(SysDate.day>29)
														SysDate.day=1;
										}
										else
										{
												if(SysDate.day>28)
														SysDate.day=1;
										}
								}
						}
						DS1302_DateSet(&SysDate);
				}
				if(setn == 3)//设置日
				{
						SysDate.day --;
						if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))
						{
								if(SysDate.day==0)
										SysDate.day=31;
						}
						else
						{
								if(SysDate.mon==2)
								{
										if(p_r==1)
										{
												if(SysDate.day==0)
														SysDate.day=29;
										}
										else
										{
												if(SysDate.day==0)
														SysDate.day=28;
										}
								}
								else
								{
										if(SysDate.day==0)
												SysDate.day=30;
								}
						}
						DS1302_DateSet(&SysDate);
				}
				if(setn == 4)//设置星期
				{
						if(SysDate.week == 1)SysDate.week=8;
						SysDate.week --;
						DS1302_DateSet(&SysDate);
				}
				if(setn == 5)//设置时
				{
						if(SysDate.hour == 0)SysDate.hour=24;
						SysDate.hour --;
						DS1302_DateSet(&SysDate);
				}
				if(setn == 6)//设置分
				{
						if(SysDate.min == 0)SysDate.min=60;
						SysDate.min --;
						DS1302_DateSet(&SysDate);
				}
				if(setn == 7)//设置秒
				{
						if(SysDate.sec == 0)SysDate.sec=60;
						SysDate.sec --;
						DS1302_DateSet(&SysDate);
				}
		}
//		if(keynum==KEY_ADD)
//		{
//			 bushu = 0;
//			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x20,&bushu,1);
//			 delay_ms(50); 
//		}
}


void KeySetting_Contrl(void)
{
  keynum = KEY_Scan(0);//获取按键值	
	KeySettings_Time();//时钟设置
		switch(setc)
		{
			case 0:
//				while(1){
				OLED_CLS();//清屏
				DisplayTime();  //进入主界面
//				delay_ms(1000);
//					if(setc != 0)break;
//				}				
				break;
			case 1:
				OLED_CLS();//清屏
				while(1){
					keynum = KEY_Scan(0);//获取按键值				
				switch(keynum)
				{
					case KEY_ADD:
						setm++;
						if(setm > 6)setm=1;//溢出
						break;
					case KEY_CUT:      
						setm--;
						if(setm < 1)setm=6;//溢出
						break;
				default:
						break;				
				}	
					Display_Items();	//第二界面显示				
				if(setc != 1)break;
				else if(keynum == KEY_SETING)break;
				}
				OLED_CLS();//清屏
				break;
			case 2:
				OLED_CLS();//清屏
				switch(setm)
				{
					case 1://选择心率检测模式
						while(1){
							DisplayHealth_Rate();		//显示心率界面	
							if(setc !=2)break;
										}
						break;
					case 2://选择血氧检测模式	
						while(1){
							Display_Blood_oxygen();	//显示血氧界面		
							if(setc !=2)break;
						}
						break;			
					case 3://选择为温度检测模式
						while(1)
						{
							DisplayTemperature();  //显示温度界面	
							if(setc !=2)break;							
						}
						break;
					case 4://选择GPS
						while(1)
						{						
						if(!GPS_state)
							GPS_Init();
					else
							DisplayGPS();  //显示GPS界面	
						if(setc !=2)break;	
						}
						break;
					case 5://选择WIFI状态显示
					while(1)
						{						
						if(!Wifi_state)
							Wifi_Init();
					else
							DisplayWifi_State();
						if(setc !=2)break;	
						}					
						break;
					case 6://选择为进入物联模式
					while(1)
						{						
						if(!OneNet_state)
							OneNet_Config();
					else			
							DisplayOneNet();
						if(setc !=2)break;	
						}						
						break;
				}
				break;	
		}			
		delay_ms(100); 	
}


void home_int(void)
{
	EXTI_InitTypeDef Home_Exti ;
	NVIC_InitTypeDef Home_Nvic;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  //设置中断向量分组：第2组 抢先优先级：0 1 2 3 子优先级：0 1 2 3
	
	Home_Exti.EXTI_Line    = EXTI_Line4 ;
	Home_Exti.EXTI_Mode    = EXTI_Mode_Interrupt;
	Home_Exti.EXTI_Trigger = EXTI_Trigger_Falling ;
	Home_Exti.EXTI_LineCmd = ENABLE;
	
	EXTI_Init(&Home_Exti);
	
	Home_Nvic.NVIC_IRQChannel                   = EXTI4_IRQn;
	Home_Nvic.NVIC_IRQChannelPreemptionPriority = 0;
	Home_Nvic.NVIC_IRQChannelSubPriority        = 0;
	Home_Nvic.NVIC_IRQChannelCmd                = ENABLE;
	
	NVIC_Init(&Home_Nvic);
}

void ok_int(void)
{
	EXTI_InitTypeDef Home_Exti ;
	NVIC_InitTypeDef Home_Nvic;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  //设置中断向量分组：第2组 抢先优先级：0 1 2 3 子优先级：0 1 2 3
	
	Home_Exti.EXTI_Line    = EXTI_Line0 ;
	Home_Exti.EXTI_Mode    = EXTI_Mode_Interrupt;
	Home_Exti.EXTI_Trigger = EXTI_Trigger_Falling ;
	Home_Exti.EXTI_LineCmd = ENABLE;
	
	EXTI_Init(&Home_Exti);
	
	Home_Nvic.NVIC_IRQChannel                   = EXTI0_IRQn;
	Home_Nvic.NVIC_IRQChannelPreemptionPriority = 0;
	Home_Nvic.NVIC_IRQChannelSubPriority        = 0;
	Home_Nvic.NVIC_IRQChannelCmd                = ENABLE;
	
	NVIC_Init(&Home_Nvic);
}

void Init(void)
{
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); //PA12配置为高电平 蜂鸣器不会响
	hrAvg = 0;
//	spo2Avg = 0;
	temperature = 0;
}

void EXTI4_IRQHandler(void)  //HOME减
{
         //返回主界面
	setc = 0;
	Init();
	EXTI_ClearFlag(EXTI_Line4);
	EXTI_ClearITPendingBit(EXTI_Line4);
//	EXTI->IMR &= ~EXTI_Line4;
}

void EXTI0_IRQHandler(void)  //OK建
{
         
	setc++;
	if(setc >2)setc=2;
	EXTI_ClearFlag(EXTI_Line0);
	EXTI_ClearITPendingBit(EXTI_Line0);
}