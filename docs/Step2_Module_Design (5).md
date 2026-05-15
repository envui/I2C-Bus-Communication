# Step 2 – Module Design & Implementation

## 1. Module Decomposition

| Module Name | Type | Responsibility | Inputs | Outputs |
|-------------|------|----------------|--------|---------|
| `WTIMER` | Driver | Configures WTIMER1A as the 1 ms system timing base. PB1 toggles every 500 ms from the timer ISR as a timing check. | 16 MHz clock, delay time, elapsed-time period | 1 ms tick count, delays, elapsed-time status, PB1 toggle |
| `UART0` | Driver | Configures UART0 for PC terminal communication and prints characters, strings, integers, floats, and hex values. | Terminal input, strings, numeric values | UART terminal output and received input |
| `I2C` | Driver | Configures I2C1 at 100 kHz and handles register reads/writes, burst reads, burst writes, direct byte sends, and error checks. | Slave address, register address, data bytes | I2C data and status/error flags |
| `ButtonLED` | Driver | Configures Port F switches and LEDs. SW1/SW2 are interrupt-based and LEDs show status/detected color. | SW1/SW2 presses, LED color request | Button event flags and LED output |
| `Servo` | Driver | Configures hardware PWM for 50 Hz servo control and maps angle to calibrated pulse width. | Desired servo angle | PWM signal to servo |
| `TCS34727` | Module | Initializes and reads the color sensor through I2C. Verifies part ID, reads RGB/clear data, normalizes RGB, and detects dominant color. | I2C bus and sensor registers | Raw color values, normalized RGB values, detected color |
| `MPU6050` | Module | Initializes and reads the IMU through I2C. Verifies ID, reads accel/gyro data, converts values, calibrates gyro, and computes angle. | I2C bus, IMU registers, timer | Raw accel/gyro, g values, deg/s values, pitch/roll/yaw, servo angle |
| `LCD` | Module | Controls the 16x2 LCD through the I2C backpack in 4-bit mode. | Text, cursor position, integer values | LCD display output |
| `main.c` | Application/Test Harness | Initializes all modules and runs independent checkpoint tests before final integration. | Module APIs and button events | UART output, LCD output, LED output, servo motion |

---

## 2. Implementation Summary

### 2.1 `WTIMER` Driver

The `WTIMER` driver uses WTIMER1A to create a 1 ms time base. The checkpoint code assumes the default 16 MHz clock and no PLL. The timer reload value is `15999`, because 16000 clock ticks equals 1 ms at 16 MHz. The prescaler is `0x00`.

Key functions:

```c
void WTIMER_Init(void);
void DELAY_1MS(uint32_t delay);
uint32_t WTIMER_Millis(void);
uint8_t WTIMER_HasElapsed(uint32_t *lastTime, uint32_t periodMs);
```

Important logic decisions:
- `WTIMER_Millis()` gives the current millisecond count.
- `WTIMER_HasElapsed()` is used for 100 ms and 1 second scheduling later.
- PB1 toggles every 500 ms inside the WTIMER ISR so timing can be verified by waveform measurement.
- `DELAY_1MS()` is still used for simple module tests and sensor/LCD setup delays.

---

### 2.2 `UART0` Driver

The `UART0` driver initializes UART0 on PA0/PA1 for PC terminal communication. It is used for checkpoint output, debugging, sensor values, and startup ID prompts.

Key functions:

```c
void UART0_Init(void);
void UART0_OutChar(char data);
void UART0_OutString(char *pt);
void UART0_OutCRLF(void);
unsigned char UART0_InChar(void);
void UART0_OutUDec(uint32_t n);
void UART0_OutSDec(int32_t n);
void UART0_OutFloat(float number, uint8_t decimals);
void UART0_OutUHex(uint32_t n);
```

Important logic decisions:
- UART0 uses a valid baud rate that is not 57600.
- Decimal, float, and hex print helpers are included.
- `UART0_OutUHex()` is used to print sensor ID/register values like the TCS34727 part ID.

---

### 2.3 `I2C` Driver

The `I2C` driver configures I2C1 using PA6 as SCL and PA7 as SDA. I2C0 is not used because the project allows I2C1-I2C3 only. The bus runs at 100 kHz standard mode.

