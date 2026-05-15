// wtimer.c
// Runs on TM4C123
// Provides interrupt-driven millisecond timing using WTIMER1 in periodic mode.
// WTIMER_Init() configures a 1ms tick via WideTimer1A_Handler(), which
// maintains a running millisecond counter and toggles PB1 at 1 Hz for
// timing verification. Includes DELAY_1MS(), WTIMER_Millis(),
// WTIMER_HasElapsed(), and a linear range map() utility.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 4, 2026


#include <stdint.h>
#include "wtimer.h"

// ----------- System control and GPIO register definitions -----------
#define SYSCTL_RCGCWTIMER_R    (*((volatile uint32_t *)0x400FE65C)) // Enable clock for wide timers
#define SYSCTL_PRWTIMER_R      (*((volatile uint32_t *)0x400FEA5C)) // Peripheral ready status

#define SYSCTL_RCGCGPIO_R      (*((volatile uint32_t *)0x400FE608)) // Enable clock for GPIO
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08)) // GPIO ready status

// ----------- GPIO Port B registers (used for PB1 toggle) -----------
#define GPIO_PORTB_DATA_R      (*((volatile uint32_t *)0x400053FC)) // Data register
#define GPIO_PORTB_DIR_R       (*((volatile uint32_t *)0x40005400)) // Direction register
#define GPIO_PORTB_AFSEL_R     (*((volatile uint32_t *)0x40005420)) // Alternate function select
#define GPIO_PORTB_DEN_R       (*((volatile uint32_t *)0x4000551C)) // Digital enable
#define GPIO_PORTB_AMSEL_R     (*((volatile uint32_t *)0x40005528)) // Analog disable
#define GPIO_PORTB_PCTL_R      (*((volatile uint32_t *)0x4000552C)) // Port control

// ----------- Wide Timer1 registers -----------
#define WTIMER1_CFG_R          (*((volatile uint32_t *)0x40037000)) // Configuration register
#define WTIMER1_TAMR_R         (*((volatile uint32_t *)0x40037004)) // Timer A mode
#define WTIMER1_CTL_R          (*((volatile uint32_t *)0x4003700C)) // Control register
#define WTIMER1_IMR_R          (*((volatile uint32_t *)0x40037018)) // Interrupt mask
#define WTIMER1_ICR_R          (*((volatile uint32_t *)0x40037024)) // Interrupt clear
#define WTIMER1_TAILR_R        (*((volatile uint32_t *)0x40037028)) // Interval load
#define WTIMER1_TAPR_R         (*((volatile uint32_t *)0x40037038)) // Prescaler

// ----------- NVIC interrupt control registers -----------
#define NVIC_EN3_R             (*((volatile uint32_t *)0xE000E10C)) // Enable interrupt
#define NVIC_PRI24_R           (*((volatile uint32_t *)0xE000E460)) // Set priority

// ----------- Constants and bit masks -----------
#define PB1                    0x02        // Pin mask for PB1
#define GPIOB_CLOCK            0x02        // Clock mask for Port B

#define TIMER_1MS_RELOAD       15999       // Reload value for 1 ms tick
#define WTIMER1A_TIMEOUT       0x01        // Timeout interrupt flag
#define NVIC_EN3_WTIMER1A      0x00000001  // NVIC enable bit

// ----------- Debug and timing globals -----------
volatile uint32_t WTIMER_Ready_Debug = 0; // Indicates timer initialized successfully

static volatile uint32_t g_msTicks = 0;   // Millisecond counter
static volatile uint16_t g_pb1Count = 0;  // Counter for PB1 toggle timing

// ----------- Initialize PB1 as digital output -----------
static void PB1_Init(void){
    SYSCTL_RCGCGPIO_R |= GPIOB_CLOCK;               // Enable Port B clock
    while((SYSCTL_PRGPIO_R & GPIOB_CLOCK) == 0){}   // Wait until ready

    GPIO_PORTB_AMSEL_R &= ~PB1; // Disable analog
    GPIO_PORTB_PCTL_R &= ~0x000000F0; // Clear PCTL for PB1
    GPIO_PORTB_DIR_R |= PB1; // Set PB1 as output
    GPIO_PORTB_AFSEL_R &= ~PB1; // Disable alternate function
    GPIO_PORTB_DEN_R |= PB1; // Enable digital function
    GPIO_PORTB_DATA_R &= ~PB1; // Initialize LOW
}

