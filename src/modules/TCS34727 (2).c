// TCS34727.c
// Runs on TM4C123
// Provides TCS34727 RGB color sensor driver functions over I2C at address
// 0x29. Includes initialization with ID verification, raw 16-bit RGBC
// channel reading, clear-channel normalized RGB scaling, and dominant
// color detection returning RED, GREEN, BLUE, or NOTHING.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 4, 2027

#include <stdint.h>
#include "TCS34727.h"
#include "I2C.h"
#include "UART0.h"

/*
 * TCS34727.c
 * Runs on TM4C123GXL / TM4C123GH6PM
 * Color sensor driver using I2C.
 *
 * The color sensor uses I2C address 0x29.
 * The ID register is 0x12.
 * For TCS34727, the ID register should return 0x4D.
 */

static uint8_t TCS_Reg(uint8_t reg){
    /*
     * The color sensor needs bit 7 set when accessing registers.
     */
    return (uint8_t)(TCS34727_CMD | reg);
}

static uint16_t TCS_Read16(uint8_t lowReg){
    uint8_t low;
    uint8_t high;
    uint16_t data;

    /*
     * Color data is stored as low byte then high byte.
     */
    low = I2C_Receive(TCS34727_ADDR, TCS_Reg(lowReg));
    high = I2C_Receive(TCS34727_ADDR, TCS_Reg((uint8_t)(lowReg + 1)));

    data = ((uint16_t)high << 8) | low;

    return data;
}

uint8_t TCS34727_CheckID(void){
    uint8_t id;
////////////
    id = I2C_Receive(TCS34727_ADDR, TCS_Reg(TCS34727_ID_R_ADDR));

    if(id == TCS34727_ID){
        return 1;
    }

    return 0;
}

/*
 * -------------------TCS34727_Init------------------
 * Initializes the TCS34727 color sensor.
 * It also prints the I2C address, ID register, and part ID.
 * Input: none
 * Output: none
 */
void TCS34727_Init(void){
    uint8_t id;

    UART0_OutString("TCS34727 Init");
    UART0_OutCRLF();

    UART0_OutString("I2C Address: 0x29");
    UART0_OutCRLF();

    UART0_OutString("ID Register: 0x12");
    UART0_OutCRLF();

    UART0_OutString("Expected Part ID: 0x4D");
    UART0_OutCRLF();

    id = I2C_Receive(TCS34727_ADDR, TCS_Reg(TCS34727_ID_R_ADDR));

    UART0_OutString("Read Part ID: 0x");
    UART0_OutUHex(id);
    UART0_OutCRLF();

    if(id != TCS34727_ID){
        UART0_OutString("TCS34727 has not been Detected");
        UART0_OutCRLF();
        return;
    }

    UART0_OutString("TCS34727 has been Detected");
    UART0_OutCRLF();

    /*
     * Set integration time.
     */
    I2C_Transmit(TCS34727_ADDR,
                 TCS_Reg(TCS34727_TIMING_R_ADDR),
                 TCS34727_ATIME_2_4_MS);

    DELAY_1MS(3);

    /*
     * Set gain.
     */
    I2C_Transmit(TCS34727_ADDR,
                 TCS_Reg(TCS34727_CTRL_R_ADDR),
                 TCS34727_CTRL_AGAIN_1);

    /*
     * Power on sensor.
     */
    I2C_Transmit(TCS34727_ADDR,
                 TCS_Reg(TCS34727_ENABLE_R_ADDR),
                 TCS34727_ENABLE_PON);

    DELAY_1MS(3);

    /*
     * Enable RGBC sensing.
     */
    I2C_Transmit(TCS34727_ADDR,
                 TCS_Reg(TCS34727_ENABLE_R_ADDR),
                 TCS34727_ENABLE_PON | TCS34727_ENABLE_AEN);

    DELAY_1MS(3);

    UART0_OutString("TCS34727 Initialized");
    UART0_OutCRLF();
}

/*
 * ---------------TCS34727_GET_RAW_CLEAR-------------
 * Reads 16-bit clear data.
 * Input: none
 * Output: raw clear value
 */
uint16_t TCS34727_GET_RAW_CLEAR(void){
    uint16_t clearData;

    clearData = TCS_Read16(TCS34727_CDATAL_R_ADDR);

    DELAY_1MS(3);

    return clearData;
}

/*
 * ---------------TCS34727_GET_RAW_RED---------------
 * Reads 16-bit red data.
 * Input: none
 * Output: raw red value
 */
