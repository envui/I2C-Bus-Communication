// main.c
// Runs on TM4C123
// CECS 447 Final Project: I2C Bus Communication System checkpoint test.
// Sequentially tests all eight modules: UART0 serial I/O, WTIMER millisecond
// timing with LED blinking, servo motor sweep, 16x2 LCD display, TCS34727
// color detection with LED and LCD feedback, and MPU6050 IMU angle tracking
// with complementary filter driving the servo in real-time.
// Press SW1 to advance through each test. Press SW2 to cycle LED color.
// Author: Rodrigo Can, Juan Miguel Constantino, Matthew Margulies, Kyle Leng
// May 13, 2026

#include <stdint.h>
#include "ButtonLED.h"
#include "wtimer.h"
#include "UART0.h"
#include "Servo.h"
#include "I2C.h"
#include "TCS34727.h"
#include "MPU6050.h"
#include "LCD.h"

/*
 * main.c
 * Runs on TM4C123GXL / TM4C123GH6PM
 *
 * Checkpoint test for:
 * 1. ButtonLED
 * 2. WTIMER
 * 3. UART0
 * 4. Servo
 * 5. I2C
 * 6. TCS34727 Color Sensor
 * 7. MPU6050 IMU
 * 8. 16x2 I2C LCD
 *
 * System clock: 16 MHz, no PLL.
 */

static void EnableInterrupts(void);

static void PrintMainHeader(void);
static void WaitForSW1(void);

static void Test_UART0(void);
static void Test_WTIMER_ButtonLED(void);
static void Test_Servo(void);
static void Test_LCD(void);
static void Test_TCS34727(void);
static void Test_MPU6050(void);

//Main application entry point that initializes and coordinates all modules.
int main(void){
    WTIMER_Init();
    LED_Init();
    BTN_Init();
    UART0_Init();
    Servo_Init();

    EnableInterrupts();

    I2C_Init();

    LCD_Init();
    LCD_StartupMessage("Rodrigo", "Can");

    PrintMainHeader();

    TCS34727_Init();
    MPU6050_Init();

    while(1){
        Test_UART0();
        WaitForSW1();

        Test_WTIMER_ButtonLED();
        WaitForSW1();

        Test_Servo();
        WaitForSW1();

        Test_LCD();
        WaitForSW1();

        Test_TCS34727();
        WaitForSW1();

        Test_MPU6050();
        WaitForSW1();
    }
}

static void EnableInterrupts(void){
    __asm("    CPSIE  I");
}

static void PrintMainHeader(void){
    UART0_OutCRLF();
    UART0_OutString("CECS 447 Final Project Checkpoint Test");
    UART0_OutCRLF();

    UART0_OutString("Modules: ButtonLED, WTIMER, UART0, Servo, I2C, TCS34727, MPU6050, LCD");
    UART0_OutCRLF();

    UART0_OutString("Clock: 16 MHz default, no PLL");
    UART0_OutCRLF();

    UART0_OutString("WTIMER1A provides 1 ms system timing.");
    UART0_OutCRLF();

    UART0_OutString("PB1 toggles every 500 ms from WTIMER ISR.");
    UART0_OutCRLF();

    UART0_OutString("I2C1 uses PA6=SCL and PA7=SDA.");
    UART0_OutCRLF();

    UART0_OutString("Press SW1 to move through each test.");
    UART0_OutCRLF();
}

static void WaitForSW1(void){
    UART0_OutCRLF();
    UART0_OutString("Press SW1 to continue...");
    UART0_OutCRLF();

    BTN_ClearSW1Event();

    while(BTN_SW1Pressed() == 0){}
}

static void Test_UART0(void){
    char c;

    UART0_OutCRLF();
    UART0_OutString("UART0 TEST");
    UART0_OutCRLF();

    UART0_OutString("Type one character: ");
    c = UART0_InChar();

    UART0_OutCRLF();
    UART0_OutString("You typed: ");
    UART0_OutChar(c);
    UART0_OutCRLF();

    UART0_OutString("Integer test: ");
    UART0_OutSDec(-12345);
    UART0_OutCRLF();

    UART0_OutString("Float test: ");
    UART0_OutFloat(45.75f, 2);
    UART0_OutCRLF();

    UART0_OutString("UART0 test complete.");
    UART0_OutCRLF();
}

