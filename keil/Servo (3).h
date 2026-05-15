// Servo.h
// Runs on TM4C123
// Header file for Servo.c. Defines PWM timing constants for 50 Hz servo
// control on PB7 (M0PWM1) using a 16 MHz clock divided by 64. Maps servo
// angles (-90 to +90 degrees) to pulse width counts (125 to 625) over
// a 5000-count period, and declares all servo control functions.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 13, 2026

#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>
#include "wtimer.h"

/*
 * Servo.h
 * Runs on TM4C123GXL / TM4C123GH6PM
 * System clock: 16 MHz, no PLL.
 * Servo output: PB7 = M0PWM1.
 */

#define PWM0_PIN                0x80

/*
 * 16 MHz / 64 = 250 kHz PWM clock.
 * 50 Hz servo PWM period = 20 ms.
 * 20 ms * 250 kHz = 5000 counts.
 *
 * Calibrated working pulse mapping:
 * -90 deg -> 0.5 ms -> 125 counts
 *   0 deg -> 1.5 ms -> 375 counts
 * +90 deg -> 2.5 ms -> 625 counts
 *
 * This is the same range as the original working servo logic.
 */
#define PWM0_COUNTER            4999

#define SERVO_MIN_CNT           125
#define SERVO_CENTER_CNT        375
#define SERVO_MAX_CNT           625

#define SERVO_MIN_ANGLE         -90
#define SERVO_CENTER_ANGLE      0
#define SERVO_MAX_ANGLE         90

void Servo_Init(void);
void Servo_SetAngle(int16_t angle);
void Drive_Servo(int16_t angle);

#endif
