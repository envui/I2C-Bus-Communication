// MPU6050.c
// Runs on TM4C123
// Provides MPU6050 IMU driver functions over I2C at address 0x68. Includes
// initialization, gyro calibration, 3-axis accelerometer and gyroscope
// reading, and angle estimation using a complementary filter (98% gyro,
// 2% accelerometer) for smooth roll, pitch, and yaw tracking. Servo
// angle output is derived from pitch and clamped to -90 to +90 degrees.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 13, 2026

#include <stdint.h>
#include <math.h>
#include "MPU6050.h"
#include "I2C.h"
#include "UART0.h"
#include "wtimer.h"

/*
 * MPU6050.c
 * Runs on TM4C123GXL
 * Reads acceleration and gyro data from MPU6050.
 *
 * The gyro gives speed in degrees per second.
 * The accelerometer gives tilt based on gravity.
 * The final angle uses both so it is smoother.
 */

static int16_t MPU6050_CombineBytes(uint8_t high, uint8_t low){
    /*
     * MPU6050 sends 16-bit values as high byte first, low byte second.
     * Combine them into one signed 16-bit number.
     */
    return (int16_t)(((uint16_t)high << 8) | low);
}

static int16_t MPU6050_LimitAngle(float angle){
    /*
     * Servo only uses -90 to +90 degrees.
     */
    if(angle < -90.0f){
        return -90;
    }

    if(angle > 90.0f){
        return 90;
    }

    return (int16_t)angle;
}

// Verify communication with the MPU6050 using the WHO_AM_I register.
uint8_t MPU6050_CheckID(void){
    uint8_t id;

    /*
     * WHO_AM_I should return 0x68 when AD0 is connected to GND.
     */
    id = I2C_Receive(MPU6050_ADDR, MPU6050_WHO_AM_I_R_ADDR);

    if(id == MPU6050_EXPECTED_ID){
        return 1;
    }

    return 0;
}

// Wake the MPU6050 and configure default operating settings.
void MPU6050_Enable(void){
    /*
     * Wake up the MPU6050.
     * 0x00 clears sleep mode.
     */
    I2C_Transmit(MPU6050_ADDR, MPU6050_PWR_MGMT_1_R_ADDR, 0x00);
    DELAY_1MS(10);

    /*
     * Sample rate divider.
     * This slows the output rate for testing.
     */
    I2C_Transmit(MPU6050_ADDR, MPU6050_SMPLRT_DIV_R_ADDR, 0x07);
    DELAY_1MS(1);

    /*
     * Basic filter setting.
     */
    I2C_Transmit(MPU6050_ADDR, MPU6050_CONFIG_R_ADDR, 0x00);
    DELAY_1MS(1);

    /*
     * Gyro range = +/-250 degrees/sec.
     * This makes 131 raw counts equal 1 degree/sec.
     */
    I2C_Transmit(MPU6050_ADDR, MPU6050_GYRO_CONFIG_R_ADDR, 0x00);
    DELAY_1MS(1);

    /*
     * Accelerometer range = +/-2g.
     * This makes 16384 raw counts equal 1g.
     */
    I2C_Transmit(MPU6050_ADDR, MPU6050_ACCEL_CONFIG_R_ADDR, 0x00);
    DELAY_1MS(1);
}

void MPU6050_Init(void){
    if(MPU6050_CheckID() == 0){
        UART0_OutString("MPU6050 has not been Detected");
        UART0_OutCRLF();
        return;
    }

    UART0_OutString("MPU6050 has been Detected");
    UART0_OutCRLF();

    MPU6050_Enable();

    UART0_OutString("MPU6050 Initialized");
    UART0_OutCRLF();
}

void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az){
    uint8_t data[6];

    if(ax == 0 || ay == 0 || az == 0){
        return;
    }

    /*
     * Read 6 bytes:
     * AX high, AX low, AY high, AY low, AZ high, AZ low.
     */
    I2C_Burst_Receive(MPU6050_ADDR,
                      MPU6050_ACCEL_XOUT_H_ADDR,
                      data,
                      6);

    *ax = MPU6050_CombineBytes(data[0], data[1]);
    *ay = MPU6050_CombineBytes(data[2], data[3]);
    *az = MPU6050_CombineBytes(data[4], data[5]);
}

