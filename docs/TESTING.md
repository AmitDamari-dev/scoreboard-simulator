# Testing

The assignment requires 3 test folders, each with all of its input and output files, and a
description of them in the documentation.

## Required files in each test folder

```
cfg.txt  memin.txt  memout.txt  regout.txt  traceinst.txt  traceunit.txt
```

## Planned tests

| folder          | focus |
|-----------------|-------|
| `tests/test1`   | Basic flow: LD, ADD, SUB, MULT, DIV, ST, HALT with one unit of each type. Checks latencies and the timing of the Issue, RO, EX and WR stages. |
| `tests/test2`   | Hazards: RAW, WAR, WAW and structural (unit busy), with several units per type. Traces a busy unit. |
| `tests/test3`   | Memory and queue: LD/ST to the same address, many parallel LD/ST units, more than 16 instructions to fill the queue, a different config. |

## Other cases to cover during development (not submitted)

- Every unit count set to 1, and large counts.
- Delay = 1 for every unit type.
- Division by zero.
- `memin.txt` shorter than 4096 lines.
- A `trace_unit` that is never used, which gives an empty `traceunit.txt`.
- HALT as the first instruction.

## Results
TODO: describe each test and its expected and actual results.
