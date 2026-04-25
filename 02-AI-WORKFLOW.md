# SGL-200 AI Workflow

## Purpose

This workflow turns the workspace into a repeatable operating system for AI-assisted engineering. Agents should use it to understand intent, implement tasks, verify outcomes, and keep project decisions traceable.

## Operating Loop

1. Intake
   - Read `00-START-HERE.md`, `01-MASTER-CONTEXT.md`, and the target issue in `03-BACKLOG.md`.
   - Identify the requested output files and acceptance criteria before editing.
   - Check whether the task depends on a locked decision in `06-DECISION-LOG.md`.

2. Design
   - Prefer existing project patterns and the canonical architecture in `04-ARCHITECTURE.md`.
   - Keep scope limited to the selected issue unless the user asks for a larger slice.
   - Resolve pin, timing, protocol, and safety constraints before writing code or documentation.

3. Implement
   - Create or update only the files needed for the task.
   - Preserve legacy source-import files unless the task explicitly asks to update them.
   - For firmware, use Zephyr-native APIs and the target project structure from the backlog.
   - For hardware documentation, include formulas, target values, and review criteria.

4. Verify
   - Run the verification listed in `03-BACKLOG.md` when the required toolchain or environment exists.
   - If hardware, Zephyr, KiCad, J-Link, or MAVLink equipment is unavailable, document the exact blocked verification and provide the next physical test.
   - Cross-check locked requirements against `01-MASTER-CONTEXT.md`.

5. Record
   - Add a decision to `06-DECISION-LOG.md` only when the work introduces or changes a meaningful technical choice.
   - Do not use the decision log for routine task progress.
   - Keep old package exports as archive material unless explicitly regenerating a release package.

## Task Format

Every implementation issue should be readable in this shape:

- Context: why the task exists and which subsystem it affects.
- Inputs: existing documents, locked decisions, datasheets, protocols, or hardware assumptions.
- Output files: exact files or artifact types expected from the task.
- Implementation constraints: hard rules the implementer must follow.
- Acceptance criteria: objective definition of done.
- Verification: command, review method, physical test, or evidence required.

## Handoff Rules For Agents

- Start from the canonical root documents, not from the package export.
- Treat `03-BACKLOG.md` as the task queue and `05-TEST-ACCEPTANCE.md` as the verification standard.
- When information conflicts, prefer this order:
  1. `06-DECISION-LOG.md`
  2. `01-MASTER-CONTEXT.md`
  3. `04-ARCHITECTURE.md`
  4. `03-BACKLOG.md`
  5. legacy source-import documents
- If a legacy document contains useful detail missing from canonical docs, port the detail into the relevant canonical file instead of making the legacy file active again.

## Definition Of Done

A task is done when:

- The requested artifacts exist in the expected location.
- Hard constraints are satisfied or a blocker is documented.
- Verification has been run, simulated, or explicitly deferred with a reason.
- New decisions, if any, are recorded in `06-DECISION-LOG.md`.
- A future agent can continue from the canonical files without reading the full legacy prompt bundle.

