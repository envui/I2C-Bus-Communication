// I2C.c
// Runs on TM4C123
// Provides I2C1 master mode communication functions at 100 kHz standard mode
// using PA6 (SCL) and PA7 (SDA). Includes single and burst read/write operations
// with timeout and error detection for use with the TCS34725, MPU6050,
// and PCF8574T LCD backpack slave devices.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 4, 2026


#include <stdint.h>
#include "I2C.h"

//System Control Registers
#define SYSCTL_RCGCI2C_R       (*((volatile uint32_t *)0x400FE620))
#define SYSCTL_RCGCGPIO_R      (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRI2C_R         (*((volatile uint32_t *)0x400FEA20))
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08))

/*
 * GPIO Port A Registers
 */
#define GPIO_PORTA_AFSEL_R     (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_ODR_R       (*((volatile uint32_t *)0x4000450C))
#define GPIO_PORTA_PUR_R       (*((volatile uint32_t *)0x40004510))
#define GPIO_PORTA_DEN_R       (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTA_AMSEL_R     (*((volatile uint32_t *)0x40004528))
#define GPIO_PORTA_PCTL_R      (*((volatile uint32_t *)0x4000452C))

/*
 * I2C1 Master Registers
 */
#define I2C1_MSA_R             (*((volatile uint32_t *)0x40021000))
#define I2C1_MCS_R             (*((volatile uint32_t *)0x40021004))
#define I2C1_MDR_R             (*((volatile uint32_t *)0x40021008))
#define I2C1_MTPR_R            (*((volatile uint32_t *)0x4002100C))
#define I2C1_MCR_R             (*((volatile uint32_t *)0x40021020))

static uint8_t I2C_LastError = 0;

static uint8_t I2C_WaitWhileBusy(void){
    uint32_t timeout;

    timeout = I2C_TIMEOUT;

    while((I2C1_MCS_R & I2C_BUSY) && (timeout > 0)){
        timeout--;
    }

    if(timeout == 0){
        I2C_LastError = I2C_CLKTO;
        return I2C_CLKTO;
    }

    return 0;
}

static uint8_t I2C_WaitWhileBusBusy(void){
    uint32_t timeout;

    timeout = I2C_TIMEOUT;

    while((I2C1_MCS_R & I2C_BUSBSY) && (timeout > 0)){
        timeout--;
    }

    if(timeout == 0){
        I2C_LastError = I2C_BUSBSY;
        return I2C_BUSBSY;
    }

    return 0;
}

static uint8_t I2C_CheckError(void){
    uint8_t error;

    error = (uint8_t)(I2C1_MCS_R & (I2C_ERROR | I2C_ADRACK |
                                    I2C_DATACK | I2C_ARBLST |
                                    I2C_CLKTO));

    I2C_LastError = error;
    return error;
}

uint8_t I2C_GetLastError(void){
    return I2C_LastError;
}

/*
 *I2C_Init
 *Basic I2C initialization function for master mode at 100 kHz.

 */
void I2C_Init(void){
    SYSCTL_RCGCI2C_R |= EN_I2C1_CLOCK;
    SYSCTL_RCGCGPIO_R |= EN_GPIOA_CLOCK;

    while((SYSCTL_PRGPIO_R & EN_GPIOA_CLOCK) == 0){}
    while((SYSCTL_PRI2C_R & EN_I2C1_CLOCK) == 0){}

    /*
     * PA6 = I2C1SCL
     * PA7 = I2C1SDA
     */
    GPIO_PORTA_DEN_R |= I2C_PINS;
    GPIO_PORTA_AFSEL_R |= I2C_PINS;

    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~I2C_ALT_FUNC_MSK) |
                         I2C_ALT_FUNC_SET;

    /*
     * I2C SDA must be open-drain.
     * SCL does not need open-drain configuration on TM4C.
     */
    GPIO_PORTA_ODR_R |= I2C_SDA_PIN;

    /*
     * External 4.7k pull-ups to 3.3V are preferred.
     * Internal pull-ups are left disabled here.
     */
    GPIO_PORTA_PUR_R &= ~I2C_PINS;
    GPIO_PORTA_AMSEL_R &= ~I2C_PINS;

    /*
     * Enable I2C1 master mode.
     */
    I2C1_MCR_R = EN_I2C_MASTER;

    /*
     * Configure 100 kHz standard mode.
     */
    I2C1_MTPR_R = I2C_MTPR_STD_SPEED;
}

/*
 * I2C_Receive
 * Uses repeated-start receive to read one byte from a slave register.
 * Input: slave address and slave register address
 * Output: received 8-bit data. If error occurs, returns 0.
 */
uint8_t I2C_Receive(uint8_t slave_addr, uint8_t slave_reg_addr){
    uint8_t data;

    I2C_LastError = 0;

    if(I2C_WaitWhileBusBusy()){
        return 0;
    }

    /*
     * First phase: write the register address.
     */
    I2C1_MSA_R = (slave_addr << 1) & ~I2C_RW_PIN;
    I2C1_MDR_R = slave_reg_addr;
    I2C1_MCS_R = I2C_START | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return 0;
    }

    if(I2C_CheckError()){
        I2C1_MCS_R = I2C_STOP;
        return 0;
    }

    /*
     * Second phase: repeated START, read one byte, then STOP.
     */
    I2C1_MSA_R = (slave_addr << 1) | I2C_RW_PIN;
    I2C1_MCS_R = I2C_START | I2C_STOP | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return 0;
    }

    if(I2C_CheckError()){
        return 0;
    }

    data = (uint8_t)(I2C1_MDR_R & 0xFF);

    return data;
}

/*
 * I2C_Transmit
 * Transmits one byte of data to a slave register.
 * Input: slave address, slave register address, data
 * Output: error bits if detected, otherwise 0
 */