Key functions:

```c
void I2C_Init(void);
uint8_t I2C_Receive(uint8_t slave_addr, uint8_t slave_reg_addr);
uint8_t I2C_Transmit(uint8_t slave_addr, uint8_t slave_reg_addr, uint8_t data);
uint8_t I2C_SendByte(uint8_t slave_addr, uint8_t data);
void I2C_Burst_Receive(uint8_t slave_addr, uint8_t slave_reg_addr, uint8_t* data, uint32_t size);
uint8_t I2C_Burst_Transmit(uint8_t slave_addr, uint8_t slave_reg_addr, uint8_t* data, uint32_t size);
uint8_t I2C_GetLastError(void);
```

Important logic decisions:
- `I2C_Transmit()` writes one byte to a device register.
- `I2C_Receive()` uses the repeated-start read sequence for sensor registers.
- `I2C_Burst_Receive()` supports multi-byte reads, especially for the MPU6050.
- `I2C_SendByte()` supports the LCD backpack because the PCF8574 does not use normal register addressing.
- Timeout/error checks are included so the system does not wait forever on a stuck bus.

---

### 2.4 `ButtonLED` Driver

The `ButtonLED` driver uses Port F for onboard switches and LEDs.

Key functions:

```c
void LED_Init(void);
void LED_Out(uint8_t led);
void LED_Off(void);
void LED_CycleNext(void);
uint8_t LED_GetCurrent(void);
void BTN_Init(void);
uint8_t BTN_SW1Pressed(void);
uint8_t BTN_SW2Pressed(void);
void BTN_ClearEvents(void);
```

Important logic decisions:
- SW1 and SW2 are GPIO interrupt based.
- The ISR sets flags only. The main loop handles the action.
- Debounce uses `WTIMER_Millis()` instead of blocking inside the ISR.
- `LED_GetCurrent()` prevents the blink test from resetting the LED back to red after it turns off.

---

### 2.5 `Servo` Driver

The `Servo` driver configures hardware PWM for servo control. The PWM output uses PB7 / M0PWM1. The physical servo used in testing needed a calibrated 0.5 ms to 2.5 ms range to move through the full visible angle range.

Key functions:

```c
void Servo_Init(void);
void Servo_SetAngle(int16_t angle);
void Drive_Servo(int16_t angle);
```

Important logic decisions:
- The requested angle is clamped to -90 to +90 degrees.
- The `map()` helper converts angle to PWM count.
- Calibrated pulse mapping:
  - -90 degrees -> 0.5 ms
  - 0 degrees -> 1.5 ms
  - +90 degrees -> 2.5 ms
- The servo is powered from an external 5 V supply with common ground.

---

### 2.6 `TCS34727` Color Sensor Module

The `TCS34727` module uses I2C address `0x29`.

Key functions:

```c
void TCS34727_Init(void);
uint8_t TCS34727_CheckID(void);
uint16_t TCS34727_GET_RAW_CLEAR(void);
uint16_t TCS34727_GET_RAW_RED(void);
uint16_t TCS34727_GET_RAW_GREEN(void);
uint16_t TCS34727_GET_RAW_BLUE(void);
void TCS34727_GET_RGB(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance);
COLOR_DETECTED Detect_Color(RGB_COLOR_HANDLE_t* RGB_COLOR_Instance);
```

Important logic decisions:
- The module reads ID register `0x12`.
- For the TCS34727, the expected part ID is `0x4D`.
- Register accesses use `0x80 | register` because the sensor needs the command bit.
- Color values are read as low byte then high byte.
- Raw values are normalized using the clear channel:

```c
R = (R_RAW / C_RAW) * 255
G = (G_RAW / C_RAW) * 255
B = (B_RAW / C_RAW) * 255
```

- Dominant color is selected by the largest normalized RGB value.
- If all RGB values are low, the output is `NOTHING_DETECT`.

---

### 2.7 `MPU6050` IMU Module

The `MPU6050` module uses I2C address `0x68` with AD0 connected to GND.

Key functions:

