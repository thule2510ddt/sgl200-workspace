# SGL-200 AI Project Hub

This workspace is the canonical AI project hub for SGL-200. Start here before reading any legacy prompt bundles or package exports.

## Canonical Read Order

1. `00-START-HERE.md` - workspace contract, read order, and current operating model.
2. `01-MASTER-CONTEXT.md` - project identity, locked hardware decisions, firmware architecture, constraints, and hard rules.
3. `02-AI-WORKFLOW.md` - how AI agents should intake work, design, implement, verify, and record decisions.
4. `03-BACKLOG.md` - normalized sprint backlog with task context, outputs, constraints, acceptance criteria, and verification.
5. `04-ARCHITECTURE.md` - hardware, firmware, thread model, MAVLink, LED, safety, and gimbal architecture.
6. `05-TEST-ACCEPTANCE.md` - DVP, field test, metrics, and required evidence.
7. `06-DECISION-LOG.md` - locked decisions and future decision records.

## Workspace Contract

- The root-level `00-*` through `06-*` files are the canonical working documents.
- `.antigravity/skills/sgl200-expert/SKILL.md` is the active Antigravity skill. It should stay compact and point back to this hub.
- `SGL200-LINEAR-SESSIONS.md`, `SGL200-MERMAID-DIAGRAMS.md`, `SGL200-ANTIGRAVITY-SKILL.md`, and `WORKSPACE_STRUCTURE.md` are legacy source-import references.
- `sgl200-antigravity-package/` and `sgl200-antigravity-package.zip` are archive/export artifacts. Do not update them as the source of truth.

## Current Project State

- The workspace currently contains planning, architecture, prompt, and workflow documentation.
- It does not yet contain the real firmware, hardware CAD/KiCad, test scripts, or generated build artifacts described by the target project structure.
- When implementing future tasks, create the real project files from `03-BACKLOG.md` and verify against `05-TEST-ACCEPTANCE.md`.

## How An AI Agent Should Work Here

1. Load the canonical read order above.
2. Confirm the target task from `03-BACKLOG.md`.
3. Check locked constraints in `01-MASTER-CONTEXT.md` and `06-DECISION-LOG.md`.
4. Implement only the requested task scope.
5. Run the verification listed for that task, or clearly report why it could not be run.
6. Update `06-DECISION-LOG.md` only when a new meaningful technical decision is made.

## Next Recommended Work

Start with Sprint 0 unless the user directs otherwise:

- `S0-01`: PCB-A schematic review checklist.
- `S0-02`: Zephyr board definition for `sgl200_v1`.
- `S0-03`: BOM and long-lead procurement analysis.

For firmware implementation, begin with `S1-01` after Sprint 0 decisions are locked.

