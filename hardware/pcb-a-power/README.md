# PCB-A Power Board

PCB-A contains the high-power input and LED drive path for SGL-200.

## KiCad Structure

| Folder | Purpose |
|---|---|
| `kicad/` | KiCad project placeholder for the PCB-A schematic and layout |
| `docs/` | Design notes, review checklists, and calculations |
| `fabrication/` | Future Gerber, drill, stackup, drawing, and fabrication notes |
| `assembly/` | Future BOM, CPL/position files, assembly drawings, and test instructions |

## Firmware Interface

PCB-A must expose the hardware counterpart for these firmware-controlled or monitored signals:

| Signal | Firmware source | Hardware role |
|---|---|---|
| Main LED PWM | TIM3_CH1 PB4 from PCB-B | LT8391A dimming/control input |
| NTC_LED | ADC1 IN1 PA0 on PCB-B | LED thermal monitoring |
| NTC_DRIVER | ADC1 IN2 PA1 on PCB-B | Driver thermal monitoring |
| Fault/current placeholders | Reserved | Future protection and telemetry inputs |

The LT8391A switching design must stay at or above the 600kHz project constraint.
