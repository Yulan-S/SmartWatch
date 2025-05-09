/** \file max30102.cpp ******************************************************
*
* Project: MAXREFDES117#
* Filename: max30102.cpp
* Description: This module is an embedded controller driver for the MAX30102
*
* Revision History:
*\n 1-18-2016 Rev 01.00 GL Initial release.
*\n
*/

#include "max30102.h"
#include "algorithm.h"
#include "ds18b20.h"
#include "myiic.h"
#include "OLED_I2C.h"


#define max30102_WR_address 0xAE
#define MAX_BRIGHTNESS 255

extern short temperature;

int32_t n_spo2;  //SPO2 value
int32_t n_ir_buffer_length; //data length
int32_t hrValidCnt = 0;
int32_t hrTimeout = 0;
int32_t n_heart_rate; //heart rate value

int32_t hrSum;
int32_t hrAvg;//心率
int32_t spo2_buf[16];
int32_t spo2Sum;
int32_t spo2Avg = 0;//血氧浓度
int32_t spo2BuffFilled;
int32_t hrBuffFilled;
int32_t spo2ValidCnt = 0;
int32_t spo2ThrowOutSamp = 0;
int32_t spo2Timeout = 0;

int32_t hr_buf[16];

uint8_t uch_dummy;
int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid
int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid

uint32_t aun_ir_buffer[150]; //infrared LED sensor data
uint32_t aun_red_buffer[150];  //red LED sensor data
uint32_t un_min, un_max,un_prev_data;
int32_t hrThrowOutSamp = 0;
uint32_t  un_brightness;  //variables to calculate the on-board LED brightness that reflects the heartbeats

//void I2C1_Configuration(void)
//{
//	I2C_InitTypeDef  I2C_InitStructure;
//	GPIO_InitTypeDef  GPIO_InitStructure; 

//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

//	/*STM32F103C8T6芯片的硬件I2C: PB6 -- SCL; PB7 -- SDA */
//	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;//I2C必须开漏输出
//	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	I2C_DeInit(I2C1);//使用I2C1
//	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
//	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
//	I2C_InitStructure.I2C_OwnAddress1 = 0x30;//主机的I2C地址,随便写的
//	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
//	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
//	I2C_InitStructure.I2C_ClockSpeed = 400000;//400K

//	I2C_Cmd(I2C1, ENABLE);
//	I2C_Init(I2C1, &I2C_InitStructure);
//}



void Init_MAX30102(void)
{
    int32_t i;

    un_brightness = 0;
    un_min = 0x3FFFF;
    un_max = 0;
    
	  bsp_InitI2C();//IIC初始化
	  maxim_max30102_reset(); //复位 MAX30102 模块
    maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_dummy); //读取/清除中断状态寄存器
    maxim_max30102_init();  //初始化 MAX30102模块
	
    n_ir_buffer_length = 150; //缓冲区长度为150可以存储以50样本/秒（sps）运行的3秒样本

    //读取150样本数据，明确样本信号范围
    for(i = 0; i < n_ir_buffer_length; i++)
    {
        //while(KEY0 == 1); //等待直到中断引脚被触发
        maxim_max30102_read_fifo((aun_ir_buffer+i), (aun_red_buffer+i));  //新版本 //read from MAX30102 FIFO

        if(un_min > aun_red_buffer[i])
            un_min = aun_red_buffer[i]; //更新样本最低值
        if(un_max < aun_red_buffer[i])
            un_max = aun_red_buffer[i]; //更新样本最高值
    }
    un_prev_data = aun_red_buffer[i];
    //要计算心率（HR）和血氧饱和度（SpO2），需要进一步处理这最初的150个样本
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
}

