/**
 * @file oled0.96.c
 * @author your name (you@domain.com)
 * @brief ������0.96���OLED����������ʾ�ֱ���Ϊ128*64
 *        IICӲ�����ӣ�
 *                  B7--SDA
 *                  B6--CLK
 *        SPIӲ�����ӣ�
 *                  A3--DC
 *                  A4--CS
 *                  A5--CLK
 *                  A6--MISO
 *                  A7--MOSI
 *        ���Խ����
 *                  �е�ַģʽ+Ӳ��IIC--10.71֡
 *                  �е�ַģʽ+Ӳ��SPI--1912.32֡
 *                  �е�ַģʽ+Ӳ��SPI+DMA--3392.20֡
 * @version 1.1
 * @date 2024-01-27
 */

#include "oled0.96.h"
#include "oledfont.h"
#include "string.h"
#include "timer.h"
#include "main.h"

static uint8_t oled_gram[128][8];

/* �����е�ַģʽ��ҳ��ַģʽ
 * �е�ַģʽ��ȫ������ʱ��ҳ��ַģʽ��
 * ҳ��ַģʽ�ڸ��²�����Ļʱ���е�ַģʽ��
*/
#define LINE_ADDRESS_MODE
//#define PAGE_ADDRESS_MODE

/* ����ͨ�ŷ�ʽ
*/
#define HARD_I2C_MODE
//#define HARD_SPI_MODE
//#define HARD_SPI_DMA_MODE

/* I2C����
*/
#if defined(HARD_I2C_MODE)
#define OLED_HI2C hi2c1
extern I2C_HandleTypeDef OLED_HI2C;
#endif

/* SPI����
*/
#if defined(HARD_SPI_MODE) | defined(HARD_SPI_DMA_MODE)
#define OLED_HSPI hspi1
#define DC_LOW HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET);
#define DC_HIGH HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
extern SPI_HandleTypeDef OLED_HSPI;
#endif
#ifdef HARD_SPI_DMA_MODE
#define OLED_SPI_HDMA hdma_spi1_tx
extern DMA_HandleTypeDef OLED_SPI_HDMA;
#endif

/*  ���ü���֡�ʵĶ�ʱ��
*/
#define FPS_ENABLE
#ifdef FPS_ENABLE
#define FPS_HTIM htim3
extern TIM_HandleTypeDef FPS_HTIM;
static float fps = 0;
static Timer timer_fps;
#endif  //FPS_ENABLE

/**
 * @brief init oled
 *
 */
