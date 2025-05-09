/************************************************************************************
* 
* Description:128*64点阵的OLED显示屏驱动文件SD1306驱动IIC通信方式显示屏
*
* Others: none;
*
* Function List:
*	1. void I2C_Configuration(void) -- 配置CPU的硬件I2C
* 2. void I2C_WriteByte(uint8_t addr,uint8_t data) -- 向寄存器地址写一个byte的数据
* 3. void WriteCmd(unsigned char I2C_Command) -- 写命令
* 4. void WriteDat(unsigned char I2C_Data) -- 写数据
* 5. void OLED_Init(void) -- OLED屏初始化
* 6. void OLED_SetPos(unsigned char x, unsigned char y) -- 设置起始点坐标
* 7. void OLED_Fill(unsigned char fill_Data) -- 全屏填充
* 8. void OLED_CLS(void) -- 清屏
* 9. void OLED_ON(void) -- 唤醒
* 10. void OLED_OFF(void) -- 睡眠
* 11. void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- 显示字符串(字体大小有6*8和8*16两种)
* 12. void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- 显示中文(中文需要先取模，然后放到codetab.h中)
* 13. void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- BMP图片
*
* History: none;
*
*************************************************************************************/

#include "OLED_I2C.h"
#include "delay.h"
#include "codetab.h"
#include "stdlib.h"
unsigned char framebuffer[128][8];  // 128 列 x 8 行（每行 8 位）

void I2C1_Configuration(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure; 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

	/*STM32F103C8T6芯片的硬件I2C: PB6 -- SCL; PB7 -- SDA */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;			//I2C必须开漏输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_DeInit(I2C1);																		//使用I2C1
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;					//配置为I2C模式
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2; //配置I2C波形为:低电平/高电平 = 2 
	I2C_InitStructure.I2C_OwnAddress1 = 0x30;						//主机的I2C地址,随便写的
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;					//应答模式为使能应答
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; //7位地址寻址模式
	I2C_InitStructure.I2C_ClockSpeed = 400000;					//波特率位400Khz

	I2C_Cmd(I2C1, ENABLE);		
	I2C_Init(I2C1, &I2C_InitStructure);
}



void I2C_WriteByte(uint8_t addr,uint8_t data)
{
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
	
	I2C_GenerateSTART(I2C1, ENABLE);//开启I2C1
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));/*EV5,主模式*/

	I2C_Send7bitAddress(I2C1, OLED_ADDRESS, I2C_Direction_Transmitter);//器件地址 -- 默认0x78
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	I2C_SendData(I2C1, addr);//寄存器地址
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_SendData(I2C1, data);//发送数据
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	
	I2C_GenerateSTOP(I2C1, ENABLE);//关闭I2C1总线
}

void WriteCmd(unsigned char I2C_Command)//写命令
{
	I2C_WriteByte(0x00, I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//写数据
{
	I2C_WriteByte(0x40, I2C_Data);
}

void OLED_Init(void)
{
	delay_ms(500); //这里的延时很重要
	WriteCmd(0xAE); //关闭显示
	WriteCmd(0x20);	//---设置存储模式	
	WriteCmd(0x10);	//00，水平寻址模式；01，垂直寻址模式；10，页寻址模式（复位）；11，无效
	WriteCmd(0xb0);	//---设置页面起始地址以用于页面寻址模式，0-7
	WriteCmd(0xc8);	//---设置COM输出扫描方向
	WriteCmd(0x00); //---设置低列地址
	WriteCmd(0x10); //---设置高列地址
	WriteCmd(0x40); //---设置起始行地址
	WriteCmd(0x81); //---设置对比度控制寄存器
	WriteCmd(0xff); //亮度调节 0x00~0xff
	WriteCmd(0xa1); //---设置段重映射为0到127
	WriteCmd(0xa6); //---设置正常显示
	WriteCmd(0xa8); //---设置复用比率（1到64)
	WriteCmd(0x3F); //
	WriteCmd(0xa4); //0xa4,输出遵循RAM内容；0xa5，输出忽略RAM内容
	WriteCmd(0xd3); //---设置显示偏移量
	WriteCmd(0x00); //不偏移
	WriteCmd(0xd5); //--设置显示时钟分频比/振荡器频率
	WriteCmd(0xf0); //--设置分频比
	WriteCmd(0xd9); //--设置预充电周期
	WriteCmd(0x22); //
	WriteCmd(0xda); //---设置 COM 引脚硬件配置
	WriteCmd(0x12);
	WriteCmd(0xdb); //---设置 VCOMH
	WriteCmd(0x20); //0x20,0.77xVcc
	WriteCmd(0x8d); //---使能DC-DC
	WriteCmd(0x14); //
	WriteCmd(0xaf); //---启动OLED面板
}

void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x01);
}

