#include "timer.h"

void timer_start(Timer *timer, TIM_HandleTypeDef *htim)
{
  timer->begin_ms = HAL_GetTick();
  timer->begin_us = __HAL_TIM_GET_COUNTER(htim);
}

void timer_end(Timer *timer, TIM_HandleTypeDef *htim)
{
  timer->end_ms = HAL_GetTick();
  timer->end_us = __HAL_TIM_GET_COUNTER(htim);
  timer->duration_ms = timer->end_ms - timer->begin_ms;
  timer->duration_us = timer->duration_ms * 1000 + timer->end_us - timer->begin_us;
}

void timer_cycle(Timer *timer, TIM_HandleTypeDef *htim)
{
  timer_end(timer, htim);
  timer->begin_ms = timer->end_ms;
  timer->begin_us = timer->end_us;
}
