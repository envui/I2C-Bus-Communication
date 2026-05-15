# Step 3 – Integration & Validation

## 1. Integration Order

The final system was integrated only after the independent module tests were completed. The modules were connected in an order that reduced risk and made debugging easier. Low-level timing and communication modules were integrated first, followed by the I2C devices, then the user outputs, and finally the full closed-loop sensor-to-servo behavior.

| Order | Module / Feature | Reason for Integration Order | Result |
|------:|------------------|------------------------------|--------|
| 1 | WTIMER | The timer provides the 1 ms software time base used by delays, button debounce, sensor sampling, LCD updates, and timing verification. | Passed. PB1 timing was verified with oscilloscope evidence at 100 ms and 1 second intervals. |
| 2 | UART0 | UART0 was needed early so that startup messages, sensor ID checks, and runtime data could be observed during integration. | Passed. Startup prompt and sensor output were displayed correctly. |
| 3 | I2C Bus | The I2C bus had to be working before the LCD, color sensor, and MPU6050 could be integrated. | Passed. I2C1 on PA6/PA7 communicated with all connected I2C devices. |
| 4 | LCD | The LCD was added after I2C communication was stable so display updates could be tested without sensor logic first. | Passed. LCD startup and final Color/Angle display worked. |
| 5 | TCS34727 Color Sensor | The color sensor was integrated after I2C and UART so its ID check and raw/scaled RGB values could be verified. | Passed. The sensor returned its part ID and produced changing RGB values. |
| 6 | MPU6050 IMU | The IMU was integrated after the I2C bus was stable because it requires multi-byte I2C reads and calibration. | Passed. WHO_AM_I was verified and angle values updated during motion. |
| 7 | Servo PWM | Servo output was connected after IMU angle calculation was stable so the servo could follow filtered pitch values. | Passed. Servo responded to IMU tilt. |
| 8 | Switch Functions | SW1/SW2 were added to the final loop after the continuous system worked. SW1 pauses/resumes the system and SW2 forces a display update. | Passed. Switch behavior did not break the 100 ms and 1 second timing structure. |

This order was chosen because each later module depends on earlier system services. WTIMER and UART0 provide observability and timing. I2C provides the shared communication bus. The LCD and sensors depend on I2C. The servo depends on valid IMU angle data. The final switch functions were added last because they control the already-integrated system behavior.

---

## 2. System Architecture

The final project is an I2C-based sensor fusion system running on the TM4C123 LaunchPad. The system reads color data from the TCS34727 color sensor, reads motion data from the MPU6050 IMU, displays status on a 16x2 I2C LCD, prints debug data over UART0, and controls a servo motor using PWM. The WTIMER module provides the timing base for 100 ms sensor updates and 1 second display/UART updates.

### 2.1 Final Call Graph

```text
main
|
|-- WTIMER_Init
|   |-- PB1_Init
|   `-- WideTimer1A_Handler
|       |-- g_msTicks++
|       `-- PB1 toggle for timing verification
|
|-- LED_Init
|-- BTN_Init
|   `-- GPIOPortF_Handler
|       |-- debounce using WTIMER_Millis
|       |-- set SW1 flag
|       `-- set SW2 flag
|
|-- UART0_Init
|   |-- UART0_OutString
|   |-- UART0_OutUDec
|   |-- UART0_OutSDec
|   |-- UART0_OutFloat
|   `-- UART0_OutUHex
|
|-- Servo_Init
|   `-- Drive_Servo / Servo_SetAngle
|
|-- I2C_Init
|   |-- I2C_Receive
|   |-- I2C_Transmit
|   |-- I2C_Burst_Receive
|   |-- I2C_Burst_Transmit
|   `-- I2C_SendByte
|
|-- LCD_Init
|   |-- LCD_StartupMessage
|   |-- LCD_Clear
|   |-- LCD_SetCursor
|   |-- LCD_PrintText
|   `-- LCD_PrintInteger
|
|-- TCS34727_Init
|   |-- TCS34727_CheckID
|   |-- TCS34727_ReadAllRaw
|   |-- TCS34727_GET_RGB
|   `-- Detect_Color
|
|-- MPU6050_Init
|   |-- MPU6050_CheckID
|   |-- MPU6050_Enable
|   |-- MPU6050_CalibrateGyro
|   |-- MPU6050_UpdateAngles
|   `-- MPU6050_GetServoAngleY
|
`-- while(1)
    |-- BTN_SW1Pressed -> pause/resume system
    |-- BTN_SW2Pressed -> force LCD/UART update
    |-- every 100 ms
    |   |-- TCS34727_GET_RGB
    |   |-- Detect_Color
    |   |-- MPU6050_UpdateAngles
    |   |-- MPU6050_GetServoAngleY
    |   |-- Drive_Servo
    |   `-- LED_ShowDetectedColor
    |
    `-- every 1 second
        |-- UART0_PrintSystemData
        `-- LCD_PrintSystemData
```

