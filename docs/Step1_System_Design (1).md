# Step 1 – System Design

## 1. Functional Requirements

-   RQ-01: The system shall initialize all modules and verify sensor
    presence at startup.
-   RQ-02: The system shall initialize a wide timer to provide a 1
    millisecond timing base.
-   RQ-03: The system shall implement a delay function with ±5% timing
    accuracy.
-   RQ-04: The system shall initialize onboard LEDs and push buttons for
    user interaction.
-   RQ-05: The system shall use SW1 to toggle system modes and SW2 to
    cycle LED colors.
-   RQ-06: The system shall initialize UART0 at a valid baud rate (not
    57600).
-   RQ-07: The system shall transmit formatted data over UART at a 1 Hz
    update rate.
-   RQ-08: The system shall initialize an I2C module and operate at 100
    kHz standard mode.
-   RQ-09: The system shall perform single-byte I2C read/write
    operations.
-   RQ-10: The system shall perform burst I2C read/write operations.
-   RQ-11: The system shall handle I2C communication errors without
    system failure.
-   RQ-12: The system shall initialize the TCS34725 sensor and verify
    its device ID.
-   RQ-13: The system shall read raw RGB and clear channel data from the
    TCS34725 sensor.
-   RQ-14: The system shall normalize RGB values and compute a dominant
    color.
-   RQ-15: The system shall initialize the MPU6050 sensor and verify its
    device ID.
-   RQ-16: The system shall read raw accelerometer and gyroscope data
    from the MPU6050.
-   RQ-17: The system shall process sensor data into physical units (g
    and deg/s).
-   RQ-18: The system shall compute tilt angles from IMU data.
-   RQ-19: The system shall initialize the LCD and display output
    without corruption.
-   RQ-20: The system shall display color and angle data on the LCD.
-   RQ-21: The system shall initialize PWM for servo control at 50 Hz.
-   RQ-22: The system shall map angle values to PWM signals to control
    servo position.
-   RQ-23: The system shall execute the required servo motion sequence
    during testing.
-   RQ-24: The system shall sample sensor data every 100 milliseconds.
-   RQ-25: The system shall update LCD and UART outputs every 1 second.
-   RQ-26: The system shall maintain continuous real-time operation
    without interruption.
-   RQ-27: The system shall control servo motion in real time based on
    IMU tilt while displaying outputs.

## 2. Constraints

-   CN-01: Sensor sampling must occur every 100 milliseconds and
    LCD/UART outputs must update every 1 second (±100 ms tolerance).
-   CN-02: The system must use only allowed peripherals (I2C1--3,
    WTIMER1--5, PWM modules; I2C0 and WTIMER0 are not allowed).
-   CN-03: UART0 must use a valid baud rate that is not 57600.
-   CN-04: The I2C bus must operate at 100 kHz and use external 4.7 kΩ
    pull-up resistors.
-   CN-05: All I2C device addresses must match required hardware
    specifications.
-   CN-06: Servo PWM must operate at 50 Hz with valid pulse width range.
-   CN-07: No integration is allowed until all modules pass their
    individual tests.

## 2.1 Design Requirements

-   DES-01: The system shall use a shared I2C bus architecture for all
    devices to minimize pin usage.
-   DES-02: The system shall use a modular software architecture
    separating drivers and application logic.
-   DES-03: The system shall use timer-based scheduling with separate
    rates for sensing (100 ms) and output updates (1 s).

## 2.2 System Requirements

-   SYS-01: The system shall initialize all modules and detect all I2C
    devices before entering the main loop.
-   SYS-02: The system shall continuously read sensors, process data,
    and update outputs at the required rates.
-   SYS-03: The system shall operate as an integrated system where
    sensing, display, communication, and actuation function
    simultaneously without failure.

## 3. System Design

---

## 3.1 Hardware Design

### Microcontroller
- TM4C123 LaunchPad (main processor)

---

### Peripheral Selection
- **I2C Module:** I2C1 (used for all external devices on shared bus)  
- **UART Module:** UART0 (used for PC communication/debugging)  
- **PWM Module:** M0PWM3 (used for servo motor control)  
- **Wide Timer:** WTIMER1 (used for 1 ms timing base)  

---

### Pin Assignments

#### UART0 (Serial Communication to PC)
- PA0 → UART0 RX  
- PA1 → UART0 TX  
- Used to send formatted color and angle data to terminal  

---

#### I2C1 Bus (Shared Communication Bus)
- PA6 → SCL (clock line)  
- PA7 → SDA (data line)  

Connected devices on same bus:
- TCS34725 Color Sensor → Address 0x29  
- MPU6050 IMU → Address 0x68  
- LCD I2C Backpack → Address 0x27  

---

#### PWM Output (Servo Control)
- PB5 → M0PWM3  
- Generates 50 Hz PWM signal for servo positioning  

---

#### Switch Inputs (User Control)
- PF4 → SW1 (mode/select input)  
- PF0 → SW2 (secondary control input)  

---

