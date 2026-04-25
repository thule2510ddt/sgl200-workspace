# SGL-200 Product Variants

Product variants select the gimbal actuator backend while keeping the common firmware, MAVLink, LED, thermal, IMU, and control layers shared.

## Variants

| Variant | Config | Overlay | Actuator backend |
|---|---|---|---|
| Servo bus | `sgl200-servo-bus.conf` | `sgl200-servo-bus.overlay` | Feetech ST3215HS over USART2 half-duplex |
| Servo PWM | `sgl200-servo-pwm.conf` | `sgl200-servo-pwm.overlay` | Reserved for PWM servo outputs |
| BLDC | `sgl200-bldc.conf` | `sgl200-bldc.overlay` | Reserved for BLDC motor control |

## Build Pattern

Use a variant config with the base board:

```bash
west build -b sgl200_v1 firmware -- -DEXTRA_CONF_FILE=products/sgl200-servo-bus.conf -DDTC_OVERLAY_FILE=products/sgl200-servo-bus.overlay
```

The current implemented backend is `sgl200-servo-bus`. PWM servo and BLDC variants intentionally select their backend Kconfig symbols, but their actuator implementations still return `-ENOTSUP` until the product hardware interface is defined.
