#ifndef TCS34727_H_
#define TCS34727_H_

#include <stdint.h>
#include "wtimer.h"

/*
 * TCS34727.h
 * Runs on TM4C123GXL / TM4C123GH6PM
 * Color sensor driver using I2C.
 */

#define TCS34727_ADDR                  0x29

/*
 * TCS34725/TCS34727 command bit.
 * This bit must be ORed with the register address.
 */
#define TCS34727_CMD                   0x80

#define TCS34727_ENABLE_R_ADDR         0x00
#define TCS34727_ENABLE_PON            0x01
#define TCS34727_ENABLE_AEN            0x02
#define TCS34727_ENABLE_WEN            0x08
#define TCS34727_ENABLE_AIEN           0x10

#define TCS34727_TIMING_R_ADDR         0x01
#define TCS34727_ATIME_2_4_MS          0xFF

#define TCS34727_CTRL_R_ADDR           0x0F
#define TCS34727_CTRL_AGAIN_1          0x00

/*
 * ID register.
 * For the project sensor TCS34727:
 * Register 0x12 should return 0x4D.
 */
#define TCS34727_ID_R_ADDR             0x12
#define TCS34727_ID                    0x4D

/*
 * Color data registers.
 * Data is low byte first, then high byte.
 */
#define TCS34727_CDATAL_R_ADDR         0x14
#define TCS34727_CDATAH_R_ADDR         0x15
#define TCS34727_RDATAL_R_ADDR         0x16
#define TCS34727_RDATAH_R_ADDR         0x17
#define TCS34727_GDATAL_R_ADDR         0x18
#define TCS34727_GDATAH_R_ADDR         0x19
#define TCS34727_BDATAL_R_ADDR         0x1A
#define TCS34727_BDATAH_R_ADDR         0x1B

typedef enum{
    RED_DETECT     = 0,
    GREEN_DETECT   = 1,
    BLUE_DETECT    = 2,
    NOTHING_DETECT = 3
} COLOR_DETECTED;

typedef struct{
    uint16_t R_RAW;
    uint16_t G_RAW;
    uint16_t B_RAW;
    uint16_t C_RAW;

    float R;
    float G;
    float B;
} RGB_COLOR_HANDLE_t;

void TCS34727_Init(void);

uint16_t TCS34727_GET_RAW_CLEAR(void);
uint16_t TCS34727_GET_RAW_RED(void);
uint16_t TCS34727_GET_RAW_GREEN(void);
uint16_t TCS34727_GET_RAW_BLUE(void);

void TCS34727_GET_RGB(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance);
COLOR_DETECTED Detect_Color(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance);

uint8_t TCS34727_CheckID(void);
void TCS34727_ReadAllRaw(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance);

#endif