#### Onboard LED Outputs (Status Indication)
- PF1 → Red LED  
- PF2 → Blue LED  
- PF3 → Green LED  
- Used to indicate detected dominant color  

---

### External Component Connections

#### TCS34725 Color Sensor
- VCC → 3.3V  
- GND → GND  
- SDA → PA7  
- SCL → PA6  

---

#### MPU6050 IMU Sensor
- VCC → 3.3V  
- GND → GND  
- SDA → PA7  
- SCL → PA6  

---

#### 16x2 LCD (I2C Backpack)
- VCC → 5V  
- GND → GND  
- SDA → PA7  
- SCL → PA6  

---

#### Servo Motor
- Signal → PB5 (PWM output)  
- Power → External 5V supply  
- GND → Common ground with TM4C  

---

#### I2C Pull-up Resistors
- SDA → 4.7 kΩ → 3.3V  
- SCL → 4.7 kΩ → 3.3V  
- Required for proper I2C communication  

---

### Key Hardware Characteristics
- I2C Bus Speed: **100 kHz (Standard Mode)**  
- PWM Frequency: **50 Hz (20 ms period)**  
- Servo Pulse Range: **1 ms – 2 ms**  
- Timer Resolution: **1 ms using WTIMER1**  

---

## 3.2 Software Architecture

### System Structure
The software is organized into **driver modules** (hardware-level control) and **application modules** (logic and coordination). Each module has a single responsibility and interacts through clearly defined functions.

---

### Module Descriptions

#### 1. WTIMER Module
- Initializes WTIMER1 for 1 ms resolution  
- Provides delay and timing functions  
- Used to schedule periodic tasks:
  - 100 ms sensor sampling  
  - 1 second output updates  

---

#### 2. UART0 Module
- Initializes UART0 with selected baud rate (e.g., 115200)  
- Sends characters, strings, integers, and formatted values  
- Outputs system data every 1 second  
- Used for debugging and monitoring system behavior  

---

#### 3. I2C1 Driver
- Initializes I2C1 at 100 kHz  
- Implements low-level functions:
  - I2C_Write (send register data)  
  - I2C_Read (read multiple bytes)  
- Handles communication with all I2C devices  
- Includes basic error handling (timeout/NAK detection)  

---

#### 4. TCS34725 Module
- Initializes color sensor and verifies ID register  
- Enables sensor operation  
- Reads 16-bit RGB values  
- Provides raw color data to processing module  

---

#### 5. MPU6050 Module
- Initializes IMU and verifies WHO_AM_I register  
- Enables accelerometer and gyroscope  
- Reads acceleration and rotation data  
- Provides motion/tilt data for angle calculation  

---

#### 6. LCD Module
- Initializes LCD via I2C backpack  
- Controls cursor position and display updates  
- Displays:
  - Row 1: "Color: [RED/GREEN/BLUE]"  
  - Row 2: "Angle: [value]"  
- Updates output every 1 second  

---

#### 7. Servo PWM Module
- Initializes PWM hardware at 50 Hz  
- Converts angle values into PWM duty cycle  
- Maps angle range (-90° to +90°) to pulse width (1–2 ms)  
- Continuously updates servo position based on IMU data  

---

#### 8. LED Module
- Controls onboard RGB LEDs  
- Turns on LED corresponding to detected dominant color  
- Provides immediate visual feedback  

---

#### 9. Switch Module
- Reads SW1 and SW2 inputs (negative logic)  
- Handles user interactions (mode changes or control input)  
- Can be used to start/stop system or change display behavior  

---

#### 10. Data Processing Module
- Processes raw RGB values to determine dominant color  
- Converts IMU readings into tilt angle  
- Applies scaling and limits to ensure valid servo range  
- Outputs processed data to display, UART, and servo modules  

---

#### 11. Main Controller
- Initializes all modules at startup  
- Verifies all sensors are connected  
- Executes main loop with timing control:

  **Every 100 ms:**
  - Read color sensor data  
  - Read IMU data  
  - Compute angle  
  - Update servo position  
  - Update LED output based on dominant color  

  **Every 1 second:**
  - Update LCD display  
  - Send formatted data over UART  

- Coordinates all modules into a single working system  
- Follows the project workflow rule that integration happens only after all modules pass their individual tests  

---

### Software Design Summary
- Modular structure separates hardware control and application logic  
- Shared I2C driver reduces redundancy across devices  
- Timer-based scheduling ensures consistent real-time behavior  
- Main controller manages system flow and integration  
- Module-first testing supports safer and more organized development  

---

## 3.3 Data Flow Chart / System Behavior

### Data Flow Chart

```text
                   +------------------+
                   |   SW1 / SW2      |
                   |  User Inputs     |
                   +--------+---------+
                            |
                            v
+-------------+     +-------+--------+     +------------------+
| TCS34725    |---->|                |---->| LED Module       |
| Color Data  |     | Data Processing|     | Dominant Color   |
+-------------+     |    Module      |     +------------------+
                    |                |
+-------------+---->|                |---->| Servo PWM Module |
| MPU6050     |     +-------+--------+     | Angle -> PWM     |
| Motion Data |             |              +------------------+
+-------------+             |
                            |
                            +-----> LCD Module
                            |       Color + Angle Display
                            |
                            +-----> UART0 Module
                                    Terminal Output
```

