// MPU6050.h
// Runs on TM4C123
// Header file for MPU6050.c. Defines I2C address, register addresses,
// scale factors, and the MPU6050_HANDLE_t struct for storing raw sensor
// readings, calibrated values, and computed roll/pitch

#ifndef MPU6050_H_
#define MPU6050_H_

#include <stdint.h>

#define MPU6050_ADDR                 0x68

#define MPU6050_SMPLRT_DIV_R_ADDR    0x19
#define MPU6050_CONFIG_R_ADDR        0x1A
#define MPU6050_GYRO_CONFIG_R_ADDR   0x1B
#define MPU6050_ACCEL_CONFIG_R_ADDR  0x1C

#define MPU6050_ACCEL_XOUT_H_ADDR    0x3B
#define MPU6050_ACCEL_XOUT_L_ADDR    0x3C
#define MPU6050_ACCEL_YOUT_H_ADDR    0x3D
#define MPU6050_ACCEL_YOUT_L_ADDR    0x3E
#define MPU6050_ACCEL_ZOUT_H_ADDR    0x3F
#define MPU6050_ACCEL_ZOUT_L_ADDR    0x40

#define MPU6050_TEMP_OUT_H_ADDR      0x41
#define MPU6050_TEMP_OUT_L_ADDR      0x42

#define MPU6050_GYRO_XOUT_H_ADDR     0x43
#define MPU6050_GYRO_XOUT_L_ADDR     0x44
#define MPU6050_GYRO_YOUT_H_ADDR     0x45
#define MPU6050_GYRO_YOUT_L_ADDR     0x46
#define MPU6050_GYRO_ZOUT_H_ADDR     0x47
#define MPU6050_GYRO_ZOUT_L_ADDR     0x48

#define MPU6050_PWR_MGMT_1_R_ADDR    0x6B
#define MPU6050_WHO_AM_I_R_ADDR      0x75

#define MPU6050_EXPECTED_ID          0x68

#define MPU6050_ACCEL_SCALE          16384.0f
#define MPU6050_GYRO_SCALE           131.0f

#define MPU6050_RAD_TO_DEG           57.2957795f
#define MPU6050_COMP_ALPHA           0.98f

typedef struct{
    int16_t AX_RAW;
    int16_t AY_RAW;
    int16_t AZ_RAW;

    int16_t GX_RAW;
    int16_t GY_RAW;
    int16_t GZ_RAW;

    float AX_G;
    float AY_G;
    float AZ_G;

    float GX_DPS;
    float GY_DPS;
    float GZ_DPS;

    float GX_Offset;
    float GY_Offset;
    float GZ_Offset;

    float AccelRoll;
    float AccelPitch;

    float GyroRoll;
    float GyroPitch;
    float GyroYaw;

    float Roll;
    float Pitch;
    float Yaw;

    int16_t TiltX;
    int16_t TiltY;

    uint32_t LastUpdateMs;
    uint8_t Calibrated;
} MPU6050_HANDLE_t;

void MPU6050_Init(void);
void MPU6050_Enable(void);

uint8_t MPU6050_CheckID(void);

void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);
void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz);

void MPU6050_ReadAll(MPU6050_HANDLE_t *MPU6050_Instance);
void MPU6050_CalibrateGyro(MPU6050_HANDLE_t *MPU6050_Instance);
void MPU6050_UpdateAngles(MPU6050_HANDLE_t *MPU6050_Instance);

int16_t MPU6050_GetServoAngleY(MPU6050_HANDLE_t *MPU6050_Instance);

#endif
