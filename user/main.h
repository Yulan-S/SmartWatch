#ifndef __MAIN_H_
#define __MAIN_H_

#include "stm32f10x.h"

#define PROID			"rPt76ojBtN"					  															//产品ID
#define ACCESS_KEY		"m/+VA71fp6PnN6cxtSWtSwtVBenVERGhjxulpTfP/UU="		//设备名  
#define DEVICE_NAME		"Watch01"																					//设备秘钥  

#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

#define ESP8266_WIFI_INFO		"AT+CWJAP=\"Mate 50 Pro\",\"12345678\"\r\n"        //WIFI名称与密码

//#define SSID   "Mate 50 Pro"              //路由器SSID名称   2.4G   
//#define PASS   "12345678"                 //路由器密码

#endif