```c
void MPU6050_Init(void);
void MPU6050_Enable(void);
uint8_t MPU6050_CheckID(void);
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);
void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz);
void MPU6050_ReadAll(MPU6050_HANDLE_t *MPU6050_Instance);
void MPU6050_CalibrateGyro(MPU6050_HANDLE_t *MPU6050_Instance);
void MPU6050_UpdateAngles(MPU6050_HANDLE_t *MPU6050_Instance);
int16_t MPU6050_GetServoAngleY(MPU6050_HANDLE_t *MPU6050_Instance);
```

Important logic decisions:
- `WHO_AM_I` register `0x75` is checked for `0x68`.
- Acceleration data is read from `0x3B` through `0x40`.
- Gyro data is read from `0x43` through `0x48`.
- Raw high/low bytes are combined into signed 16-bit values.
- Accelerometer conversion:

```c
AX_G = AX_RAW / 16384.0
AY_G = AY_RAW / 16384.0
AZ_G = AZ_RAW / 16384.0
```

- Gyro conversion:

```c
GX_DPS = GX_RAW / 131.0 - GX_Offset
GY_DPS = GY_RAW / 131.0 - GY_Offset
GZ_DPS = GZ_RAW / 131.0 - GZ_Offset
```

- Gyro offsets are calculated by averaging stationary readings.
- Pitch and roll use a complementary filter. The gyro responds quickly but drifts. The accelerometer gives a gravity reference but is noisy.
- The servo follows filtered pitch.

---

### 2.8 `LCD` Module

The `LCD` module controls the 16x2 LCD through the I2C backpack.

Key functions:

```c
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_PrintText(char *text);
void LCD_PrintChar(char data);
void LCD_PrintInteger(int32_t number);
void LCD_StartupMessage(char *firstName, char *lastName);
```

Important logic decisions:
- LCD uses 4-bit mode.
- Each byte is split into upper and lower nibbles.
- The enable bit is pulsed to latch each nibble.
- `I2C_SendByte()` is used because the LCD backpack does not use normal register addressing.
- LCD text is kept short because each row only has 16 characters.

---

### 2.9 `main.c` Module Test Harness

The current `main.c` is a checkpoint test harness. It initializes all modules and uses SW1 to step through each test.

Current tests:
1. UART0 test
2. WTIMER + ButtonLED test
3. Servo motion test
4. LCD test
5. TCS34727 color sensor test
6. MPU6050 IMU/gyro test

Important logic decisions:
- Each module is tested separately.
- SW1 moves between tests or exits continuous tests.
- SW2 cycles LED color during the ButtonLED test.
- PB1 toggles continuously from the WTIMER ISR.
- LCD and UART both show color/angle information during sensor tests.

---

## 3. Module Testing

Each module has at least one valid checkpoint test. The following evidence names match the screenshots/videos collected for Step 2.

### Test Cases

