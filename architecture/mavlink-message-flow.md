# MAVLink Message Flow

This diagram shows SGL-200 interactions with the flight controller and ground station over MAVLink.

```mermaid
sequenceDiagram
    participant GCS as Ground Station
    participant FC as Flight Controller / GIMBAL_MANAGER
    participant RX as mavlink_rx_thread
    participant APP as Gimbal / LED / Param Handlers
    participant TX as mavlink_tx_thread

    GCS->>FC: Operator command or parameter change
    FC->>RX: HEARTBEAT
    RX->>APP: Update FC system ID, arming state, heartbeat timestamp

    FC->>RX: GIMBAL_DEVICE_SET_ATTITUDE
    RX->>APP: Validate and update gimbal setpoint
    APP->>TX: Queue COMMAND_ACK
    TX->>FC: COMMAND_ACK

    FC->>RX: COMMAND_LONG LED / brightness command
    RX->>APP: Queue LED command
    APP->>TX: Queue COMMAND_ACK
    TX->>FC: COMMAND_ACK

    FC->>RX: PARAM_SET or parameter request
    RX->>APP: Update or read parameter store
    APP->>TX: Queue PARAM_VALUE
    TX->>FC: PARAM_VALUE
    FC-->>GCS: Forward parameter response

    loop 1Hz
        TX->>FC: HEARTBEAT
    end

    loop 4Hz
        TX->>FC: GIMBAL_DEVICE_ATTITUDE_STATUS
    end

    APP->>TX: Queue STATUSTEXT on fault
    TX->>FC: STATUSTEXT
    FC-->>GCS: Forward fault text
```

Notes: MAVLink heartbeat timeout greater than 3000ms triggers safe state.
