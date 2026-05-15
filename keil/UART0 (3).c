#include <stdint.h>
#include "UART0.h"

/*
 * UART0.c
 * Runs on TM4C123GXL / TM4C123GH6PM
 * UART0 uses PA0 RX and PA1 TX.
 *
 * System clock: 16 MHz, no PLL.
 * Baud rate: 115200.
 */

#define SYSCTL_RCGCUART_R      (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_RCGCGPIO_R      (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08))

#define GPIO_PORTA_AFSEL_R     (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_DEN_R       (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTA_AMSEL_R     (*((volatile uint32_t *)0x40004528))
#define GPIO_PORTA_PCTL_R      (*((volatile uint32_t *)0x4000452C))

#define UART0_DR_R             (*((volatile uint32_t *)0x4000C000))
#define UART0_FR_R             (*((volatile uint32_t *)0x4000C018))
#define UART0_IBRD_R           (*((volatile uint32_t *)0x4000C024))
#define UART0_FBRD_R           (*((volatile uint32_t *)0x4000C028))
#define UART0_LCRH_R           (*((volatile uint32_t *)0x4000C02C))
#define UART0_CTL_R            (*((volatile uint32_t *)0x4000C030))
#define UART0_CC_R             (*((volatile uint32_t *)0x4000CFC8))

#define UART0_CLOCK            0x01
#define PORTA_CLOCK            0x01

// Initialize UART0 for serial communication with the PC terminal.
void UART0_Init(void){
    SYSCTL_RCGCUART_R |= UART0_CLOCK;
    SYSCTL_RCGCGPIO_R |= PORTA_CLOCK;
    while((SYSCTL_PRGPIO_R & PORTA_CLOCK) == 0){}

    UART0_CTL_R &= ~0x01;

    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x70;
    UART0_CC_R = 0x00;
    UART0_CTL_R = 0x0301;

    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_DEN_R |= 0x03;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0x000000FF) | 0x00000011;
    GPIO_PORTA_AMSEL_R &= ~0x03;
}

void UART0_OutCRLF(void){
    UART0_OutChar(UART_CR);
    UART0_OutChar(UART_LF);
}

unsigned char UART0_InChar(void){
    while((UART0_FR_R & 0x10) != 0){}
    return (unsigned char)(UART0_DR_R & 0xFF);
}

// Send one character through the UART transmit register.
void UART0_OutChar(char data){
    while((UART0_FR_R & 0x20) != 0){}
    UART0_DR_R = data;
}

void UART0_OutString(char *pt){
    while(*pt){
        UART0_OutChar(*pt);
        pt++;
    }
}

void UART0_InString(char *bufPt, unsigned short max){
    int length = 0;
    char character;

    character = UART0_InChar();

    while(character != UART_CR){
        if(character == UART_BS || character == UART_DEL){
            if(length){
                bufPt--;
                length--;
                UART0_OutChar(UART_BS);
                UART0_OutChar(UART_SP);
                UART0_OutChar(UART_BS);
            }
        }
        else if(length < max - 1){
            *bufPt = character;
            bufPt++;
            length++;
            UART0_OutChar(character);
        }

        character = UART0_InChar();
    }

    *bufPt = 0;
    UART0_OutCRLF();
}

void UART0_OutUDec(uint32_t n){
    if(n >= 10){
        UART0_OutUDec(n / 10);
    }

    UART0_OutChar((char)((n % 10) + '0'));
}

void UART0_OutSDec(int32_t n){
    if(n < 0){
        UART0_OutChar('-');
        n = -n;
    }

    UART0_OutUDec((uint32_t)n);
}

void UART0_OutFloat(float number, uint8_t decimals){
    int32_t integerPart;
    uint32_t fractionalPart;
    uint32_t scale;
    uint8_t i;

    if(number < 0){
        UART0_OutChar('-');
        number = -number;
    }

    integerPart = (int32_t)number;
    UART0_OutUDec((uint32_t)integerPart);

    if(decimals == 0){
        return;
    }

    UART0_OutChar('.');

    scale = 1;
    for(i = 0; i < decimals; i++){
        scale *= 10;
    }

    fractionalPart = (uint32_t)((number - (float)integerPart) * (float)scale + 0.5f);

    if(fractionalPart >= scale){
        fractionalPart = scale - 1;
    }

    scale /= 10;

    while(scale > 0){
        UART0_OutChar((char)((fractionalPart / scale) + '0'));
        fractionalPart %= scale;
        scale /= 10;
    }
}

void UART0_OutUHex(uint32_t n){
    uint8_t digit;

    if(n >= 16){
        UART0_OutUHex(n / 16);
    }

    digit = (uint8_t)(n % 16);

    if(digit < 10){
        UART0_OutChar((char)(digit + '0'));
    }
    else{
        UART0_OutChar((char)(digit - 10 + 'A'));
    }
}
