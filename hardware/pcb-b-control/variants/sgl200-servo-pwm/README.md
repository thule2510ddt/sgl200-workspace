# SGL-200 Servo PWM Variant

This actuator variant uses two PWM outputs for pitch and yaw servo commands.

## Firmware Link

- Config: [`firmware/products/sgl200-servo-pwm.conf`](../../../../firmware/products/sgl200-servo-pwm.conf)
- Overlay: [`firmware/products/sgl200-servo-pwm.overlay`](../../../../firmware/products/sgl200-servo-pwm.overlay)
- Actuator facade: [`firmware/app/gimbal_actuator.c`](../../../../firmware/app/gimbal_actuator.c)

## Hardware Contract

| Function | MCU pin | Timer channel | PWM contract |
|---|---|---|---|
| Pitch servo PWM | PB6 | TIM4_CH1 | 50Hz, 20ms period, 1000-2000us pulse, 1500us neutral |
| Yaw servo PWM | PB7 | TIM4_CH2 | 50Hz, 20ms period, 1000-2000us pulse, 1500us neutral |

## KiCad Notes

- Route PB6 and PB7 to distinct actuator connectors with ground reference.
- Keep servo power budgeting separate from STM32 logic power.
- Do not place pitch/yaw PWM on TIM3_CH1 PB4 because that pin is reserved for main LED PWM.
