#ifndef __BUZZER_H__
#define __BUZZER_H__
#include "sys.h"

#define  Buzzer   PBout(5) 

void Buzzer_init();
void Buzzer_Init_on();
void Buzzer_Error_on();
void Buzzer_Cmp(int32_t A,int32_t B);
void Buzzer_Cmp_f(float A,float B);

#endif
