/**
 * @file oled0.96.c
 * @author your name (you@domain.com)
 * @brief 适用于0.96寸的OLED屏，屏的显示分辨率为128*64
 *        IIC硬件连接：
 *                  B7--SDA
 *                  B6--CLK
 *        SPI硬件连接：
 *                  A3--DC
 *                  A4--CS
 *                  A5--CLK
 *                  A6--MISO
 *                  A7--MOSI
 *        测试结果：
 *                  行地址模式+硬件IIC--10.71帧
 *                  行地址模式+硬件SPI--1912.32帧
 *                  行地址模式+硬件SPI+DMA--3392.20帧
 * @version 1.1
 * @date 2024-01-27
 */

#include "oled0.96.h"
#include "oledfont.h"
#include "string.h"
#include "timer.h"
#include "main.h"

static uint8_t oled_gram[128][8];

/* 设置行地址模式或页地址模式
 * 行地址模式在全屏更新时比页地址模式快
 * 页地址模式在更新部分屏幕时比行地址模式快
*/
#define LINE_ADDRESS_MODE
//#define PAGE_ADDRESS_MODE

/* 设置通信方式
*/
#define HARD_I2C_MODE
//#define HARD_SPI_MODE
//#define HARD_SPI_DMA_MODE

/* I2C设置
*/
#if defined(HARD_I2C_MODE)
#define OLED_HI2C hi2c1
extern I2C_HandleTypeDef OLED_HI2C;
#endif

/* SPI设置
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

/*  设置计算帧率的定时器
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
	OledWriteByte(0xAE,OLEDCMD); //关闭显示
	OledWriteByte(0xD5,OLEDCMD); //设置时钟分频因子,震荡频率
	OledWriteByte(80,OLEDCMD);   //[3:0],分频因子;[7:4],震荡频率
	OledWriteByte(0xA8,OLEDCMD); //设置驱动路数
	OledWriteByte(0X3F,OLEDCMD); //默认0X3F(1/64)
	OledWriteByte(0xD3,OLEDCMD); //设置显示偏移
	OledWriteByte(0X00,OLEDCMD); //默认为0

	OledWriteByte(0x40,OLEDCMD); //设置显示开始行 [5:0],行数.

	OledWriteByte(0x8D,OLEDCMD); //电荷泵设置
	OledWriteByte(0x14,OLEDCMD); //bit2，开启/关闭

	OledWriteByte(0x20,OLEDCMD); //设置内存地址模式
#ifdef PAGE_ADDRESS_MODE
	OledWriteByte(0x02,OLEDCMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
#endif  //PAGE_ADDRESS_MODE
#ifdef LINE_ADDRESS_MODE
	OledWriteByte(0x01,OLEDCMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
	OledWriteByte(0x21,OLEDCMD); //设置列地址
	OledWriteByte(0x00,OLEDCMD);
	OledWriteByte(0x7f,OLEDCMD);
	OledWriteByte(0x22,OLEDCMD); //设置页地址
	OledWriteByte(0x00,OLEDCMD);
	OledWriteByte(0x07,OLEDCMD);
#endif  //LINE_ADDRESS_MODE
	OledWriteByte(0xA1,OLEDCMD); //段重定义设置,bit0:0,0->0;1,0->127;
	OledWriteByte(0xC0,OLEDCMD); //设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
	OledWriteByte(0xDA,OLEDCMD); //设置COM硬件引脚配置
	OledWriteByte(0x12,OLEDCMD); //[5:4]配置

	OledWriteByte(0x81,OLEDCMD); //对比度设置
	OledWriteByte(0xEF,OLEDCMD); //1~255;默认0X7F (亮度设置,越大越亮)
	OledWriteByte(0xD9,OLEDCMD); //设置预充电周期
	OledWriteByte(0xf1,OLEDCMD); //[3:0],PHASE 1;[7:4],PHASE 2;
	OledWriteByte(0xDB,OLEDCMD); //设置VCOMH 电压倍率
	OledWriteByte(0x30,OLEDCMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OledWriteByte(0xA4,OLEDCMD); //全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
	OledWriteByte(0xA6,OLEDCMD); //设置显示方式;bit0:1,反相显示;0,正常显示
	OledWriteByte(0xAF,OLEDCMD); //开启显示
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
		OledWriteByte(0xb0+i,OLEDCMD);    //设置页地址（0~7）
		OledWriteByte(0x00,OLEDCMD);      //设置显示位置—列低地址
		OledWriteByte(0x10,OLEDCMD);      //设置显示位置—列高地址
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
 * @brief 反色显示
 *
 * @param i 1,反色显示;0,正常显示
 */
