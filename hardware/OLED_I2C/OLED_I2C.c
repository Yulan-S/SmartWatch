/************************************************************************************
* 
* Description:128*64�����OLED��ʾ�������ļ�SD1306����IICͨ�ŷ�ʽ��ʾ��
*
* Others: none;
*
* Function List:
*	1. void I2C_Configuration(void) -- ����CPU��Ӳ��I2C
* 2. void I2C_WriteByte(uint8_t addr,uint8_t data) -- ��Ĵ�����ַдһ��byte������
* 3. void WriteCmd(unsigned char I2C_Command) -- д����
* 4. void WriteDat(unsigned char I2C_Data) -- д����
* 5. void OLED_Init(void) -- OLED����ʼ��
* 6. void OLED_SetPos(unsigned char x, unsigned char y) -- ������ʼ������
* 7. void OLED_Fill(unsigned char fill_Data) -- ȫ�����
* 8. void OLED_CLS(void) -- ����
* 9. void OLED_ON(void) -- ����
* 10. void OLED_OFF(void) -- ˯��
* 11. void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- ��ʾ�ַ���(�����С��6*8��8*16����)
* 12. void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- ��ʾ����(������Ҫ��ȡģ��Ȼ��ŵ�codetab.h��)
* 13. void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- BMPͼƬ
*
* History: none;
*
*************************************************************************************/

#include "OLED_I2C.h"
#include "delay.h"
#include "codetab.h"
#include "stdlib.h"
unsigned char framebuffer[128][8];  // 128 �� x 8 �У�ÿ�� 8 λ��

void I2C1_Configuration(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure; 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

	/*STM32F103C8T6оƬ��Ӳ��I2C: PB6 -- SCL; PB7 -- SDA */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;			//I2C���뿪©���
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_DeInit(I2C1);																		//ʹ��I2C1
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;					//����ΪI2Cģʽ
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2; //����I2C����Ϊ:�͵�ƽ/�ߵ�ƽ = 2 
	I2C_InitStructure.I2C_OwnAddress1 = 0x30;						//������I2C��ַ,���д��
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;					//Ӧ��ģʽΪʹ��Ӧ��
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; //7λ��ַѰַģʽ
	I2C_InitStructure.I2C_ClockSpeed = 400000;					//������λ400Khz

	I2C_Cmd(I2C1, ENABLE);		
	I2C_Init(I2C1, &I2C_InitStructure);
}



void I2C_WriteByte(uint8_t addr,uint8_t data)
{
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
	
	I2C_GenerateSTART(I2C1, ENABLE);//����I2C1
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));/*EV5,��ģʽ*/

	I2C_Send7bitAddress(I2C1, OLED_ADDRESS, I2C_Direction_Transmitter);//������ַ -- Ĭ��0x78
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	I2C_SendData(I2C1, addr);//�Ĵ�����ַ
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_SendData(I2C1, data);//��������
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	
	I2C_GenerateSTOP(I2C1, ENABLE);//�ر�I2C1����
}

void WriteCmd(unsigned char I2C_Command)//д����
{
	I2C_WriteByte(0x00, I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//д����
{
	I2C_WriteByte(0x40, I2C_Data);
}

void OLED_Init(void)
{
	delay_ms(500); //�������ʱ����Ҫ
	WriteCmd(0xAE); //�ر���ʾ
	WriteCmd(0x20);	//---���ô洢ģʽ	
	WriteCmd(0x10);	//00��ˮƽѰַģʽ��01����ֱѰַģʽ��10��ҳѰַģʽ����λ����11����Ч
	WriteCmd(0xb0);	//---����ҳ����ʼ��ַ������ҳ��Ѱַģʽ��0-7
	WriteCmd(0xc8);	//---����COM���ɨ�跽��
	WriteCmd(0x00); //---���õ��е�ַ
	WriteCmd(0x10); //---���ø��е�ַ
	WriteCmd(0x40); //---������ʼ�е�ַ
	WriteCmd(0x81); //---���öԱȶȿ��ƼĴ���
	WriteCmd(0xff); //���ȵ��� 0x00~0xff
	WriteCmd(0xa1); //---���ö���ӳ��Ϊ0��127
	WriteCmd(0xa6); //---����������ʾ
	WriteCmd(0xa8); //---���ø��ñ��ʣ�1��64)
	WriteCmd(0x3F); //
	WriteCmd(0xa4); //0xa4,�����ѭRAM���ݣ�0xa5���������RAM����
	WriteCmd(0xd3); //---������ʾƫ����
	WriteCmd(0x00); //��ƫ��
	WriteCmd(0xd5); //--������ʾʱ�ӷ�Ƶ��/����Ƶ��
	WriteCmd(0xf0); //--���÷�Ƶ��
	WriteCmd(0xd9); //--����Ԥ�������
	WriteCmd(0x22); //
	WriteCmd(0xda); //---���� COM ����Ӳ������
	WriteCmd(0x12);
	WriteCmd(0xdb); //---���� VCOMH
	WriteCmd(0x20); //0x20,0.77xVcc
	WriteCmd(0x8d); //---ʹ��DC-DC
	WriteCmd(0x14); //
	WriteCmd(0xaf); //---����OLED���
}

void OLED_SetPos(unsigned char x, unsigned char y) //������ʼ������
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x01);
}

void OLED_Fill(unsigned char fill_Data)//ȫ�����
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		WriteCmd(0xb0+m);		//page0-page1
		WriteCmd(0x00);		//low column start address
		WriteCmd(0x10);		//high column start address
		for(n=0;n<128;n++)
			{
				WriteDat(fill_Data);
			}
	}
}

