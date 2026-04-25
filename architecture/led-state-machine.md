# LED State Machine

This diagram shows the LED manager states, command transitions, thermal throttle, and emergency shutdown behavior.

```mermaid
stateDiagram-v2
    [*] --> LED_IDLE: boot / LT8391A disabled

    LED_IDLE --> LED_SOFT_START: valid on command and FC armed
    LED_IDLE --> LED_EMERGENCY_OFF: NTC > 95C

    LED_SOFT_START --> LED_NORMAL: 500ms ramp complete
    LED_SOFT_START --> LED_IDLE: off command or FC disarmed
    LED_SOFT_START --> LED_THERMAL_THROTTLE: NTC > 75C
    LED_SOFT_START --> LED_EMERGENCY_OFF: NTC > 95C

    LED_NORMAL --> LED_IDLE: off command or FC disarmed
    LED_NORMAL --> LED_STROBE_WHITE: white strobe command
    LED_NORMAL --> LED_STROBE_POLICE: police strobe command
    LED_NORMAL --> LED_STROBE_SOS: SOS strobe command
    LED_NORMAL --> LED_THERMAL_THROTTLE: NTC > 75C
    LED_NORMAL --> LED_EMERGENCY_OFF: NTC > 95C

    LED_STROBE_WHITE --> LED_NORMAL: steady command
    LED_STROBE_WHITE --> LED_IDLE: off command or FC disarmed
    LED_STROBE_WHITE --> LED_THERMAL_THROTTLE: NTC > 75C
    LED_STROBE_WHITE --> LED_EMERGENCY_OFF: NTC > 95C

    LED_STROBE_POLICE --> LED_NORMAL: steady command
    LED_STROBE_POLICE --> LED_IDLE: off command or FC disarmed
    LED_STROBE_POLICE --> LED_THERMAL_THROTTLE: NTC > 75C
    LED_STROBE_POLICE --> LED_EMERGENCY_OFF: NTC > 95C

    LED_STROBE_SOS --> LED_NORMAL: steady command
    LED_STROBE_SOS --> LED_IDLE: off command or FC disarmed
    LED_STROBE_SOS --> LED_THERMAL_THROTTLE: NTC > 75C
    LED_STROBE_SOS --> LED_EMERGENCY_OFF: NTC > 95C

    LED_THERMAL_THROTTLE --> LED_NORMAL: NTC <= 75C and steady command active
    LED_THERMAL_THROTTLE --> LED_IDLE: off command or FC disarmed
    LED_THERMAL_THROTTLE --> LED_EMERGENCY_OFF: NTC > 95C

    LED_EMERGENCY_OFF --> [*]: manual reboot / recovery policy
```

Notes: Thermal throttle reduces brightness by 10% per degree C down to a 20% minimum. Emergency off cuts PWM immediately.
