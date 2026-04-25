# SGL-200 Backlog

This is the canonical task queue for AI-assisted implementation. The original prompt bundle is preserved in `SGL200-LINEAR-SESSIONS.md` as a legacy source-import reference.

## Project Setup

- Project: `SGL-200-SAR`
- Team: Saolatek R&D
- Labels: `firmware`, `hardware`, `integration`, `test`, `docs`
- Milestones: Sprint 0 through Sprint 5
- Default implementation rule: follow `01-MASTER-CONTEXT.md` and record new decisions in `06-DECISION-LOG.md`.

## Sprint 0 - Architecture And Design Lock

### S0-01 - PCB-A Power Board Schematic Review Checklist

- Labels: `hardware`, `docs`
- Priority: Urgent
- Estimate: 3 points

Context:
PCB-A is the SGL-200 power board. It uses LT8391A as a 4-switch buck-boost constant-current LED driver for a 36V, 3A, 108W LED load from 18-54V input.

Inputs:
- LT8391A design requirements.
- DC2345A reference design.
- `SGL200-SDS-SAR-001-A`, Section 3.2.
- Locked LED driver decision: LT8391A switching frequency must be at least 600kHz.

Output files:
- Markdown schematic review checklist for PCB-A.
- Optional CSV/checklist table if needed for review tracking.

Implementation constraints:
- Include LT8391A pins: `BOOST`, `SW`, `SN`, `SP`, `CTRL`, `EN`, `SYNC`.
- Use current sense target `Rsense = 10mohm`, `Vsense = 30mV at 3A`.
- Include input filter target `L = 4.7uH`, `C = 2 x 220uF / 63V`.
- Include recommended MOSFET gate resistor `33ohm`.
- Include output capacitor ESR budget `< 5mohm total`.
- Include EMI and layout rules for switching loop minimization and inter-board ferrite bead.

Acceptance criteria:
- Checklist has pass/fail fields for every electrical and layout review item.
- All formulas, target values, and review thresholds are explicit.
- No recommendation violates the 600kHz LT8391A minimum switching constraint.

Verification:
- Peer review against LT8391A datasheet/reference design and SDS Section 3.2.
- Confirm all required checklist items are present before PCB-A schematic signoff.

### S0-02 - KiCad And Zephyr Board Definition For STM32G431CBU6

- Labels: `hardware`, `firmware`
- Priority: Urgent
- Estimate: 5 points

Context:
PCB-B is the control board using STM32G431CBU6 in UFQFPN-48. The Zephyr board definition must match the locked schematic pin assignment.

Inputs:
- Locked pin map in `01-MASTER-CONTEXT.md`.
- Target Zephyr board name: `sgl200_v1`.
- Zephyr v3.6+ board definition conventions.

Output files:
- `firmware/boards/arm/sgl200_v1/sgl200_v1.dts`
- `firmware/boards/arm/sgl200_v1/sgl200_v1_defconfig`
- `firmware/boards/arm/sgl200_v1/Kconfig.board`
- `firmware/boards/arm/sgl200_v1/Kconfig.defconfig`
- `firmware/boards/arm/sgl200_v1/board.cmake`

Implementation constraints:
- SPI1: SCK PA5, MISO PA6, MOSI PA7, CS PA4.
- IMU INT: PB0, active high.
- USART2: TX PA2, half-duplex single-wire servo bus at 1Mbps.
- USART3: TX PB10, RX PB11 for MAVLink at 921600bps.
- Main LED PWM: TIM3 CH1 PB4 at 20kHz. Do not use PA6 or PA7 because they are assigned to SPI1.
- ADC1: PA0 for NTC_LED, PA1 for NTC_DRIVER.
- AUX GPIO: PC6 red, PC7 blue, PC8 IR.
- Status LED: PB8.
- FDCAN1 PA11/PA12 reserved and disabled in v1.
- Debug: PA13 SWDIO, PA14 SWDCLK, PB3 SWO.

Acceptance criteria:
- DTS includes all required peripherals and labels.
- Each pin has only one active function assignment.
- Board files follow Zephyr custom board structure.
- J-Link runner is configured in `board.cmake`.

Verification:
- Static pin conflict review.
- `west build -b sgl200_v1 firmware/ --target menuconfig`
- Build should pass once the firmware scaffold exists.

### S0-03 - Component Order List And Lead Time Analysis

