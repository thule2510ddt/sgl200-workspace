# SGL-200 Product Variants

Product variants select the gimbal actuator backend while keeping the common firmware, MAVLink, LED, thermal, IMU, and control layers shared.

The current branch is `feature/actuator-servo-pwm`, so this branch retains only the Servo PWM product config and hardware scaffold.

## Variants

| Variant | Config | Overlay | Actuator backend |
|---|---|---|---|
| Servo PWM | `sgl200-servo-pwm.conf` | `sgl200-servo-pwm.overlay` | Pitch on TIM4_CH1 PB6, yaw on TIM4_CH2 PB7 |

## Build Pattern

Use a variant config with the base board:

```bash
west build -b sgl200_v1 firmware -- -DEXTRA_CONF_FILE=products/sgl200-servo-pwm.conf -DDTC_OVERLAY_FILE=products/sgl200-servo-pwm.overlay
```

This branch defaults to Servo PWM and its hardware scaffold implements only Servo PWM. Servo bus and BLDC firmware/hardware belong on separate product branches.

Servo PWM uses 50Hz output, a 20ms period, 1000-2000us command pulses, and 1500us neutral.
