# Specification summary

This is an English summary of the assignment (Computer Architecture, Semester A 2026/27,
"Scoreboard project"). The original Hebrew PDF is the authority. It is kept out of this public
repository.

## Processor

- **Registers.** 16 floating-point registers `F0`–`F15`, each 32-bit IEEE-754 single precision.
  At start, `F[i] = (float)i` (`F0 = 0.0`, `F1 = 1.0`, ...). There is no limit on parallel
  register reads or writes in a cycle.
- **Fetch.** Fetches up to one instruction per cycle from memory into a 16-entry instruction
  queue while the queue has room. Cycle 0 fetches the first instruction (PC = 0).
- **Issue.** Up to one instruction per cycle. Decode happens during issue and takes no extra
  cycle. Cycle 1 can issue the instruction fetched in cycle 0 while the next one is fetched.
- **Functional units.** add, sub, mul, div, load and store. They are not pipelined, and their
  count and latency come from the config file.
  - The Read Operands cycle is also the first Execute cycle.
  - With latency `d`, Execute ends `d - 1` cycles after Read Operands. For example, with `d = 2`
    it ends one cycle after RO.
  - Write Result is the cycle after Execute ends.
- **Memory.** 4096 words of 32 bits. Addresses are word addresses, not byte addresses. All LD and
  ST units may access memory in parallel, with these limits:
  - no read and write active at the same time on the same address,
  - no two writes active at the same time on the same address,
  - parallel reads of one address are allowed when nobody writes it.
  - LD and ST never touch addresses that hold program instructions.

## Instruction format (32 bits)

| bits  | 31–28 | 27–24  | 23–20 | 19–16 | 15–12 | 11–0 |
|-------|-------|--------|-------|-------|-------|------|
| field | 0     | OPCODE | DST   | SRC0  | SRC1  | IMM  |

| opcode | # | meaning                     |
|--------|---|-----------------------------|
| LD     | 0 | `F[DST] = MEM[IMM]`         |
| ST     | 1 | `MEM[IMM] = F[SRC1]`        |
| ADD    | 2 | `F[DST] = F[SRC0] + F[SRC1]`|
| SUB    | 3 | `F[DST] = F[SRC0] - F[SRC1]`|
| MULT   | 4 | `F[DST] = F[SRC0] * F[SRC1]`|
| DIV    | 5 | `F[DST] = F[SRC0] / F[SRC1]`|
| HALT   | 6 | exit simulator              |

## Build and run

- C, Windows, Visual Studio 2026 Community. Submit the whole solution folder so it builds with
  *Build Solution*.
- Command line:

  ```
  sim.exe cfg.txt memin.txt memout.txt regout.txt traceinst.txt traceunit.txt
  ```

  Inputs: `cfg.txt` and `memin.txt`. The other four files are outputs.

## Files

### cfg.txt
Lines of the form `parameter = value`:

| parameter      | meaning                         |
|----------------|---------------------------------|
| `add_nr_units` | number of ADD units             |
| `sub_nr_units` | number of SUB units             |
| `mul_nr_units` | number of MUL units             |
| `div_nr_units` | number of DIV units             |
| `ld_nr_units`  | number of LD units              |
| `st_nr_units`  | number of ST units              |
| `add_delay`    | ADD latency (cycles)            |
| `sub_delay`    | SUB latency                     |
| `mul_delay`    | MUL latency                     |
| `div_delay`    | DIV latency                     |
| `ld_delay`     | LD latency                      |
| `st_delay`     | ST latency                      |
| `trace_unit`   | unit traced into traceunit.txt  |

### memin.txt / memout.txt
Up to 4096 lines, each holding 8 hex digits. Line 0 is address 0, where execution starts.
Trailing zero lines may be left out. `memout.txt` uses the same format and holds memory at the
end of the run.

### regout.txt
16 lines. Line `i` is the decimal value of `F[i]` at the end of the run.

### traceinst.txt
One line per instruction except HALT, in **issue order**, not completion order:

```
instruction pc unit cycle_issued cycle_read_operands cycle_execute_end cycle_write_result
```

- `instruction`: the encoding in 8 hex digits, as read from memory.
- `pc`: the instruction's PC (0, 1, ...).
- `unit`: the unit name, numbered from 0, for example `ADD0`, `MUL2`, `DIV1`, `LD3`.
- `cycle_issued`: the cycle the instruction entered a unit.
- `cycle_read_operands`: the cycle operands were read and execution started.
- `cycle_execute_end`: the last cycle still executing.
- `cycle_write_result`: the cycle the result was written. ST also gets a write-result cycle,
  for consistency.

### traceunit.txt
One line for each cycle in which `trace_unit` is Busy:

```
cycle unit Fi Fj Fk Qj Qk Rj Rk
```

- `Fi`, `Fj` and `Fk` are register names as in the scoreboard algorithm.
- `Qj` and `Qk` are unit names, or `-` when there is none.
- `Rj` and `Rk` are `Yes` or `No`.

## Documentation and tests (graded)

- The code must be readable, with comments on data structures and functions.
- Submit external documentation that describes the project.
- Hidden test programs will be run, so test many code sequences and config values.
- Submit **3 test folders**, each with its input and output files.
- Describe the tests in the documentation.

## Open questions and assumptions to settle before coding

Resolve each item from the lecture's scoreboard definition or by asking the course staff, then
record the decision in `DESIGN.md`.

1. **Same-cycle visibility.** Can an instruction read operands in the cycle its producer writes
   the result (classic scoreboard: no, it waits until the next cycle)? Can a unit freed in
   Write Result be issued to in the same cycle?
2. **HALT.** Does fetch stop once HALT is fetched? Does the run end only after every issued
   instruction has finished Write Result? HALT gets no traceinst line. Does HALT ever occupy a
   unit?
3. **Field mapping for LD and ST in traceunit.** LD: Fi = DST, and Fj/Fk unused. ST: Fk = SRC1
   (or Fj?), and Fi unused. What should an unused field print?
4. **Register name format.** For example `F2`.
5. **regout number format.** "Decimal number" only. Pick a printf format, such as `%f`, and
   confirm it.
6. **Memory hazards.** How exactly should LD and ST conflicts on the same IMM address stall?
   Which stage stalls (issue, read operands or write result), and is order preserved?
7. **WAR rule.** Write Result waits while an earlier-issued unit still has `Fj == Fi` or
   `Fk == Fi` with `Rj` or `Rk = Yes`.
8. **Division by zero.** Follow IEEE (inf/NaN), as C `float` does.
9. **Unit ordering.** Which free unit gets picked when several of a type are free? Lowest index.
10. **Cycle counting.** When does the run end, and is anything printed for the last cycle?
11. **Instruction memory.** Fetch reads the same memory image that LD and ST use (unified
    memory).
