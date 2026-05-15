// wtimer.h
// Runs on TM4C123
// Header file for wtimer.c. Defines WTIMER1 configuration constants and
// declares millisecond timing, delay, elapsed time checking, and linear
// range mapping functions for use across all project modules.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 4, 2026


#ifndef WTIMER_H_
#define WTIMER_H_

#include <stdint.h>

//registers
#define EN_WTIMER_CLOCK     0x02
#define WTIMER_TAEN_BIT     0x01
#define WTIMER_32_BIT_CFG   0x04
#define PRESCALER_VALUE     0x00

void WTIMER_Init(void);
void DELAY_1MS(uint32_t delay);
uint32_t WTIMER_Millis(void);
uint8_t WTIMER_HasElapsed(uint32_t *lastTime, uint32_t periodMs);

int16_t map(int16_t x, int16_t in_min, int16_t in_max,
            int16_t out_min, int16_t out_max);

#endif
