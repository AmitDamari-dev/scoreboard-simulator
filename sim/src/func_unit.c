#include "func_unit.h"

#include <stdlib.h>

static bool valid_type(UnitType type)
{
    return type >= UNIT_ADD && type < UNIT_TYPE_COUNT;
}

bool fu_pool_init(FuncUnitPool *pool, const Config *cfg)
{
    *pool = (FuncUnitPool){ 0 };
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        int count = cfg->nr_units[type];
        if (count == 0) {
            continue;
        }
        if (count < 0 || cfg->delay[type] < 1 || (size_t)count > SIZE_MAX / sizeof(FuncUnit)) {
            fu_pool_free(pool);
            return false;
        }

        FuncUnit *units = malloc((size_t)count * sizeof(*units));
        if (units == NULL) {
            fu_pool_free(pool);
            return false;
        }
        pool->units[type] = units;
        pool->nr_units[type] = count;

        for (int index = 0; index < count; ++index) {
            FuncUnit *unit = &units[index];
            unit->ref = (UnitRef){ (UnitType)type, index };
            unit->delay = cfg->delay[type];
            fu_clear(unit);
        }
    }
    return true;
}

void fu_pool_free(FuncUnitPool *pool)
{
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        free(pool->units[type]);
    }
    *pool = (FuncUnitPool){ 0 };
}

FuncUnit *fu_find_free(FuncUnitPool *pool, UnitType type)
{
    if (!valid_type(type)) {
        return NULL;
    }
    for (int index = 0; index < pool->nr_units[type]; ++index) {
        FuncUnit *unit = &pool->units[type][index];
        if (!unit->busy) {
            return unit;
        }
    }
    return NULL;
}

FuncUnit *fu_get(FuncUnitPool *pool, UnitRef ref)
{
    if (!valid_type(ref.type) || ref.index < 0 || ref.index >= pool->nr_units[ref.type]) {
        return NULL;
    }
    return &pool->units[ref.type][ref.index];
}

void fu_clear(FuncUnit *unit)
{
    // Preserve unit identity and configured delay across instructions.
    *unit = (FuncUnit){
        .ref = unit->ref,
        .delay = unit->delay,
        .op = OP_COUNT,
        .fi = -1,
        .fj = -1,
        .fk = -1,
        .qj = UNIT_REF_NONE,
        .qk = UNIT_REF_NONE,
        .pc = -1,
        .cycle_issued = -1,
        .cycle_read_operands = -1,
        .cycle_execute_end = -1,
        .cycle_write_result = -1,
        .issue_order = UINT64_MAX
    };
}

bool fu_any_busy(const FuncUnitPool *pool)
{
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        for (int index = 0; index < pool->nr_units[type]; ++index) {
            if (pool->units[type][index].busy) {
                return true;
            }
        }
    }
    return false;
}

bool fu_assign_instruction(FuncUnit *unit, const Instruction *inst, int pc,
                           int cycle_issued, uint64_t issue_order)
{
    UnitType type = isa_unit_type_for_opcode(inst->opcode);
    if (unit->busy || type == UNIT_NONE || type != unit->ref.type || !isa_is_valid(inst) ||
        pc < 0 || cycle_issued < 0 || issue_order == UINT64_MAX) {
        return false;
    }

    fu_clear(unit);
    unit->raw = inst->raw;
    unit->pc = pc;
    unit->op = inst->opcode;
    unit->imm = inst->imm;
    if (inst->opcode != OP_ST) {
        unit->fi = inst->dst;
    }
    if (inst->opcode != OP_LD) {
        unit->fk = inst->src1;
        if (inst->opcode != OP_ST) {
            unit->fj = inst->src0;
        }
    }
    unit->rj = unit->fj >= 0;
    unit->rk = unit->fk >= 0;
    unit->cycle_issued = cycle_issued;
    unit->issue_order = issue_order;
    unit->busy = true;
    return true;
}
