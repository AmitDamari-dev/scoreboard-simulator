# Design

This is the external project documentation. The final version is submitted with the project.

> Status: skeleton only. Fill this in during implementation.

## 1. Overview
TODO: what the simulator does and how a run flows (load, then a cycle loop, then dump outputs).

## 2. Module layout

| module         | responsibility                                         |
|----------------|--------------------------------------------------------|
| `main.c`       | argument handling, top-level flow                      |
| `config.*`     | parse `cfg.txt`                                        |
| `memory.*`     | 4096-word memory, `memin`/`memout`, address conflicts  |
| `isa.*`        | opcodes and instruction decoding                       |
| `registers.*`  | F0–F15 and the register result status                  |
| `inst_queue.*` | fetch unit and 16-entry instruction queue              |
| `func_unit.*`  | functional unit status (Busy, Op, Fi, Fj, Fk, Qj, Qk, Rj, Rk) |
| `scoreboard.*` | per-cycle Issue, Read Operands, Execute and Write Result |
| `trace.*`      | `traceinst.txt` and `traceunit.txt`                    |

## 3. Data structures
TODO

## 4. Cycle model and stage ordering
TODO: in what order stages are evaluated within a cycle, and why.

## 5. Decisions on the open questions
TODO: one entry for each item in `SPEC.md`, "Open questions".

## 6. Tests
See `TESTING.md`.