| Module | Test ID | Description | Expected Result | Evidence |
|--------|---------|-------------|-----------------|----------|
| `WTIMER` | MOD-WTIMER-01 | Measure PB1 timer output from WTIMER ISR. | Waveform shows PB1 toggling every 500 ms, giving a 1 Hz square wave. | `Evidence/Step2_ModuleTests/MOD-WTIMER-01_WaveformMeasurement.png` |
| `WTIMER` / `ButtonLED` | MOD-WTIMER-02 | Run WTIMER/ButtonLED prompt and observe millisecond counter. | UART shows WTIMER/ButtonLED test prompt and millisecond counter updating every second. | `Evidence/Step2_ModuleTests/MOD-WTIMER-02_ButtonTimerPrompt_MillisCounter.png` |
| `UART0` | MOD-UART0-01 | Run UART0 output/input test. | Terminal shows UART0 test prompt, typed/echoed character, integer test, float test, and completion message. | `Evidence/Step2_ModuleTests/MOD-UART0-01_UARTOutputTest.png` |
| `I2C` | MOD-I2C-01 | Verify I2C device reads through startup prompt. | Startup terminal output shows I2C device ID reads, including color sensor part ID and MPU6050 ID. | `Evidence/Step2_ModuleTests/MOD-I2C-01_StartupDeviceRead.png` |
| `ButtonLED` | MOD-BTNLED-01 | Press SW2 during WTIMER/ButtonLED test. | Video shows onboard LED cycling Red -> Green -> Blue while the selected LED state is maintained. | `Evidence/Step2_ModuleTests/MOD-BTNLED-01_ButtonLEDCycling.mp4` |
| `Servo` | MOD-SERVO-01 | Run servo test sequence and capture UART prompt. | UART shows servo test prompt and each commanded angle in sequence. | `Evidence/Step2_ModuleTests/MOD-SERVO-01_ServoTestPrompt.png` |
| `Servo` | MOD-SERVO-02 | Record physical servo motion during servo test. | Video shows servo moving through 0, -45, 0, +45, 0, -90, 0, +90, 0. | `Evidence/Step2_ModuleTests/MOD-SERVO-02_ServoTest.mp4` |
| `TCS34727` | MOD-TCS-01 | Capture UART output during color sensor test. | UART shows raw RGB/clear values, normalized RGB values, and detected color changing. | `Evidence/Step2_ModuleTests/MOD-TCS-01_ColorSensorUARTOutput.png` |
| `TCS34727` | MOD-TCS-02 | Record color sensor test with colored objects. | Video shows detected color changing and onboard LED/LCD responding to red, green, and blue objects. | `Evidence/Step2_ModuleTests/MOD-TCS-02_ColorSensor.mp4` |
| `MPU6050` | MOD-MPU-01 | Capture UART output during gyro/IMU test. | UART shows raw accel/gyro values, gyro deg/s values, pitch/roll/yaw or tilt values, and servo angle. | `Evidence/Step2_ModuleTests/MOD-MPU-01_GyroUARTOutput.png` |
| `MPU6050` | MOD-MPU-02 | Record gyro/servo test. | Video shows IMU board tilting, angle changing, and servo/stepper output responding. | `Evidence/Step2_ModuleTests/MOD-MPU-02_GyroAndStepper.mp4` |
| `LCD` | MOD-LCD-01 | Record LCD test. | Video shows LCD displaying startup/test output, color text, and angle values without corrupted characters. | `Evidence/Step2_ModuleTests/MOD-LCD-01_LCDTest.mp4` |

---

## 4. Evidence File Naming Summary

Use the following names for the files before uploading them into the repository.

### PNG Evidence

| Current Evidence | Rename To |
|------------------|-----------|
| Waveform measurement screenshot | `MOD-WTIMER-01_WaveformMeasurement.png` |
| UART output test screenshot | `MOD-UART0-01_UARTOutputTest.png` |
| I2C device read/startup prompt screenshot | `MOD-I2C-01_StartupDeviceRead.png` |
| Color sensor UART output screenshot | `MOD-TCS-01_ColorSensorUARTOutput.png` |
| Gyro UART output screenshot | `MOD-MPU-01_GyroUARTOutput.png` |
| Servo test UART prompt screenshot | `MOD-SERVO-01_ServoTestPrompt.png` |
| Button timer test prompt and millisecond counter screenshot | `MOD-WTIMER-02_ButtonTimerPrompt_MillisCounter.png` |

### Video Evidence

| Current Evidence | Rename To |
|------------------|-----------|
| Servo test video | `MOD-SERVO-02_ServoTest.mp4` |
| Color sensor video | `MOD-TCS-02_ColorSensor.mp4` |
| ButtonLED cycling video | `MOD-BTNLED-01_ButtonLEDCycling.mp4` |
| Gyro and stepper video | `MOD-MPU-02_GyroAndStepper.mp4` |
| LCD test video | `MOD-LCD-01_LCDTest.mp4` |

All evidence files should be stored in:

```text
Evidence/Step2_ModuleTests/
```

---

## 5. Traceability Summary