void OledInit(void)
{
	OledWriteByte(0xAE,OLEDCMD); //�ر���ʾ
	OledWriteByte(0xD5,OLEDCMD); //����ʱ�ӷ�Ƶ����,��Ƶ��
	OledWriteByte(80,OLEDCMD);   //[3:0],��Ƶ����;[7:4],��Ƶ��
	OledWriteByte(0xA8,OLEDCMD); //��������·��
	OledWriteByte(0X3F,OLEDCMD); //Ĭ��0X3F(1/64)
	OledWriteByte(0xD3,OLEDCMD); //������ʾƫ��
	OledWriteByte(0X00,OLEDCMD); //Ĭ��Ϊ0

	OledWriteByte(0x40,OLEDCMD); //������ʾ��ʼ�� [5:0],����.

	OledWriteByte(0x8D,OLEDCMD); //��ɱ�����
	OledWriteByte(0x14,OLEDCMD); //bit2������/�ر�

	OledWriteByte(0x20,OLEDCMD); //�����ڴ��ַģʽ
#ifdef PAGE_ADDRESS_MODE
	OledWriteByte(0x02,OLEDCMD); //[1:0],00���е�ַģʽ;01���е�ַģʽ;10,ҳ��ַģʽ;Ĭ��10;
#endif  //PAGE_ADDRESS_MODE
#ifdef LINE_ADDRESS_MODE
	OledWriteByte(0x01,OLEDCMD); //[1:0],00���е�ַģʽ;01���е�ַģʽ;10,ҳ��ַģʽ;Ĭ��10;
	OledWriteByte(0x21,OLEDCMD); //�����е�ַ
	OledWriteByte(0x00,OLEDCMD);
	OledWriteByte(0x7f,OLEDCMD);
	OledWriteByte(0x22,OLEDCMD); //����ҳ��ַ
	OledWriteByte(0x00,OLEDCMD);
	OledWriteByte(0x07,OLEDCMD);
#endif  //LINE_ADDRESS_MODE
	OledWriteByte(0xA1,OLEDCMD); //���ض�������,bit0:0,0->0;1,0->127;
	OledWriteByte(0xC0,OLEDCMD); //����COMɨ�跽��;bit3:0,��ͨģʽ;1,�ض���ģʽ COM[N-1]->COM0;N:����·��
	OledWriteByte(0xDA,OLEDCMD); //����COMӲ����������
	OledWriteByte(0x12,OLEDCMD); //[5:4]����

	OledWriteByte(0x81,OLEDCMD); //�Աȶ�����
	OledWriteByte(0xEF,OLEDCMD); //1~255;Ĭ��0X7F (��������,Խ��Խ��)
	OledWriteByte(0xD9,OLEDCMD); //����Ԥ�������
	OledWriteByte(0xf1,OLEDCMD); //[3:0],PHASE 1;[7:4],PHASE 2;
	OledWriteByte(0xDB,OLEDCMD); //����VCOMH ��ѹ����
	OledWriteByte(0x30,OLEDCMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OledWriteByte(0xA4,OLEDCMD); //ȫ����ʾ����;bit0:1,����;0,�ر�;(����/����)
	OledWriteByte(0xA6,OLEDCMD); //������ʾ��ʽ;bit0:1,������ʾ;0,������ʾ
	OledWriteByte(0xAF,OLEDCMD); //������ʾ
	OledClear();

#ifdef FPS_ENABLE
	HAL_TIM_Base_Start(&FPS_HTIM);
#endif  //FPS_ENABLE
}

/**
 * @brief
 *
 */
void OledClear(void)
{
	memset(oled_gram, 0, 1024);
	OledRefreshGram();
}

/**
 * @brief
 *
 */
void OledShowInformation(void)
{
#ifdef FPS_ENABLE
	static uint8_t tick = 0;
	OledShowString(8,16,(const uint8_t*)"fps is ",16);
	OledShowNum(64,16,fps,4,16);
	OledShowString(80+16,16,(const uint8_t*)".",16);
	OledShowNum(88+16,16,(uint16_t)(fps*100.0)%100,2,16);
	OledShowNum(40,32,tick,3,16);
	tick++;
#else
	OledShowString(8,16,(const uint8_t*)"fps is disable",16);
#endif  //FPS_ENABLE
}

/**
 * @brief
 *
 */
void OledRefreshGram(void)
{
#ifdef FPS_ENABLE
	timer_start(&timer_fps, &FPS_HTIM);
#endif  //FPS_ENABLE
#ifdef PAGE_ADDRESS_MODE
	uint8_t i,n;
	for(i=0;i<8;i++)
	{
		OledWriteByte(0xb0+i,OLEDCMD);    //����ҳ��ַ��0~7��
		OledWriteByte(0x00,OLEDCMD);      //������ʾλ�á��е͵�ַ
		OledWriteByte(0x10,OLEDCMD);      //������ʾλ�á��иߵ�ַ
		for(n=0;n<128;n++)
		{
#ifdef HARD_I2C_MODE
			HAL_I2C_Mem_Write(&OLED_HI2C, OLEDADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &oled_gram[n][i], 1, 100);
#endif  //HARD_I2C_MODE
#ifdef HARD_SPI_MODE
			OledWriteByte(oled_gram[n][i], OLEDDATA);
#endif  //HARD_SPI_MODE
		}
	}
#endif  //PAGE_ADDRESS_MODE
#ifdef LINE_ADDRESS_MODE
#ifdef HARD_I2C_MODE
	HAL_I2C_Mem_Write(&OLED_HI2C,OLEDADDRESS,0x40,I2C_MEMADD_SIZE_8BIT,(uint8_t*)oled_gram,1024,100);
#endif  //HARD_I2C_MODE
#ifdef HARD_SPI_MODE
	DC_HIGH
//	CS_LOW
	HAL_SPI_Transmit(&OLED_HSPI, (uint8_t*)oled_gram, 1024, 100);
//	CS_HIGH
#endif  //HARD_SPI_MODE
#ifdef HARD_SPI_DMA_MODE
	DC_HIGH
	HAL_SPI_Transmit_DMA(&OLED_HSPI, (uint8_t*)oled_gram, 1024);
#endif  //HARD_SPI_DMA_MODE
#endif  //LINE_ADDRESS_MODE
#ifdef FPS_ENABLE
	timer_end(&timer_fps, &FPS_HTIM);
	fps = 1000000.0 / timer_fps.duration_us;
#endif  //FPS_ENABLE
}

/**
 * @brief
 *
 * @param dat data
 * @param mode 1,date;0,command;
 */
void OledWriteByte(uint8_t dat,uint8_t mode)
{
	if(mode)
	{
		WriteDat(dat);
	}
  else
	{
		WriteCmd(dat);
	}
}

/**
 * @brief
 *
 * @param data
 */
void WriteDat(uint8_t data)
{
	uint8_t *pData;
	pData = &data;
#ifdef HARD_I2C_MODE
	HAL_I2C_Mem_Write(&OLED_HI2C,OLEDADDRESS,0x40,I2C_MEMADD_SIZE_8BIT,pData,1,100);
#endif  //HARD_I2C_MODE
#ifdef HARD_SPI_MODE
	DC_HIGH
//	CS_LOW
	HAL_SPI_Transmit(&OLED_HSPI, pData, 1, 100);
//	CS_HIGH
#endif  //HARD_SPI_MODE
#ifdef HARD_SPI_DMA_MODE
	DC_HIGH
	HAL_SPI_Transmit_DMA(&OLED_HSPI, pData, 1);
#endif  //HARD_SPI_DMA_MODE
}

/**
 * @brief
 *
 * @param command command
 */
void WriteCmd(uint8_t command)
{
	uint8_t *pData;
	pData = &command;
#ifdef HARD_I2C_MODE
	HAL_I2C_Mem_Write(&OLED_HI2C,OLEDADDRESS,0x00,I2C_MEMADD_SIZE_8BIT,pData,1,100);
#endif  //HARD_I2C_MODE
#ifdef HARD_SPI_MODE
	DC_LOW
//	CS_LOW
	HAL_SPI_Transmit(&OLED_HSPI, pData, 1, 100);
//	CS_HIGH
	DC_HIGH
#endif  //HARD_SPI_MODE
#ifdef HARD_SPI_DMA_MODE
	DC_LOW
	HAL_SPI_Transmit_DMA(&OLED_HSPI, pData, 1);
	DC_HIGH
#endif  //HARD_SPI_DMA_MODE
}

/**
 * @brief ��ɫ��ʾ
 *
 * @param i 1,��ɫ��ʾ;0,������ʾ
 */
void OledColorTurn(uint8_t i)
{
	if(i==0)
	{
		OledWriteByte(0xA6,OLEDCMD);  //������ʾ
	}
	else if(i==1)
	{
		OledWriteByte(0xA7,OLEDCMD);  //��ɫ��ʾ
	}
}

/**
 * @brief ��Ļ��ת180��
 *
 * @param i
 */
void OledDisplayTurn(uint8_t i)
{
	if(i==0)
	{
		OledWriteByte(0xC8,OLEDCMD);//������ʾ
		OledWriteByte(0xA1,OLEDCMD);
	}
	else if(i==1)
	{
		OledWriteByte(0xC0,OLEDCMD);//��ת��ʾ
		OledWriteByte(0xA0,OLEDCMD);
	}
}

/**
 * @brief ����OLED��ʾ
 *
 */
void OledDisPlayOn(void)
{
	OledWriteByte(0x8D,OLEDCMD);  //��ɱ�ʹ��
	OledWriteByte(0x14,OLEDCMD);  //������ɱ�
	OledWriteByte(0xAF,OLEDCMD);  //������Ļ
}

/**
 * @brief �ر�OLED��ʾ
 *
 */
void OledDisPlayOff(void)
{
	OledWriteByte(0x8D,OLEDCMD);  //��ɱ�ʹ��
	OledWriteByte(0x10,OLEDCMD);  //�رյ�ɱ�
	OledWriteByte(0xAF,OLEDCMD);  //�ر���Ļ
}

/**
 * @brief ����
 *
 * @param x 0~127
 * @param y 0~63
 * @param t 1,���;0,���
 */
void OledDrawPoint(uint8_t x,uint8_t y,uint8_t t)
{
	uint8_t pos,bx,temp=0;
	if(x>127||y>63)return;//������Χ��.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)oled_gram[x][pos]|=temp;
	else oled_gram[x][pos]&=~temp;
}

/**
 * @brief ���һ����
 *
 * @param x 0~127
 * @param y 0~63
 */
void OledClearPoint(uint8_t x,uint8_t y)
{
	uint8_t i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	oled_gram[x][i]=~oled_gram[x][i];
	oled_gram[x][i]|=n;
	oled_gram[x][i]=~oled_gram[x][i];
}

/**
 * @brief ��ָ��λ����ʾһ���ַ�,���������ַ�
 *
 * @param x 0~127
 * @param y 0~63
 * @param chr
 * @param size ѡ������ 12/16/24
 * @param mode 0,������ʾ;1,������ʾ
 */
void OledShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
{
	uint8_t temp,t,t1;
	uint8_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//�õ�����һ���ַ���Ӧ������ռ���ֽ���
	chr=chr-' ';//�õ�ƫ�ƺ��ֵ
    for(t=0;t<csize;t++)
    {
		if(size==12)temp=asc2_1206[chr][t]; 	 	//����1206����
		else if(size==16)temp=asc2_1608[chr][t];	//����1608����
		else if(size==24)temp=asc2_2412[chr][t];	//����2412����
		else return;								//û�е��ֿ�
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OledDrawPoint(x,y,mode);
			else OledDrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}
    }
}

