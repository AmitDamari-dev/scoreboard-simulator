# Scoreboard Simulator

A cycle-accurate simulator of a floating-point processor that schedules instructions with the
**scoreboard** algorithm. It is the project for Computer Architecture, Semester A 2026/27.

> Status: project skeleton. Nothing is implemented yet.

## Requirements
- Windows
- Visual Studio 2026 Community, with the "Desktop development with C++" workload (toolset v145)
- C (C11)

## Build
Open `sim.sln` and run **Build ▸ Build Solution**. The output is `x64\Debug\sim.exe` (or
`x64\Release\sim.exe`).

## Run
```
sim.exe cfg.txt memin.txt memout.txt regout.txt traceinst.txt traceunit.txt
```
`cfg.txt` and `memin.txt` are inputs. The other four files are outputs.

## Repository layout
```
sim.sln                 Visual Studio solution
sim/sim.vcxproj         C project, builds sim.exe
sim/src/                source (one module per concern, see docs/DESIGN.md)
docs/SPEC.md            English summary of the assignment + open questions
docs/DESIGN.md          external documentation (to be written)
docs/TESTING.md         test plan and results
tests/test1..3/         the 3 required test folders (inputs + outputs)
```

## Processor at a glance
- 16 single-precision FP registers, F0–F15. Each `F[i]` starts as `i`.
- Fetch of 1 instruction per cycle into a 16-entry queue. Issue of 1 instruction per cycle.
- Non-pipelined ADD, SUB, MUL, DIV, LD and ST units. Their counts and latencies come from the
  config file.
- 4096 × 32-bit word-addressed memory.
- ISA: `LD ST ADD SUB MULT DIV HALT` (opcodes 0–6). See `docs/SPEC.md`.