static void Test_WTIMER_ButtonLED(void){
    uint32_t blinkTime;
    uint32_t printTime;
    uint8_t ledOn;

    UART0_OutCRLF();
    UART0_OutString("WTIMER + BUTTON + LED TEST");
    UART0_OutCRLF();

    UART0_OutString("PB1 toggles every 500 ms from WTIMER ISR.");
    UART0_OutCRLF();

    UART0_OutString("Selected onboard LED blinks every 500 ms.");
    UART0_OutCRLF();

    UART0_OutString("Press SW2 to cycle selected LED color.");
    UART0_OutCRLF();

    UART0_OutString("Press SW1 to exit this test.");
    UART0_OutCRLF();

    BTN_ClearEvents();

    blinkTime = WTIMER_Millis();
    printTime = WTIMER_Millis();
    ledOn = 0;

    while(BTN_SW1Pressed() == 0){
        if(WTIMER_HasElapsed(&blinkTime, 500)){
            if(ledOn){
                LED_Off();
                ledOn = 0;
            }
            else{
                LED_Out(LED_GetCurrent());
                ledOn = 1;
            }
        }

        if(BTN_SW2Pressed()){
            LED_CycleNext();
            ledOn = 1;

            UART0_OutString("SW2 event: LED cycled to ");

            if(LED_GetCurrent() == LED_RED){
                UART0_OutString("RED");
            }
            else if(LED_GetCurrent() == LED_GREEN){
                UART0_OutString("GREEN");
            }
            else if(LED_GetCurrent() == LED_BLUE){
                UART0_OutString("BLUE");
            }

            UART0_OutCRLF();
        }

        if(WTIMER_HasElapsed(&printTime, 1000)){
            UART0_OutString("Millis = ");
            UART0_OutUDec(WTIMER_Millis());
            UART0_OutCRLF();
        }
    }

    LED_Off();
}

static void Test_Servo(void){
    UART0_OutCRLF();
    UART0_OutString("SERVO TEST");
    UART0_OutCRLF();

    UART0_OutString("Sequence: 0, -45, 0, +45, 0, -90, 0, +90, 0");
    UART0_OutCRLF();

    UART0_OutString("Using calibrated 0.5 ms to 2.5 ms servo range.");
    UART0_OutCRLF();

    UART0_OutString("PB1 should continue toggling every 500 ms during servo delays.");
    UART0_OutCRLF();

    UART0_OutString("0 degrees");
    UART0_OutCRLF();
    Drive_Servo(0);
    DELAY_1MS(1000);

    UART0_OutString("-45 degrees");
    UART0_OutCRLF();
    Drive_Servo(-45);
    DELAY_1MS(1000);

    UART0_OutString("0 degrees");
    UART0_OutCRLF();
    Drive_Servo(0);
    DELAY_1MS(1000);

    UART0_OutString("+45 degrees");
    UART0_OutCRLF();
    Drive_Servo(45);
    DELAY_1MS(1000);

    UART0_OutString("0 degrees");
    UART0_OutCRLF();
    Drive_Servo(0);
    DELAY_1MS(1000);

    UART0_OutString("-90 degrees");
    UART0_OutCRLF();
    Drive_Servo(-90);
    DELAY_1MS(1000);

    UART0_OutString("0 degrees");
    UART0_OutCRLF();
    Drive_Servo(0);
    DELAY_1MS(1000);

    UART0_OutString("+90 degrees");
    UART0_OutCRLF();
    Drive_Servo(90);
    DELAY_1MS(1000);

    UART0_OutString("0 degrees");
    UART0_OutCRLF();
    Drive_Servo(0);
    DELAY_1MS(1000);

    UART0_OutString("Servo test complete.");
    UART0_OutCRLF();
}

