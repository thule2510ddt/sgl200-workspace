# SGL-200 Hardware Workspace

This directory contains the KiCad-oriented hardware structure for the product implemented by the current branch. It is scaffolding only: schematic capture, PCB layout, netlists, and production outputs still need to be created by hardware design work.

## Boards

| Board | Folder | Firmware relationship |
|---|---|---|
| PCB-A Power | [pcb-a-power](pcb-a-power/README.md) | LT8391A power board, main LED driver, power input, LED output, and board-to-board power/control interface |
| PCB-B Control | [pcb-b-control](pcb-b-control/README.md) | STM32G431 control board matching `firmware/boards/arm/sgl200_v1/sgl200_v1.dts` |

## Current Branch Product

| Branch | Hardware folder | Firmware config | Actuator interface |
|---|---|---|---|
| `feature/actuator-servo-pwm` | [pcb-b-control/variants/sgl200-servo-pwm](pcb-b-control/variants/sgl200-servo-pwm/README.md) | [`firmware/products/sgl200-servo-pwm.conf`](../firmware/products/sgl200-servo-pwm.conf) | TIM4_CH1 PB6 pitch, TIM4_CH2 PB7 yaw, 50Hz servo PWM |

Other actuator products must live on their own branches with their own hardware folders. This branch does not implement servo bus or BLDC hardware.

## Shared Hardware Assets

- [common/interface-control.md](common/interface-control.md) is the firmware-to-hardware pin and connector contract.
- `common/symbols/`, `common/footprints/`, and `common/3dmodels/` are reserved for local KiCad libraries.
- `common/datasheets/` is reserved for component datasheets used during schematic capture and review.
