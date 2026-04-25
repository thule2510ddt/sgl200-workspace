# SGL-200 Decision Log

This file records project decisions that future AI agents must preserve. Add entries only for meaningful technical choices or changes.

## Locked Decisions

### D-001 - MCU

- Decision: Use STM32G431CBU6 at 170MHz.
- Status: Locked.
- Rationale: Provides required real-time performance, STM32G4 peripheral set, and CORDIC coprocessor for AHRS optimization.

### D-002 - RTOS

- Decision: Use Zephyr RTOS v3.6+.
- Status: Locked.
- Rationale: Native driver model, deterministic threading, IPC, west/CMake ecosystem, and embedded test support.

### D-003 - IMU

- Decision: Use ICM-42688-P on SPI1 Mode 3 at up to 24MHz with 1kHz ODR.
- Status: Locked.
- Rationale: High-rate gyro/accel source for the 1kHz stabilization loop.

### D-004 - Main LED Driver

- Decision: Use LT8391A 4-switch buck-boost constant-current driver.
- Status: Locked.
- Rationale: Supports the required wide input range and high-power LED output.

### D-005 - LT8391A Switching Frequency Minimum

- Decision: Do not set or recommend LT8391A switching frequency below 600kHz.
- Status: Locked.
- Rationale: 600kHz is a hard design constraint from the current power architecture.

### D-006 - Main LED

- Decision: Use Luminus SBT-90.2 Gen3 CW 5600K, about 8500lm at 3A.
- Status: Locked.
- Rationale: Meets SAR spotlight output target in the selected optical and thermal architecture.

### D-007 - Actuator Product Branches

- Decision: Actuator-specific firmware and hardware live on separate product branches.
- Status: Accepted.
- Rationale: Servo bus, servo PWM, and BLDC products need different hardware and should not appear implemented in the same product branch.

### D-008 - MAVLink Role

- Decision: SGL-200 implements MAVLink GIMBAL_DEVICE only; the flight controller remains GIMBAL_MANAGER.
- Status: Locked.
- Rationale: Matches MAVLink Gimbal Protocol v2 architecture and keeps high-level arbitration on the FC.

### D-009 - MAVLink Link

- Decision: Use USART3 at 921600bps for MAVLink v2.
- Status: Locked.
- Rationale: Provides enough bandwidth for heartbeat, gimbal telemetry, parameters, ACKs, and fault messages.

### D-010 - Thermal Thresholds

- Decision: Thermal throttle starts at NTC = 75C and emergency shutdown occurs above 95C.
- Status: Locked.
- Rationale: Protects LED and driver hardware from over-temperature operation.

### D-011 - AI Workspace Source Of Truth

- Decision: Root-level `00-*` through `06-*` documents are the canonical AI project hub.
- Status: Locked.
- Rationale: Removes ambiguity from duplicated root, package, and hidden skill snapshots.

### D-012 - Active Skill Location

- Decision: `.antigravity/skills/sgl200-expert/SKILL.md` is the active Antigravity skill.
- Status: Locked.
- Rationale: Antigravity loads the workspace skill from the hidden skill directory, while root/package copies are legacy or export material.

### D-013 - Product Actuator Variants

- Decision: Keep one firmware repository and select gimbal actuator hardware through Kconfig/product overlays behind a common actuator abstraction.
- Status: Accepted.
- Rationale: SGL-200 products may ship with servo bus, servo PWM, or BLDC actuation while sharing MAVLink, IMU, LED, thermal, and gimbal control logic.
- Consequences: Control code must call the gimbal actuator abstraction instead of backend-specific drivers such as Feetech directly.

### D-014 - Servo PWM Variant Pin Map

- Decision: Use TIM4_CH1 PB6 for pitch servo PWM and TIM4_CH2 PB7 for yaw servo PWM.
- Status: Accepted.
- Rationale: PB6 and PB7 are available on STM32G431CBU6, share TIM4 for paired servo outputs, and avoid locked pins for SPI1, USART2/3, LED PWM, ADC, AUX LEDs, FDCAN, and debug.
- Consequences: Servo PWM products use 50Hz output with 1000-2000us command pulses and 1500us neutral.

## Decision Entry Template

Use this template for future decisions:

```markdown
### D-015 - Short Title

- Decision: ...
- Status: Proposed | Accepted | Locked | Superseded.
- Rationale: ...
- Consequences: ...
- Supersedes: D-xxx, if applicable.
```
