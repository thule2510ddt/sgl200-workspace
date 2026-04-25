# Thread Data Flow

This diagram shows the Zephyr thread model, IPC primitives, and main runtime data movement.

```mermaid
flowchart TD
    IMU_IRQ[IMU Data Ready IRQ<br/>PB0] -->|k_sem give| IMU_THREAD[imu_thread<br/>Priority 0 / 1ms]
    IMU_THREAD -->|Quaternion, Euler attitude,<br/>gyro rates| ATTITUDE[(Attitude State)]

    MAV_UART[USART3 DMA RX<br/>MAVLink v2] --> MAV_RX[mavlink_rx_thread<br/>Priority 3 / async]
    MAV_RX -->|Gimbal setpoint<br/>atomic or protected state| SETPOINT[(Gimbal Setpoint)]
    MAV_RX -->|LED commands<br/>k_msgq| LED_CMD[(LED Command Queue)]
    MAV_RX -->|Parameter updates| PARAMS[(Parameter Store)]
    MAV_RX -->|ACK requests<br/>k_msgq| ACK_Q[(ACK Queue)]

    ATTITUDE --> ANGLE[angle_thread<br/>Priority 2 / 5ms]
    SETPOINT --> ANGLE
    ANGLE -->|Pitch/yaw rate setpoints| RATE_SP[(Rate Setpoints)]

    RATE_SP --> CONTROL[control_thread<br/>Priority 1 / 1ms]
    ATTITUDE --> CONTROL
    CONTROL -->|Actuator commands| ACTUATOR[Gimbal Servo PWM Backend<br/>Pitch PB6 / Yaw PB7]

    ADC[ADC NTC Samples] --> THERMAL[thermal_thread<br/>Priority 6 / 100ms]
    THERMAL -->|Temperature state| THERMAL_STATE[(Thermal State)]
    THERMAL -->|Thermal faults<br/>k_msgq| FAULT_Q[(Fault Queue)]

    LED_CMD --> LED_MANAGER[led_manager_thread<br/>Priority 5 / 10ms]
    THERMAL_STATE --> LED_MANAGER
    LED_MANAGER -->|PWM and GPIO outputs| LED_HW[LT8391A PWM<br/>AUX LED GPIO]
    LED_MANAGER -->|Faults<br/>k_msgq| FAULT_Q

    ATTITUDE --> MAV_TX[mavlink_tx_thread<br/>Priority 4 / 250ms]
    ACK_Q --> MAV_TX
    FAULT_Q --> MAV_TX
    PARAMS --> MAV_TX
    MAV_TX -->|HEARTBEAT, attitude,<br/>ACK, PARAM_VALUE, STATUSTEXT| MAV_OUT[USART3 DMA TX]

    MAV_RX -->|Heartbeat timestamp| HEALTH[(Thread and Link Health)]
    IMU_THREAD --> HEALTH
    CONTROL --> HEALTH
    LED_MANAGER --> HEALTH
    THERMAL --> HEALTH
    HEALTH --> WATCHDOG[watchdog_thread<br/>Priority 7 / 400ms]
    WATCHDOG -->|IWDG feed or safe-state trigger| SAFETY[Safe State]
    SAFETY --> SETPOINT
    SAFETY --> LED_CMD
```

Notes: Cross-thread mutable state should use Zephyr IPC, atomics, or protected shared state.
