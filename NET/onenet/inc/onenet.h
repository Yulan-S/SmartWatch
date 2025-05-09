#ifndef _ONENET_H_
#define _ONENET_H_



_Bool OneNet_DevLink(void);

void OneNet_SendData(void);

void OneNET_Subscribe(void);

void OneNet_RevPro(unsigned char *cmd);

void NetWork_Rst(void);

void OneNet_Init(void);

void OneNet_Link(void);

void OneNet_Config(void);
#endif
