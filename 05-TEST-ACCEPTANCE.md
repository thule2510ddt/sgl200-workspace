# SGL-200 Test And Acceptance

This file is the canonical verification standard for AI-generated work. Task-specific verification is listed in `03-BACKLOG.md`; this document defines the higher-level acceptance evidence.

## Global Acceptance Metrics

| Area | Acceptance target | Evidence |
|---|---|---|
| Firmware build | Build passes for `sgl200_v1` | `west build` output |
| Warnings | Fewer than 5 build warnings for scaffold tasks | Build log |
| Boot time | < 3000ms to first MAVLink HEARTBEAT TX | Logic analyzer capture |
| MAVLink command latency | < 200ms end-to-end | MAVLink log timestamp comparison |
| Command ACK latency | < 100ms where ACK is required | MAVLink log |
| Gimbal stability | < 0.3deg RMS under specified disturbance | DVP-03 log analysis |
| Control jitter | < 50us | SEGGER SystemView trace |
| LED output | >= 8000lm target at 25C | DVP-01 photometric data |
| 80m center beam | >= 15 lux | Lux meter data |
| Thermal throttle | Starts at 75C NTC | Thermal test log |
| Thermal shutdown | Emergency off above 95C NTC | Thermal test log |
| Heartbeat timeout safety | Safe state in < 4s | UART disconnect test |
| Field SAR scenario | Victim recognized in < 30s from 50m | Video and observer record |

## DVP-01 Photometric Test

Purpose:
Verify spotlight brightness and beam usability.

Setup:

- Darkroom or outdoor night environment.
- Distances: 5m, 10m, 20m, 50m, 80m.
- Lux meter: Konica Minolta T-10A preferred, Testo 540 acceptable.
- LED stabilized at target brightness before measurement.

Procedure:

1. Mount payload and align beam center with measurement target.
2. Measure center beam lux at each distance.
3. Record 3 readings per distance and calculate average.
4. Estimate luminous flux using `lux x illuminated_area x correction_factor`.
5. Record ambient temperature and LED thermal state.

Pass criteria:

- Center beam lux at 80m is >= 15 lux.
- Thermal behavior remains below emergency shutdown during nominal test.
- Data sheet includes raw readings, averages, distance, and temperature.

Required evidence:

- Completed data table.
- Photos of setup.
- Lux meter model and calibration status when available.

## DVP-03 Gimbal Stabilization Test

Purpose:
Verify stabilized pointing accuracy.

Setup:

- Gimbal mounted to prototype or test rig.
- Command fixed pitch/yaw setpoint.
- Disturb pitch by +/-20deg at about 0.5Hz for 60s.
- Log `GIMBAL_DEVICE_ATTITUDE_STATUS`.

Procedure:

1. Start MAVLink logging.
2. Command the gimbal setpoint.
3. Apply disturbance profile for 60s.
4. Parse actual attitude and commanded attitude.
5. Calculate RMS deviation of actual pitch minus commanded pitch.
6. Plot time series for review.

Pass criteria:

- RMS deviation < 0.3deg.
- No visible drift during the test window.
- No servo communication fault during the run.

Required evidence:

- MAVLink log.
- RMS calculation output.
- Plot image or report.
- Test notes with disturbance method and duration.

## DVP-09 Boot Time Test

Purpose:
Verify startup time from power-on to first MAVLink HEARTBEAT.

Setup:

- Logic analyzer channel 1 on VBUS rising edge.
- Logic analyzer channel 2 on UART3 TX.
- Firmware configured for normal boot.

Procedure:

1. Arm logic analyzer trigger on VBUS rising edge.
2. Apply payload power.
3. Detect first valid MAVLink HEARTBEAT on UART3 TX.
4. Measure elapsed time.

Pass criteria:

- First HEARTBEAT TX occurs within 3000ms.

Required evidence:

- Logic analyzer capture.
- Decoded UART/MAVLink frame or timestamp annotation.

## MAVLink Integration Acceptance

Required checks:

- FC receives SGL-200 HEARTBEAT as `MAV_TYPE_GIMBAL`.
- GCS displays gimbal device information.
- `GIMBAL_DEVICE_ATTITUDE_STATUS` appears at 4Hz.
- Gimbal setpoint command moves the servo control path.
- `MAV_CMD_DO_SET_RELAY` toggles LED/strobe behavior.
- Brightness command updates LED target brightness.
- PARAM_SET/PARAM_VALUE round trip works and persists after reboot.
- Faults appear as STATUSTEXT.

Evidence:

- Mission Planner or QGroundControl screenshot.
- MAVLink log.
- Notes on FC firmware version and serial settings.

## Safety Acceptance

Required checks:

- FC disarm forces main LED off immediately.
- Heartbeat timeout > 3000ms triggers safe state.
- Safe state sets gimbal neutral and LED brightness 50%.
- IWDG reset is detected and reported after reboot.
- Thermal throttle and shutdown cannot be overridden by normal LED commands.

Evidence:

- Test logs with timestamps.
- Fault queue/STATUSTEXT output.
- Scope or video evidence for immediate LED off when practical.

## Field Night Test

Scenario:
Simulated SAR search over a 100m x 100m area at 50m altitude at night.

Pre-flight constraints:

- Wind < 7m/s.
- Visibility > 1km.
- No rain.
- Payload mount and cables checked.
- MAVLink link verified.
- LED full brightness and strobe modes verified.
- Gimbal manual slew and stabilization checked.
- Drone and payload power budget verified.

Execution outline:

1. Take off and hover at 10m.
2. Verify SGL-200 responds to GCS.
3. Climb to 50m.
4. Turn LED on full brightness.
5. Set gimbal pitch downward for area scan.
6. Locate hidden subject in the search area.
7. Test police strobe mode.
8. Simulate heartbeat timeout and verify safe state.
9. RTL and land.

Pass criteria:

- Subject recognizable in HD video within 30s.
- No visible gimbal drift during the mission.
- Heartbeat timeout safe state activates in < 4s.

Required evidence:

- MAVLink log.
- Drone camera video.
- Ground observer video or notes.
- Debrief report with pass/fail result.

