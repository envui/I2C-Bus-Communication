<div align="center">
  
# Project Report
      
---
   
<img width="770" height="717" alt="CSULB Logo" src="https://github.com/user-attachments/assets/b6bfdc50-95bf-4ea0-94c4-216e01c7c104" />

California State University, Long Beach

Department of Computer Engineering & Computer Science

**Project Title:** I2C Bus Communication System  
**Course:** CECS 447 - Embedded Systems  
**Project Number:** Final Project  
**Team Number:** Group 9  
**Instructor:** Min He  
**Submission Date:** 5/13/2026  
**Team Members:** Rodrigo Can, Miguel Constantino, Matthew Margulies, Kyle Leng

</div>

---

# Table of Contents

1. [Introduction](#1-introduction)
   - [1.1 Project Objective](#11-project-objective)
   - [1.2 System Overview](#12-system-overview)
2. [Requirements & Constraints](#2-requirements--constraints)
   - [2.1 Functional Requirements](#21-functional-requirements)
   - [2.2 Design & Hardware Constraints](#22-design--hardware-constraints)
3. [Design Alternatives & Architecture](#3-design-alternatives--architecture)
   - [3.1 Design Alternative A](#31-design-alternative-a)
   - [3.2 Design Alternative B](#32-design-alternative-b)
   - [3.3 Design Comparison Table](#33-design-comparison-table)
   - [3.4 Final Design Selection & Justification](#34-final-design-selection--justification)
4. [System Architecture](#4-system-architecture)
   - [4.1 Block Diagram](#41-block-diagram)
   - [4.2 Module Decomposition](#42-module-decomposition)
   - [4.3 Call Graph](#43-call-graph)
   - [4.4 Data Flow Diagram](#44-data-flow-diagram)
   - [4.5 Software Flowchart](#45-software-flowchart)
5. [Module Implementation & Testing](#5-module-implementation--testing)
   - [5.1 Module List and Responsibility](#51-module-list-and-responsibility)
   - [5.2 Module Test Strategy](#52-module-test-strategy)
   - [5.3 Module Test Summary Table](#53-module-test-summary-table)
   - [5.4 Team Assignment](#54-team-assignment)
6. [System Integration & Validation](#6-system-integration--validation)
   - [6.1 Integration Order & Rationale](#61-integration-order--rationale)
   - [6.2 System-Level Test Matrix](#62-system-level-test-matrix)
   - [6.3 Team Assignment](#63-team-assignment)
7. [Claim–Evidence–Reasoning (CER)](#7-claimevidencereasoning-cer)
8. [Demonstration Summary](#8-demonstration-summary)
9. [Conclusion](#9-conclusion)
10. [References](#10-references)

---

# 1. Introduction

## 1.1 Project Objective

The technical objective of this project is to design, implement, and validate a real-time I2C bus communication system on the TM4C123GH6PM microcontroller. The final system integrates multiple I2C devices on one shared bus, reads color data from a TCS34727 RGB color/light sensor, reads motion data from an MPU6050 IMU, displays output on a 16x2 I2C LCD, controls an onboard RGB LED, and drives a servo motor using hardware PWM.

The project demonstrates multi-device I2C bus management, real-time timer-based scheduling, sensor data processing, PWM actuation, UART debugging, LCD user feedback, switch input handling, and final system integration after independent module validation.

## 1.2 System Overview

At power-on, the system initializes all hardware modules in dependency order. WTIMER1A starts first to provide the 1 ms timing base used by delays, debounce logic, and scheduled updates. UART0 initializes next so startup and sensor verification messages can be printed to the PC terminal. The system then initializes the Port F LEDs/buttons, servo PWM, I2C1, LCD, TCS34727 color sensor, and MPU6050 IMU.

During startup, the LCD displays the startup name message. UART0 keeps the original checkpoint-style startup prompt and then prints final integrated system status information. The TCS34727 part ID register is read and printed, and the MPU6050 WHO_AM_I register is read and printed. These startup ID checks confirm that the I2C bus and both sensors are responding before the final integrated loop begins.

After initialization, the program enters the final integrated superloop. Every 100 ms, the program reads the TCS34727 color sensor, reads the MPU6050 IMU, computes the detected color, computes the filtered tilt angle, updates the onboard RGB LED, and drives the servo motor using the IMU pitch angle. Every 1 second, the program updates the LCD and UART terminal with the detected color, raw RGB values, scaled RGB values, roll/pitch/yaw values, and servo angle.

SW1 pauses or resumes the integrated system. When the system is paused, the sensor/servo update path stops, the LED is turned off, and the LCD shows the paused state. SW2 forces an immediate LCD and UART update without waiting for the normal 1 second display interval. The WTIMER1A interrupt continues to provide the 1 ms timing base, while the Port F GPIO interrupt captures debounced SW1 and SW2 button events.

---

# 2. Requirements & Constraints

## 2.1 Functional Requirements

- RQ-01: WTIMER_Init() initializes WTIMER1A for 1 ms delay resolution.
- RQ-02: DELAY_1MS(n) blocks for n ms within timing tolerance.
- RQ-03: LED and button initialization works correctly.
- RQ-04: SW1 and SW2 press events are detected through the ButtonLED module.
- RQ-05: UART0 initializes at a valid baud rate that is not 57600.
- RQ-06: UART0 transmits characters and strings correctly.
- RQ-07: UART0 outputs formatted integer, float, and hexadecimal values.
- RQ-08: I2C1 initializes at 100 kHz standard mode.
- RQ-09: Single-byte I2C register read/write works.
- RQ-10: Burst I2C receive supports multi-byte sensor reads.
- RQ-11: I2C timeout and error handling prevents system hang.
- RQ-12: TCS34725/TCS34727 initializes and checks the ID register.
- RQ-13: TCS34725/TCS34727 reads raw clear, red, green, and blue data.
- RQ-14: TCS34725/TCS34727 normalizes RGB values using the clear channel.
- RQ-15: Dominant color detection selects red, green, blue, or none.
- RQ-16: MPU6050 initializes and checks the WHO_AM_I register.
- RQ-17: MPU6050 reads raw accelerometer and gyroscope data.
- RQ-18: MPU6050 converts raw data to g units and degrees/second.
- RQ-19: MPU6050 calculates pitch/roll angle from accelerometer and gyroscope data.
- RQ-20: LCD initializes through the I2C backpack.
- RQ-21: LCD displays text, numbers, color, and angle values.
- RQ-22: Servo PWM initializes at 50 Hz.
- RQ-23: Servo angle mapping drives the servo from -90 to +90 degrees.
- RQ-24: Servo executes the required independent-test motion sequence.
- RQ-25: The final 100 ms sampling loop reads sensors and updates the servo.
- RQ-26: The final 1 second update loop updates LCD and UART output.
- RQ-27: The final real-time integrated system combines sensing, display, UART, LED, and servo behavior.
- RQ-28: SW1 pauses and resumes the final integrated system.
- RQ-29: SW2 forces an immediate LCD and UART display update.

## 2.2 Design & Hardware Constraints

### Hardware and Project Constraints

- CN-01: Sampling must occur every 100 ms, and LCD/UART output must update every 1 second.
- CN-02: Only allowed peripherals may be used: WTIMER1-WTIMER5, I2C1-I2C3, UART0, and allowed PWM outputs. WTIMER0, I2C0, and M0PWM0 are not used.
- CN-03: UART0 must use a valid baud rate that is not 57600. This implementation uses 115200 baud.
- CN-04: The I2C bus must use pull-up resistors on SDA and SCL for stable communication.
- CN-05: The correct I2C device addresses must be used: TCS34727 at 0x29, MPU6050 at 0x68, and LCD backpack at 0x27.
- CN-06: Servo PWM must operate at 50 Hz.
- CN-07: Full system integration must not begin until all independent module tests pass.

### Design Constraints and Architecture Decisions

- DES-01: A shared I2C bus is used for all I2C devices.
- DES-02: The software architecture separates low-level drivers from higher-level sensor, display, and application modules.
- DES-03: Timer-based scheduling is used for final integration, with `WTIMER_HasElapsed()` controlling the 100 ms sensor/servo update rate and the 1 second LCD/UART output rate.

---

# 3. Design Alternatives & Architecture

## 3.1 Design Alternative A

### Hardware Plan

Design Alternative A uses a shared I2C1 bus for all three I2C devices. The TCS34727 color sensor, MPU6050 IMU, and PCF8574 LCD backpack all connect to PA6/SCL and PA7/SDA. Each device has a unique I2C address, so only one physical I2C bus is needed. The servo motor is controlled with hardware PWM using PB7/M0PWM1. UART0 uses PA0/PA1 for the PC terminal. WTIMER1A provides the 1 ms timing base.

### Software Implementation Method

The software uses modular drivers for WTIMER, UART0, I2C, ButtonLED, Servo, LCD, TCS34727, and MPU6050. The final application in `main.c` initializes these modules in dependency order and then runs a timed superloop.

The superloop uses `WTIMER_HasElapsed()` to schedule different tasks at different rates. Sensor sampling and servo updates occur every 100 ms. LCD and UART output occur every 1 second. SW1 and SW2 are handled through debounced button flags set by `GPIOPortF_Handler()`. The timer ISR does not perform I2C, LCD, UART, or sensor operations; it only increments the millisecond counter and toggles the PB1 timing debug output.

### Risks & Tradeoffs

The main risk is that all I2C devices share one bus, so a bus lockup caused by one device could affect every I2C device. This is reduced by timeout checks in the I2C driver. The benefit is that the hardware design uses fewer pins and keeps the I2C architecture simple. Hardware PWM is also safer than software PWM because it keeps the servo timing stable even when I2C, LCD, or UART code is executing.

---

## 3.2 Design Alternative B

### Hardware Plan

Design Alternative B uses separate I2C modules for different devices and uses software-generated PWM for the servo. For example, the color sensor could use I2C1, the IMU could use I2C2, and the LCD could use I2C3. The servo signal would be generated by toggling a GPIO pin with software delays.

### Software Implementation Method

Each I2C device would have a separate low-level I2C configuration. The main loop would use blocking delays to create approximate servo pulse timing and a single uniform output update rate. This design would reduce sharing on the I2C bus, but it would increase configuration complexity and pin usage.

### Risks & Tradeoffs

This alternative was rejected because it uses more pins, duplicates I2C setup logic, and makes servo timing less reliable. Software PWM is vulnerable to jitter because I2C reads, LCD writes, and UART output can delay the software pulse timing. A single uniform update rate is also not ideal because the servo needs faster updates than the LCD and UART.

---

## 3.3 Design Comparison Table

| Criteria | Design A | Design B |
|----------|----------|----------|
| I2C organization | One shared I2C1 bus | Separate I2C modules per device |
| Pin usage | Lower pin usage | Higher pin usage |
| Servo control | Hardware PWM on PB7/M0PWM1 | Software PWM using GPIO delays |
| Timing reliability | Strong: hardware PWM and WTIMER scheduling | Weaker: blocking delays can cause jitter |
| Code complexity | Shared I2C driver functions | More duplicate low-level I2C setup |
| LCD/UART update rate | 1 second scheduled output | Uniform blocking update rate |
| Sensor/servo update rate | 100 ms scheduled update | Less predictable under blocking delays |
| Constraint alignment | Satisfies CN-01 through CN-07 and DES-01 through DES-03 | Risks violating timing and peripheral-use constraints |
| Selected? | Yes | No |

---

## 3.4 Final Design Selection & Justification

Design A was selected because it best satisfies the project requirements while keeping the hardware and software architecture manageable. A shared I2C1 bus is appropriate because all three I2C devices have unique addresses. Hardware PWM on PB7/M0PWM1 provides stable 50 Hz servo control without depending on software timing. WTIMER1A provides a reliable 1 ms time base that supports 100 ms sensor/servo updates and 1 second LCD/UART updates.

This design also supports modular testing and incremental integration. The low-level drivers are separate from sensor processing and application logic, which made it possible to test each module independently before full integration. The selected architecture directly matches DES-01, DES-02, and DES-03.

---

# 4. System Architecture

## 4.1 Block Diagram

```text
+-------------------+          +----------------------+
| TCS34727          |          | MPU6050              |
| Color Sensor      |          | IMU Sensor           |
| I2C Addr: 0x29    |          | I2C Addr: 0x68       |
+---------+---------+          +----------+-----------+
          |                               |
          +-----------+     +-------------+
                      |     |
                +-----v-----v-----+
                | I2C1 Bus        |
                | PA6 = SCL       |
                | PA7 = SDA       |
                | 100 kHz         |
                +-----+-----+-----+
                      |     |
          +-----------+     +-------------+
          |                               |
+---------v---------+           +---------v----------+
| LCD Backpack      |           | TM4C123GH6PM       |
| PCF8574           |           | Main Controller    |
| I2C Addr: 0x27    |           +---------+----------+
+-------------------+                     |
                                          |
          +-------------------------------+-------------------------------+
          |                               |                               |
+---------v---------+           +---------v----------+           +--------v---------+
| Servo Motor       |           | Onboard RGB LED    |           | UART0 Terminal   |
| PB7 / M0PWM1      |           | PF1/PF2/PF3        |           | PA0/PA1          |
| 50 Hz PWM         |           | Color Indicator    |           | 115200 baud      |
+-------------------+           +--------------------+           +------------------+

+-------------------+           +--------------------+
| WTIMER1A          |           | Port F GPIO ISR    |
| 1 ms interrupt    |           | SW1 / SW2 flags    |
| PB1 debug toggle  |           | 50 ms debounce     |
+-------------------+           +--------------------+
```

## 4.2 Module Decomposition

The system is organized into independent modules. Hardware-specific drivers directly configure registers, while sensor/display modules use those drivers to provide higher-level behavior.

| Module | File(s) | Responsibility |
|--------|---------|----------------|
| WTIMER | `wtimer.c / wtimer.h` | WTIMER1A 1 ms interrupt, delay function, elapsed-time scheduling, PB1 timing debug output, `map()` utility |
| UART0 | `UART0.c / UART0.h` | PC terminal communication at 115200 baud; string, character, integer, float, and hex output |
| I2C | `I2C.c / I2C.h` | I2C1 master at 100 kHz; single-byte register read/write; burst receive/transmit; timeout/error support |
| ButtonLED | `ButtonLED.c / ButtonLED.h` | PF1/PF2/PF3 LED output; SW1/SW2 interrupt input; debounce using `WTIMER_Millis()` |
| Servo | `Servo.c / Servo.h` | PB7/M0PWM1 hardware PWM at 50 Hz; angle-to-pulse mapping |
| LCD | `LCD.c / LCD.h` | 16x2 LCD over PCF8574 I2C backpack; 4-bit command/data output; text and integer display |
| TCS34727 | `TCS34727.c / TCS34727.h` | Color sensor ID check, raw RGBC reads, scaled RGB computation, dominant color detection |
| MPU6050 | `MPU6050.c / MPU6050.h` | IMU ID check, accel/gyro reads, gyro calibration, roll/pitch/yaw calculation, servo angle output |
| main | `main.c` | Final integration loop, startup sequence, 100 ms sensor/servo scheduling, 1 second LCD/UART scheduling, switch behavior |

## 4.3 Call Graph

```text
main()
├── WTIMER_Init()
├── LED_Init()
├── BTN_Init()
├── UART0_Init()
├── Servo_Init()
├── EnableInterrupts()
├── I2C_Init()
├── LCD_Init()
├── LCD_StartupMessage("Rodrigo", "Can")
├── PrintStartupHeader()
├── TCS34727_Init()
│   ├── I2C_Receive()
│   └── I2C_Transmit()
├── MPU6050_Init()
│   ├── MPU6050_CheckID()
│   │   └── I2C_Receive()
│   └── MPU6050_Enable()
│       └── I2C_Transmit()
├── IMU_ClearHandle()
├── MPU6050_CalibrateGyro()
│   └── MPU6050_ReadGyro()
│       └── I2C_Burst_Receive()
└── while(1)
    ├── BTN_SW1Pressed()
    │   └── pause/resume systemRunning
    ├── BTN_SW2Pressed()
    │   ├── UART0_PrintSystemData()
    │   └── LCD_PrintSystemData()
    ├── every 100 ms
    │   ├── TCS34727_GET_RGB()
    │   │   └── TCS34727_ReadAllRaw()
    │   │       └── I2C_Receive()
    │   ├── Detect_Color()
    │   ├── MPU6050_UpdateAngles()
    │   │   └── MPU6050_ReadAll()
    │   │       ├── MPU6050_ReadAccel()
    │   │       │   └── I2C_Burst_Receive()
    │   │       └── MPU6050_ReadGyro()
    │   │           └── I2C_Burst_Receive()
    │   ├── MPU6050_GetServoAngleY()
    │   ├── Drive_Servo()
    │   │   └── Servo_SetAngle()
    │   └── LED_ShowDetectedColor()
    └── every 1 second
        ├── UART0_PrintSystemData()
        └── LCD_PrintSystemData()

Interrupts:
├── WideTimer1A_Handler()
│   ├── g_msTicks++
│   └── PB1 timing toggle
└── GPIOPortF_Handler()
    ├── SW1_PressedFlag
    └── SW2_PressedFlag
```

## 4.4 Data Flow Diagram

```text
TCS34727 Color Sensor
    |
    | Raw C/R/G/B values over I2C1
    v
TCS34727_GET_RGB()
    |
    | Raw R/G/B/C + Scaled R/G/B
    v
Detect_Color()
    |
    +-----------------------> LED_ShowDetectedColor()
    |
    +-----------------------> LCD_PrintSystemData()
    |
    +-----------------------> UART0_PrintSystemData()

MPU6050 IMU
    |
    | Raw accel/gyro over I2C1
    v
MPU6050_UpdateAngles()
    |
    | Roll, Pitch, Yaw, TiltY
    v
MPU6050_GetServoAngleY()
    |
    +-----------------------> Drive_Servo()
    |
    +-----------------------> LCD_PrintSystemData()
    |
    +-----------------------> UART0_PrintSystemData()

WTIMER1A
    |
    | 1 ms tick
    v
WTIMER_HasElapsed()
    |
    +---- 100 ms schedule: sensors + servo + LED
    |
    +---- 1 second schedule: LCD + UART

SW1/SW2
    |
    | GPIO interrupt flags
    v
main loop
    |
    +---- SW1: pause/resume
    +---- SW2: force LCD/UART update
```

## 4.5 Software Flowchart

```text
+-------------------------+
| Power On                |
+-----------+-------------+
            |
+-----------v-------------+
| Initialize WTIMER       |
| Initialize LED/Button   |
| Initialize UART0        |
| Initialize Servo PWM    |
| Enable Interrupts       |
| Initialize I2C1         |
| Initialize LCD          |
+-----------+-------------+
            |
+-----------v-------------+
| LCD Startup Message     |
| Print UART Header       |
| Check TCS34727 ID       |
| Check MPU6050 ID        |
| Calibrate MPU6050 Gyro  |
+-----------+-------------+
            |
+-----------v-------------+
| Final Integrated Loop   |
+-----------+-------------+
            |
      +-----v------+
      | SW1 Press? |
      +-----+------+
            | Yes
            v
  Toggle systemRunning
            |
      +-----v------+
      | SW2 Press? |
      +-----+------+
            | Yes
            v
  Force UART/LCD Update
            |
      +-----v---------------------------+
      | systemRunning and 100 ms passed?|
      +-----+---------------------------+
            | Yes
            v
  Read color sensor
  Read IMU
  Detect color
  Compute servo angle
  Update servo
  Update LED
            |
      +-----v---------------------------+
      | systemRunning and 1 s passed?   |
      +-----+---------------------------+
            | Yes
            v
  Print UART data
  Update LCD data
            |
            v
       Loop forever
```

---

# 5. Module Implementation & Testing

## 5.1 Module List and Responsibility

| Module | Responsibility |
|--------|----------------|
| WTIMER | Provides a 1 ms system tick, blocking millisecond delay, elapsed-time scheduler, and PB1 timing output. |
| UART0 | Provides serial terminal debugging and formatted output at 115200 baud. |
| I2C | Provides I2C1 initialization, single-byte register access, burst access, LCD direct byte write, and error/time-out handling. |
| ButtonLED | Provides onboard RGB LED control and interrupt-driven SW1/SW2 button detection with debounce. |
| Servo | Provides 50 Hz hardware PWM and maps angle commands to servo pulse width. |
| LCD | Provides 16x2 display control through an I2C backpack. |
| TCS34727 | Provides color sensor initialization, part ID verification, raw RGBC reads, scaled RGB processing, and color detection. |
| MPU6050 | Provides IMU initialization, WHO_AM_I verification, accel/gyro reads, gyro calibration, complementary filter, and servo angle generation. |
| main.c | Provides final integrated behavior, startup sequence, timed superloop scheduling, switch behavior, UART output, and LCD output. |

## 5.2 Module Test Strategy

All modules were tested independently before final integration. This followed CN-07: full system integration must not begin until all independent module tests pass. The module tests verified that each hardware peripheral and sensor module worked in isolation before being combined in the final main loop.

WTIMER was verified with oscilloscope timing evidence. UART0 was verified with readable terminal output and formatted number printing. I2C was verified through startup device ID reads. TCS34727 was tested with raw/scaled RGB output and color detection. MPU6050 was tested with raw accel/gyro values, filtered angle output, and servo response. LCD was tested with startup and color/angle text. Servo was tested with commanded angle movement.

For final timing validation, PB1 was configured to verify the 100 ms sampling interval and the 1 second output interval using oscilloscope screenshots.

## 5.3 Module Test Summary Table

| Module | Test ID | Test Type | Evidence Reference |
|--------|---------|----------|-------------------|
| WTIMER | MOD-WTIMER-01 | Oscilloscope timing test | `Evidence/Step3_SystemTests/SYS-01_WTIMER_100ms_Oscilloscope.png`, `Evidence/Step3_SystemTests/SYS-02_WTIMER_1s_Oscilloscope.png` |
| UART0 | MOD-UART0-01 | Terminal output test | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png`, `Evidence/Step3_SystemTests/SYS-04_UART_StartupPrompt_IDChecks.png` |
| I2C | MOD-I2C-01 | Device ID verification | `Evidence/Step3_SystemTests/SYS-04_UART_StartupPrompt_IDChecks.png` |
| ButtonLED | MOD-BTNLED-01 | Switch/LED behavior test | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |
| Servo | MOD-SERVO-01 | PWM/angle movement test | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |
| TCS34727 | MOD-TCS-01 | Color sensor output test | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png`, `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |
| MPU6050 | MOD-MPU-01 | IMU sensor output test | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png`, `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |
| LCD | MOD-LCD-01 | LCD display test | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |
| Full System | SYS-01 | Integrated runtime test | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` |

Evidence files match the naming convention defined in the `/evidence` folder.

## 5.4 Team Assignment

| Team Member | Role | Assigned Work |
|-------------|------|---------------|
| Rodrigo Can | Integration / Firmware | Final integration, WTIMER, UART0, I2C, ButtonLED, Servo, LCD, TCS34727, MPU6050, main.c, evidence organization |
| Miguel Constantino | Testing / Documentation | System validation support, timing evidence support, report/traceability updates |
| Matthew Margulies | Hardware / Evidence | Sensor and servo evidence collection, hardware setup review |
| Kyle Leng | Verification / Review | Requirements validation, UART evidence review, final report support |

---

# 6. System Integration & Validation

## 6.1 Integration Order & Rationale

The final system was integrated in dependency order so that each added module had its required lower-level support already working. This maintained the module-first workflow required by CN-07.

| Step | Module(s) Integrated | Rationale |
|------|----------------------|-----------|
| 1 | WTIMER | Provides 1 ms timing for delays, debounce, scheduling, and timing evidence. |
| 2 | UART0 | Provides startup messages and debugging output for all later modules. |
| 3 | ButtonLED | Adds switch input and LED output using WTIMER-based debounce. |
| 4 | Servo | Adds hardware PWM output before IMU-driven motion is added. |
| 5 | I2C | Adds the shared communication bus required by LCD, TCS34727, and MPU6050. |
| 6 | LCD | Verifies I2C output device and startup display. |
| 7 | TCS34727 | Adds color sensor ID check, raw RGB reads, scaled RGB processing, and LED color output. |
| 8 | MPU6050 | Adds IMU ID check, gyro calibration, filtered pitch/roll, and servo control. |
| 9 | Final main loop | Combines 100 ms sensor/servo updates, 1 second LCD/UART updates, SW1 pause/resume, and SW2 force update. |

This order was chosen because the timer and UART are needed for nearly every test. I2C was added before I2C devices, and each I2C device was verified before the next one was added. The final loop was added only after the independent modules passed their tests.

## 6.2 System-Level Test Matrix

| Requirement | Test Method | Evidence | Pass/Fail |
|-------------|------------|----------|-----------|
| RQ-01/RQ-12/RQ-16: System initializes modules and verifies sensors | Observe UART startup prompt and part ID register output for both sensors | `Evidence/Step3_SystemTests/SYS-04_UART_StartupPrompt_IDChecks.png` | Pass |
| RQ-25/CN-01: 100 ms sensor/servo timing | Toggle PB1 using WTIMER timing test and measure with oscilloscope | `Evidence/Step3_SystemTests/SYS-01_WTIMER_100ms_Oscilloscope.png` | Pass |
| RQ-26/CN-01: 1 second LCD/UART output timing | Toggle PB1 using 1 second timing test and measure with oscilloscope | `Evidence/Step3_SystemTests/SYS-02_WTIMER_1s_Oscilloscope.png` | Pass |
| RQ-07/RQ-13/RQ-14/RQ-17/RQ-18: UART prints final sensor output | Observe detected color, raw RGB, scaled RGB, roll/pitch/yaw, and servo angle in terminal | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png` | Pass |
| RQ-13/RQ-14/RQ-15: TCS34727 reads and processes color data | Present colored objects and observe detected color, LED, LCD, and UART output | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png`; `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-17/RQ-18/RQ-19/RQ-23: MPU6050 reads and processes IMU data | Tilt board and observe roll/pitch/yaw values and servo response | `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png`; `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-20/RQ-21: LCD displays color and angle | Observe LCD row 1 showing color and row 2 showing angle | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-22/RQ-23/RQ-27: Servo follows IMU tilt | Tilt MPU6050 and observe servo movement following pitch angle | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-15/RQ-27: LED indicates detected color | Present red/green/blue objects and observe onboard LED color behavior | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-28: SW1 pauses/resumes system | Press SW1 and observe paused/resumed UART/LCD behavior | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-29: SW2 forces display update | Press SW2 and observe immediate LCD/UART update | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| RQ-26/RQ-27/ROB-01: Full system runs without freeze or reset | Run integrated system while changing color inputs and tilting IMU | `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |

## 6.3 Team Assignment

| Team Member | Integration Responsibility |
|-------------|----------------------------|
| Rodrigo Can | Final system integration, full main loop, switch behavior, evidence naming, timing validation |
| Miguel Constantino | Validation table support, Step 3 evidence review, final timing checks |
| Matthew Margulies | Hardware setup support, full-system demo evidence, sensor/servo demonstration |
| Kyle Leng | Requirements verification, UART output review, final report consistency check |

---

# 7. Claim–Evidence–Reasoning (CER)

### Claim 1: The WTIMER module provides the timing base required for final integration.

**Evidence:** `Evidence/Step3_SystemTests/SYS-01_WTIMER_100ms_Oscilloscope.png` and `Evidence/Step3_SystemTests/SYS-02_WTIMER_1s_Oscilloscope.png`

**Reasoning:** The final system depends on two timing intervals: 100 ms for sensor/servo updates and 1 second for LCD/UART updates. The oscilloscope screenshots show that the WTIMER-based timing signal was measurable at both required timing intervals. This supports that the software timing base used by the final loop is functioning. The final design keeps `WideTimer1A_Handler()` short by only updating the millisecond counter and PB1 timing signal, while the main loop uses `WTIMER_HasElapsed()` to schedule 100 ms and 1 second work.

---

### Claim 2: The I2C sensors are detected correctly during startup.

**Evidence:** `Evidence/Step3_SystemTests/SYS-04_UART_StartupPrompt_IDChecks.png`

**Reasoning:** The startup prompt displays the sensor address/register checks for the color sensor and MPU6050. The TCS34727 ID register returns the detected color sensor part ID, and the MPU6050 WHO_AM_I register returns 0x68. These startup checks confirm that both I2C sensors responded during initialization, which reduces the chance that later sensor values are coming from an uninitialized, disconnected, or incorrectly addressed device.

---

### Claim 3: The final system successfully combines sensor input, processing, and output.

**Evidence:** `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png` and `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`

**Reasoning:** The UART output shows raw RGB values, scaled RGB values, detected color, IMU angles, and servo angle. The video confirms that these values are not isolated debug prints only; they correspond to visible LCD updates, LED color changes, and servo movement during one integrated run. This shows that the WTIMER, UART0, I2C, TCS34727, MPU6050, LCD, ButtonLED, and Servo modules are operating together as one complete system.

---

### Claim 4: The servo control is linked to MPU6050 angle data.

**Evidence:** `Evidence/Step3_SystemTests/SYS-03_UART_SensorOutput.png` and `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`

**Reasoning:** The MPU6050 calculates filtered angle values from accelerometer and gyroscope data, and the final system maps the filtered pitch angle to the servo command. The UART evidence shows the angle and servo value, while the video shows the servo responding when the IMU is moved. This supports that the servo is controlled by sensor input rather than by a fixed unit-test sequence.

---

### Claim 5: The system remains stable under normal integrated operation.

**Evidence:** `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`

**Reasoning:** The robustness demo runs multiple features at once: I2C color sensing, I2C IMU sensing, I2C LCD output, UART output, PWM servo control, onboard RGB LED output, and switch handling. Since the system continues to update and respond without freezing, restarting, locking the I2C bus, or corrupting the LCD output, the integration is stable enough for final demonstration.

**Rejected AI Suggestion:** An AI suggestion recommended moving the 100 ms sensor reads and 1 second LCD/UART updates into `WideTimer1A_Handler`. This suggestion was rejected because I2C, LCD, and UART operations are too slow for an ISR and could block other interrupts or create timing problems. The accepted design keeps the timer ISR short and performs scheduled tasks in the main loop using `WTIMER_HasElapsed()`.

---

# 8. Demonstration Summary

The final demonstration showed the complete I2C bus communication system running on the TM4C123GXL. At startup, the LCD displayed the name message and UART printed the startup prompt. The UART startup prompt also showed sensor ID verification for the TCS34727 and MPU6050.

During the demo, the TCS34727 color sensor detected color changes, the onboard RGB LED indicated the detected color, the LCD displayed the color and angle, and UART printed raw RGB values, scaled RGB values, roll/pitch/yaw values, and servo angle. The MPU6050 was tilted to change the filtered pitch value, and the servo followed the IMU tilt in real time.

The robustness condition tested was continuous full-system operation under combined activity. The system was run while color objects were changed, the IMU was tilted, the servo moved, the LCD updated, UART output continued, and switch behavior was tested. The expected behavior was that the system would not freeze, reset, corrupt the LCD, or lose I2C communication. The actual result matched the expected behavior.

**Demonstration video:** `Evidence/Step3_SystemTests/SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`

---

# 9. Conclusion

The final project successfully implemented an I2C-based sensor fusion and servo control system on the TM4C123GH6PM microcontroller. The system integrates WTIMER, UART0, I2C1, TCS34727, MPU6050, LCD, ButtonLED, and Servo PWM modules into one final application.

The main technical achievements were the successful use of one shared I2C bus for three devices, timer-based scheduling for multiple update rates, color detection from raw and scaled RGB values, IMU angle estimation using a complementary filter, and real-time servo control from IMU tilt. The final system also supports SW1 pause/resume and SW2 force display update behavior.

The main challenges were timing validation, I2C device verification, LCD update stability, and servo power/current stability. These were solved through oscilloscope timing tests, UART startup ID checks, careful separation of ISR and foreground work, and stable servo wiring. A key lesson learned was that embedded systems integration should be done in dependency order, with every module tested independently before final integration.

---

# 10. References

Texas Instruments. *Tiva TM4C123GH6PM Microcontroller Data Sheet.* Texas Instruments.

Texas Instruments. *TivaWare Peripheral Driver Library User's Guide.* Texas Instruments.

ams OSRAM. *TCS3472 Color Light-to-Digital Converter Datasheet.* ams OSRAM.

TDK InvenSense. *MPU-6050 Product Specification.* TDK InvenSense.

Hitachi. *HD44780U LCD Controller/Driver Datasheet.* Hitachi.

CECS 447 Final Project Specification. *I2C Bus Communication System.* California State University, Long Beach.

AI Usage Log. `ai_log_summary.csv` and Step 3 AI tutoring discussion records.

OpenAI. ChatGPT. Used for debugging support, integration planning, report editing, AI log support, and verification discussion.
