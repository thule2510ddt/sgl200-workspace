# SGL-200 Hardware Workspace

This directory contains the KiCad-oriented hardware structure for the SGL-200 firmware variants. It is scaffolding only: schematic capture, PCB layout, netlists, and production outputs still need to be created by hardware design work.

## Boards

| Board | Folder | Firmware relationship |
|---|---|---|
| PCB-A Power | [pcb-a-power](pcb-a-power/README.md) | LT8391A power board, main LED driver, power input, LED output, and board-to-board power/control interface |
| PCB-B Control | [pcb-b-control](pcb-b-control/README.md) | STM32G431 control board matching `firmware/boards/arm/sgl200_v1/sgl200_v1.dts` |

## Product Variants

| Variant | Hardware folder | Firmware config | Actuator interface |
|---|---|---|---|
| Servo bus | [pcb-b-control/variants/sgl200-servo-bus](pcb-b-control/variants/sgl200-servo-bus/README.md) | [`firmware/products/sgl200-servo-bus.conf`](../firmware/products/sgl200-servo-bus.conf) | USART2 PA2 half-duplex, 1Mbps, Feetech ST3215HS IDs 1/2 |
| Servo PWM | [pcb-b-control/variants/sgl200-servo-pwm](pcb-b-control/variants/sgl200-servo-pwm/README.md) | [`firmware/products/sgl200-servo-pwm.conf`](../firmware/products/sgl200-servo-pwm.conf) | TIM4_CH1 PB6 pitch, TIM4_CH2 PB7 yaw, 50Hz servo PWM |
| BLDC | [pcb-b-control/variants/sgl200-bldc](pcb-b-control/variants/sgl200-bldc/README.md) | [`firmware/products/sgl200-bldc.conf`](../firmware/products/sgl200-bldc.conf) | Reserved until motor transport, feedback, enable, and fault pins are assigned |

## Shared Hardware Assets

- [common/interface-control.md](common/interface-control.md) is the firmware-to-hardware pin and connector contract.
- `common/symbols/`, `common/footprints/`, and `common/3dmodels/` are reserved for local KiCad libraries.
- `common/datasheets/` is reserved for component datasheets used during schematic capture and review.