### System Behavior

At startup, the main controller initializes WTIMER1, UART0, I2C1, PWM, GPIO, switches, LEDs, the LCD, the TCS34725 sensor, and the MPU6050 sensor. During initialization, each hardware module is checked so the system can verify that the required devices are present before normal operation begins.

Once initialization is complete, the system enters a timed superloop. WTIMER1 provides a 1 ms software time base that allows the application to schedule periodic tasks without blocking the rest of the system.

Every 100 ms, the system reads RGB values from the TCS34725 and motion data from the MPU6050. The data processing module then determines the dominant color and computes the servo angle from the IMU tilt data. The servo PWM module immediately updates the output pulse width to move the servo, and the LED module updates the onboard LED color for quick visual feedback.

Every 1 second, the system refreshes the LCD and UART output. The LCD shows the current detected color and angle, while UART0 sends formatted diagnostic information to the terminal for debugging and system monitoring.

SW1 and SW2 provide local user input for any mode or control features required by the final implementation. These inputs are handled separately from the sensor-processing path so that user control does not interfere with periodic sampling and output updates.

Development follows a module-first workflow. Each module is tested independently before any system-level integration is allowed. This reduces debugging complexity and ensures that failures can be isolated to individual modules before the complete system is assembled.

---

## 4. Design Justification

### 4.1 Why This Design Was Chosen

The selected design was chosen to meet all project requirements while maintaining a clear, modular, and efficient system structure. A modular architecture was used to separate hardware-level drivers from application-level logic, making the system easier to develop, test, and debug. Each module performs a single well-defined function, which improves readability and allows individual components (such as UART, I2C, or PWM) to be tested independently before full system integration.

A shared I2C bus design was chosen because all three external devices (TCS34725 color sensor, MPU6050 IMU, and LCD) support I2C communication. Using a single I2C module reduces the number of required pins and simplifies wiring, while still allowing multiple devices to operate simultaneously using unique addresses. This approach also reinforces the project objective of managing multiple devices on a shared communication interface.

The system uses timer-based scheduling with WTIMER1 to ensure consistent and predictable real-time behavior. By separating tasks into two time intervals (100 ms for sensor sampling and 1 second for output updates), the design balances responsiveness and efficiency. Fast updates allow the servo to react smoothly to motion, while slower updates prevent unnecessary LCD refreshes and excessive UART output.

The use of hardware PWM for servo control was selected because it provides precise and stable signal generation at the required 50 Hz frequency. Hardware PWM reduces CPU load and eliminates timing inconsistencies that could occur with software-based PWM, resulting in smoother and more reliable servo movement.

Finally, a central main controller (superloop) was used to coordinate all modules. This approach simplifies system flow by keeping control logic in one place while still leveraging modular components. It ensures that initialization, sensor reading, data processing, and output updates are all executed in a structured and predictable manner.

### 4.2 Tradeoffs Considered

Several alternative design choices were considered during development:

- A separate communication interface for each device (e.g., SPI or multiple I2C modules) was considered but rejected because it would increase hardware complexity and pin usage. A shared I2C bus provides a simpler and more scalable solution.
- A single monolithic program structure was considered but rejected in favor of a modular design. While a single large program may be quicker to implement initially, it becomes difficult to debug, maintain, and expand as system complexity increases.
- Software-based PWM was considered for servo control but rejected because it is less precise and more CPU-intensive. Hardware PWM ensures accurate timing and more stable actuator behavior.
- A single uniform update rate for all tasks was considered but rejected. Using the same frequency for sensor reading and display updates would either slow down responsiveness or overload the system with unnecessary updates. The chosen split-rate scheduling provides a better balance between performance and efficiency.
- Powering the servo directly from the microcontroller was considered but rejected due to current limitations. Using an external 5V power source ensures stable operation and prevents voltage drops or system instability.
- Early integration was considered but rejected because it makes debugging more difficult when several unverified modules fail at the same time. Requiring each module to pass its individual tests first provides a more controlled development process.

### 4.3 Final Design Decision

The final design combines:

- A shared I2C communication system
- Modular software architecture
- Timer-based task scheduling
- Hardware PWM for actuator control
- A centralized main control loop
- A module-first validation workflow before integration

This design provides a balanced solution that is efficient, scalable, easy to debug, and fully aligned with the project requirements. It demonstrates proper integration of sensing, processing, communication, and actuation within a real-time embedded system.

---

## 5. AI Verification Summary

- **What AI was used for:** AI was used to review the document for formatting and completeness.
- **What was verified:** The AI was used to check section organization, requirement/constraint coverage, consistency of formatting, and whether the hardware design, software architecture, data flow/system behavior, and design justification sections were fully included.
- **What was accepted/rejected:** Formatting and completeness suggestions were accepted where they improved clarity and organization. Any suggestion that would change the intended project requirements, selected hardware choices, or system behavior without user approval would be rejected.