- Labels: `hardware`
- Priority: Urgent
- Estimate: 2 points

Context:
Long-lead components must be ordered before hardware bring-up.

Inputs:
- Prototype quantity: 5 units.
- Target BOM cost: less than USD 180/unit at 50-piece volume.
- Critical components: SBT-90.2 Gen3, LT8391A, STM32G431CBU6, ICM-42688-P, NVMFS5C628NL, Feetech ST3215HS.

Output files:
- PCB-A BOM table.
- PCB-B BOM table.
- Raw CSV suitable for spreadsheet or ERP import.
- Top 5 long-lead order list.

Implementation constraints:
- Include MPN, value/spec, quantity, preferred supplier, estimated lead time, unit cost, and risk notes.
- Check for EOL/NRND risk and recommend alternatives only when necessary.
- Keep PCB-A and PCB-B BOMs separate.

Acceptance criteria:
- Top 5 long-lead items are clearly identified.
- Total estimated cost for 5 prototype units is calculated.
- 50-piece volume cost is compared against the target.

Verification:
- Supplier stock and lead time check at time of procurement.
- BOM review by hardware owner before purchase order.

## Sprint 1 - Hardware Bring-Up

### S1-01 - Zephyr Project Scaffold And Board Bring-Up

- Labels: `firmware`
- Priority: Urgent
- Estimate: 8 points

Context:
Create the initial buildable Zephyr firmware project for SGL-200.

Inputs:
- Zephyr v3.6.0 target.
- Board definition from S0-02.
- Project config requirements from `WORKSPACE_STRUCTURE.md` and `01-MASTER-CONTEXT.md`.

Output files:
- `firmware/west.yml`
- `firmware/CMakeLists.txt`
- `firmware/Kconfig`
- `firmware/prj.conf`
- `firmware/app/main.c`
- Initial `CMakeLists.txt` files for app, drivers, and libraries as needed.

Implementation constraints:
- Enable SPI async DMA, UART async, PWM, ADC async, GPIO, IWDG/watchdog, NVS flash, FPU, RTT logging, and SystemView.
- Create the eight-thread startup model from `01-MASTER-CONTEXT.md`.
- Startup sequence: clock, GPIO, SPI, UART, ADC, then application threads.
- Use Zephyr-native APIs and avoid STM32 HAL calls.

Acceptance criteria:
- Project builds a minimal hello/status application for `sgl200_v1`.
- RTT logging is enabled.
- Thread names, priorities, and stack sizes match the architecture.
- Warnings are fewer than 5 when the toolchain is available.

Verification:
- `west build -b sgl200_v1 firmware/`
- `west flash --runner jlink` when hardware and J-Link are available.

### S1-02 - ICM-42688-P SPI Driver

- Labels: `firmware`
- Priority: High
- Estimate: 5 points

Context:
Implement the SGL-200 IMU driver using Zephyr driver patterns. The IMU feeds the 1kHz control loop.

Inputs:
- ICM-42688-P register map.
- SPI1 Mode 3, CS PA4, INT PB0.
- WHO_AM_I expected value `0x47`.

Output files:
- `firmware/drivers/icm42688/icm42688.c`
- `firmware/drivers/icm42688/icm42688.h`
- `firmware/drivers/icm42688/icm42688_reg.h`
- `firmware/dts/bindings/saolatek,icm42688.yaml`
- `firmware/tests/drivers/test_icm42688.c`

Implementation constraints:
- Use Zephyr SPI APIs and DMA-capable transfer where supported.
- SPI write format: `[reg | 0x00, data]`.
- SPI read format: `[reg | 0x80, dummy]`.
- Driver init must check WHO_AM_I, soft reset, and configure ODR 1kHz.
- INT handler gives a semaphore for the IMU/control path.
- `data_get()` reads accel, temperature, and gyro samples and converts raw int16 to SI/control units.
- Return `-ENODEV` for wrong WHO_AM_I and `-EIO` for SPI errors.

Acceptance criteria:
- Driver compiles in the Zephyr project.
- Mock unit test verifies WHO_AM_I handling and conversion math.
- API exposes data needed by AHRS and control loops.

Verification:
- Unit test with mocked SPI transaction.
- Hardware read test confirms WHO_AM_I `0x47` and stable 1kHz data-ready behavior.

### S1-03 - Feetech ST3215HS Servo Driver

- Labels: `firmware`
- Priority: High
- Estimate: 5 points