### 2.2 Data Flow Diagram

```text
                  +----------------------------+
                  |     TM4C123 LaunchPad      |
                  |                            |
                  |  WTIMER1A 1 ms time base   |
                  |       |                    |
                  |       v                    |
                  |  Main Scheduling Loop      |
                  |  - 100 ms sensor update    |
                  |  - 1 s display update      |
                  +------------+---------------+
                               |
             +-----------------+-----------------+
             |                                   |
             v                                   v
       +-------------+                     +-------------+
       | I2C1 Bus    |                     | PWM Output  |
       | PA6 / PA7   |                     | PB7 M0PWM1  |
       +------+------+                     +------+------+ 
              |                                   |
   +----------+-----------+                       v
   |          |           |                 +------------+
   v          v           v                 | Servo Motor |
+------+  +---------+  +---------+          +------------+
| LCD  |  | TCS34727|  | MPU6050 |
|0x27  |  | 0x29    |  | 0x68    |
+------+  +---------+  +---------+
   ^          |           |
   |          v           v
   |     RGB raw/scaled   Roll/Pitch/Yaw
   |          |           |
   +----------+-----------+
              |
              v
       +--------------+
       | System State |
       | Color, Angle |
       +------+-------+
              |
   +----------+-----------+
   |          |           |
   v          v           v
+------+  +--------+  +----------+
| LCD  |  | UART0  |  | RGB LEDs |
|Text  |  |Terminal|  |PF1-PF3   |
+------+  +--------+  +----------+

Switch Inputs:
SW1 -> pause/resume final system loop
SW2 -> force immediate LCD/UART update
```

---

## 3. System-Level Validation

| Requirement | Test Method | Evidence | Pass/Fail |
|-------------|------------|----------|-----------|
| WTIMER provides a stable 100 ms timing reference for sensor and servo updates. | PB1 was toggled using the WTIMER interrupt timing logic and measured on an oscilloscope. | `SYS-01_WTIMER_100ms_Oscilloscope.png` | Pass |
| WTIMER supports a 1 second interval for LCD and UART updates. | PB1 timing was changed to the 1 second interval and measured on an oscilloscope. | `SYS-02_WTIMER_1s_Oscilloscope.png` | Pass |
| UART0 prints final sensor and system data. | The final integrated system was run and the UART terminal output was captured. The output included detected color, raw RGB, scaled RGB, IMU angles, and servo angle. | `SYS-03_UART_SensorOutput.png` | Pass |
| Startup verifies I2C sensor presence. | The system startup prompt printed the color sensor ID register result and the MPU6050 WHO_AM_I result. | `SYS-04_UART_StartupPrompt_IDChecks.png` | Pass |
| The final integrated system runs all major outputs together. | A full-system video showed the color sensor, LED output, LCD output, UART output, IMU motion, and servo response operating together. | `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| LCD displays user-facing final data. | During the full system demo, the LCD displayed color and angle information while the system was running. | `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| Servo follows IMU tilt in real time. | The MPU6050 was tilted during the full system demo and the servo position changed in response to the measured pitch angle. | `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| LED indicates detected color. | Red, green, and blue color cases were tested during full-system operation and the onboard LED matched the detected color state. | `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |
| SW1 and SW2 provide final user control. | SW1 was used for pause/resume behavior and SW2 was used to force a manual LCD/UART update. | `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov` | Pass |

---

## 4. Robustness Test

### Robustness Test: Continuous Full-System Operation

- **What was tested:** The full integrated system was allowed to run while the color sensor input changed, the MPU6050 was tilted, the servo moved, the LCD updated, and UART data continued printing. This test stressed multiple system activities at the same time: I2C sensor reads, I2C LCD writes, UART output, PWM servo control, timer scheduling, LED output, and switch handling.

- **Expected behavior:** The system should continue running without freezing, restarting, losing I2C communication, producing LCD garbage, stopping UART output, or losing servo response. The LCD and UART should update at the 1 second interval, while sensor reading and servo control should continue at the 100 ms interval.

- **Actual result:** The system continued operating during the full integration and stability demo. UART output continued, the LCD remained readable, the servo responded to IMU tilt, and the detected color output continued updating. No system freeze or reset was observed during the recorded test.

