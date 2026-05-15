// UART0.c
// Runs on TM4C123
// Provides UART0 serial communication functions at 115200 baud using
// PA0 (RX) and PA1 (TX). Includes character, string, signed/unsigned
// integer, float, and hexadecimal output functions, along with string
// input with backspace handling for PC terminal debugging.
// May 4, 2026


#include <stdint.h>
#include "UART0.h"

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

void UART0_Init(void){
    // Enable the UART0 peripheral clock before configuring UART registers.
    SYSCTL_RCGCUART_R |= UART0_CLOCK;
    // Enable GPIO Port A because PA0 and PA1 are used for UART0.
    SYSCTL_RCGCGPIO_R |= PORTA_CLOCK;
    // Wait until Port A is ready before accessing its registers.
    while((SYSCTL_PRGPIO_R & PORTA_CLOCK) == 0){}

    // Disable UART0 during configuration to prevent unwanted behavior.
    UART0_CTL_R &= ~0x01;

    // Integer baud-rate divisor for 115200 baud with a 16 MHz clock.
    UART0_IBRD_R = 8;
    // Fractional baud-rate divisor for 115200 baud.
    UART0_FBRD_R = 44;
    // Configure UART for 8-bit data, FIFO enabled, no parity.
    UART0_LCRH_R = 0x70;
    // Use the system clock as the UART clock source.
    UART0_CC_R = 0x00;
    // Enable UART0 along with transmit and receive.
    UART0_CTL_R = 0x0301;
    // Enable alternate function mode on PA0 and PA1.
    GPIO_PORTA_AFSEL_R |= 0x03;
    // Enable digital functionality on UART pins.
    GPIO_PORTA_DEN_R |= 0x03;
    // Select UART function for PA0 and PA1.
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0x000000FF) | 0x00000011;
    // Disable analog mode on UART pins.
    GPIO_PORTA_AMSEL_R &= ~0x03;
}

void UART0_OutCRLF(void){
    // Send carriage return before line feed for terminal formatting.
    UART0_OutChar(UART_CR);
    UART0_OutChar(UART_LF);
}

unsigned char UART0_InChar(void){
    // Wait until a character is available in the RX FIFO.
    while((UART0_FR_R & 0x10) != 0){}
    // Mask data register to return only the received byte.
    return (unsigned char)(UART0_DR_R & 0xFF);
}

void UART0_OutChar(char data){
    // Wait until TX FIFO has space before sending.
    while((UART0_FR_R & 0x20) != 0){}
    // Write one byte to UART data register.
    UART0_DR_R = data;
}

void UART0_OutString(char *pt){
    // Send characters until the null terminator is reached.
    while(*pt){
        UART0_OutChar(*pt);
        pt++;
    }
}

void UART0_InString(char *bufPt, unsigned short max){
    int length = 0;
    char character;

    character = UART0_InChar();
    
    // Store input characters until Enter is pressed.
    while(character != UART_CR){
        // Support backspace and delete while entering text.
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
            // Echo valid input characters back to the terminal.
            UART0_OutChar(character);
        }

        character = UART0_InChar();
    }

    // Null-terminate the received input string.
    *bufPt = 0;
    UART0_OutCRLF();
}

void UART0_OutUDec(uint32_t n){
    // Recursively print higher decimal digits first.
    if(n >= 10){
        UART0_OutUDec(n / 10);
    }

    // Convert digit value to ASCII before output.
    UART0_OutChar((char)((n % 10) + '0'));
}

void UART0_OutSDec(int32_t n){
    if(n < 0){
        // Print negative sign before converting signed value.
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
    // Output the integer portion of the floating-point number before decimals.
    UART0_OutUDec((uint32_t)integerPart);

    // If no decimal places are requested, skip fractional output.
    if(decimals == 0){
        return;
    }

    UART0_OutChar('.');

    scale = 1;
    for(i = 0; i < decimals; i++){
        // Scale fractional part based on requested decimal places.
        scale *= 10;
    }

    fractionalPart = (uint32_t)((number - (float)integerPart) * (float)scale + 0.5f);

    if(fractionalPart >= scale){
        fractionalPart = scale - 1;
    }

    scale /= 10;

    // Output fractional digits from most significant to least significant.
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
