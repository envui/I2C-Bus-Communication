// Servo.c
// Runs on TM4C123
// Provides PWM-based servo motor control using M0PWM1 on PB7 at 50 Hz.
// Servo_Init() configures the PWM clock divider and generator for a 20ms
// period. Servo_SetAngle() maps an angle (-90 to +90 degrees) to the
// corresponding pulse width (1.0ms to 2.0ms) and updates the compare register.


#include <stdint.h>
#include "Servo.h"

//registers
#define SYSCTL_RCGCPWM_R        (*((volatile uint32_t *)0x400FE640))
#define SYSCTL_RCGCGPIO_R       (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R         (*((volatile uint32_t *)0x400FEA08))
#define SYSCTL_RCC_R            (*((volatile uint32_t *)0x400FE060))

#define GPIO_PORTB_AFSEL_R      (*((volatile uint32_t *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile uint32_t *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile uint32_t *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile uint32_t *)0x4000552C))
#define GPIO_PORTB_DR8R_R       (*((volatile uint32_t *)0x40005508))

#define PWM0_0_CTL_R            (*((volatile uint32_t *)0x40028040))
#define PWM0_0_GENB_R           (*((volatile uint32_t *)0x40028064))
#define PWM0_0_LOAD_R           (*((volatile uint32_t *)0x40028050))
#define PWM0_0_CMPB_R           (*((volatile uint32_t *)0x4002805C))
#define PWM0_ENABLE_R           (*((volatile uint32_t *)0x40028008))

#define PWM0_CLOCK     0x01
#define PORTB_CLOCK    0x02

void Servo_Init(void){
    SYSCTL_RCGCPWM_R |= PWM0_CLOCK;
    SYSCTL_RCGCGPIO_R |= PORTB_CLOCK;
    while((SYSCTL_PRGPIO_R & PORTB_CLOCK) == 0){}

    GPIO_PORTB_AFSEL_R |= PWM0_PIN;
    GPIO_PORTB_DEN_R |= PWM0_PIN;
    GPIO_PORTB_AMSEL_R &= ~PWM0_PIN;
    GPIO_PORTB_DR8R_R |= PWM0_PIN;

    /*
     * PB7 = M0PWM1.
     */
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0xF0000000) | 0x40000000;

    /*
     * Enable PWM clock divider.
     * USEPWMDIV = bit 20.
     * PWMDIV = 0x5 gives /64.
     */
    SYSCTL_RCC_R |= 0x00100000;
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~0x000E0000) | 0x000A0000;

    PWM0_0_CTL_R = 0x00000000;

    /*
     * Generator B:
     * Output high on LOAD.
     * Output low on CMPB while counting down.
     */
    PWM0_0_GENB_R = 0x0000080C;

    PWM0_0_LOAD_R = PWM0_COUNTER;
    PWM0_0_CMPB_R = PWM0_COUNTER - SERVO_CENTER_CNT;

    PWM0_0_CTL_R |= 0x01;
    PWM0_ENABLE_R |= 0x02;
}

void Servo_SetAngle(int16_t angle){
    uint16_t pulse;

    if(angle < SERVO_MIN_ANGLE){
        angle = SERVO_MIN_ANGLE;
    }

    if(angle > SERVO_MAX_ANGLE){
        angle = SERVO_MAX_ANGLE;
    }

    pulse = (uint16_t)map(angle,
                          SERVO_MIN_ANGLE,
                          SERVO_MAX_ANGLE,
                          SERVO_MIN_CNT,
                          SERVO_MAX_CNT);

    PWM0_0_CMPB_R = PWM0_COUNTER - pulse;
}

void Drive_Servo(int16_t angle){
    Servo_SetAngle(angle);
}
