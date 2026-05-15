// LCD.c
// Runs on TM4C123
// Provides 16x2 LCD control functions via I2C backpack (PCF8574T) at
// address 0x27 using 4-bit mode. Includes initialization, cursor
// positioning, character and string output, integer printing, and a
// startup message sequence displaying first and last name on each row.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 13, 2026

#include <stdint.h>
#include "LCD.h"
#include "I2C.h"
#include "wtimer.h"

/*
 * LCD.c
 * Runs on TM4C123GXL / TM4C123GH6PM
 * 16x2 LCD using I2C backpack through PCF8574.
 */

// Send a 4-bit value to the LCD through the I2C backpack.
static void LCD_Write4Bits(uint8_t data);
static void LCD_SendNibble(uint8_t nibble, uint8_t mode);
static void LCD_SendByte(uint8_t data, uint8_t mode);
static void LCD_Command(uint8_t command);

static void LCD_Write4Bits(uint8_t data){
    I2C_SendByte(LCD_ADDR, data | LCD_BACKLIGHT);
    DELAY_1MS(1);

    I2C_SendByte(LCD_ADDR, data | LCD_BACKLIGHT | LCD_EN);
    DELAY_1MS(1);

    I2C_SendByte(LCD_ADDR, data | LCD_BACKLIGHT);
    DELAY_1MS(1);
}

static void LCD_SendNibble(uint8_t nibble, uint8_t mode){
    uint8_t data;

    data = (uint8_t)((nibble & 0x0F) << 4);
    data |= mode;
    data &= ~LCD_RW;

    LCD_Write4Bits(data);
}

static void LCD_SendByte(uint8_t data, uint8_t mode){
    LCD_SendNibble((uint8_t)(data >> 4), mode);
    LCD_SendNibble((uint8_t)(data & 0x0F), mode);
}

static void LCD_Command(uint8_t command){
    LCD_SendByte(command, LCD_CMD_MODE);

    if(command == LCD_CLEAR_DISPLAY || command == LCD_RETURN_HOME){
        DELAY_1MS(2);
    }
    else{
        DELAY_1MS(1);
    }
}

/*
 * -------------------LCD_Init------------------
 * Initializes the 16x2 LCD in 4-bit mode through PCF8574 backpack.
 * Input: none
 * Output: none
 */

// Initialize the LCD display for 4-bit communication mode.
void LCD_Init(void){
    DELAY_1MS(50);

    /*
     * LCD 4-bit initialization sequence.
     */
    LCD_SendNibble(0x03, LCD_CMD_MODE);
    DELAY_1MS(5);

    LCD_SendNibble(0x03, LCD_CMD_MODE);
    DELAY_1MS(5);

    LCD_SendNibble(0x03, LCD_CMD_MODE);
    DELAY_1MS(1);

    LCD_SendNibble(0x02, LCD_CMD_MODE);
    DELAY_1MS(1);

    LCD_Command(LCD_FUNCTION_SET);
    LCD_Command(LCD_DISPLAY_ON);
    LCD_Command(LCD_CLEAR_DISPLAY);
    LCD_Command(LCD_ENTRY_MODE);
}

/*
 * -------------------LCD_Clear------------------
 * Clears the LCD display and returns cursor home.
 * Input: none
 * Output: none
 */
void LCD_Clear(void){
    LCD_Command(LCD_CLEAR_DISPLAY);
}

/*
 * -------------------LCD_SetCursor------------------
 * Sets cursor to selected row and column.
 * Input: row 0-1, col 0-15
 * Output: none
 */
void LCD_SetCursor(uint8_t row, uint8_t col){
    uint8_t address;

    if(row > 1){
        row = 1;
    }

    if(col > 15){
        col = 15;
    }

    if(row == 0){
        address = (uint8_t)(0x00 + col);
    }
    else{
        address = (uint8_t)(0x40 + col);
    }

    LCD_Command((uint8_t)(LCD_SET_DDRAM_ADDR | address));
}

/*
 * -------------------LCD_PrintChar------------------
 * Prints one character at the current cursor position.
 * Input: character
 * Output: none
 */
void LCD_PrintChar(char data){
    LCD_SendByte((uint8_t)data, LCD_DATA_MODE);
}

/*
 * -------------------LCD_PrintText------------------
 * Prints a null-terminated string.
 * Input: string pointer
 * Output: none
 */
void LCD_PrintText(char *text){
    while(*text){
        LCD_PrintChar(*text);
        text++;
    }
}

/*
 * -------------------LCD_PrintInteger------------------
 * Prints a signed decimal integer.
 * Input: signed integer
 * Output: none
 */
void LCD_PrintInteger(int32_t number){
    char buffer[12];
    int8_t i;
    uint8_t negative;

    negative = 0;
    i = 0;

    if(number == 0){
        LCD_PrintChar('0');
        return;
    }

    if(number < 0){
        negative = 1;
        number = -number;
    }

    while(number > 0 && i < 11){
        buffer[i] = (char)((number % 10) + '0');
        number /= 10;
        i++;
    }

    if(negative){
        LCD_PrintChar('-');
    }

    while(i > 0){
        i--;
        LCD_PrintChar(buffer[i]);
    }
}

/*
 * -------------------LCD_StartupMessage------------------
 * Displays first name on row 1, last name on row 2, then clears.
 * Input: first name string, last name string
 * Output: none
 */
void LCD_StartupMessage(char *firstName, char *lastName){
    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_PrintText(firstName);

    DELAY_1MS(1000);

    LCD_SetCursor(1, 0);
    LCD_PrintText(lastName);

    DELAY_1MS(1000);

    LCD_Clear();
}
