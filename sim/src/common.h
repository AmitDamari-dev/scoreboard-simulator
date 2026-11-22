/*
 * common.h
 *
 * Shared constants and types for the scoreboard simulator.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MEM_SIZE        4096    /* words of 32 bits */
#define NUM_REGS        16      /* F0..F15 */
#define INST_QUEUE_SIZE 16

/* Functional unit types. The six actual types index per-type arrays. */
typedef enum {
    UNIT_NONE = -1,
    UNIT_ADD = 0,
    UNIT_SUB,
    UNIT_MUL,
    UNIT_DIV,
    UNIT_LD,
    UNIT_ST,
    UNIT_TYPE_COUNT
} UnitType;

/* Identifies a functional unit by its type and zero-based index within that type. */
typedef struct {
    UnitType type;
    int index;
} UnitRef;

/* Sentinel for the absence of a functional unit. */
#define UNIT_REF_NONE ((UnitRef){ UNIT_NONE, -1 })

#endif /* COMMON_H */