static void Test_LCD(void){
    int16_t angle;

    UART0_OutCRLF();
    UART0_OutString("LCD TEST");
    UART0_OutCRLF();

    UART0_OutString("LCD should show color and angle examples.");
    UART0_OutCRLF();

    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_PrintText("Color: RED");

    LCD_SetCursor(1, 0);
    LCD_PrintText("Angle: ");
    LCD_PrintInteger(45);
    LCD_PrintText(" deg");

    DELAY_1MS(2000);

    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_PrintText("Color: GREEN");

    LCD_SetCursor(1, 0);
    LCD_PrintText("Angle: ");
    LCD_PrintInteger(-30);
    LCD_PrintText(" deg");

    DELAY_1MS(2000);

    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_PrintText("Color: BLUE");

    LCD_SetCursor(1, 0);
    LCD_PrintText("Angle: ");
    angle = 90;
    LCD_PrintInteger(angle);
    LCD_PrintText(" deg");

    DELAY_1MS(2000);

    LCD_Clear();

    UART0_OutString("LCD test complete.");
    UART0_OutCRLF();
}

static void Test_TCS34727(void){
    RGB_COLOR_HANDLE_t color;
    COLOR_DETECTED detected;
    uint32_t printTime;
    uint32_t rDisplay;
    uint32_t gDisplay;
    uint32_t bDisplay;

    UART0_OutCRLF();
    UART0_OutString("TCS34727 COLOR SENSOR TEST");
    UART0_OutCRLF();

    UART0_OutString("Color sensor values print every 1 second.");
    UART0_OutCRLF();

    UART0_OutString("LCD shows detected color and RGB values during test.");
    UART0_OutCRLF();

    UART0_OutString("Onboard LED shows detected dominant color.");
    UART0_OutCRLF();

    UART0_OutString("Press SW1 to exit this test.");
    UART0_OutCRLF();

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_PrintText("Color Test");
    LCD_SetCursor(1, 0);
    LCD_PrintText("Reading...");

    BTN_ClearEvents();
    printTime = WTIMER_Millis();

    while(BTN_SW1Pressed() == 0){
        if(WTIMER_HasElapsed(&printTime, 1000)){
            TCS34727_GET_RGB(&color);
            detected = Detect_Color(&color);

            rDisplay = (uint32_t)color.R;
            gDisplay = (uint32_t)color.G;
            bDisplay = (uint32_t)color.B;

            UART0_OutString("Raw R=");
            UART0_OutUDec(color.R_RAW);

            UART0_OutString(" G=");
            UART0_OutUDec(color.G_RAW);

            UART0_OutString(" B=");
            UART0_OutUDec(color.B_RAW);

            UART0_OutString(" C=");
            UART0_OutUDec(color.C_RAW);

            UART0_OutString(" | RGB R=");
            UART0_OutFloat(color.R, 1);

            UART0_OutString(" G=");
            UART0_OutFloat(color.G, 1);

            UART0_OutString(" B=");
            UART0_OutFloat(color.B, 1);

            UART0_OutString(" | Detected: ");

            LCD_Clear();
            LCD_SetCursor(0, 0);
            LCD_PrintText("Color: ");

            if(detected == RED_DETECT){
                UART0_OutString("RED");
                LED_Out(LED_RED);
                LCD_PrintText("RED");
            }
            else if(detected == GREEN_DETECT){
                UART0_OutString("GREEN");
                LED_Out(LED_GREEN);
                LCD_PrintText("GREEN");
            }
            else if(detected == BLUE_DETECT){
                UART0_OutString("BLUE");
                LED_Out(LED_BLUE);
                LCD_PrintText("BLUE");
            }
            else{
                UART0_OutString("NONE");
                LED_Off();
                LCD_PrintText("NONE");
            }

            /*
             * 16-character LCD row:
             * R:### G:### B:###
             */
            LCD_SetCursor(1, 0);
            LCD_PrintText("R:");
            LCD_PrintInteger(rDisplay);

            LCD_PrintText(" G:");
            LCD_PrintInteger(gDisplay);

            LCD_PrintText(" B:");
            LCD_PrintInteger(bDisplay);

            UART0_OutCRLF();
        }
    }

    LED_Off();
    LCD_Clear();

    UART0_OutString("TCS34727 color sensor test complete.");
    UART0_OutCRLF();
}

