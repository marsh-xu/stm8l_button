#ifndef TIMER_H_
#define TIMER_H_

#include "stm8l15x.h"
#include "stm8l15x_tim4.h"

typedef void (*app_timer_timeout_handler_t)(void);

void timer_init(void);

void timer_create(u8* timer_index, app_timer_timeout_handler_t timerout_hander);

void timer_start(u8 timer_index, u32 duration);

void timer_stop(u8 timer_index);

void tick_timeout_handler(void);

#endif // TIMER_H_
