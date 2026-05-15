// I2C.h
// Runs on TM4C123
// Header file for I2C.c. Defines I2C1 hardware pin assignments, master
// configuration constants, MCS command and status bits, and declares all
// I2C communication functions including single and burst read/write
// operations and error handling for use with I2C slave devices.
// May 4, 2026


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include "wtimer.h"

/*
 * I2C1 Hardware Selection
 * PA6 = I2C1SCL
 * PA7 = I2C1SDA
 */
#define EN_I2C1_CLOCK          0x02
#define EN_GPIOA_CLOCK         0x01

#define I2C_PINS               0xC0
#define I2C_SCL_PIN            0x40
#define I2C_SDA_PIN            0x80

#define I2C_ALT_FUNC_MSK       0xFF000000
#define I2C_ALT_FUNC_SET       0x33000000

/*
 * I2C Master Configuration
 */
#define EN_I2C_MASTER          0x10

/*
 * 100 kHz standard mode at 16 MHz system clock:
 * TPR = (System Clock / (2 * (SCL_LP + SCL_HP) * SCL_CLK)) - 1
 * TPR = (16 MHz / (2 * (6 + 4) * 100 kHz)) - 1
 * TPR = 7
 */
#define I2C_MTPR_TPR_VALUE     0x07
#define I2C_MTPR_STD_SPEED     I2C_MTPR_TPR_VALUE

/*
 * MSA read/write bit
 */
#define I2C_RW_PIN             0x01

/*
 * I2C MCS command bits
 */
#define I2C_RUN                0x01
#define I2C_START              0x02
#define I2C_STOP               0x04
#define I2C_ACK                0x08

#define RUN_CMD                I2C_RUN

/*
 * I2C MCS status bits
 */
#define I2C_BUSY               0x01
#define I2C_ERROR              0x02
#define I2C_ADRACK             0x04
#define I2C_DATACK             0x08
#define I2C_ARBLST             0x10
#define I2C_IDLE               0x20
#define I2C_BUSBSY             0x40
#define I2C_CLKTO              0x80
#define I2C_TIMEOUT            100000

void I2C_Init(void);

uint8_t I2C_Receive(uint8_t slave_addr, uint8_t slave_reg_addr);
uint8_t I2C_Transmit(uint8_t slave_addr, uint8_t slave_reg_addr, uint8_t data);
uint8_t I2C_SendByte(uint8_t slave_addr, uint8_t data); 

void I2C_Burst_Receive(uint8_t slave_addr,
                       uint8_t slave_reg_addr,
                       uint8_t* data,
                       uint32_t size);

uint8_t I2C_Burst_Transmit(uint8_t slave_addr,
                           uint8_t slave_reg_addr,
                           uint8_t* data,
                           uint32_t size);

uint8_t I2C_GetLastError(void);

#endif
