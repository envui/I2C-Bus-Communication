#include <stdint.h>
#include "wtimer.h"



#define SYSCTL_RCGCWTIMER_R    (*((volatile uint32_t *)0x400FE65C))
#define SYSCTL_PRWTIMER_R      (*((volatile uint32_t *)0x400FEA5C))

#define SYSCTL_RCGCGPIO_R      (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08))

#define GPIO_PORTB_DATA_R      (*((volatile uint32_t *)0x400053FC))
#define GPIO_PORTB_DIR_R       (*((volatile uint32_t *)0x40005400))
#define GPIO_PORTB_AFSEL_R     (*((volatile uint32_t *)0x40005420))
#define GPIO_PORTB_DEN_R       (*((volatile uint32_t *)0x4000551C))
#define GPIO_PORTB_AMSEL_R     (*((volatile uint32_t *)0x40005528))
#define GPIO_PORTB_PCTL_R      (*((volatile uint32_t *)0x4000552C))

#define WTIMER1_CFG_R          (*((volatile uint32_t *)0x40037000))
#define WTIMER1_TAMR_R         (*((volatile uint32_t *)0x40037004))
#define WTIMER1_CTL_R          (*((volatile uint32_t *)0x4003700C))
#define WTIMER1_IMR_R          (*((volatile uint32_t *)0x40037018))
#define WTIMER1_ICR_R          (*((volatile uint32_t *)0x40037024))
#define WTIMER1_TAILR_R        (*((volatile uint32_t *)0x40037028))
#define WTIMER1_TAPR_R         (*((volatile uint32_t *)0x40037038))

#define NVIC_EN3_R             (*((volatile uint32_t *)0xE000E10C))
#define NVIC_PRI24_R           (*((volatile uint32_t *)0xE000E460))

#define PB1                    0x02
#define GPIOB_CLOCK            0x02

#define TIMER_1MS_RELOAD       15999
#define WTIMER1A_TIMEOUT       0x01
#define NVIC_EN3_WTIMER1A      0x00000001

volatile uint32_t WTIMER_Ready_Debug = 0;

static volatile uint32_t g_msTicks = 0;
static volatile uint16_t g_pb1Count = 0;

static void PB1_Init(void){
    SYSCTL_RCGCGPIO_R |= GPIOB_CLOCK;
    while((SYSCTL_PRGPIO_R & GPIOB_CLOCK) == 0){}

    GPIO_PORTB_AMSEL_R &= ~PB1;
    GPIO_PORTB_PCTL_R &= ~0x000000F0;
    GPIO_PORTB_DIR_R |= PB1;
    GPIO_PORTB_AFSEL_R &= ~PB1;
    GPIO_PORTB_DEN_R |= PB1;
    GPIO_PORTB_DATA_R &= ~PB1;
}

// Initialize the wide timer module for timing measurements.
void WTIMER_Init(void){
    volatile uint32_t delay;
    uint32_t timeout = 1000000;

    PB1_Init();

    SYSCTL_RCGCWTIMER_R |= EN_WTIMER_CLOCK;

    delay = SYSCTL_RCGCWTIMER_R;
    delay = SYSCTL_RCGCWTIMER_R;
    delay = SYSCTL_RCGCWTIMER_R;

    while(((SYSCTL_PRWTIMER_R & EN_WTIMER_CLOCK) == 0) && (timeout > 0)){
        timeout--;
    }

    if((SYSCTL_PRWTIMER_R & EN_WTIMER_CLOCK) != 0){
        WTIMER_Ready_Debug = 1;
    }
    else{
        WTIMER_Ready_Debug = 0;
    }

    WTIMER1_CTL_R &= ~WTIMER_TAEN_BIT;
    WTIMER1_CFG_R = WTIMER_32_BIT_CFG;
    WTIMER1_TAMR_R = 0x02;             
    WTIMER1_TAPR_R = PRESCALER_VALUE;
    WTIMER1_TAILR_R = TIMER_1MS_RELOAD;
    WTIMER1_ICR_R = WTIMER1A_TIMEOUT;
    WTIMER1_IMR_R |= WTIMER1A_TIMEOUT;

 
    NVIC_PRI24_R = (NVIC_PRI24_R & 0xFFFFFF00) | 0x00000040;
    NVIC_EN3_R |= NVIC_EN3_WTIMER1A;

    WTIMER1_CTL_R |= WTIMER_TAEN_BIT;
}

uint32_t WTIMER_Millis(void){
    return g_msTicks;
}

uint8_t WTIMER_HasElapsed(uint32_t *lastTime, uint32_t periodMs){
    uint32_t now;

    now = WTIMER_Millis();

    if((now - *lastTime) >= periodMs){
        *lastTime = now;
        return 1;
    }

    return 0;
}

void DELAY_1MS(uint32_t delay){
    uint32_t start;

    start = WTIMER_Millis();

    while((WTIMER_Millis() - start) < delay){}
}

int16_t map(int16_t x, int16_t in_min, int16_t in_max,
            int16_t out_min, int16_t out_max){
    return (int16_t)((int32_t)(x - in_min) * (out_max - out_min) /
                    (in_max - in_min) + out_min);
}

// Interrupt handler executed whenever the timer capture event occurs.
void WideTimer1A_Handler(void){
    WTIMER1_ICR_R = WTIMER1A_TIMEOUT;

    g_msTicks++;
    g_pb1Count++;

    if(g_pb1Count >= 500){
        GPIO_PORTB_DATA_R ^= PB1;
        g_pb1Count = 0;
    }
}