void OLED_CLS(void)//����
{
	OLED_Fill(0x00);
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          : 
// Parameters     : none
// Description    : ��OLED�������л���
//--------------------------------------------------------------
void OLED_ON(void)
{
	WriteCmd(0X8D);  //���õ�ɱ�
	WriteCmd(0X14);  //������ɱ�
	WriteCmd(0XAF);  //OLED����
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          : 
// Parameters     : none
// Description    : ��OLED���� -- ����ģʽ��,OLED���Ĳ���10uA
//--------------------------------------------------------------
void OLED_OFF(void)
{
	WriteCmd(0X8D);  //���õ�ɱ�
	WriteCmd(0X10);  //�رյ�ɱ�
	WriteCmd(0XAE);  //OLED����
}

//��ָ��λ����ʾһ���ַ�,���������ַ�
//x:0~127
//y:0~7
//chr:��ʾ���ַ�		 
//TextSize:�ַ���С(1:6*8 ; 2:8*16)
//mode:1,������ʾ;0,������ʾ		
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char TextSize,u8 mode)
{      	
		unsigned char c=0,i=0;	
	
		c=chr-' ';//�õ�ƫ�ƺ��ֵ	
		if(TextSize == 2)
		{
				if(x>120){x=0;y++;}
			  OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					if(mode==1)WriteDat(~(F8X16[c*16+i]));else WriteDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					if(mode==1)WriteDat(~(F8X16[c*16+i+8]));else WriteDat(F8X16[c*16+i+8]);
		}
		else 
		{	
				if(x>126){x=0;y++;}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
					if(mode==1)WriteDat(~(F6x8[c][i])); else WriteDat(F6x8[c][i]);
		}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          : 
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); ch[] -- Ҫ��ʾ���ַ���; TextSize -- �ַ���С(1:6*8 ; 2:8*16)
// Description    : ��ʾcodetab.h�е�ASCII�ַ�,��6*8��8*16��ѡ��
//--------------------------------------------------------------
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char TextSize,char ch[],... )
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)  //�жϴ�ӡ�ַ���С��
	{
		case 1:  // TextSize = 1 ��ӡ6x8��С�ַ�
		{
			while(ch[j] != '\0') //�����������ַ�����'\0'�����ַ�����
			{
				c = ch[j] - 32;    //�ַ���ֻ��¼�˴�space֮���ascll��Ա��space��Ӧ��ascllֵΪ32
				if(x > 126)				//�ж���Ҫ��ʾ��λ���Ƿ񳬳���ʾ��Χ
				{
					x = 0;					//����һ��
					y++;
				}
				OLED_SetPos(x,y);   //������ʾ�ĳ�ʼ����
				for(i=0;i<6;i++)		//ѭ��������ʾ
					WriteDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:  // TextSize = 2 ��ӡ8x16��С�ַ�
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					WriteDat(F8X16[c*16+i]);  	//ѭ�������ַ��ϰ��
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					WriteDat(F8X16[c*16+i+8]);	//ѭ�������ַ��°��
				x += 8;
				j++;
			}
		}break;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
// Calls          : 
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); N:������codetab.h�е�����
// Description    : ��ʾcodetab.h�еĺ���,16*16����
//mode:1,������ʾ;0,������ʾ	
//--------------------------------------------------------------
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N,u8 mode)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	OLED_SetPos(x , y);
	for(wm = 0;wm < 16;wm++)
	{
		if(mode==1)WriteDat(~(F16x16[adder]));else WriteDat(F16x16[adder]);
		adder += 1;
	}
	OLED_SetPos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		if(mode==1)WriteDat(~(F16x16[adder]));else WriteDat(F16x16[adder]);
		adder += 1;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          : 
// Parameters     : x0,y0 -- ��ʼ������(x0:0~127, y0:0~7); x1,y1 -- ���Խ���(������)������(x1:1~128,y1:1~8)
// Description    : ��ʾBMPλͼ
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<=y1;y++)
	{
		OLED_SetPos(x0,y);
    for(x=x0;x<=x1;x++)
		{
			WriteDat(BMP[j++]);
		}
	}
}


// ���㺯��
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color) {

    uint8_t page ;       // ����ҳ��ÿҳ 8 �У�
		int n;
    uint8_t bit_mask = 1 << (y % 64); // ����λ����
		page = y / 64;  

		WriteCmd(0xb0+y);		//page0-page1
//		WriteCmd(0x00);		//low column start address
//		WriteCmd(0x10);		//high column start address
		WriteCmd(((x&0xf0)>>4)|0x10);
		WriteCmd((x&0x0f)|0x00);	
//		

	//	WriteCmd(((x&0xf0)>>4)|0x10);
//	WriteCmd((x&0x0f)|0x01);
	
    if (color) {
        framebuffer[x][page] |= bit_mask;  // ��������
    } else {
        framebuffer[x][page] &= ~bit_mask; // Ϩ������
    }
		WriteDat(framebuffer[x][page]);
}



//// ���ߺ�����Bresenham �㷨��
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {

    int dx,dy,sx,sy,err,e2;
		dx = abs(x1 - x0);dy = abs(y1 - y0);
		sx = (x0 < x1) ? 1 : -1;sy = (y0 < y1) ? 1 : -1;
		err = dx - dy;
		OLED_SetPos(x0,y0);
    while (1) {
        OLED_DrawPoint(x0, y0, color);  // ����

        if (x0 == x1 && y0 == y1) break;  // �����յ�

         e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}


