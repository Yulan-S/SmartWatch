#ifndef __DISPLAY__H_
#define __DISPLAY__H_

#include "stm32f10x.h"

void DisplayTime(void);

void DisplayStatute(void);
void Display_Items(void);

void Display_Items(void);
void DisplayTemperature(void);//显示温度
void DisplayHealth_Rate(void);//显示心率
void Display_Blood_oxygen(void);//显示血氧
void DisplayGPS(void);//显示GPS
void DisplayWifi_State(void);//显示WIFI
void DisplayOneNet(void);  //显示平台信息
#endif