void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz){
    uint8_t data[6];

    if(gx == 0 || gy == 0 || gz == 0){
        return;
    }

    /*
     * Read 6 bytes:
     * GX high, GX low, GY high, GY low, GZ high, GZ low.
     */
    I2C_Burst_Receive(MPU6050_ADDR,
                      MPU6050_GYRO_XOUT_H_ADDR,
                      data,
                      6);

    *gx = MPU6050_CombineBytes(data[0], data[1]);
    *gy = MPU6050_CombineBytes(data[2], data[3]);
    *gz = MPU6050_CombineBytes(data[4], data[5]);
}

void MPU6050_ReadAll(MPU6050_HANDLE_t *MPU6050_Instance){
    if(MPU6050_Instance == 0){
        return;
    }

    MPU6050_ReadAccel(&MPU6050_Instance->AX_RAW,
                      &MPU6050_Instance->AY_RAW,
                      &MPU6050_Instance->AZ_RAW);

    MPU6050_ReadGyro(&MPU6050_Instance->GX_RAW,
                     &MPU6050_Instance->GY_RAW,
                     &MPU6050_Instance->GZ_RAW);

    /*
     * Convert raw accelerometer values into g units.
     * Example: 16384 raw counts = 1g.
     */
    MPU6050_Instance->AX_G =
        (float)MPU6050_Instance->AX_RAW / MPU6050_ACCEL_SCALE;

    MPU6050_Instance->AY_G =
        (float)MPU6050_Instance->AY_RAW / MPU6050_ACCEL_SCALE;

    MPU6050_Instance->AZ_G =
        (float)MPU6050_Instance->AZ_RAW / MPU6050_ACCEL_SCALE;

    /*
     * Convert raw gyro values into degrees per second.
     * Example: 131 raw counts = 1 degree/sec.
     *
     * The offset is subtracted because the gyro may not read exactly 0
     * even when it is sitting still.
     */
    MPU6050_Instance->GX_DPS =
        ((float)MPU6050_Instance->GX_RAW / MPU6050_GYRO_SCALE) -
        MPU6050_Instance->GX_Offset;

    MPU6050_Instance->GY_DPS =
        ((float)MPU6050_Instance->GY_RAW / MPU6050_GYRO_SCALE) -
        MPU6050_Instance->GY_Offset;

    MPU6050_Instance->GZ_DPS =
        ((float)MPU6050_Instance->GZ_RAW / MPU6050_GYRO_SCALE) -
        MPU6050_Instance->GZ_Offset;
}

void MPU6050_CalibrateGyro(MPU6050_HANDLE_t *MPU6050_Instance){
    int32_t gxSum;
    int32_t gySum;
    int32_t gzSum;

    int16_t gx;
    int16_t gy;
    int16_t gz;

    uint16_t i;
    uint16_t samples;

    if(MPU6050_Instance == 0){
        return;
    }

    gxSum = 0;
    gySum = 0;
    gzSum = 0;
    samples = 200;

    UART0_OutString("Keep MPU6050 still for gyro calibration...");
    UART0_OutCRLF();

    /*
     * Read the gyro many times while it is not moving.
     * The average value becomes the offset.
     */
    for(i = 0; i < samples; i++){
        MPU6050_ReadGyro(&gx, &gy, &gz);

        gxSum += gx;
        gySum += gy;
        gzSum += gz;

        DELAY_1MS(5);
    }

    /*
     * Convert the average raw value into degrees/sec.
     * This is the amount we subtract later.
     */
    MPU6050_Instance->GX_Offset =
        ((float)gxSum / (float)samples) / MPU6050_GYRO_SCALE;

    MPU6050_Instance->GY_Offset =
        ((float)gySum / (float)samples) / MPU6050_GYRO_SCALE;

    MPU6050_Instance->GZ_Offset =
        ((float)gzSum / (float)samples) / MPU6050_GYRO_SCALE;

    /*
     * Start angles at 0 after calibration.
     * This makes the starting position the reference.
     */
    MPU6050_Instance->GyroRoll = 0.0f;
    MPU6050_Instance->GyroPitch = 0.0f;
    MPU6050_Instance->GyroYaw = 0.0f;

    MPU6050_Instance->Roll = 0.0f;
    MPU6050_Instance->Pitch = 0.0f;
    MPU6050_Instance->Yaw = 0.0f;

    MPU6050_Instance->TiltX = 0;
    MPU6050_Instance->TiltY = 0;

    MPU6050_Instance->LastUpdateMs = WTIMER_Millis();
    MPU6050_Instance->Calibrated = 1;

    UART0_OutString("Gyro calibration complete.");
    UART0_OutCRLF();
}