/**
 * @brief ��ʾ����
 *
 * @param x �������
 * @param y �������
 * @param num ���ֶ�Ӧ�����
 * @param size ѡ������ 12/16/24
 */
void OledShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size)
{
	uint8_t i,m,n=0,temp,chr1;
	uint8_t x0=x,y0=y;
	uint8_t size3=size/8;
	while(size3--)
	{
		chr1=num*size/8+n;
		n++;
			for(i=0;i<size;i++)
			{
				if(size==16)
						{temp=Hzk1[chr1][i];}//����16*16����
				else if(size==24)
						{temp=Hzk2[chr1][i];}//����24*24����
				else if(size==32)
						{temp=Hzk3[chr1][i];}//����32*32����
				else if(size==64)
						{temp=Hzk4[chr1][i];}//����64*64����
				else return;

						for(m=0;m<8;m++)
							{
								if(temp&0x01)OledDrawPoint(x,y,1);
								else OledClearPoint(x,y);
								temp>>=1;
								y++;
							}
							x++;
							if((x-x0)==size)
							{x=x0;y0=y0+8;}
							y=y0;
			 }
	}
}

/**
 * @brief show string
 *
 * @param x
 * @param y
 * @param p string address
 * @param size font size
 */
void OledShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size)
{
    while((*p<='~')&&(*p>=' '))//�ж��ǲ��ǷǷ��ַ�!
    {
        if(x>(128-(size/2))){x=0;y+=size;}
        if(y>(64-size)){y=x=0;OledClear();}
        OledShowChar(x,y,*p,size,1);
        x+=size/2;
        p++;
    }
}

