# Branch Map

This document records the current branch strategy and the responsibility of each generated branch.

## Long-Lived Branches

| Branch | Current commit | Purpose | Merge policy |
|---|---|---|---|
| `main` | `4ace794` | Stable baseline for reviewed milestones | Receives release merges only |
| `dev` | `4ace794` | Integration branch for active development | Feature branches merge here after review |
| `release/v0.1.0` | `4ace794` | Initial release preparation branch | Accepts only release stabilization changes |

## Feature Branches

| Branch | Current commit | Function | Status |
|---|---|---|---|
| `feature/product-actuator-abstraction` | `6f066b0` | Adds common gimbal actuator abstraction, product variant configs, Kconfig backend choice, and architecture updates | Pushed; should merge to `dev` before actuator-specific branches are merged |
| `feature/actuator-servo-pwm` | `78209b8` | Implements servo PWM backend using TIM4_CH1 PB6 for pitch and TIM4_CH2 PB7 for yaw | Pushed; depends on `feature/product-actuator-abstraction` |

## Branch Dependency Graph

```mermaid
flowchart LR
    MAIN[main<br/>4ace794] --> DEV[dev<br/>4ace794]
    MAIN --> REL[release/v0.1.0<br/>4ace794]
    DEV --> ABS[feature/product-actuator-abstraction<br/>6f066b0]
    ABS --> PWM[feature/actuator-servo-pwm<br/>78209b8]
```

## Recommended Merge Order

1. Merge `feature/product-actuator-abstraction` into `dev`.
2. Rebase or update `feature/actuator-servo-pwm` on the updated `dev`.
3. Merge `feature/actuator-servo-pwm` into `dev`.
4. Create future actuator branches from `dev` after abstraction is merged.

## Future Branches

| Branch | Purpose |
|---|---|
| `feature/actuator-servo-bus-hardening` | Add readback, torque-enable policy, timeout handling, and tests for Feetech servo bus |
| `feature/actuator-bldc` | Implement BLDC backend once transport, feedback, enable, and fault pins are defined |
| `feature/product-build-matrix` | Add CI or scripts that build each product config/overlay combination |
| `docs/architecture-source-map` | Documentation-only updates to architecture and source maps |
