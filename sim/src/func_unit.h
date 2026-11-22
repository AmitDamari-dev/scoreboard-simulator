// Non-pipelined functional units and their per-instruction scoreboard status.

#ifndef FUNC_UNIT_H
#define FUNC_UNIT_H

#include <stdbool.h>

#include "config.h"
#include "isa.h"

// Fi is the destination, Fj is SRC0, and Fk is SRC1.
// LD uses Fi only; ST uses Fk only; unused register slots hold -1.
// Unused sources keep Q = UNIT_REF_NONE and R = false.
typedef struct {
    UnitRef ref;
    int delay;

    bool busy;
    Opcode op;                 // OP_COUNT while idle
    int fi, fj, fk;
    UnitRef qj, qk;
    bool rj, rk;

    uint32_t raw;
    int pc;
    uint16_t imm;              // memory word address for LD/ST
    int exec_cycles_left;
    float result_val;

    // Timestamps hold -1 until each stage completes.
    int cycle_issued;
    int cycle_read_operands;
    int cycle_execute_end;
    int cycle_write_result;
    uint64_t issue_order;      // UINT64_MAX while idle
} FuncUnit;

typedef struct {
    FuncUnit *units[UNIT_TYPE_COUNT];
    int nr_units[UNIT_TYPE_COUNT];
} FuncUnitPool;

bool fu_pool_init(FuncUnitPool *pool, const Config *cfg);

void fu_pool_free(FuncUnitPool *pool);

// Returns the lowest-index idle unit of the requested type, or NULL if all are busy.
FuncUnit *fu_find_free(FuncUnitPool *pool, UnitType type);

FuncUnit *fu_get(FuncUnitPool *pool, UnitRef ref);

// Reset instruction state while keeping the unit's identity and configured delay.
void fu_clear(FuncUnit *unit);

bool fu_any_busy(const FuncUnitPool *pool);

// Assign an instruction to an idle matching unit. Caller sets qj/qk and updates rj/rk.
bool fu_assign_instruction(FuncUnit *unit, const Instruction *inst, int pc,
                           int cycle_issued, uint64_t issue_order);

#endif
