# SGL-200 Product Variants

Product variants select the gimbal actuator backend while keeping the common firmware, MAVLink, LED, thermal, IMU, and control layers shared.

## Variants

| Variant | Config | Overlay | Actuator backend |
|---|---|---|---|
| Servo bus | `sgl200-servo-bus.conf` | `sgl200-servo-bus.overlay` | Feetech ST3215HS over USART2 half-duplex |
| Servo PWM | `sgl200-servo-pwm.conf` | `sgl200-servo-pwm.overlay` | Pitch on TIM4_CH1 PB6, yaw on TIM4_CH2 PB7 |
| BLDC | `sgl200-bldc.conf` | `sgl200-bldc.overlay` | Reserved for BLDC motor control |

## Build Pattern

Use a variant config with the base board:

```bash
west build -b sgl200_v1 firmware -- -DEXTRA_CONF_FILE=products/sgl200-servo-bus.conf -DDTC_OVERLAY_FILE=products/sgl200-servo-bus.overlay
```

The current implemented backends are `sgl200-servo-bus` and `sgl200-servo-pwm`. The BLDC variant intentionally selects its backend Kconfig symbol, but its actuator implementation still returns `-ENOTSUP` until the product hardware interface is defined.

Servo PWM uses 50Hz output, a 20ms period, 1000-2000us command pulses, and 1500us neutral.