//m^n
uint16_t OledPow(uint8_t m,uint8_t n)
{
	uint16_t result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}

/**
 * @brief ��ʾ����
 *
 * @param x �������
 * @param y �������
 * @param num ��ֵ
 * @param len ���ֵ�λ��
 * @param size �����С
 */
void OledShowNum(uint8_t x,uint8_t y,uint16_t num,uint8_t len,uint8_t size)
{
	uint8_t t,temp;
	uint8_t enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/OledPow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OledShowChar(x+(size/2)*t,y,' ',size,1);
				continue;
			}else enshow=1;

		}
	 	OledShowChar(x+(size/2)*t,y,temp+'0',size,1);
	}
}

/**
 * @brief ����д�����ݵ���ʼλ��
 *
 * @param x
 * @param y
 */
void OledConfitWriteBeginPoint(uint8_t x,uint8_t y)
{
	OledWriteByte(0xb0+y,OLEDCMD);//��������ʼ��ַ
	OledWriteByte(((x&0xf0)>>4)|0x10,OLEDCMD);
	OledWriteByte((x&0x0f)|0x01,OLEDCMD);
}

#if defined(HARD_I2C_MODE) & defined(HARD_SPI_MODE)
#error choose too many mode
#elif defined(HARD_I2C_MODE) & defined(HARD_SPI_DMA_MODE)
#error choose too many mode
#elif defined(HARD_SPI_MODE) & defined(HARD_SPI_DMA_MODE)
#error choose too many mode
#elif defined(HARD_SPI_MODE) & defined(HARD_SPI_DMA_MODE) & defined(HARD_I2C_MODE)
#error choose too many mode
#elif !defined(HARD_SPI_MODE) & !defined(HARD_SPI_DMA_MODE) & !defined(HARD_I2C_MODE)
#error no choose mode
#endif

#if defined(LINE_ADDRESS_MODE) & defined(PAGE_ADDRESS_MODE)
#error choose too many mode
#endif
