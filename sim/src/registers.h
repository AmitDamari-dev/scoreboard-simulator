/*
 * registers.h
 *
 * Register file: F0..F15, 32-bit IEEE-754 single precision.
 * Initial value of F[i] is (float)i. Unlimited parallel reads/writes.
 * Also holds the scoreboard "Result" status (which unit will write F[i]).
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdbool.h>

#include "common.h"

/* Register values and the pending writer of each register (UNIT_REF_NONE when ready). */
typedef struct {
    float val[NUM_REGS];
    UnitRef result[NUM_REGS];
} Registers;

/* Set F[i] to (float)i and clear all pending writers. */
void regs_init(Registers *registers);

/* Write F0 through F15, one decimal value per line using %f (six fractional digits). */
bool regs_dump(const Registers *registers, const char *path);

/* Return the value of F[index]. Return false for an invalid index or missing output pointer. */
bool regs_get_value(const Registers *registers, int index, float *value);

/* Update the value of F[index]. Return false for an invalid index. */
bool regs_set_value(Registers *registers, int index, float value);

/* Get the pending writer of F[index]. Return false for an invalid index or missing output pointer. */
bool regs_get_result(const Registers *registers, int index, UnitRef *unit);

/* Set the pending writer of F[index]. Passing UNIT_NONE as the unit type clears it. */
bool regs_set_result(Registers *registers, int index, UnitRef unit);

#endif /* REGISTERS_H */
