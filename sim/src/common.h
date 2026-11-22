// Shared constants and functional unit identifiers for the scoreboard simulator.

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MEM_SIZE        4096    // words of 32 bits
#define NUM_REGS        16      // F0..F15
#define INST_QUEUE_SIZE 16

// The six active unit types index per-type arrays directly.
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

typedef struct {
    UnitType type;
    int index;
} UnitRef;

#define UNIT_REF_NONE ((UnitRef){ UNIT_NONE, -1 })

#endif
