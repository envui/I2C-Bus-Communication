// UART0.h
// Runs on TM4C123
// Header file for UART0.c. Defines ASCII control character constants and
// declares all UART0 serial communication functions for PC terminal
// debugging at 115200 baud on PA0 (RX) and PA1 (TX).
// May 4, 2026

#ifndef UART0_H_
#define UART0_H_

#include <stdint.h>

//registers
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