void MPU6050_UpdateAngles(MPU6050_HANDLE_t *MPU6050_Instance){
    uint32_t now;
    uint32_t elapsedMs;
    float dt;

    if(MPU6050_Instance == 0){
        return;
    }

    /*
     * If calibration has not happened yet, do it first.
     */
    if(MPU6050_Instance->Calibrated == 0){
        MPU6050_CalibrateGyro(MPU6050_Instance);
    }

    MPU6050_ReadAll(MPU6050_Instance);

    /*
     * Find how much time passed since the last angle update.
     * Gyro gives degrees per second, so time must be in seconds.
     */
    now = WTIMER_Millis();
    elapsedMs = now - MPU6050_Instance->LastUpdateMs;

    if(elapsedMs == 0){
        return;
    }

    MPU6050_Instance->LastUpdateMs = now;

    dt = (float)elapsedMs / 1000.0f;

    /*
     * Accelerometer angle calculation.
     *
     * The accelerometer sees gravity.
     * When the board tilts, gravity appears on different axes.
     *
     * atan2 gives the angle from those gravity values.
     * The result is in radians, so multiply by 57.2958 to get degrees.
     */
    MPU6050_Instance->AccelRoll =
        atan2f(MPU6050_Instance->AY_G,
               MPU6050_Instance->AZ_G) * MPU6050_RAD_TO_DEG;

    MPU6050_Instance->AccelPitch =
        atan2f(-MPU6050_Instance->AX_G,
               sqrtf((MPU6050_Instance->AY_G * MPU6050_Instance->AY_G) +
                     (MPU6050_Instance->AZ_G * MPU6050_Instance->AZ_G))) *
        MPU6050_RAD_TO_DEG;

    /*
     * Gyro angle calculation.
     *
     * Gyro gives speed:
     * degrees per second.
     *
     * Angle change = speed * time.
     *
     * Example:
     * 30 deg/sec * 0.1 sec = 3 degrees.
     */
    MPU6050_Instance->GyroRoll += MPU6050_Instance->GX_DPS * dt;
    MPU6050_Instance->GyroPitch += MPU6050_Instance->GY_DPS * dt;
    MPU6050_Instance->GyroYaw += MPU6050_Instance->GZ_DPS * dt;

    /*
     * Complementary filter.
     *
     * Gyro is smooth and fast, but it slowly drifts.
     * Accelerometer does not drift, but it can be noisy.
     *
     * This combines both:
     * 98% gyro
     * 2% accelerometer
     */
    MPU6050_Instance->Roll =
        (MPU6050_COMP_ALPHA *
        (MPU6050_Instance->Roll + MPU6050_Instance->GX_DPS * dt)) +
        ((1.0f - MPU6050_COMP_ALPHA) * MPU6050_Instance->AccelRoll);

    MPU6050_Instance->Pitch =
        (MPU6050_COMP_ALPHA *
        (MPU6050_Instance->Pitch + MPU6050_Instance->GY_DPS * dt)) +
        ((1.0f - MPU6050_COMP_ALPHA) * MPU6050_Instance->AccelPitch);

    /*
     * Yaw is only from the gyro.
     * MPU6050 has no magnetometer, so yaw will drift over time.
     * This is why the servo should use pitch or roll, not yaw.
     */
    MPU6050_Instance->Yaw = MPU6050_Instance->GyroYaw;

    /*
     * Store integer versions for LCD and servo.
     */
    MPU6050_Instance->TiltX = MPU6050_LimitAngle(MPU6050_Instance->Roll);
    MPU6050_Instance->TiltY = MPU6050_LimitAngle(MPU6050_Instance->Pitch);
}

int16_t MPU6050_GetServoAngleY(MPU6050_HANDLE_t *MPU6050_Instance){
    int16_t angle;

    if(MPU6050_Instance == 0){
        return 0;
    }

    /*
     * Servo follows pitch.
     * Pitch is the forward/backward tilt.
     */
    angle = MPU6050_Instance->TiltY;

    if(angle < -90){
        angle = -90;
    }

    if(angle > 90){
        angle = 90;
    }

    return angle;
}
