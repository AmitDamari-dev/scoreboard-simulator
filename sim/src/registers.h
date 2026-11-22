// Floating-point register file (F0..F15) and scoreboard Register Result status.

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdbool.h>

#include "common.h"

// result[i] tracks the unit scheduled to write F[i], or UNIT_REF_NONE when idle.
typedef struct {
    float val[NUM_REGS];
    UnitRef result[NUM_REGS];
} Registers;

// Initialize F[i] = (float)i and clear all pending writers.
void regs_init(Registers *registers);

bool regs_dump(const Registers *registers, const char *path);

bool regs_get_value(const Registers *registers, int index, float *value);

bool regs_set_value(Registers *registers, int index, float value);

bool regs_get_result(const Registers *registers, int index, UnitRef *unit);

// Passing unit.type == UNIT_NONE clears the pending writer for F[index].
bool regs_set_result(Registers *registers, int index, UnitRef unit);

#endif