void GetHeartRateSpO2(void)
{
	  int32_t i;
	  float f_temp;
	  static u8 COUNT=8;
	  unsigned char x=0;
	
		i = 0;
		un_min = 0x3FFFF;
		un_max = 0;

		//将前50个样本从内存中移除或保存（dump），将剩下的100个样本向前移动，填补原来前50个样本的位置
		for(i = 50; i < 150; i++)
		{
				aun_red_buffer[i - 50] = aun_red_buffer[i];
				aun_ir_buffer[i - 50] = aun_ir_buffer[i];

				//更新样本的最大值和最小值
				if(un_min > aun_red_buffer[i])
						un_min = aun_red_buffer[i];
				if(un_max < aun_red_buffer[i])
						un_max = aun_red_buffer[i];
		}

		//在计算心率之前，先获取50组样本数据.
		for(i = 100; i < 150; i++)
		{
				un_prev_data = aun_red_buffer[i - 1];
				maxim_max30102_read_fifo((aun_ir_buffer+i), (aun_red_buffer+i));  //新版本

				//计算LED的亮度
				if(aun_red_buffer[i] > un_prev_data)
				{
						f_temp = aun_red_buffer[i] - un_prev_data;
						f_temp /= (un_max - un_min);
						f_temp *= MAX_BRIGHTNESS;
						f_temp = un_brightness - f_temp;
						if(f_temp < 0)
								un_brightness = 0;
						else
								un_brightness = (int)f_temp;
				}
				else
				{
						f_temp = un_prev_data - aun_red_buffer[i];
						f_temp /= (un_max - un_min);
						f_temp *= MAX_BRIGHTNESS;
						un_brightness += (int)f_temp;
						if(un_brightness > MAX_BRIGHTNESS)
								un_brightness = MAX_BRIGHTNESS;
				}
		}
		maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
		if(COUNT++ > 8)   
		{
					COUNT = 0;
					//对心率数据处理
					if ((ch_hr_valid == 1) && (n_heart_rate < 150) && (n_heart_rate > 60))
					{
							hrTimeout = 0;
							// 丢弃每五个数据中的一个，如果数据异常
							if (hrValidCnt == 4)
							{
									hrThrowOutSamp = 1;
									hrValidCnt = 0;
									for (i = 12; i < 16; i++)
									{
											if (n_heart_rate < hr_buf[i] + 10)
											{
													hrThrowOutSamp = 0;
													hrValidCnt   = 4;
											}
									}
							}
							else
									hrValidCnt = hrValidCnt + 1;  
							if (hrThrowOutSamp == 0)
							{
									// 将新的样本数据加入到缓冲区中
									for(i = 0; i < 15; i++)
											hr_buf[i] = hr_buf[i + 1];
									hr_buf[15] = n_heart_rate;
									// 更新缓冲区的填充值
									if (hrBuffFilled < 16)  		//如果小于16，则平移
											hrBuffFilled = hrBuffFilled + 1;
									// 对缓冲区值进行移动平均处理
									hrSum = 0;
									if (hrBuffFilled < 2)				//如果小于2，最终结果为0
											hrAvg = 0;
									else if (hrBuffFilled < 4)	//如果小于4
									{
											for(i = 14; i < 16; i++){  
													hrSum = hrSum + hr_buf[i];}		// 求和14、15位
											hrAvg = hrSum >> 1;		// 右移一位---相当于除2
									}
									else if (hrBuffFilled < 8)	//如果小于8
									{
											for(i = 12; i < 16; i++){
													hrSum = hrSum + hr_buf[i];}// 求和12到15的四个样本
											hrAvg = hrSum >> 2;	// 右移两位位---相当于除4
									}
									else if (hrBuffFilled < 16)//如果小于16
									{
											for(i = 8; i < 16; i++){ 
													hrSum = hrSum + hr_buf[i];}// 求和8到15的8个样本
											hrAvg = hrSum >> 3;// 右移三位位---相当于除8
									}
									else
									{
											for(i = 0; i < 16; i++){
													hrSum = hrSum + hr_buf[i];}// 求和0到15的8个样本
											hrAvg = hrSum >> 4;// 右移四位位---相当于除16
									}
							}
							hrThrowOutSamp = 0;
					}
					else
					{
							hrValidCnt = 0;
							if (hrTimeout == 4)//如果超过次数
							{
//									hrAvg = 0;				//清除平均结果
									hrBuffFilled = 0;	//清除缓冲区数据
							}
							else
									hrTimeout++;
					}
					//对血氧饱和度数据处理
					if ((ch_spo2_valid == 1) && (n_spo2 > 80))
					{
							spo2Timeout = 0;
							// 在每5个有效的样本中，如果有异常值（wacky表示奇怪的、异常的），可以丢弃其中最多1个样本
							if (spo2ValidCnt == 4)
							{
									spo2ThrowOutSamp = 1;
									spo2ValidCnt = 0;
									for (i = 12; i < 16; i++)
									{
											if (n_spo2 > spo2_buf[i] - 10)
											{
													spo2ThrowOutSamp = 0;
													spo2ValidCnt   = 4;
											}
									}
							}
							else
									spo2ValidCnt = spo2ValidCnt + 1;
							
							if (spo2ThrowOutSamp == 0)
							{
									// 将新的样本数据加入到缓冲区中
									for(i = 0; i < 15; i++)
											spo2_buf[i] = spo2_buf[i + 1];
									spo2_buf[15] = n_spo2;

									// 更新缓冲区的填充值
									if (spo2BuffFilled < 16)	//如果小于16，则平移
											spo2BuffFilled = spo2BuffFilled + 1;
									
									// 对缓冲区值进行移动平均处理
									spo2Sum = 0;
									if (spo2BuffFilled < 2)		//如果小于2，最终结果为0
											spo2Avg = 0;
									else if (spo2BuffFilled < 4)			//如果小于4
									{
											for(i = 14; i < 16; i++){
													spo2Sum = spo2Sum + spo2_buf[i];}// 求和14、15位
											spo2Avg = spo2Sum >> 1;			// 右移一位---相当于除2
									}
									else if (spo2BuffFilled < 8)		//如果小于8
									{
											for(i = 12; i < 16; i++){
													spo2Sum = spo2Sum + spo2_buf[i];}// 求和12到15位
											spo2Avg = spo2Sum >> 2;		// 右移两位---相当于除4
									}
									else if (spo2BuffFilled < 16)		//如果小于16		
									{
											for(i = 8; i < 16; i++){		
													spo2Sum = spo2Sum + spo2_buf[i];}// 求和12到15位
											spo2Avg = spo2Sum >> 3;		// 右移三位---相当于除8
									}
									else
									{
											for(i = 0; i < 16; i++)
											{
													spo2Sum = spo2Sum + spo2_buf[i];// 求和16位
											}
											spo2Avg = spo2Sum >> 4;		// 右移四位---相当于除16
									}
							}
							spo2ThrowOutSamp = 0;
					}
					else
					{
							spo2ValidCnt = 0;
							if (spo2Timeout == 4)
							{
//									spo2Avg = 0;
									spo2BuffFilled = 0;
							}
							else
									spo2Timeout++;
					}
		}
}