Context:
Implement the half-duplex UART protocol driver for two Feetech ST3215HS servos.

Inputs:
- USART2 single-wire mode at 1Mbps, 8N1.
- SCS packet format.
- Servo IDs: pitch = 1, yaw = 2.
- Servo center: raw 2048, resolution about 0.088deg/step.

Output files:
- `firmware/drivers/feetech_servo/feetech_servo.c`
- `firmware/drivers/feetech_servo/feetech_servo.h`
- `firmware/drivers/feetech_servo/feetech_scs_proto.h`
- `firmware/tests/drivers/test_feetech.c`

Implementation constraints:
- Support `WRITE_DATA`, `REG_WRITE`, `ACTION`, `READ_DATA`, and `SYNC_WRITE`.
- Packet checksum: bitwise inverse of the sum of ID, length, instruction/error, and params.
- Provide set/get raw position and set/get angle helpers.
- Pitch range: -90deg to +30deg.
- Yaw range: -160deg to +160deg.
- RX timeout is 5ms and returns `-ETIMEDOUT`.
- Use Zephyr UART APIs and single-wire/half-duplex configuration.

Acceptance criteria:
- Driver builds and unit tests packet encode/decode and checksum.
- Angle-to-raw mapping clamps safely to axis limits.
- Sync write can command pitch and yaw together.

Verification:
- Unit packet tests.
- Physical servo test at 0deg, +30deg, and -45deg with angle measured externally.

## Sprint 2 - Firmware Core

### S2-01 - Madgwick AHRS With STM32G431 CORDIC

- Labels: `firmware`
- Priority: Urgent
- Estimate: 8 points

Context:
Implement the 1kHz AHRS layer that converts gyro and accel data into quaternion and Euler attitude outputs.

Inputs:
- ICM-42688-P gyro and accel samples.
- STM32G431 CORDIC peripheral.
- Default beta parameter: `0.1`.

Output files:
- `firmware/lib/madgwick/madgwick.c`
- `firmware/lib/madgwick/madgwick.h`
- `firmware/lib/madgwick/madgwick_cordic.c`
- `firmware/lib/madgwick/madgwick_cordic.h`
- `firmware/tests/lib/test_madgwick.c`

Implementation constraints:
- Provide `madgwick_init`, `madgwick_update`, `madgwick_get_quaternion`, and `madgwick_get_euler`.
- Output quaternion as `[w, x, y, z]`.
- Output Euler as pitch, yaw, roll in degrees.
- Implement CORDIC sin/cos wrapper using peripheral registers, not STM32 HAL.
- Convert float radians to Q1.31 format for CORDIC.
- Add gyro bias estimation when angular rate norm is below 0.05rad/s.
- Expose beta through the parameter system when available.

Acceptance criteria:
- `madgwick_update()` target runtime is under 80us at 170MHz.
- Static level input converges to pitch 0, yaw 0, roll 0 within tolerance.
- Simulated 90deg rotation produces expected Euler output.

Verification:
- Unit tests for static and rotation cases.
- Cycle count comparison between CORDIC and software sin/cos.

### S2-02 - Cascaded PID Gimbal Control

- Labels: `firmware`
- Priority: Urgent
- Estimate: 8 points

Context:
Implement cascaded PID control for pitch and yaw, with an outer angle loop at 200Hz and inner rate loop at 1kHz.

Inputs:
- AHRS attitude and gyro rates.
- Feetech servo driver.
- Initial PID gains from the architecture document.
- Gimbal modes: `STABILIZE`, `FOLLOW`, `ROI`, `LOCK`, `NEUTRAL`.

Output files:
- `firmware/lib/pid/pid.c`
- `firmware/lib/pid/pid.h`
- `firmware/app/gimbal/gimbal_control.c`
- `firmware/app/gimbal/gimbal_control.h`
- `firmware/app/gimbal/gimbal_modes.h`
- `firmware/tests/lib/test_pid.c`

Implementation constraints:
- PID uses back-calculation anti-windup, not simple integral clamp.
- Rate setpoint limit: +/-500deg/s.
- Servo command output limit maps to valid servo raw range.
- Shared setpoints must use atomic or synchronized data exchange.
- ROI mode computes angles from GPS using haversine/bearing logic.
- Gains must be available through MAVLink PARAM once the parameter system exists.

