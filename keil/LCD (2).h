// LCD.h
// Runs on TM4C123
// Header file for LCD.c. Defines PCF8574T I2C backpack pin mappings,
// LCD command constants for 4-bit mode operation, and declares all
// display control and output functions for the 16x2 LCD at address 0x27.
// May 13, 2026

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

/*
 * LCD.h
 * Runs on TM4C123GXL / TM4C123GH6PM
 * 16x2 LCD using I2C backpack through PCF8574.
 */

#define LCD_ADDR              0x27

/*
 * Common PCF8574 LCD backpack bit mapping:
 * P0 = RS
 * P1 = RW
 * P2 = EN
 * P3 = Backlight
 * P4 = D4
 * P5 = D5
 * P6 = D6
 * P7 = D7
 */
#define LCD_RS                0x01
#define LCD_RW                0x02
#define LCD_EN                0x04
#define LCD_BACKLIGHT         0x08

#define LCD_CMD_MODE          0x00
#define LCD_DATA_MODE         LCD_RS

#define LCD_CLEAR_DISPLAY     0x01
#define LCD_RETURN_HOME       0x02
#define LCD_ENTRY_MODE        0x06
#define LCD_DISPLAY_ON        0x0C
#define LCD_FUNCTION_SET      0x28
#define LCD_SET_DDRAM_ADDR    0x80

// Prepare the LCD module for displaying characters and sensor data.
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_PrintText(char *text);
void LCD_PrintChar(char data);
void LCD_PrintInteger(int32_t number);

void LCD_StartupMessage(char *firstName, char *lastName);

#endif
