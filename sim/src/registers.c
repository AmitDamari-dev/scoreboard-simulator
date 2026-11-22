/*
 * registers.c
 *
 * Store register values and pending writers, and dump the values to regout.txt.
 */

#include "registers.h"

#include <stdio.h>

static bool valid_index(int index)
{
    return index >= 0 && index < NUM_REGS;
}

void regs_init(Registers *registers)
{
    for (int index = 0; index < NUM_REGS; ++index) {
        registers->val[index] = (float)index;
        registers->result[index] = UNIT_REF_NONE;
    }
}

bool regs_dump(const Registers *registers, const char *path)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "%s: cannot open register output\n", path);
        return false;
    }

    bool ok = true;
    for (int index = 0; index < NUM_REGS; ++index) {
        if (fprintf(file, "%f\n", (double)registers->val[index]) < 0) {
            ok = false;
            break;
        }
    }
    if (fclose(file) != 0) {
        ok = false;
    }
    if (!ok) {
        fprintf(stderr, "%s: cannot write register values\n", path);
    }
    return ok;
}

bool regs_get_value(const Registers *registers, int index, float *value)
{
    if (!valid_index(index) || value == NULL) {
        return false;
    }
    *value = registers->val[index];
    return true;
}

bool regs_set_value(Registers *registers, int index, float value)
{
    if (!valid_index(index)) {
        return false;
    }
    registers->val[index] = value;
    return true;
}

bool regs_get_result(const Registers *registers, int index, UnitRef *unit)
{
    if (!valid_index(index) || unit == NULL) {
        return false;
    }
    *unit = registers->result[index];
    return true;
}

bool regs_set_result(Registers *registers, int index, UnitRef unit)
{
    if (!valid_index(index)) {
        return false;
    }
    if (unit.type == UNIT_NONE) {
        unit = UNIT_REF_NONE;
    } else if (unit.type < UNIT_ADD || unit.type >= UNIT_TYPE_COUNT || unit.index < 0) {
        return false;
    }
    registers->result[index] = unit;
    return true;
}