Acceptance criteria:
- Four PID instances initialize with correct gains.
- Mode transitions are explicit and safe.
- Neutral mode returns to 0deg pitch and 0deg yaw and disables active tracking.
- Step response target: rise time < 500ms and overshoot < 10% during HIL tuning.

Verification:
- Unit tests for PID update, reset, gain setting, saturation, and anti-windup.
- HIL step test: command 30deg and measure response.

## Sprint 3 - MAVLink Integration

### S3-01 - MAVLink Gimbal Protocol v2 Implementation

- Labels: `firmware`, `integration`
- Priority: Urgent
- Estimate: 13 points

Context:
SGL-200 implements MAVLink Gimbal Protocol v2 as a GIMBAL_DEVICE component connected to an ArduPilot/PX4 flight controller.

Inputs:
- Component ID `MAV_COMP_ID_GIMBAL = 154`.
- USART3 at 921600bps.
- ArduPilot Copter 4.5+ integration target.
- Gimbal attitude and LED control APIs.

Output files:
- `firmware/app/mavlink/mavlink_agent.h`
- `firmware/app/mavlink/mavlink_rx.c`
- `firmware/app/mavlink/mavlink_tx.c`
- `firmware/app/mavlink/mavlink_params.c`
- `firmware/tests/app/test_mavlink_parse.c`

Implementation constraints:
- Implement GIMBAL_DEVICE only; do not implement GIMBAL_MANAGER on the payload.
- TX HEARTBEAT at 1Hz as `MAV_TYPE_GIMBAL`.
- Respond to `GIMBAL_DEVICE_INFORMATION` requests with vendor, model, firmware version, capabilities, and pitch/yaw limits.
- TX `GIMBAL_DEVICE_ATTITUDE_STATUS` at 4Hz.
- RX `GIMBAL_DEVICE_SET_ATTITUDE` and pass setpoints to gimbal control.
- RX `COMMAND_LONG` for `MAV_CMD_DO_SET_RELAY` and `MAV_CMD_DO_CONTROL_VIDEO` mapping to LED/strobe/brightness behavior.
- ACK accepted commands within 100ms.
- Support PARAM_SET/PARAM_VALUE and persist parameters through NVS.
- Send STATUSTEXT for faults.

Acceptance criteria:
- Mission Planner or QGroundControl detects SGL-200 gimbal device.
- Gimbal tab shows attitude.
- Gimbal commands move servos through the control API.
- Relay/video commands toggle LED and brightness.
- Parameter writes persist across reboot.

Verification:
- Unit tests for frame parsing and command dispatch.
- Integration test with Pixhawk 6C and ArduCopter 4.5+.

## Sprint 4 - LED Control And Safety

### S4-01 - LED Manager FSM And Strobe Patterns

- Labels: `firmware`
- Priority: High
- Estimate: 8 points

Context:
Control the main LT8391A PWM dim input plus red, blue, and IR auxiliary LEDs.

Inputs:
- Main LED PWM: TIM3 CH1 PB4 at 20kHz.
- AUX LED GPIO: PC6 red, PC7 blue, PC8 IR.
- Thermal thresholds: throttle at 75C, emergency off at 95C.

Output files:
- `firmware/app/led/led_manager.c`
- `firmware/app/led/led_manager.h`
- `firmware/app/led/led_modes.h`
- `firmware/tests/app/test_led_fsm.c`

Implementation constraints:
- FSM states: `IDLE`, `SOFT_START`, `NORMAL`, `STROBE_WHITE`, `STROBE_POLICE`, `STROBE_SOS`, `THERMAL_THROTTLE`, `EMERGENCY_OFF`.
- Command interface uses `k_msgq` with mode and brightness percent.
- Soft start ramps 0 to target over 500ms using 10ms steps.
- Thermal throttle reduces brightness by 10% per degree C above 75C, minimum 20%.
- Emergency shutdown cuts PWM immediately.
- White strobe: 1Hz square wave on main LED.
- Police strobe: red 250ms on/off and blue 250ms offset.
- SOS strobe: dot 200ms, dash 600ms, intra-symbol space 200ms, word space 1400ms.
- No blocking calls in `led_manager_thread`.

Acceptance criteria:
- Every state transition is deterministic and unit tested.
- Strobe timing accuracy target is +/-5ms.
- Thermal emergency state cannot be overridden by normal commands.

Verification:
- Unit test FSM transitions and thermal injection.
- Oscilloscope timing verification for PWM and strobe outputs.