| Requirement ID | Related Module(s) | Verification Test |
|----------------|-------------------|-------------------|
| RQ-01 | `main.c`, `TCS34727`, `MPU6050`, `LCD` | MOD-I2C-01 |
| RQ-02 | `WTIMER` | MOD-WTIMER-01 |
| RQ-03 | `WTIMER` | MOD-WTIMER-01, MOD-WTIMER-02 |
| RQ-04 | `ButtonLED` | MOD-BTNLED-01 |
| RQ-05 | `ButtonLED`, `main.c` | MOD-BTNLED-01 |
| RQ-06 | `UART0` | MOD-UART0-01 |
| RQ-07 | `UART0`, `main.c` | MOD-UART0-01 |
| RQ-08 | `I2C` | MOD-I2C-01 |
| RQ-09 | `I2C`, `TCS34727` | MOD-I2C-01, MOD-TCS-01 |
| RQ-10 | `I2C`, `MPU6050` | MOD-I2C-01, MOD-MPU-01 |
| RQ-11 | `I2C` | MOD-I2C-01 |
| RQ-12 | `TCS34727` | MOD-I2C-01, MOD-TCS-01 |
| RQ-13 | `TCS34727` | MOD-TCS-01, MOD-TCS-02 |
| RQ-14 | `TCS34727`, `ButtonLED`, `LCD` | MOD-TCS-01, MOD-TCS-02 |
| RQ-15 | `MPU6050` | MOD-I2C-01, MOD-MPU-01 |
| RQ-16 | `MPU6050` | MOD-MPU-01 |
| RQ-17 | `MPU6050` | MOD-MPU-01 |
| RQ-18 | `MPU6050`, `Servo` | MOD-MPU-01, MOD-MPU-02 |
| RQ-19 | `LCD` | MOD-LCD-01 |
| RQ-20 | `LCD`, `TCS34727`, `MPU6050` | MOD-LCD-01, MOD-TCS-02, MOD-MPU-02 |
| RQ-21 | `Servo` | MOD-SERVO-01, MOD-SERVO-02 |
| RQ-22 | `Servo`, `MPU6050` | MOD-SERVO-02, MOD-MPU-02 |
| RQ-23 | `Servo` | MOD-SERVO-01, MOD-SERVO-02 |
| RQ-24 | `WTIMER`, `TCS34727`, `MPU6050`, `main.c` | MOD-WTIMER-02, MOD-TCS-01, MOD-MPU-01 |
| RQ-25 | `WTIMER`, `UART0`, `LCD`, `main.c` | MOD-WTIMER-02, MOD-TCS-01, MOD-MPU-01, MOD-LCD-01 |
| RQ-26 | `main.c`, all modules | MOD-I2C-01, MOD-WTIMER-02, MOD-SERVO-02, MOD-TCS-02, MOD-MPU-02, MOD-LCD-01 |
| RQ-27 | `MPU6050`, `Servo`, `LCD`, `UART0` | MOD-MPU-01, MOD-MPU-02 |

---

## 6. Hardware Connections Used for Module Testing

### I2C1 Shared Bus

| Signal | TM4C123GXL Pin | Connected Devices |
|--------|----------------|------------------|
| SCL | PA6 | TCS34727 SCL, MPU6050 SCL, LCD SCL |
| SDA | PA7 | TCS34727 SDA, MPU6050 SDA, LCD SDA |
| 3.3 V | 3.3 V pin | TCS34727 VCC, MPU6050 VCC, LCD VCC if compatible |
| GND | GND pin | TCS34727 GND, MPU6050 GND, LCD GND |

### I2C Device Addresses

| Device | Address |
|--------|---------|
| TCS34727 Color Sensor | `0x29` |
| MPU6050 IMU | `0x68` when AD0 is connected to GND |
| LCD Backpack | `0x27` by default, test `0x3F` if needed |

### Servo

| Servo Wire | Connection |
|------------|------------|
| Signal | PB7 / M0PWM1 |
| Power | External 5 V |
| Ground | Common ground with TM4C |

### Buttons and LEDs

| Function | Pin |
|----------|-----|
| SW1 | PF4 |
| SW2 | PF0 |
| Red LED | PF1 |
| Blue LED | PF2 |
| Green LED | PF3 |
| PB1 timer check output | PB1 |

---

## 7. AI Verification Summary

AI was used to check the Step 2 document for formatting, completeness, and consistency with the Step 1 requirements. The module list, testing table, evidence filenames, and traceability table were organized using the current code structure and the evidence files collected during module testing. Suggestions that improved clarity and formatting were accepted. Suggestions that would change the selected hardware pins, timing behavior, or module responsibilities without user approval were rejected.
