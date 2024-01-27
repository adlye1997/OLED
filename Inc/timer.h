#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f1xx_hal.h"

typedef struct 
{
  uint32_t begin_us;
  uint32_t begin_ms;
  uint32_t end_us;
  uint32_t end_ms;
  uint32_t duration_us;
  uint32_t duration_ms;
}Timer;

void timer_start(Timer *timer, TIM_HandleTypeDef *htim);
void timer_end(Timer *timer, TIM_HandleTypeDef *htim);
void timer_cycle(Timer *timer, TIM_HandleTypeDef *htim);

#endif