### S4-02 - Safety Manager, Watchdog, And Fault System

- Labels: `firmware`
- Priority: Urgent
- Estimate: 5 points

Context:
Safety behavior must recover from deadlock, handle MAVLink loss, turn off LED when the FC is disarmed, and report faults to the GCS.

Inputs:
- IWDG timeout target: 500ms.
- Heartbeat timeout: 3000ms.
- Fault codes from architecture and SDS.

Output files:
- `firmware/app/safety/safety_manager.c`
- `firmware/app/safety/safety_manager.h`
- Fault code definitions in the appropriate shared header.

Implementation constraints:
- `watchdog_thread` feeds IWDG every 400ms.
- On IWDG reset, detect reset reason and log `FAULT_WATCHDOG_RESET`.
- If FC heartbeat age exceeds 3000ms, enter safe state.
- Safe state: gimbal neutral, LED brightness 50%, fault report.
- If FC heartbeat indicates disarmed state, turn LED off immediately.
- Fault pipeline uses `k_msgq fault_queue`.
- `mavlink_tx_thread` drains faults and sends STATUSTEXT.
- Periodically check stack usage using Zephyr stack info support.

Acceptance criteria:
- Heartbeat loss enters safe state in less than 4000ms.
- Watchdog reset is detectable after reboot.
- All listed fault codes have a reporting path.
- Disarmed FC state forces LED off immediately.

Verification:
- Inject UART disconnect and measure safe state timing.
- Force watchdog reset in controlled test and confirm fault reporting after boot.

## Sprint 5 - Qualification And Field Test

### S5-01 - DVP Execution: Photometric, Gimbal Accuracy, Boot Time

- Labels: `test`
- Priority: High
- Estimate: 5 points

Context:
Create procedures and data templates for Design Verification Plan items.

Inputs:
- DVP-01 photometric requirements.
- DVP-03 gimbal stabilization requirements.
- DVP-09 boot time requirements.
- MAVLink logs and field measurements.

Output files:
- `test/dvp/dvp-01-photometric.md`
- `test/dvp/dvp-03-gimbal-stability.md`
- `test/dvp/dvp-09-boot-time.md`
- `test/dvp/dvp03_analysis.py`
- Data collection template suitable for spreadsheet import.

Implementation constraints:
- Photometric distances: 5m, 10m, 20m, 50m, 80m.
- Photometric pass target: center beam at 80m >= 15 lux, equivalent to >= 8000lm source with 15deg FOV assumption.
- Gimbal stability test: +/-20deg pitch motion at 0.5Hz for 60s.
- Calculate RMS deviation of actual pitch minus commanded pitch.
- Boot test trigger: VBUS rising edge to first MAVLink HEARTBEAT TX on UART3.

Acceptance criteria:
- Procedures include setup, equipment, steps, calculations, pass/fail criteria, and evidence capture.
- Python analysis computes RMS and generates a time-series plot.
- Boot time pass threshold is < 3000ms.

Verification:
- Dry review of procedures.
- Execute tests on prototype hardware when available and attach measured evidence.

### S5-02 - Field Night Test: SAR Simulation

- Labels: `test`
- Priority: High
- Estimate: 3 points

Context:
Final acceptance scenario simulates night SAR over a 100m x 100m area at 50m altitude.

Inputs:
- SGL-200 mounted to drone.
- GCS with Mission Planner or QGroundControl.
- MAVLink log, onboard video, and ground observer video.

Output files:
- `test/field/field-test-protocol.md`
- `test/field/field-data-template.xlsx` or CSV equivalent.
- Debrief template.

Implementation constraints:
- Include pre-flight checklist for mounting, cabling, MAVLink link, LED modes, gimbal movement, power budget, and weather.
- Weather constraints: wind < 7m/s, visibility > 1km, no rain.
- Test timeline includes takeoff, hover, climb, LED on, gimbal pitch down, SAR search, police strobe, heartbeat timeout, RTL, and landing.
- Heartbeat timeout test must verify safe mode within 3s nominal and < 4s pass limit.

Acceptance criteria:
- Victim is recognizable in HD video within 30s from 50m altitude.
- Gimbal has no visible drift during the test.
- Heartbeat timeout safe state activates in < 4s.
- All evidence is collected and linked in the debrief.

Verification:
- Field execution with MAVLink logs, video evidence, observer notes, and debrief.