static void Test_MPU6050(void){
    MPU6050_HANDLE_t imu;
    uint32_t sampleTime;
    uint32_t printTime;
    int16_t servoAngle;

    UART0_OutCRLF();
    UART0_OutString("MPU6050 IMU TEST");
    UART0_OutCRLF();

    UART0_OutString("Reads IMU every 100 ms.");
    UART0_OutCRLF();

    UART0_OutString("Uses gyro calibration, gyro integration, and complementary filter.");
    UART0_OutCRLF();

    UART0_OutString("LCD shows filtered pitch angle and servo position.");
    UART0_OutCRLF();

    UART0_OutString("Servo follows filtered pitch angle.");
    UART0_OutCRLF();

    UART0_OutString("Press SW1 to exit this test.");
    UART0_OutCRLF();

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_PrintText("MPU6050 Test");
    LCD_SetCursor(1, 0);
    LCD_PrintText("Keep still");

    BTN_ClearEvents();

    imu.GX_Offset = 0.0f;
    imu.GY_Offset = 0.0f;
    imu.GZ_Offset = 0.0f;
    imu.GyroRoll = 0.0f;
    imu.GyroPitch = 0.0f;
    imu.GyroYaw = 0.0f;
    imu.Roll = 0.0f;
    imu.Pitch = 0.0f;
    imu.Yaw = 0.0f;
    imu.TiltX = 0;
    imu.TiltY = 0;
    imu.LastUpdateMs = WTIMER_Millis();
    imu.Calibrated = 0;

    MPU6050_CalibrateGyro(&imu);

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_PrintText("Gyro Ready");
    LCD_SetCursor(1, 0);
    LCD_PrintText("Reading...");

    sampleTime = WTIMER_Millis();
    printTime = WTIMER_Millis();
    servoAngle = 0;

    while(BTN_SW1Pressed() == 0){
        if(WTIMER_HasElapsed(&sampleTime, 100)){
            MPU6050_UpdateAngles(&imu);

            servoAngle = MPU6050_GetServoAngleY(&imu);
            Drive_Servo(servoAngle);
        }

        if(WTIMER_HasElapsed(&printTime, 1000)){
            UART0_OutString("Accel RAW AX=");
            UART0_OutSDec(imu.AX_RAW);

            UART0_OutString(" AY=");
            UART0_OutSDec(imu.AY_RAW);

            UART0_OutString(" AZ=");
            UART0_OutSDec(imu.AZ_RAW);

            UART0_OutString(" | Gyro RAW GX=");
            UART0_OutSDec(imu.GX_RAW);

            UART0_OutString(" GY=");
            UART0_OutSDec(imu.GY_RAW);

            UART0_OutString(" GZ=");
            UART0_OutSDec(imu.GZ_RAW);

            UART0_OutCRLF();

            UART0_OutString("Gyro DPS GX=");
            UART0_OutFloat(imu.GX_DPS, 2);

            UART0_OutString(" GY=");
            UART0_OutFloat(imu.GY_DPS, 2);

            UART0_OutString(" GZ=");
            UART0_OutFloat(imu.GZ_DPS, 2);

            UART0_OutCRLF();

            UART0_OutString("Roll=");
            UART0_OutFloat(imu.Roll, 2);

            UART0_OutString(" Pitch=");
            UART0_OutFloat(imu.Pitch, 2);

            UART0_OutString(" Yaw=");
            UART0_OutFloat(imu.Yaw, 2);

            UART0_OutString(" Servo=");
            UART0_OutSDec(servoAngle);

            UART0_OutString(" deg");
            UART0_OutCRLF();

            LCD_Clear();

            LCD_SetCursor(0, 0);
            LCD_PrintText("Angle:");
            LCD_PrintInteger(imu.TiltY);
            LCD_PrintText(" deg");

            LCD_SetCursor(1, 0);
            LCD_PrintText("Servo:");
            LCD_PrintInteger(servoAngle);
            LCD_PrintText(" deg");
        }
    }

    LCD_Clear();

    UART0_OutString("MPU6050 IMU test complete.");
    UART0_OutCRLF();
}
