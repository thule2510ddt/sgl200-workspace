# Gimbal Control Flow

This diagram shows command intake through the cascaded gimbal control loops and actuator output path.

```mermaid
flowchart LR
    FC[Flight Controller] -->|GIMBAL_DEVICE_SET_ATTITUDE| MAV_RX[mavlink_rx_thread]
    FC -->|COMMAND_LONG ROI / mode commands| MAV_RX
    MAV_RX -->|Validate command and mode| MODE[Gimbal Mode Resolver]

    MODE --> STABILIZE[GIMBAL_STABILIZE]
    MODE --> FOLLOW[GIMBAL_FOLLOW]
    MODE --> ROI[GIMBAL_ROI]
    MODE --> LOCK[GIMBAL_LOCK]
    MODE --> NEUTRAL[GIMBAL_NEUTRAL]

    STABILIZE --> SETPOINT[(Pitch/Yaw Angle Setpoint)]
    FOLLOW --> SETPOINT
    ROI -->|Target GPS to angles| SETPOINT
    LOCK -->|Hold current angle| SETPOINT
    NEUTRAL -->|0deg pitch / 0deg yaw| SETPOINT

    IMU[ICM-42688-P IMU] -->|SPI1 DMA samples| AHRS[Madgwick AHRS<br/>1kHz]
    AHRS --> ATTITUDE[(Attitude and Gyro State)]

    SETPOINT --> ANGLE_PID[Outer Angle PID<br/>200Hz]
    ATTITUDE --> ANGLE_PID
    ANGLE_PID -->|Rate setpoint<br/>limit +/-500deg/s| RATE_SETPOINT[(Rate Setpoint)]

    RATE_SETPOINT --> RATE_PID[Inner Rate PID<br/>1kHz]
    ATTITUDE --> RATE_PID
    RATE_PID -->|Pitch/Yaw angle command| ACTUATOR[Gimbal Actuator Abstraction]
    ACTUATOR -->|50Hz PWM<br/>1000-2000us pulses| SERVO_PWM[Servo PWM Outputs<br/>Pitch PB6 TIM4_CH1<br/>Yaw PB7 TIM4_CH2]

    WATCHDOG[Safety Manager / Watchdog] -->|heartbeat timeout or fault| NEUTRAL
```

Notes: Pitch command range is -90deg to +30deg; yaw command range is -160deg to +160deg.
