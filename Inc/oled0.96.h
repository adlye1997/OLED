#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"

#define OLEDCMD  0
#define OLEDDATA 1
#define OLEDADDRESS 0x78

/* public */
void OledInit(void);
void OledClear(void);
void OledShowInformation(void);
void OledRefreshGram(void);
void OledShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size);
void OledShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size);
void OledShowNum(uint8_t x,uint8_t y,uint16_t num,uint8_t len,uint8_t size);
void OledShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode);

/* private */
void OledClearPoint(uint8_t x,uint8_t y);
void OledWriteByte(uint8_t dat,uint8_t mode);
void OledDrawPoint(uint8_t x,uint8_t y,uint8_t t);
void WriteDat(uint8_t data);
void WriteCmd(uint8_t command);

/* unused */
void OledDisPlayOn(void);
void OledDisPlayOff(void);
void OledColorTurn(uint8_t i);
void OledDisplayTurn(uint8_t i);
void OledConfitWriteBeginPoint(uint8_t x,uint8_t y);

#endif
