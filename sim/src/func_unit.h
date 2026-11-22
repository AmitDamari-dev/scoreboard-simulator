/*
 * func_unit.h
 *
 * Functional units (non-pipelined), named <TYPE><index> from 0: ADD0, SUB1,
 * MUL0, DIV0, LD3, ST0 ... Each holds scoreboard status:
 * Busy, Op, Fi, Fj, Fk, Qj, Qk, Rj, Rk + remaining exec cycles + trace data.
 */

#ifndef FUNC_UNIT_H
#define FUNC_UNIT_H

#include <stdbool.h>

#include "config.h"
#include "isa.h"

/*
 * One non-pipelined functional unit and its current instruction.
 * Fi is the destination, and Fj and Fk correspond to SRC0 and SRC1.
 * LD uses Fi only. ST uses Fk only. An unused register field is -1.
 * Qj and Qk identify pending writers, or UNIT_REF_NONE when absent.
 * Rj and Rk mark ready, unread sources and are false for unused sources.
 */
typedef struct {
    UnitRef ref;
    int delay;                 /* Configured execution latency in cycles. */

    bool busy;
    Opcode op;                /* OP_COUNT while idle. */
    int fi, fj, fk;
    UnitRef qj, qk;
    bool rj, rk;

    uint32_t raw;
    int pc;                   /* Word address, or -1 while idle. */
    uint16_t imm;              /* Word address for LD and ST. */
    int exec_cycles_left;      /* Countdown once execution starts. */
    float result_val;          /* Computed result or store data. */

    /* Each cycle is -1 until its stage occurs. */
    int cycle_issued;
    int cycle_read_operands;
    int cycle_execute_end;
    int cycle_write_result;
    uint64_t issue_order;      /* Zero-based issue sequence, or UINT64_MAX while idle. */
} FuncUnit;

/* Owns a dynamically allocated array per configured unit type. */
typedef struct {
    FuncUnit *units[UNIT_TYPE_COUNT];
    int nr_units[UNIT_TYPE_COUNT];
} FuncUnitPool;

/* Initialize a fresh or freed pool. Return false and leave it empty on failure. */
bool fu_pool_init(FuncUnitPool *pool, const Config *cfg);

/* Release the arrays and reset the pool. Safe to repeat after initialization. */
void fu_pool_free(FuncUnitPool *pool);

/* Return the lowest-index idle unit of this type, or NULL if none is free. */
FuncUnit *fu_find_free(FuncUnitPool *pool, UnitType type);

/* Return a unit by reference, or NULL if the reference is outside the pool. */
FuncUnit *fu_get(FuncUnitPool *pool, UnitRef ref);

/* Reset instruction state while preserving the unit's identity and delay. */
void fu_clear(FuncUnit *unit);

/* Return whether any unit holds an instruction. */
bool fu_any_busy(const FuncUnitPool *pool);

/*
 * Assign a decoded instruction and its issue metadata to an idle, matching unit.
 * Used operands start ready. The caller supplies any pending writers before reads.
 * PC and cycle_issued must be non-negative. UINT64_MAX is reserved for an idle unit.
 */
bool fu_assign_instruction(FuncUnit *unit, const Instruction *inst, int pc,
                           int cycle_issued, uint64_t issue_order);

#endif /* FUNC_UNIT_H */