- **Evidence:** `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`

---

## 5. Claim–Evidence–Reasoning (CER)

### Claim 1: The WTIMER module provides the timing base required for final integration.

- **Evidence:** `SYS-01_WTIMER_100ms_Oscilloscope.png` and `SYS-02_WTIMER_1s_Oscilloscope.png`
- **Reasoning:** The final system depends on two timing intervals: 100 ms for sensor/servo updates and 1 second for LCD/UART updates. The oscilloscope screenshots show that the WTIMER-based timing signal was measurable at both required timing intervals. This supports that the software timing base used by the final loop is functioning.

### Claim 2: The I2C sensors are detected correctly during startup.

- **Evidence:** `SYS-04_UART_StartupPrompt_IDChecks.png`
- **Reasoning:** The startup prompt displays the sensor address/register checks for the color sensor and MPU6050. The color sensor ID register and the MPU6050 WHO_AM_I register confirm that both I2C devices responded during initialization. This reduces the chance that later sensor values are coming from an uninitialized or missing device.

### Claim 3: The final system successfully combines sensor input, processing, and output.

- **Evidence:** `SYS-03_UART_SensorOutput.png` and `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`
- **Reasoning:** The UART output shows raw RGB values, scaled RGB values, detected color, IMU angles, and servo angle. The video confirms that these values are not isolated prints only; they correspond to visible LCD updates, LED changes, and servo movement. This shows that the modules are integrated into one working system.

### Claim 4: The servo control is linked to MPU6050 angle data.

- **Evidence:** `SYS-03_UART_SensorOutput.png` and `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`
- **Reasoning:** The MPU6050 calculates filtered angle values from accelerometer and gyro data, and the final system maps the pitch angle to the servo command. The UART evidence shows the angle and servo value, while the video shows the servo responding when the IMU is moved. This supports that the servo is controlled by sensor input rather than a fixed test sequence.

### Claim 5: The system remains stable under normal integrated operation.

- **Evidence:** `SYS-ROB-01_FullSystem_Integration_StabilityDemo.mp4.mov`
- **Reasoning:** The robustness demo runs multiple features at once: I2C sensors, I2C LCD, UART, PWM servo, RGB LEDs, and switch handling. Since the system continues to update and respond without freezing or restarting, the integration is stable enough for final demonstration.

---

## 6. Final Demo Summary

The final demonstration showed the complete I2C Bus Communication System operating as an integrated embedded system. The system initialized the timer, UART0, I2C bus, LCD, color sensor, MPU6050, LEDs, buttons, and servo PWM. During startup, the UART terminal displayed the initialization prompt and the I2C sensor ID checks.

During normal operation, the system read the color sensor and IMU at the 100 ms update interval. The detected color was shown through the onboard RGB LED and the LCD. The MPU6050 pitch angle was mapped to the servo command, causing the servo to follow the IMU tilt. Every 1 second, the UART terminal and LCD updated with the current color and angle information.

Key results:

- The system initialized successfully.
- The color sensor and MPU6050 responded to ID checks.
- UART0 printed final system data.
- The LCD showed color and angle information.
- The servo followed IMU tilt.
- The onboard LED matched the detected color.
- SW1 and SW2 provided final user control.
- The system continued running during the stability demo.

Observed behavior matched the final project requirements. No freeze or reset was observed during the recorded full-system demonstration.

---

## 7. AI Verification Summary

AI was used only as verification and debugging support during the integration process. The final implementation decisions, code testing, hardware setup, and evidence collection were completed by the team.

### Debugging assistance

AI was used to help reason about integration order, timing intervals, UART output formatting, and possible causes of random resets during servo testing. The servo reset discussion helped identify that high servo current draw or wiring issues could cause voltage drops, but the team corrected the hardware/testing setup and verified system stability directly.

### Verification support

AI was used to help compare the implemented behavior against the final project description. This included checking that the final system performed 100 ms sensor/servo updates, 1 second LCD/UART updates, sensor ID verification, LED color output, servo response, and switch functionality.

### One rejected AI suggestion
An AI suggestion recommended placing the I2C sensor reads and LCD updates directly inside the timer interrupt service routine so that the 100 ms and 1 second timing would be handled completely inside `WideTimer1A_Handler`. This suggestion was rejected because I2C transactions, LCD writes, and UART output can take too long and should not run inside an ISR. The final design keeps the ISR short by only incrementing millisecond counters and toggling PB1 for timing verification, while the main loop uses `WTIMER_HasElapsed()` to schedule sensor reads, servo updates, LCD output, and UART output.