bool maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
/**
* \brief        Write a value to a MAX30102 register
* \par          Details
*               This function writes a value to a MAX30102 register
*
* \param[in]    uch_addr    - register address
* \param[in]    uch_data    - register data
*
* \retval       true on success
*/
{
    /* 第1步：发起I2C总线启动信号 */
    i2c_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* 此处是写指令 */

    /* 第3步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址 */
    i2c_SendByte(uch_addr);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 第5步：开始写入数据 */
    i2c_SendByte(uch_data);

    /* 第6步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return true;	/* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return false;
}

bool maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
/**
* \brief        Read a MAX30102 register
* \par          Details
*               This function reads a MAX30102 register
*
* \param[in]    uch_addr    - register address
* \param[out]   puch_data    - pointer that stores the register data
*
* \retval       true on success
*/
{
    /* 第1步：发起I2C总线启动信号 */
    i2c_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* 此处是写指令 */

    /* 第3步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    i2c_SendByte((uint8_t)uch_addr);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }


    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    i2c_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    i2c_SendByte(max30102_WR_address | I2C_RD);	/* 此处是读指令 */

    /* 第8步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    {
        *puch_data = i2c_ReadByte();	/* 读1个字节 */

        i2c_NAck();	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    }
    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return true;	/* 执行成功 返回data值 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return false;
}

bool maxim_max30102_init(void)
/**
* \brief        Initialize the MAX30102
* \par          Details
*               This function initializes the MAX30102
*
* \param        None
*
* \retval       true on success
*/
{
    if(!maxim_max30102_write_reg(REG_INTR_ENABLE_1, 0xc0)) // INTR setting
        return false;
    if(!maxim_max30102_write_reg(REG_INTR_ENABLE_2, 0x00))
        return false;
    if(!maxim_max30102_write_reg(REG_FIFO_WR_PTR, 0x00)) //FIFO_WR_PTR[4:0]
        return false;
    if(!maxim_max30102_write_reg(REG_OVF_COUNTER, 0x00)) //OVF_COUNTER[4:0]
        return false;
    if(!maxim_max30102_write_reg(REG_FIFO_RD_PTR, 0x00)) //FIFO_RD_PTR[4:0]
        return false;
    if(!maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x6f)) //sample avg = 8, fifo rollover=false, fifo almost full = 17
        return false;
    if(!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x03))  //0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
        return false;
    if(!maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x2F)) // SPO2_ADC range = 4096nA, SPO2 sample rate (400 Hz), LED pulseWidth (411uS)
        return false;

    if(!maxim_max30102_write_reg(REG_LED1_PA, 0x17))  //Choose value for ~ 4.5mA for LED1
        return false;
    if(!maxim_max30102_write_reg(REG_LED2_PA, 0x17))  // Choose value for ~ 4.5mA for LED2
        return false;
    if(!maxim_max30102_write_reg(REG_PILOT_PA, 0x7f))  // Choose value for ~ 25mA for Pilot LED
        return false;
    return true;
}