// ----------- Initialize Wide Timer1 for 1ms interrupts -----------
void WTIMER_Init(void){
    volatile uint32_t delay;
    uint32_t timeout = 1000000;

    PB1_Init(); // Initialize debug pin

    SYSCTL_RCGCWTIMER_R |= EN_WTIMER_CLOCK; // Enable timer clock

    // Small delay to allow clock to stabilize
    delay = SYSCTL_RCGCWTIMER_R;
    delay = SYSCTL_RCGCWTIMER_R;
    delay = SYSCTL_RCGCWTIMER_R;

    // Wait until timer peripheral is ready (with timeout)
    while(((SYSCTL_PRWTIMER_R & EN_WTIMER_CLOCK) == 0) && (timeout > 0)){
        timeout--;
    }

    // Set debug flag based on readiness
    if((SYSCTL_PRWTIMER_R & EN_WTIMER_CLOCK) != 0){
        WTIMER_Ready_Debug = 1;
    }
    else{
        WTIMER_Ready_Debug = 0;
    }

    // Configure timer
    WTIMER1_CTL_R &= ~WTIMER_TAEN_BIT; // Disable Timer A during setup
    WTIMER1_CFG_R = WTIMER_32_BIT_CFG; // 32-bit mode
    WTIMER1_TAMR_R = 0x02;             // Periodic mode
    WTIMER1_TAPR_R = PRESCALER_VALUE;  // Prescaler
    WTIMER1_TAILR_R = TIMER_1MS_RELOAD; // Load for 1ms
    WTIMER1_ICR_R = WTIMER1A_TIMEOUT;  // Clear interrupt flag
    WTIMER1_IMR_R |= WTIMER1A_TIMEOUT; // Enable timeout interrupt

    // Configure NVIC
    NVIC_PRI24_R = (NVIC_PRI24_R & 0xFFFFFF00) | 0x00000040; // Set priority
    NVIC_EN3_R |= NVIC_EN3_WTIMER1A; // Enable interrupt in NVIC

    WTIMER1_CTL_R |= WTIMER_TAEN_BIT; // Enable Timer A
}

// ----------- Return current millisecond count -----------
uint32_t WTIMER_Millis(void){
    return g_msTicks;
}

// ----------- Check if a time period has elapsed -----------
uint8_t WTIMER_HasElapsed(uint32_t *lastTime, uint32_t periodMs){
    uint32_t now;

    now = WTIMER_Millis();

    // Check elapsed time using unsigned wrap-safe subtraction
    if((now - *lastTime) >= periodMs){
        *lastTime = now; // Update last time
        return 1;
    }

    return 0;
}

// ----------- Blocking delay in milliseconds -----------
void DELAY_1MS(uint32_t delay){
    uint32_t start;

    start = WTIMER_Millis();

    // Busy-wait until desired time passes
    while((WTIMER_Millis() - start) < delay){}
}

// ----------- Map value from one range to another -----------
int16_t map(int16_t x, int16_t in_min, int16_t in_max,
            int16_t out_min, int16_t out_max){
    return (int16_t)((int32_t)(x - in_min) * (out_max - out_min) /
                    (in_max - in_min) + out_min);
}

// ----------- Timer interrupt handler (fires every 1 ms) -----------
void WideTimer1A_Handler(void){
    WTIMER1_ICR_R = WTIMER1A_TIMEOUT; // Clear interrupt flag

    g_msTicks++;   // Increment millisecond counter
    g_pb1Count++;  // Increment toggle counter

    // Toggle PB1 every 500 ms (1 Hz square wave)
    if(g_pb1Count >= 500){
        GPIO_PORTB_DATA_R ^= PB1;
        g_pb1Count = 0;
    }
}
