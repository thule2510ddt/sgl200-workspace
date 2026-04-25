# SGL-200 Servo Bus Variant

This actuator variant uses Feetech ST3215HS servos over the STM32G431 USART2 single-wire half-duplex bus.

## Firmware Link

- Config: [`firmware/products/sgl200-servo-bus.conf`](../../../../firmware/products/sgl200-servo-bus.conf)
- Overlay: [`firmware/products/sgl200-servo-bus.overlay`](../../../../firmware/products/sgl200-servo-bus.overlay)
- Driver: [`firmware/drivers/feetech/`](../../../../firmware/drivers/feetech/)
- Actuator facade: [`firmware/app/gimbal_actuator.c`](../../../../firmware/app/gimbal_actuator.c)

## Hardware Contract

| Function | MCU pin | Electrical contract |
|---|---|---|
| Servo bus TX/RX | PA2 / USART2 TX | Half-duplex single-wire UART, 1Mbps |
| Pitch servo | Feetech ID 1 | Connector must support power, ground, and single-wire bus |
| Yaw servo | Feetech ID 2 | Connector must support power, ground, and single-wire bus |

## KiCad Notes

- Add connector symbols for pitch and yaw servo ports.
- Include bus protection and level/power-domain notes during schematic capture.
- Keep this variant separate from Servo PWM outputs; PA2 must not be reused when this product is selected.
