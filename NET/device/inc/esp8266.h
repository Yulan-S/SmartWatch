#ifndef _ESP8266_H_
#define _ESP8266_H_


#define REV_OK		0	//������ɱ�־
#define REV_WAIT	1	//����δ��ɱ�־


#define RESET_IO(x)    GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)x)  //PA4����WiFi�ĸ�λ

void ESP8266_Init(void);
void ESP8266_Clear(void);
_Bool ESP8266_SendCmd(char *cmd, char *res,short timeout);
void ESP8266_SendData(unsigned char *data, unsigned short len);
unsigned char *ESP8266_GetIPD(unsigned short timeOut);
void ESP8266_GPIO_Init(void);
void Wifi_Init(void);

#endif
