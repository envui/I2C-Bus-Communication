#ifndef UART0_H_
#define UART0_H_

#include <stdint.h>

/*
 * UART0.h
 * Runs on TM4C123GXL / TM4C123GH6PM
 * UART0 serial driver for PC terminal debugging.
 */

#define UART_CR   0x0D
#define UART_LF   0x0A
#define UART_BS   0x08
#define UART_DEL  0x7F
#define UART_SP   0x20

void UART0_Init(void);
void UART0_OutCRLF(void);

unsigned char UART0_InChar(void);
void UART0_OutChar(char data);

void UART0_OutString(char *pt);
void UART0_InString(char *bufPt, unsigned short max);

void UART0_OutUDec(uint32_t n);
void UART0_OutSDec(int32_t n);
void UART0_OutFloat(float number, uint8_t decimals);
void UART0_OutUHex(uint32_t n);

#endif