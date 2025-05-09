#ifndef __ATGM336H_H_

#define __ATGM336H_H_

#include "stm32f10x.h"
#include "stdbool.h"


void Read_Data(void);
void ATGM336_Init(void);
void ATGM336_Baisc_Init(void);
void GPS_Init(void);

#endif