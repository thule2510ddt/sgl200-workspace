# Source Code Map

This document maps the generated source code to architecture responsibilities so future AI agents can find the right implementation surface quickly.

## Firmware Entry And Runtime

| Path | Responsibility | Notes |
|---|---|---|
| `firmware/app/main.c` | Boot sequence, shared state, Zephyr thread creation, watchdog safe state | Initializes the selected gimbal actuator through `gimbal_actuator_init()` |
| `firmware/app/sgl200_types.h` | Shared runtime types and IPC extern declarations | Central contract for IMU, attitude, gimbal, LED, thermal, MAVLink, ACK, and fault state |
| `firmware/app/CMakeLists.txt` | App source registration and include paths | Builds all app modules including the actuator abstraction |

## Gimbal And Actuator

| Path | Responsibility | Notes |
|---|---|---|
| `firmware/app/gimbal_control.c` | Gimbal modes and cascaded angle/rate PID loop | Backend-neutral; writes pitch/yaw angle commands through `gimbal_actuator_set_angle()` |
| `firmware/app/gimbal_control.h` | Gimbal control public API | Called by `angle_thread` and `control_thread` |
| `firmware/app/gimbal_actuator.c` | Product actuator facade | Selects servo bus, servo PWM, or BLDC behavior through Kconfig |
| `firmware/app/gimbal_actuator.h` | Backend-neutral actuator API | Defines `GIMBAL_AXIS_PITCH`, `GIMBAL_AXIS_YAW`, init, and set-angle calls |
| `firmware/drivers/feetech/feetech_servo.c` | Feetech ST3215HS half-duplex UART protocol | Used only by the servo-bus actuator backend |
| `firmware/drivers/feetech/feetech_servo.h` | Feetech driver API and servo IDs | Pitch ID 1, yaw ID 2 |

## Product Variants

| Path | Responsibility | Notes |
|---|---|---|
| `firmware/products/README.md` | Product variant build guide | Documents servo bus, servo PWM, and BLDC variants |
| `firmware/products/sgl200-servo-bus.conf` | Servo bus Kconfig selection | Selects `CONFIG_SGL200_ACTUATOR_SERVO_BUS` |
| `firmware/products/sgl200-servo-bus.overlay` | Servo bus hardware overlay | Empty because base board DTS already enables USART2 single-wire |
| `firmware/products/sgl200-servo-pwm.conf` | Servo PWM Kconfig selection | Selects `CONFIG_SGL200_ACTUATOR_SERVO_PWM` |
| `firmware/products/sgl200-servo-pwm.overlay` | Servo PWM hardware overlay | Pitch TIM4_CH1 PB6, yaw TIM4_CH2 PB7 |
| `firmware/products/sgl200-bldc.conf` | BLDC Kconfig selection | Selects `CONFIG_SGL200_ACTUATOR_BLDC` |
| `firmware/products/sgl200-bldc.overlay` | BLDC hardware overlay placeholder | Transport, feedback, enable, and fault pins are still undefined |

## Sensors, LED, Thermal, MAVLink

| Path | Responsibility | Notes |
|---|---|---|
| `firmware/drivers/icm42688/icm42688.c` | ICM-42688-P SPI driver | Feeds IMU samples to AHRS at 1kHz |
| `firmware/lib/madgwick/madgwick_ahrs.c` | Madgwick AHRS | Produces quaternion and Euler attitude |
| `firmware/lib/pid/pid.c` | Generic PID | Used by gimbal angle/rate loops |
| `firmware/app/led_manager.c` | Main LED FSM and PWM output | Uses TIM3_CH1 PB4 through Zephyr PWM |
| `firmware/app/thermal_manager.c` | ADC NTC sampling and thermal state | Supports throttle and emergency shutdown decisions |
| `firmware/app/mavlink_agent.c` | MAVLink RX/TX agent | Handles GIMBAL_DEVICE commands, LED commands, params, ACK, and telemetry |
| `firmware/lib/mavlink/mavlink2_minimal.c` | Minimal MAVLink v2 frame helpers | Local encode/decode support for current protocol subset |

## Board And Build

| Path | Responsibility | Notes |
|---|---|---|
| `firmware/boards/arm/sgl200_v1/sgl200_v1.dts` | Base board devicetree | Locked v1 pin map, IMU, USART2 servo bus, USART3 MAVLink, LED PWM, ADC, watchdog |
| `firmware/boards/arm/sgl200_v1/sgl200_v1_defconfig` | Base board defaults | Enables STM32G431 platform essentials |
| `firmware/Kconfig` | Application and actuator backend Kconfig | Defines the actuator backend choice |
| `firmware/prj.conf` | Default firmware config | Defaults to servo-bus actuator variant |
| `firmware/CMakeLists.txt` | Zephyr project entry | Adds drivers, libraries, and app layers |

## Generated Architecture Documents

| Path | Responsibility |
|---|---|
| `architecture/README.md` | Architecture index |
| `architecture/system-overview.md` | System hardware/firmware integration diagram |
| `architecture/thread-data-flow.md` | Zephyr thread and IPC data flow diagram |
| `architecture/led-state-machine.md` | LED FSM diagram |
| `architecture/gimbal-control-flow.md` | Gimbal control and actuator backend diagram |
| `architecture/mavlink-message-flow.md` | MAVLink sequence diagram |
| `architecture/core-data-structure.md` | Runtime structure diagram |
| `architecture/branch-map.md` | Branch and feature responsibility map |
