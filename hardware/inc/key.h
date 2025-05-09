#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"
#include "ds1302.h"
#include "usart1.h"
/*
		PA12  --- SET
		PA13 	--- ADD
		PA14  --- CUT
		PB4   --- HOME
		PA0   --- OK
*/

#define KEY0  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)//��ȡ����0 SETTING
#define KEY2  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)//��ȡ����1 ADD
#define KEY1  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)//��ȡ����2 CUT
//#define KEY3  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)//��ȡ����3 OK
//#define KEY4  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4) //��ȡ����4 HOME
//#define KEY5  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)//��ȡ����5
//#define KEY6  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)//��ȡ����6
//#define KEY7  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8)//��ȡ����7
//#define KEY8  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)//��ȡ����8

extern unsigned char setn;
extern unsigned char setm;
extern unsigned char setc;
extern unsigned char keynum ;

extern unsigned char  p_r;	
extern u16 bushu;

//extern DATE SysDate;

#define KEY_SETING       1
#define KEY_ADD          2
#define KEY_CUT          3
#define KEY_CONFIRM      4
#define KEY_HOME         5
//#define KEY_CONFIRM 6
//#define KEY_HOME   7

void KEY_Init(void);//IO��ʼ��
u8 KEY_Scan(u8 mode);  	//����ɨ�躯��		
void KeySettings_Time(void);
void KeySetting_Contrl(void);
void home_int(void);
void ok_int(void);
#endif