uint16_t TCS34727_GET_RAW_RED(void){
    uint16_t redData;

    redData = TCS_Read16(TCS34727_RDATAL_R_ADDR);

    DELAY_1MS(3);

    return redData;
}

/*
 * ---------------TCS34727_GET_RAW_GREEN-------------
 * Reads 16-bit green data.
 * Input: none
 * Output: raw green value
 */
uint16_t TCS34727_GET_RAW_GREEN(void){
    uint16_t greenData;

    greenData = TCS_Read16(TCS34727_GDATAL_R_ADDR);

    DELAY_1MS(3);

    return greenData;
}

/*
 * ---------------TCS34727_GET_RAW_BLUE--------------
 * Reads 16-bit blue data.
 * Input: none
 * Output: raw blue value
 */
uint16_t TCS34727_GET_RAW_BLUE(void){
    uint16_t blueData;

    blueData = TCS_Read16(TCS34727_BDATAL_R_ADDR);

    DELAY_1MS(3);

    return blueData;
}

/*
 * ---------------TCS34727_ReadAllRaw----------------
 * Reads clear, red, green, and blue raw values.
 * Input: RGB color struct pointer
 * Output: none
 */
void TCS34727_ReadAllRaw(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance){
    if(RGB_COLOR_Instance == 0){
        return;
    }

    RGB_COLOR_Instance->C_RAW = TCS34727_GET_RAW_CLEAR();
    RGB_COLOR_Instance->R_RAW = TCS34727_GET_RAW_RED();
    RGB_COLOR_Instance->G_RAW = TCS34727_GET_RAW_GREEN();
    RGB_COLOR_Instance->B_RAW = TCS34727_GET_RAW_BLUE();
}

/*
 * ---------------TCS34727_GET_RGB-------------------
 * Converts raw RGB values into scaled RGB values.
 * Input: RGB color struct pointer
 * Output: none
 */
void TCS34727_GET_RGB(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance){
    if(RGB_COLOR_Instance == 0){
        return;
    }

    TCS34727_ReadAllRaw(RGB_COLOR_Instance);

    /*
     * Clear is used as the total light amount.
     * Dividing by clear helps reduce brightness effects.
     */
    if(RGB_COLOR_Instance->C_RAW == 0){
        RGB_COLOR_Instance->R = 0.0f;
        RGB_COLOR_Instance->G = 0.0f;
        RGB_COLOR_Instance->B = 0.0f;
        return;
    }

    RGB_COLOR_Instance->R =
        ((float)RGB_COLOR_Instance->R_RAW / (float)RGB_COLOR_Instance->C_RAW) * 255.0f;

    RGB_COLOR_Instance->G =
        ((float)RGB_COLOR_Instance->G_RAW / (float)RGB_COLOR_Instance->C_RAW) * 255.0f;

    RGB_COLOR_Instance->B =
        ((float)RGB_COLOR_Instance->B_RAW / (float)RGB_COLOR_Instance->C_RAW) * 255.0f;

    if(RGB_COLOR_Instance->R > 255.0f){
        RGB_COLOR_Instance->R = 255.0f;
    }

    if(RGB_COLOR_Instance->G > 255.0f){
        RGB_COLOR_Instance->G = 255.0f;
    }

    if(RGB_COLOR_Instance->B > 255.0f){
        RGB_COLOR_Instance->B = 255.0f;
    }
}

/*
 * -----------------Detect_Color---------------------
 * Chooses the largest RGB value as the detected color.
 * Input: RGB color struct pointer
 * Output: detected color
 */
COLOR_DETECTED Detect_Color(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance){
    if(RGB_COLOR_Instance == 0){
        return NOTHING_DETECT;
    }

    if(RGB_COLOR_Instance->R < 20.0f &&
       RGB_COLOR_Instance->G < 20.0f &&
       RGB_COLOR_Instance->B < 20.0f){
        return NOTHING_DETECT;
    }

    if(RGB_COLOR_Instance->R > RGB_COLOR_Instance->G &&
       RGB_COLOR_Instance->R > RGB_COLOR_Instance->B){
        return RED_DETECT;
    }

    if(RGB_COLOR_Instance->G > RGB_COLOR_Instance->R &&
       RGB_COLOR_Instance->G > RGB_COLOR_Instance->B){
        return GREEN_DETECT;
    }

    if(RGB_COLOR_Instance->B > RGB_COLOR_Instance->R &&
       RGB_COLOR_Instance->B > RGB_COLOR_Instance->G){
        return BLUE_DETECT;
    }

    return NOTHING_DETECT;
}
