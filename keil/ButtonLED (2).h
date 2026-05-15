// ButtonLED.h
// Runs on TM4C123
// Header file for ButtonLED.c. Defines pin masks for the onboard RGB LEDs
// (PF1-PF3) and push buttons SW1 (PF4) and SW2 (PF0), and declares all
// LED and button interface functions.
// May 13, 2026\
#ifndef BUTTONLED_H_
#define BUTTONLED_H_

#include <stdint.h>



#define LED_RED     0x02
#define LED_BLUE    0x04
#define LED_GREEN   0x08
#define LED_ALL     0x0E

#define SW2         0x01
#define SW1         0x10

void LED_Init(void);
void LED_Out(uint8_t led);
void LED_Off(void);
void LED_CycleNext(void);
uint8_t LED_GetCurrent(void);

void BTN_Init(void);
uint8_t BTN_SW1Pressed(void);
uint8_t BTN_SW2Pressed(void);
void BTN_ClearSW1Event(void);
void BTN_ClearSW2Event(void);
void BTN_ClearEvents(void);

#endif
