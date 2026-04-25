# SGL-200 BLDC Variant

This actuator variant is reserved for future BLDC motor control hardware.

## Firmware Link

- Config: [`firmware/products/sgl200-bldc.conf`](../../../../firmware/products/sgl200-bldc.conf)
- Overlay: [`firmware/products/sgl200-bldc.overlay`](../../../../firmware/products/sgl200-bldc.overlay)
- Actuator facade: [`firmware/app/gimbal_actuator.c`](../../../../firmware/app/gimbal_actuator.c)

## Current Status

No BLDC transport, feedback, enable, or fault pins are assigned yet. The firmware backend currently returns `-ENOTSUP`.

## Decisions Required Before Schematic Work

| Area | Decision needed |
|---|---|
| Motor control transport | PWM phases, external driver interface, SPI, UART, CAN, or other |
| Position feedback | Encoder, Hall sensors, IMU-only stabilization, or external controller telemetry |
| Safety | Enable, fault, current sense, over-temperature, and brake behavior |
| Power | Motor voltage/current budget and isolation from logic rails |

Do not allocate MCU pins for BLDC until these decisions are recorded in `06-DECISION-LOG.md` and reflected in firmware product overlays.
