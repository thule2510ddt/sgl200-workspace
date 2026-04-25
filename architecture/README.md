# SGL-200 Architecture Diagrams

This folder is the canonical source for Mermaid architecture diagrams, including state machines, data flow, data structures, and system interaction views.

The root `04-ARCHITECTURE.md` remains the human-readable architecture summary. Keep detailed Mermaid sources here and link back to them from summary documents when needed.

## Naming Convention

- Use lowercase kebab-case filenames.
- Use Markdown files with Mermaid fenced blocks.
- Keep each file focused on one diagram.

## Diagrams

| Diagram | Purpose |
|---|---|
| [System Overview](system-overview.md) | High-level hardware, firmware, power, and external integration flow |
| [Thread Data Flow](thread-data-flow.md) | Zephyr threads, IPC primitives, and runtime data movement |
| [LED State Machine](led-state-machine.md) | LED manager states, command transitions, thermal throttle, and emergency shutdown |
| [Gimbal Control Flow](gimbal-control-flow.md) | MAVLink command intake through cascaded gimbal control and actuator output |
| [MAVLink Message Flow](mavlink-message-flow.md) | Flight controller, ground station, RX/TX thread, ACK, telemetry, and fault reporting interactions |
| [Core Data Structure](core-data-structure.md) | Main runtime data structures and ownership relationships |

## Source And Branch Maps

| Document | Purpose |
|---|---|
| [Source Code Map](source-code-map.md) | Maps generated source files to architecture responsibilities |
| [Branch Map](branch-map.md) | Records current branch roles, feature dependencies, and merge order |
