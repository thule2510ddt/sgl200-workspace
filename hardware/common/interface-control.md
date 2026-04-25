# SGL-200 Interface Control

This document is the shared hardware/firmware contract for PCB-A, PCB-B, and the Servo PWM product implemented by the current branch. It mirrors the active firmware board DTS and Servo PWM product overlay.

## Firmware Sources Of Truth

| Firmware artifact | Hardware meaning |
|---|---|
| [`firmware/boards/arm/sgl200_v1/sgl200_v1.dts`](../../firmware/boards/arm/sgl200_v1/sgl200_v1.dts) | Base PCB-B pin map and enabled peripherals |
| [`firmware/products/sgl200-servo-pwm.overlay`](../../firmware/products/sgl200-servo-pwm.overlay) | Servo PWM product wiring on TIM4_CH1 PB6 and TIM4_CH2 PB7 |

## PCB-A To PCB-B Interface

| Signal | Direction | Firmware pin | Hardware note |
|---|---|---|---|
| VIN / power input | External to PCB-A | N/A | Drone power bus, 18-54V DC |
| Regulated logic power | PCB-A to PCB-B or local PCB-B regulator | N/A | Final partition depends on power architecture review |
| Main LED PWM | PCB-B to PCB-A | TIM3_CH1 PB4 | 20kHz PWM control for LT8391A dimming |
| NTC_LED | PCB-A to PCB-B | ADC1 IN1 PA0 | LED thermal feedback |
| NTC_DRIVER | PCB-A to PCB-B | ADC1 IN2 PA1 | Driver thermal feedback |
| LED fault/current placeholders | PCB-A to PCB-B | Reserved | Assign pins only through a future decision record |
| Ground | Shared | N/A | Maintain low-impedance return for control and sensing |

## PCB-B Base Signals

| Function | Pins | Constraint |
|---|---|---|
| MCU | STM32G431CBU6 | 170MHz target, Zephyr RTOS |
| IMU SPI1 | PA5 SCK, PA6 MISO, PA7 MOSI, PA4 CS | ICM-42688-P, SPI Mode 3, up to 24MHz |
| IMU interrupt | PB0 | Active high, 1kHz data-ready path |
| MAVLink USART3 | PB10 TX, PB11 RX | 921600bps |
| Main LED PWM | PB4 / TIM3_CH1 | 20kHz |
| Thermal ADC | PA0 ADC1 IN1, PA1 ADC1 IN2 | NTC_LED and NTC_DRIVER |
| AUX LEDs | PC6 red, PC7 blue, PC8 IR | GPIO outputs |
| Status LED | PB8 | GPIO output |
| Debug | PA13 SWDIO, PA14 SWDCLK, PB3 SWO | J-Link, RTT, SystemView |
| FDCAN reserved | PA11 RX, PA12 TX | Disabled in v1 |

## Current Branch Actuator Signals

| Product | Signals | Firmware artifact |
|---|---|---|
| Servo PWM | Pitch TIM4_CH1 PB6, yaw TIM4_CH2 PB7, 50Hz, 1000-2000us pulses, 1500us neutral | [`sgl200-servo-pwm.conf`](../../firmware/products/sgl200-servo-pwm.conf) and [overlay](../../firmware/products/sgl200-servo-pwm.overlay) |

USART2 PA2 servo bus hardware and BLDC hardware are intentionally out of scope for this branch.

## Review Rules

- Do not reuse locked firmware pins without updating the firmware DTS/product overlay and decision log.
- Keep PCB-A high-power LED switching and PCB-B sensor/control routing separated in schematic sheets and layout constraints.
- Product branches must only include the hardware variant they implement.