void OLED_Fill(unsigned char fill_Data)//全屏填充
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

void OLED_CLS(void)//清屏
{
	OLED_Fill(0x00);
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          : 
// Parameters     : none
// Description    : 将OLED从休眠中唤醒
//--------------------------------------------------------------
void OLED_ON(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X14);  //开启电荷泵
	WriteCmd(0XAF);  //OLED唤醒
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          : 
// Parameters     : none
// Description    : 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
//--------------------------------------------------------------
void OLED_OFF(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X10);  //关闭电荷泵
	WriteCmd(0XAE);  //OLED休眠
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~7
//chr:显示的字符		 
//TextSize:字符大小(1:6*8 ; 2:8*16)
//mode:1,反白显示;0,正常显示		
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char TextSize,u8 mode)
{      	
		unsigned char c=0,i=0;	
	
		c=chr-' ';//得到偏移后的值	
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
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[] -- 要显示的字符串; TextSize -- 字符大小(1:6*8 ; 2:8*16)
// Description    : 显示codetab.h中的ASCII字符,有6*8和8*16可选择
//--------------------------------------------------------------
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char TextSize,char ch[],... )
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)  //判断打印字符大小：
	{
		case 1:  // TextSize = 1 打印6x8大小字符
		{
			while(ch[j] != '\0') //检索到输入字符串中'\0'，即字符结束
			{
				c = ch[j] - 32;    //字符库只收录了从space之后的ascll成员，space对应的ascll值为32
				if(x > 126)				//判断需要显示的位置是否超出显示范围
				{
					x = 0;					//另起一行
					y++;
				}
				OLED_SetPos(x,y);   //配置显示的初始坐标
				for(i=0;i<6;i++)		//循环进行显示
					WriteDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:  // TextSize = 2 打印8x16大小字符
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
					WriteDat(F8X16[c*16+i]);  	//循环传输字符上半段
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					WriteDat(F8X16[c*16+i+8]);	//循环传输字符下半段
				x += 8;
				j++;
			}
		}break;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
// Calls          : 
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); N:汉字在codetab.h中的索引
// Description    : 显示codetab.h中的汉字,16*16点阵
//mode:1,反白显示;0,正常显示	
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
// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
// Description    : 显示BMP位图
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


// 画点函数
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color) {

    uint8_t page ;       // 计算页（每页 8 行）
		int n;
    uint8_t bit_mask = 1 << (y % 64); // 计算位掩码
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
        framebuffer[x][page] |= bit_mask;  // 点亮像素
    } else {
        framebuffer[x][page] &= ~bit_mask; // 熄灭像素
    }
		WriteDat(framebuffer[x][page]);
}



//// 画线函数（Bresenham 算法）
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {

    int dx,dy,sx,sy,err,e2;
		dx = abs(x1 - x0);dy = abs(y1 - y0);
		sx = (x0 < x1) ? 1 : -1;sy = (y0 < y1) ? 1 : -1;
		err = dx - dy;
		OLED_SetPos(x0,y0);
    while (1) {
        OLED_DrawPoint(x0, y0, color);  // 画点

        if (x0 == x1 && y0 == y1) break;  // 到达终点

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


