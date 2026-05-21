#ifndef TIMER_COMMON_H
#define TIMER_COMMON_H
#include <stdint.h>

extern int g_is_tim2_init;

int TIM2_Init(void);

int DelayUs(uint32_t us);
#endif