bool maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)

/**
* \brief        Read a set of samples from the MAX30102 FIFO register
* \par          Details
*               This function reads a set of samples from the MAX30102 FIFO register
*
* \param[out]   *pun_red_led   - pointer that stores the red LED reading data
* \param[out]   *pun_ir_led    - pointer that stores the IR LED reading data
*
* \retval       true on success
*/
{
    uint32_t un_temp;
    uint8_t uch_temp;
    *pun_ir_led = 0;
    *pun_red_led = 0;
    maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
    maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);



    /* 第1步：发起I2C总线启动信号 */
    i2c_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* 此处是写指令 */

    /* 第3步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        printf("read fifo failed");
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    i2c_SendByte((uint8_t)REG_FIFO_DATA);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }


    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    i2c_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    i2c_SendByte(max30102_WR_address | I2C_RD);	/* 此处是读指令 */

    /* 第8步：发送ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM器件无应答 */
    }

    un_temp = i2c_ReadByte();
    i2c_Ack();
    un_temp <<= 16;
    *pun_red_led += un_temp;
    un_temp = i2c_ReadByte();
    i2c_Ack();
    un_temp <<= 8;
    *pun_red_led += un_temp;
    un_temp = i2c_ReadByte();
    i2c_Ack();
    *pun_red_led += un_temp;

    un_temp = i2c_ReadByte();
    i2c_Ack();
    un_temp <<= 16;
    *pun_ir_led += un_temp;
    un_temp = i2c_ReadByte();
    i2c_Ack();
    un_temp <<= 8;
    *pun_ir_led += un_temp;
    un_temp = i2c_ReadByte();
    i2c_Ack();
    *pun_ir_led += un_temp;
    *pun_red_led &= 0x03FFFF; //Mask MSB [23:18]
    *pun_ir_led &= 0x03FFFF; //Mask MSB [23:18]

    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return true;
cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    i2c_Stop();
    return false;
}

bool maxim_max30102_reset()
/**
* \brief        Reset the MAX30102
* \par          Details
*               This function resets the MAX30102
*
* \param        None
*
* \retval       true on success
*/
{
    if(!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x40))
        return false;
    else
        return true;
}

