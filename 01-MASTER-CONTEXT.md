# SGL-200 Master Context

## Project Identity

- Project: SGL-200 Gimbal LED Spotlight Payload.
- Company: Saolatek, Ho Chi Minh City, Vietnam.
- Product role: 2-axis stabilized high-output LED spotlight payload for industrial drones.
- Primary use cases: Search and Rescue, public safety, and law enforcement operations.
- Engineering role: embedded R&D across firmware, hardware, integration, and test.
- Reference document IDs: `SGL200-PRD-SAR-001-A`, `SGL200-SDS-SAR-001-A`.

## Locked Hardware Decisions

These decisions are non-negotiable unless a future decision record explicitly supersedes them.

| Area | Decision |
|---|---|
| MCU | STM32G431CBU6 at 170MHz, with CORDIC coprocessor |
| RTOS | Zephyr RTOS v3.6+ |
| IMU | ICM-42688-P over SPI1 Mode 3 at up to 24MHz, ODR 1kHz, INT on PB0 |
| LED driver | LT8391A 4-switch buck-boost constant-current driver |
| LED driver switching | 600kHz minimum; do not propose a lower switching frequency |
| Main LED | Luminus SBT-90.2 Gen3 CW 5600K, about 8500lm at 3A |
| Gimbal actuator variant | v1 uses Feetech ST3215HS x2, half-duplex UART at 1Mbps; future product variants may use servo PWM or BLDC |
| Servo IDs | Pitch = ID 1, Yaw = ID 2 |
| MAVLink | MAVLink v2 over USART3 at 921600bps |
| MAVLink role | Payload implements GIMBAL_DEVICE, not GIMBAL_MANAGER |
| Component ID | MAV_COMP_ID_GIMBAL = 154 |
| Debug | SWD, SWO, SEGGER RTT, SEGGER SystemView via J-Link |
| Board split | PCB-A power board and PCB-B control board |

## Locked Pin Map

| Function | Pins |
|---|---|
| SPI1 IMU | SCK PA5, MISO PA6, MOSI PA7, CS PA4 |
| IMU interrupt | PB0, active high |
| v1 servo bus | USART2 TX PA2, half-duplex single-wire |
| USART3 MAVLink | TX PB10, RX PB11 |
| Main LED PWM | TIM3 CH1 PB4, 20kHz |
| Thermal ADC | ADC1 IN1 PA0 for NTC_LED, ADC1 IN2 PA1 for NTC_DRIVER |
| AUX LEDs | PC6 red, PC7 blue, PC8 IR |
| Status LED | PB8 |
| FDCAN reserved | RX PA11, TX PA12, disabled in v1 |
| Debug | SWDIO PA13, SWDCLK PA14, SWO PB3 |

## Firmware Architecture

The firmware is organized into five layers:

1. HAL and drivers: ICM-42688-P SPI driver, Feetech UART driver, LT8391A PWM control, ADC NTC thermal sensing.
2. Sensor fusion: Madgwick AHRS at 1kHz with STM32G431 CORDIC acceleration where useful.
3. Gimbal control: cascaded PID with outer angle loop at 200Hz and inner rate loop at 1kHz, writing through the gimbal actuator abstraction.
4. LED manager: FSM for idle, soft start, normal, strobe, thermal throttle, and emergency off.
5. MAVLink agent: Gimbal Protocol v2 as GIMBAL_DEVICE, telemetry TX, ACKs, parameters, and fault reporting.

## Thread Model

Priority 0 is highest.

| Thread | Priority | Period | Responsibility |
|---|---:|---|---|
| `imu_thread` | 0 | 1ms IRQ | SPI DMA read from ICM-42688-P and AHRS update |
| `control_thread` | 1 | 1ms | Inner rate PID and actuator command write |
| `angle_thread` | 2 | 5ms | Outer angle PID and ROI angle calculation |
| `mavlink_rx_thread` | 3 | async | UART DMA receive, MAVLink parse, command dispatch |
| `mavlink_tx_thread` | 4 | 250ms | HEARTBEAT, gimbal attitude, ACKs, STATUSTEXT |
| `led_manager_thread` | 5 | 10ms | LED FSM, brightness, strobe timing, thermal throttle |
| `thermal_thread` | 6 | 100ms | ADC NTC sampling and OTP state |
| `watchdog_thread` | 7 | 400ms | IWDG feed, heartbeat timeout, stack checks |

## Performance Requirements

| Requirement | Target |
|---|---|
| Gimbal stability | < 0.3 deg RMS at wind 9m/s |
| Control loop jitter | < 50us, verified with SystemView |
| MAVLink command latency | < 200ms end-to-end |
| Command ACK latency | < 100ms for relevant commands |
| Main LED flux | >= 8000lm at 25C |
| Main LED PWM | Flicker-free, > 2kHz; target implementation uses 20kHz |
| Thermal throttle | Start at NTC = 75C, equivalent Tj about 125C |
| Emergency thermal shutdown | NTC > 95C |
| Boot time | < 3s to first MAVLink HEARTBEAT TX |
| Payload weight | <= 400g |
| Environmental | IP65, operating -20C to +55C |

## Hard Engineering Rules

1. Use Zephyr-native APIs. Do not use `HAL_xxx()` from STM32 HAL in application or driver code unless a decision record explicitly allows it.
2. Shared data between threads must use Zephyr IPC or synchronization primitives such as `k_mutex`, `atomic`, `k_msgq`, or `k_sem`.
3. Driver functions that can fail must return errors such as `-ENODEV`, `-EIO`, or `-ETIMEDOUT`; do not hide hardware failures behind `void` APIs.
4. Keep LT8391A switching frequency at or above 600kHz.
5. Implement MAVLink as GIMBAL_DEVICE only. The flight controller is the GIMBAL_MANAGER.
6. Thermal thresholds at 75C throttle and 95C shutdown are hard requirements.
7. Before outputting DTS or board files, verify pin conflicts against the locked pin map.
8. Firmware tasks must target buildable Zephyr project structure and include verification commands.
9. Product-specific gimbal hardware must be selected through Kconfig/product overlays and accessed through the gimbal actuator abstraction, not by calling a specific actuator driver from control code.