void OledColorTurn(uint8_t i)
{
	if(i==0)
	{
		OledWriteByte(0xA6,OLEDCMD);  //正常显示
	}
	else if(i==1)
	{
		OledWriteByte(0xA7,OLEDCMD);  //反色显示
	}
}

/**
 * @brief 屏幕旋转180度
 *
 * @param i
 */
void OledDisplayTurn(uint8_t i)
{
	if(i==0)
	{
		OledWriteByte(0xC8,OLEDCMD);//正常显示
		OledWriteByte(0xA1,OLEDCMD);
	}
	else if(i==1)
	{
		OledWriteByte(0xC0,OLEDCMD);//反转显示
		OledWriteByte(0xA0,OLEDCMD);
	}
}

/**
 * @brief 开启OLED显示
 *
 */
void OledDisPlayOn(void)
{
	OledWriteByte(0x8D,OLEDCMD);  //电荷泵使能
	OledWriteByte(0x14,OLEDCMD);  //开启电荷泵
	OledWriteByte(0xAF,OLEDCMD);  //点亮屏幕
}

/**
 * @brief 关闭OLED显示
 *
 */
void OledDisPlayOff(void)
{
	OledWriteByte(0x8D,OLEDCMD);  //电荷泵使能
	OledWriteByte(0x10,OLEDCMD);  //关闭电荷泵
	OledWriteByte(0xAF,OLEDCMD);  //关闭屏幕
}

/**
 * @brief 画点
 *
 * @param x 0~127
 * @param y 0~63
 * @param t 1,填充;0,清空
 */
void OledDrawPoint(uint8_t x,uint8_t y,uint8_t t)
{
	uint8_t pos,bx,temp=0;
	if(x>127||y>63)return;//超出范围了.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)oled_gram[x][pos]|=temp;
	else oled_gram[x][pos]&=~temp;
}

/**
 * @brief 清除一个点
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
 * @brief 在指定位置显示一个字符,包括部分字符
 *
 * @param x 0~127
 * @param y 0~63
 * @param chr
 * @param size 选择字体 12/16/24
 * @param mode 0,反白显示;1,正常显示
 */
void OledShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
{
	uint8_t temp,t,t1;
	uint8_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	chr=chr-' ';//得到偏移后的值
    for(t=0;t<csize;t++)
    {
		if(size==12)temp=asc2_1206[chr][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[chr][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
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
 * @brief 显示汉字
 *
 * @param x 起点坐标
 * @param y 起点坐标
 * @param num 汉字对应的序号
 * @param size 选择字体 12/16/24
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
						{temp=Hzk1[chr1][i];}//调用16*16字体
				else if(size==24)
						{temp=Hzk2[chr1][i];}//调用24*24字体
				else if(size==32)
						{temp=Hzk3[chr1][i];}//调用32*32字体
				else if(size==64)
						{temp=Hzk4[chr1][i];}//调用64*64字体
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
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
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
 * @brief 显示数字
 *
 * @param x 起点坐标
 * @param y 起点坐标
 * @param num 数值
 * @param len 数字的位数
 * @param size 字体大小
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
 * @brief 配置写入数据的起始位置
 *
 * @param x
 * @param y
 */
void OledConfitWriteBeginPoint(uint8_t x,uint8_t y)
{
	OledWriteByte(0xb0+y,OLEDCMD);//设置行起始地址
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
