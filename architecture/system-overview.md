# System Overview

This diagram shows the high-level SGL-200 hardware, firmware, power, and external integration flow.

```mermaid
flowchart LR
    BUS[Drone Power Bus<br/>18-54V DC] --> PROTECT[Input Protection<br/>and LC Filter]
    PROTECT --> DRIVER[PCB-A LT8391A<br/>Buck-Boost LED Driver]
    DRIVER --> MAIN_LED[SBT-90.2 Main LED<br/>36V 3A Target]

    FC[Flight Controller] <-->|MAVLink v2<br/>USART3 921600bps| MCU[PCB-B STM32G431CBU6<br/>Zephyr RTOS]
    GCS[Ground Station<br/>Mission Planner or QGroundControl] <-->|Telemetry Link| FC

    MCU <-->|SPI1 Mode 3<br/>1kHz ODR| IMU[ICM-42688-P IMU]
    MCU -->|TIM4_CH1 PB6 / TIM4_CH2 PB7<br/>50Hz servo PWM| ACTUATOR[Gimbal Servo PWM<br/>Pitch / Yaw]
    MCU -->|TIM3 CH1 PWM<br/>20kHz| DRIVER
    THERMAL[LED and Driver NTCs] -->|ADC1 IN1 / IN2| MCU
    MCU -->|GPIO| AUX[AUX LEDs<br/>Red / Blue / IR]
    MCU -->|SWD / SWO / RTT| DEBUG[Debug and Trace]
```

Notes: SGL-200 implements MAVLink `GIMBAL_DEVICE`; the flight controller remains the `GIMBAL_MANAGER`.
