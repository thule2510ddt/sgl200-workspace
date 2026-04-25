# PCB-B Control Board

PCB-B contains the STM32G431 control electronics, IMU, MAVLink interface, debug, thermal ADC inputs, AUX LEDs, and product-specific gimbal actuator interface.

## KiCad Structure

| Folder | Purpose |
|---|---|
| `kicad/` | KiCad project placeholder for the PCB-B schematic and layout |
| `docs/` | Board bring-up notes, pin review, and design checklists |
| `fabrication/` | Future Gerber, drill, stackup, drawing, and fabrication notes |
| `assembly/` | Future BOM, CPL/position files, assembly drawings, and test instructions |
| `variants/` | Product-specific actuator wiring contracts |

## Base Firmware Pin Contract

PCB-B must match `firmware/boards/arm/sgl200_v1/sgl200_v1.dts`.

| Function | MCU pins |
|---|---|
| SPI1 IMU | SCK PA5, MISO PA6, MOSI PA7, CS PA4 |
| IMU interrupt | PB0 active high |
| USART3 MAVLink | TX PB10, RX PB11 |
| Main LED PWM | TIM3_CH1 PB4 |
| Thermal ADC | PA0 for NTC_LED, PA1 for NTC_DRIVER |
| AUX LEDs | PC6 red, PC7 blue, PC8 IR |
| Status LED | PB8 |
| Debug | SWDIO PA13, SWDCLK PA14, SWO PB3 |
| FDCAN reserved | PA11 RX, PA12 TX, disabled in v1 |

## Actuator Variants

- [sgl200-servo-bus](variants/sgl200-servo-bus/README.md): USART2 PA2 half-duplex Feetech servo bus.
- [sgl200-servo-pwm](variants/sgl200-servo-pwm/README.md): TIM4_CH1 PB6 pitch and TIM4_CH2 PB7 yaw PWM servo outputs.
- [sgl200-bldc](variants/sgl200-bldc/README.md): reserved BLDC hardware interface placeholder.