uint8_t I2C_Transmit(uint8_t slave_addr, uint8_t slave_reg_addr, uint8_t data){
    uint8_t error;

    I2C_LastError = 0;

    if(I2C_WaitWhileBusBusy()){
        return I2C_LastError;
    }

    /*
     * First phase: slave address + register address.
     */
    I2C1_MSA_R = (slave_addr << 1) & ~I2C_RW_PIN;
    I2C1_MDR_R = slave_reg_addr;
    I2C1_MCS_R = I2C_START | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return I2C_LastError;
    }

    error = I2C_CheckError();
    if(error){
        I2C1_MCS_R = I2C_STOP;
        return error;
    }

    /*
     * Second phase: data byte + STOP.
     */
    I2C1_MDR_R = data;
    I2C1_MCS_R = I2C_STOP | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return I2C_LastError;
    }

    error = I2C_CheckError();
    if(error){
        return error;
    }

    I2C_WaitWhileBusBusy();

    return I2C_LastError;
}

/*
 * I2C_Burst_Receive
 * Receives multiple bytes from a slave starting at a register address.
 * Input: slave address, starting slave register address, data buffer, size
 * Output: none
 */
void I2C_Burst_Receive(uint8_t slave_addr,
                       uint8_t slave_reg_addr,
                       uint8_t* data,
                       uint32_t size){
    uint32_t i;

    I2C_LastError = 0;

    if(data == 0 || size == 0){
        return;
    }

    if(I2C_WaitWhileBusBusy()){
        return;
    }

    /*
     * First phase: write starting register address.
     */
    I2C1_MSA_R = (slave_addr << 1) & ~I2C_RW_PIN;
    I2C1_MDR_R = slave_reg_addr;
    I2C1_MCS_R = I2C_START | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return;
    }

    if(I2C_CheckError()){
        I2C1_MCS_R = I2C_STOP;
        return;
    }

    /*
     * Second phase: read bytes using repeated START.
     */
    I2C1_MSA_R = (slave_addr << 1) | I2C_RW_PIN;

    if(size == 1){
        I2C1_MCS_R = I2C_START | I2C_STOP | I2C_RUN;

        if(I2C_WaitWhileBusy()){
            return;
        }

        if(I2C_CheckError()){
            return;
        }

        data[0] = (uint8_t)(I2C1_MDR_R & 0xFF);
        return;
    }

    /*
     * First byte of burst receive.
     */
    I2C1_MCS_R = I2C_START | I2C_ACK | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return;
    }

    if(I2C_CheckError()){
        return;
    }

    data[0] = (uint8_t)(I2C1_MDR_R & 0xFF);

    /*
     * Middle bytes.
     */
    for(i = 1; i < size - 1; i++){
        I2C1_MCS_R = I2C_ACK | I2C_RUN;

        if(I2C_WaitWhileBusy()){
            return;
        }

        if(I2C_CheckError()){
            return;
        }

        data[i] = (uint8_t)(I2C1_MDR_R & 0xFF);
    }

    /*
     * Last byte: NACK + STOP.
     */
    I2C1_MCS_R = I2C_STOP | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return;
    }

    if(I2C_CheckError()){
        return;
    }

    data[size - 1] = (uint8_t)(I2C1_MDR_R & 0xFF);
}

/*
 * I2C_Burst_Transmit
 * Transmits multiple bytes to a slave starting at a register address.
 * Input: slave address, starting register address, data buffer, size
 * Output: error bits if detected, otherwise 0
 */
uint8_t I2C_Burst_Transmit(uint8_t slave_addr,
                           uint8_t slave_reg_addr,
                           uint8_t* data,
                           uint32_t size){
    uint8_t error;
    uint32_t i;

    I2C_LastError = 0;

    if(data == 0 || size == 0){
        return 0;
    }

    if(I2C_WaitWhileBusBusy()){
        return I2C_LastError;
    }

    /*
     * First phase: slave address + starting register.
     */
    I2C1_MSA_R = (slave_addr << 1) & ~I2C_RW_PIN;
    I2C1_MDR_R = slave_reg_addr;
    I2C1_MCS_R = I2C_START | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return I2C_LastError;
    }

    error = I2C_CheckError();
    if(error){
        I2C1_MCS_R = I2C_STOP;
        return error;
    }

    /*
     * Data bytes.
     */
    for(i = 0; i < size; i++){
        I2C1_MDR_R = data[i];

        if(i == size - 1){
            I2C1_MCS_R = I2C_STOP | I2C_RUN;
        }
        else{
            I2C1_MCS_R = I2C_RUN;
        }

        if(I2C_WaitWhileBusy()){
            return I2C_LastError;
        }

        error = I2C_CheckError();
        if(error){
            return error;
        }
    }

    I2C_WaitWhileBusBusy();

    return I2C_LastError;
} 
													 



/*
 * I2C_SendByte
 * Sends one byte directly to an I2C slave.
 * This is used for devices like the PCF8574 LCD backpack,
 * which does not use normal register addressing.
 * Input: slave address and data byte
 * Output: error bits if detected, otherwise 0
 */
uint8_t I2C_SendByte(uint8_t slave_addr, uint8_t data){
    uint8_t error;

    I2C_LastError = 0;

    if(I2C_WaitWhileBusBusy()){
        return I2C_LastError;
    }

    I2C1_MSA_R = (slave_addr << 1) & ~I2C_RW_PIN;
    I2C1_MDR_R = data;
    I2C1_MCS_R = I2C_START | I2C_STOP | I2C_RUN;

    if(I2C_WaitWhileBusy()){
        return I2C_LastError;
    }

    error = I2C_CheckError();
    if(error){
        return error;
    }

    I2C_WaitWhileBusBusy();

    return I2C_LastError;
}
