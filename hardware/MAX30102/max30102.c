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
int32_t hrAvg;//����
int32_t spo2_buf[16];
int32_t spo2Sum;
int32_t spo2Avg = 0;//Ѫ��Ũ��
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

//	/*STM32F103C8T6оƬ��Ӳ��I2C: PB6 -- SCL; PB7 -- SDA */
//	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;//I2C���뿪©���
//	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	I2C_DeInit(I2C1);//ʹ��I2C1
//	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
//	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
//	I2C_InitStructure.I2C_OwnAddress1 = 0x30;//������I2C��ַ,���д��
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
    
	  bsp_InitI2C();//IIC��ʼ��
	  maxim_max30102_reset(); //��λ MAX30102 ģ��
    maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_dummy); //��ȡ/����ж�״̬�Ĵ���
    maxim_max30102_init();  //��ʼ�� MAX30102ģ��
	
    n_ir_buffer_length = 150; //����������Ϊ150���Դ洢��50����/�루sps�����е�3������

    //��ȡ150�������ݣ���ȷ�����źŷ�Χ
    for(i = 0; i < n_ir_buffer_length; i++)
    {
        //while(KEY0 == 1); //�ȴ�ֱ���ж����ű�����
        maxim_max30102_read_fifo((aun_ir_buffer+i), (aun_red_buffer+i));  //�°汾 //read from MAX30102 FIFO

        if(un_min > aun_red_buffer[i])
            un_min = aun_red_buffer[i]; //�����������ֵ
        if(un_max < aun_red_buffer[i])
            un_max = aun_red_buffer[i]; //�����������ֵ
    }
    un_prev_data = aun_red_buffer[i];
    //Ҫ�������ʣ�HR����Ѫ�����Ͷȣ�SpO2������Ҫ��һ�������������150������
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

		//��ǰ50���������ڴ����Ƴ��򱣴棨dump������ʣ�µ�100��������ǰ�ƶ����ԭ��ǰ50��������λ��
		for(i = 50; i < 150; i++)
		{
				aun_red_buffer[i - 50] = aun_red_buffer[i];
				aun_ir_buffer[i - 50] = aun_ir_buffer[i];

				//�������������ֵ����Сֵ
				if(un_min > aun_red_buffer[i])
						un_min = aun_red_buffer[i];
				if(un_max < aun_red_buffer[i])
						un_max = aun_red_buffer[i];
		}

		//�ڼ�������֮ǰ���Ȼ�ȡ50����������.
		for(i = 100; i < 150; i++)
		{
				un_prev_data = aun_red_buffer[i - 1];
				maxim_max30102_read_fifo((aun_ir_buffer+i), (aun_red_buffer+i));  //�°汾

				//����LED������
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
					//���������ݴ���
					if ((ch_hr_valid == 1) && (n_heart_rate < 150) && (n_heart_rate > 60))
					{
							hrTimeout = 0;
							// ����ÿ��������е�һ������������쳣
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
									// ���µ��������ݼ��뵽��������
									for(i = 0; i < 15; i++)
											hr_buf[i] = hr_buf[i + 1];
									hr_buf[15] = n_heart_rate;
									// ���»����������ֵ
									if (hrBuffFilled < 16)  		//���С��16����ƽ��
											hrBuffFilled = hrBuffFilled + 1;
									// �Ի�����ֵ�����ƶ�ƽ������
									hrSum = 0;
									if (hrBuffFilled < 2)				//���С��2�����ս��Ϊ0
											hrAvg = 0;
									else if (hrBuffFilled < 4)	//���С��4
									{
											for(i = 14; i < 16; i++){  
													hrSum = hrSum + hr_buf[i];}		// ���14��15λ
											hrAvg = hrSum >> 1;		// ����һλ---�൱�ڳ�2
									}
									else if (hrBuffFilled < 8)	//���С��8
									{
											for(i = 12; i < 16; i++){
													hrSum = hrSum + hr_buf[i];}// ���12��15���ĸ�����
											hrAvg = hrSum >> 2;	// ������λλ---�൱�ڳ�4
									}
									else if (hrBuffFilled < 16)//���С��16
									{
											for(i = 8; i < 16; i++){ 
													hrSum = hrSum + hr_buf[i];}// ���8��15��8������
											hrAvg = hrSum >> 3;// ������λλ---�൱�ڳ�8
									}
									else
									{
											for(i = 0; i < 16; i++){
													hrSum = hrSum + hr_buf[i];}// ���0��15��8������
											hrAvg = hrSum >> 4;// ������λλ---�൱�ڳ�16
									}
							}
							hrThrowOutSamp = 0;
					}
					else
					{
							hrValidCnt = 0;
							if (hrTimeout == 4)//�����������
							{
//									hrAvg = 0;				//���ƽ�����
									hrBuffFilled = 0;	//�������������
							}
							else
									hrTimeout++;
					}
					//��Ѫ�����Ͷ����ݴ���
					if ((ch_spo2_valid == 1) && (n_spo2 > 80))
					{
							spo2Timeout = 0;
							// ��ÿ5����Ч�������У�������쳣ֵ��wacky��ʾ��ֵġ��쳣�ģ������Զ����������1������
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
									// ���µ��������ݼ��뵽��������
									for(i = 0; i < 15; i++)
											spo2_buf[i] = spo2_buf[i + 1];
									spo2_buf[15] = n_spo2;

									// ���»����������ֵ
									if (spo2BuffFilled < 16)	//���С��16����ƽ��
											spo2BuffFilled = spo2BuffFilled + 1;
									
									// �Ի�����ֵ�����ƶ�ƽ������
									spo2Sum = 0;
									if (spo2BuffFilled < 2)		//���С��2�����ս��Ϊ0
											spo2Avg = 0;
									else if (spo2BuffFilled < 4)			//���С��4
									{
											for(i = 14; i < 16; i++){
													spo2Sum = spo2Sum + spo2_buf[i];}// ���14��15λ
											spo2Avg = spo2Sum >> 1;			// ����һλ---�൱�ڳ�2
									}
									else if (spo2BuffFilled < 8)		//���С��8
									{
											for(i = 12; i < 16; i++){
													spo2Sum = spo2Sum + spo2_buf[i];}// ���12��15λ
											spo2Avg = spo2Sum >> 2;		// ������λ---�൱�ڳ�4
									}
									else if (spo2BuffFilled < 16)		//���С��16		
									{
											for(i = 8; i < 16; i++){		
													spo2Sum = spo2Sum + spo2_buf[i];}// ���12��15λ
											spo2Avg = spo2Sum >> 3;		// ������λ---�൱�ڳ�8
									}
									else
									{
											for(i = 0; i < 16; i++)
											{
													spo2Sum = spo2Sum + spo2_buf[i];// ���16λ
											}
											spo2Avg = spo2Sum >> 4;		// ������λ---�൱�ڳ�16
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
    /* ��1��������I2C���������ź� */
    i2c_Start();

    /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* �˴���дָ�� */

    /* ��3��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ��4���������ֽڵ�ַ */
    i2c_SendByte(uch_addr);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ��5������ʼд������ */
    i2c_SendByte(uch_data);

    /* ��6��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return true;	/* ִ�гɹ� */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
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
    /* ��1��������I2C���������ź� */
    i2c_Start();

    /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* �˴���дָ�� */

    /* ��3��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ��4���������ֽڵ�ַ�� */
    i2c_SendByte((uint8_t)uch_addr);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }


    /* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
    i2c_Start();

    /* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(max30102_WR_address | I2C_RD);	/* �˴��Ƕ�ָ�� */

    /* ��8��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ��9������ȡ���� */
    {
        *puch_data = i2c_ReadByte();	/* ��1���ֽ� */

        i2c_NAck();	/* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
    }
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return true;	/* ִ�гɹ� ����dataֵ */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
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



    /* ��1��������I2C���������ź� */
    i2c_Start();

    /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(max30102_WR_address | I2C_WR);	/* �˴���дָ�� */

    /* ��3��������ACK */
    if (i2c_WaitAck() != 0)
    {
        printf("read fifo failed");
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }

    /* ��4���������ֽڵ�ַ�� */
    i2c_SendByte((uint8_t)REG_FIFO_DATA);
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
    }


    /* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
    i2c_Start();

    /* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(max30102_WR_address | I2C_RD);	/* �˴��Ƕ�ָ�� */

    /* ��8��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;	/* EEPROM������Ӧ�� */
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

    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return true;
cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
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

