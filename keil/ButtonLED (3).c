// ButtonLED.c
// Runs on TM4C123
// Provides initialization and control functions for the onboard RGB LEDs
// and push buttons on Port F. LED_Init() configures PF1-PF3 as digital
// outputs for the RGB LED with cycling support. BTN_Init() configures
// PF0 (SW2) and PF4 (SW1) as interrupt-driven inputs with pull-up resistors
// and 50ms software debouncing via GPIOPortF_Handler().
// May 13, 2026

#include <stdint.h>
#include "ButtonLED.h"
#include "wtimer.h"


#define SYSCTL_RCGCGPIO_R      (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08))

#define GPIO_PORTF_DATA_R      (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_DIR_R       (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_AFSEL_R     (*((volatile uint32_t *)0x40025420))
#define GPIO_PORTF_PUR_R       (*((volatile uint32_t *)0x40025510))
#define GPIO_PORTF_DEN_R       (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_LOCK_R      (*((volatile uint32_t *)0x40025520))
#define GPIO_PORTF_CR_R        (*((volatile uint32_t *)0x40025524))
#define GPIO_PORTF_AMSEL_R     (*((volatile uint32_t *)0x40025528))
#define GPIO_PORTF_PCTL_R      (*((volatile uint32_t *)0x4002552C))

#define GPIO_PORTF_IS_R        (*((volatile uint32_t *)0x40025404))
#define GPIO_PORTF_IBE_R       (*((volatile uint32_t *)0x40025408))
#define GPIO_PORTF_IEV_R       (*((volatile uint32_t *)0x4002540C))
#define GPIO_PORTF_IM_R        (*((volatile uint32_t *)0x40025410))
#define GPIO_PORTF_RIS_R       (*((volatile uint32_t *)0x40025414))
#define GPIO_PORTF_ICR_R       (*((volatile uint32_t *)0x4002541C))

#define NVIC_EN0_R             (*((volatile uint32_t *)0xE000E100))
#define NVIC_PRI7_R            (*((volatile uint32_t *)0xE000E41C))

#define PORTF_CLOCK            0x20
#define NVIC_EN0_PORTF         0x40000000
#define GPIO_LOCK_KEY          0x4C4F434B
#define DEBOUNCE_MS            50

static volatile uint8_t SW1_PressedFlag = 0;
static volatile uint8_t SW2_PressedFlag = 0;

static volatile uint32_t SW1_LastTime = 0;
static volatile uint32_t SW2_LastTime = 0;

static uint8_t CurrentLED = LED_RED;

// Configure onboard RGB LED pins as digital outputs.
void LED_Init(void){
    SYSCTL_RCGCGPIO_R |= PORTF_CLOCK;
    while((SYSCTL_PRGPIO_R & PORTF_CLOCK) == 0){}

    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R |= LED_ALL;

    GPIO_PORTF_AMSEL_R &= ~LED_ALL;
    GPIO_PORTF_PCTL_R &= ~0x0000FFF0;
    GPIO_PORTF_DIR_R |= LED_ALL;
    GPIO_PORTF_AFSEL_R &= ~LED_ALL;
    GPIO_PORTF_DEN_R |= LED_ALL;

    GPIO_PORTF_DATA_R &= ~LED_ALL;
    CurrentLED = LED_RED;
}

void LED_Out(uint8_t led){
    GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~LED_ALL) | (led & LED_ALL);
}

void LED_Off(void){
    GPIO_PORTF_DATA_R &= ~LED_ALL;
}

// Advance to the next LED color in the sequence.
void LED_CycleNext(void){
    if(CurrentLED == LED_RED){
        CurrentLED = LED_GREEN;
    }
    else if(CurrentLED == LED_GREEN){
        CurrentLED = LED_BLUE;
    }
    else{
        CurrentLED = LED_RED;
    }

    LED_Out(CurrentLED);
}

uint8_t LED_GetCurrent(void){
    return CurrentLED;
}

// Configure push button inputs with pull-up resistors enabled.
void BTN_Init(void){
    SYSCTL_RCGCGPIO_R |= PORTF_CLOCK;
    while((SYSCTL_PRGPIO_R & PORTF_CLOCK) == 0){}

    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R |= (SW1 | SW2);

    GPIO_PORTF_AMSEL_R &= ~(SW1 | SW2);
    GPIO_PORTF_PCTL_R &= ~0x000F000F;
    GPIO_PORTF_DIR_R &= ~(SW1 | SW2);
    GPIO_PORTF_AFSEL_R &= ~(SW1 | SW2);
    GPIO_PORTF_PUR_R |= (SW1 | SW2);
    GPIO_PORTF_DEN_R |= (SW1 | SW2);

    GPIO_PORTF_IM_R &= ~(SW1 | SW2);
    GPIO_PORTF_IS_R &= ~(SW1 | SW2);
    GPIO_PORTF_IBE_R &= ~(SW1 | SW2);
    GPIO_PORTF_IEV_R &= ~(SW1 | SW2);
    GPIO_PORTF_ICR_R = (SW1 | SW2);
    GPIO_PORTF_IM_R |= (SW1 | SW2);

    NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000;
    NVIC_EN0_R |= NVIC_EN0_PORTF;
}

uint8_t BTN_SW1Pressed(void){
    if(SW1_PressedFlag){
        SW1_PressedFlag = 0;
        return 1;
    }

    return 0;
}

uint8_t BTN_SW2Pressed(void){
    if(SW2_PressedFlag){
        SW2_PressedFlag = 0;
        return 1;
    }

    return 0;
}

void BTN_ClearSW1Event(void){
    SW1_PressedFlag = 0;
}

void BTN_ClearSW2Event(void){
    SW2_PressedFlag = 0;
}

void BTN_ClearEvents(void){
    SW1_PressedFlag = 0;
    SW2_PressedFlag = 0;
}

void GPIOPortF_Handler(void){
    uint32_t status;
    uint32_t now;

    status = GPIO_PORTF_RIS_R;
    GPIO_PORTF_ICR_R = status;

    now = WTIMER_Millis();

    if(status & SW1){
        if((now - SW1_LastTime) >= DEBOUNCE_MS){
            if((GPIO_PORTF_DATA_R & SW1) == 0){
                SW1_PressedFlag = 1;
            }

            SW1_LastTime = now;
        }
    }

    if(status & SW2){
        if((now - SW2_LastTime) >= DEBOUNCE_MS){
            if((GPIO_PORTF_DATA_R & SW2) == 0){
                SW2_PressedFlag = 1;
            }

            SW2_LastTime = now;
        }
    }
}
