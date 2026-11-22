#include "scoreboard.h"

#include <limits.h>
#include <stdlib.h>

bool scoreboard_init(Scoreboard *sb, const Config *cfg, Memory *mem)
{
    *sb = (Scoreboard){ .mem = mem, .status = SCOREBOARD_RUNNING, .trace_unit = cfg->trace_unit };
    iq_init(&sb->iq);
    regs_init(&sb->regs);
    if (!fu_pool_init(&sb->pool, cfg)) {
        sb->status = SCOREBOARD_ERROR;
        sb->error = "could not initialize functional units";
        return false;
    }

    // Each memory word can be issued at most once in this straight-line ISA.
    sb->inst_records = malloc(MEM_SIZE * sizeof(*sb->inst_records));
    if (sb->inst_records == NULL) {
        fu_pool_free(&sb->pool);
        sb->status = SCOREBOARD_ERROR;
        sb->error = "could not allocate instruction records";
        return false;
    }
    return true;
}

void scoreboard_free(Scoreboard *sb)
{
    fu_pool_free(&sb->pool);
    free(sb->inst_records);
    *sb = (Scoreboard){ .status = SCOREBOARD_DONE };
}

void scoreboard_set_unit_hook(Scoreboard *sb, ScoreboardUnitHook hook, void *context)
{
    sb->unit_hook = hook;
    sb->hook_context = context;
}

static bool fetch(Scoreboard *sb)
{
    if (sb->halted_fetch || iq_is_full(&sb->iq)) {
        return true;
    }

    uint32_t raw;
    if (!mem_read_word(sb->mem, (uint32_t)sb->pc, &raw)) {
        sb->error = "reached the end of memory before fetching HALT";
        return false;
    }
    (void)iq_push(&sb->iq, raw, sb->pc);
    ++sb->pc;

    Instruction inst = isa_decode(raw);
    sb->halted_fetch = inst.opcode == OP_HALT && isa_is_valid(&inst);
    return true;
}

static bool issue(Scoreboard *sb)
{
    const InstQueueEntry *entry = iq_peek(&sb->iq);
    if (entry == NULL) {
        return true;
    }

    const Instruction *inst = &entry->inst;
    if (!isa_is_valid(inst)) {
        sb->error = "invalid instruction encoding at the queue head";
        return false;
    }
    if (inst->opcode == OP_HALT) {
        (void)iq_pop(&sb->iq, NULL);
        sb->halt_issued = true;
        return true;
    }

    UnitType type = isa_unit_type_for_opcode(inst->opcode);
    if (sb->pool.nr_units[type] == 0) {
        sb->error = "instruction requires a functional unit type with zero configured units";
        return false;
    }
    FuncUnit *unit = fu_find_free(&sb->pool, type);
    if (unit == NULL) {
        return true;
    }
    if (inst->opcode != OP_ST) {
        UnitRef writer;
        (void)regs_get_result(&sb->regs, inst->dst, &writer);
        if (writer.type != UNIT_NONE) {
            return true;
        }
    }

    if (!fu_assign_instruction(unit, inst, entry->pc, sb->cycle, (uint64_t)sb->inst_count)) {
        sb->error = "could not assign instruction to an idle functional unit";
        return false;
    }
    // Capture source producers first so an instruction can read its own destination.
    if (unit->fj >= 0) {
        (void)regs_get_result(&sb->regs, unit->fj, &unit->qj);
        unit->rj = unit->qj.type == UNIT_NONE;
    }
    if (unit->fk >= 0) {
        (void)regs_get_result(&sb->regs, unit->fk, &unit->qk);
        unit->rk = unit->qk.type == UNIT_NONE;
    }
    if (unit->fi >= 0) {
        (void)regs_set_result(&sb->regs, unit->fi, unit->ref);
    }

    sb->inst_records[sb->inst_count++] = (ScoreboardInstRecord){
        .raw = unit->raw,
        .pc = unit->pc,
        .unit = unit->ref,
        .cycle_issued = unit->cycle_issued,
        .cycle_read_operands = -1,
        .cycle_execute_end = -1,
        .cycle_write_result = -1
    };
    sb->active[sb->active_count++] = unit;
    (void)iq_pop(&sb->iq, NULL);
    return true;
}

static void record_unread(const Scoreboard *sb, uint64_t oldest_reader[NUM_REGS])
{
    for (int reg = 0; reg < NUM_REGS; ++reg) {
        oldest_reader[reg] = UINT64_MAX;
    }
    for (size_t i = 0; i < sb->active_count; ++i) {
        const FuncUnit *unit = sb->active[i];
        if (unit->rj && oldest_reader[unit->fj] == UINT64_MAX) {
            oldest_reader[unit->fj] = unit->issue_order;
        }
        if (unit->rk && oldest_reader[unit->fk] == UINT64_MAX) {
            oldest_reader[unit->fk] = unit->issue_order;
        }
    }
}

static float latch_result(const Scoreboard *sb, const FuncUnit *unit)
{
    if (unit->op == OP_LD) {
        uint32_t word = 0;
        (void)mem_read_word(sb->mem, unit->imm, &word);
        return mem_word_to_float(word);
    }

    float j = 0.0f;
    float k = 0.0f;
    if (unit->fj >= 0) {
        (void)regs_get_value(&sb->regs, unit->fj, &j);
    }
    (void)regs_get_value(&sb->regs, unit->fk, &k);
    switch (unit->op) {
        case OP_ST: return k;
        case OP_ADD: return j + k;
        case OP_SUB: return j - k;
        case OP_MULT: return j * k;
        case OP_DIV: return j / k;
        default: return 0.0f;
    }
}

static void read_operands(Scoreboard *sb)
{
    enum { ADDRESS_READ = 1, ADDRESS_WRITE = 2 };
    unsigned char older_access[MEM_SIZE] = { 0 };

    for (size_t i = 0; i < sb->active_count; ++i) {
        FuncUnit *unit = sb->active[i];

        // Reserve reads through Execute and stores through their memory write.
        // A waiting older access also reserves its address to preserve program order.
        if (unit->op == OP_ST || (unit->op == OP_LD && unit->cycle_execute_end < 0)) {
            unsigned char *access = &older_access[unit->imm];
            unsigned char kind = unit->op == OP_ST ? ADDRESS_WRITE : ADDRESS_READ;
            bool conflict = (*access & ADDRESS_WRITE) != 0 || (kind == ADDRESS_WRITE && *access != 0);
            *access |= kind;
            if (conflict) {
                continue;
            }
        }
        if (unit->cycle_read_operands >= 0 || (unit->fj >= 0 && !unit->rj) ||
            (unit->fk >= 0 && !unit->rk)) {
            continue;
        }

        unit->result_val = latch_result(sb, unit);
        unit->rj = false;
        unit->rk = false;
        unit->cycle_read_operands = sb->cycle;
        unit->exec_cycles_left = unit->delay - 1;
        if (unit->exec_cycles_left == 0) {
            unit->cycle_execute_end = sb->cycle;
        }
    }
}

static void execute(Scoreboard *sb)
{
    for (size_t i = 0; i < sb->active_count; ++i) {
        FuncUnit *unit = sb->active[i];
        // Read Operands already counted this cycle for units that just started.
        if (unit->cycle_read_operands == sb->cycle || unit->exec_cycles_left == 0) {
            continue;
        }
        if (--unit->exec_cycles_left == 0) {
            unit->cycle_execute_end = sb->cycle;
        }
    }
}

static bool same_unit(UnitRef a, UnitRef b)
{
    return a.type == b.type && a.index == b.index;
}

static void publish_register(Scoreboard *sb, const FuncUnit *writer, size_t first_consumer)
{
    (void)regs_set_value(&sb->regs, writer->fi, writer->result_val);
    UnitRef pending;
    (void)regs_get_result(&sb->regs, writer->fi, &pending);
    if (same_unit(pending, writer->ref)) {
        (void)regs_set_result(&sb->regs, writer->fi, UNIT_REF_NONE);
    }

    // A consumer could only have captured this producer after it was issued.
    for (size_t i = first_consumer; i < sb->active_count; ++i) {
        FuncUnit *unit = sb->active[i];
        if (same_unit(unit->qj, writer->ref)) {
            unit->qj = UNIT_REF_NONE;
            unit->rj = true;
        }
        if (same_unit(unit->qk, writer->ref)) {
            unit->qk = UNIT_REF_NONE;
            unit->rk = true;
        }
    }
}

static void write_result(Scoreboard *sb, const uint64_t oldest_reader[NUM_REGS])
{
    size_t remaining = 0;
    for (size_t i = 0; i < sb->active_count; ++i) {
        FuncUnit *unit = sb->active[i];
        // Only an older unread source needs the value this writer would replace.
        if (unit->cycle_execute_end < 0 || unit->cycle_execute_end >= sb->cycle ||
            (unit->fi >= 0 && oldest_reader[unit->fi] < unit->issue_order)) {
            sb->active[remaining++] = unit;
            continue;
        }

        if (unit->op == OP_ST) {
            (void)mem_write_word(sb->mem, unit->imm, mem_float_to_word(unit->result_val));
        } else {
            publish_register(sb, unit, i + 1);
        }

        unit->cycle_write_result = sb->cycle;
        ScoreboardInstRecord *record = &sb->inst_records[(size_t)unit->issue_order];
        record->cycle_read_operands = unit->cycle_read_operands;
        record->cycle_execute_end = unit->cycle_execute_end;
        record->cycle_write_result = unit->cycle_write_result;
        fu_clear(unit);
    }
    sb->active_count = remaining;
}

ScoreboardStatus scoreboard_step(Scoreboard *sb)
{
    if (sb->status != SCOREBOARD_RUNNING) {
        return sb->status;
    }
    if (sb->unit_hook != NULL) {
        const FuncUnit *unit = fu_get(&sb->pool, sb->trace_unit);
        if (unit != NULL && unit->busy && !sb->unit_hook(sb->hook_context, sb->cycle, unit)) {
            sb->error = "unit trace hook failed";
            return sb->status = SCOREBOARD_ERROR;
        }
    }

    // WAR checks must see sources that were unread at the start of the cycle.
    uint64_t oldest_reader[NUM_REGS];
    record_unread(sb, oldest_reader);
    read_operands(sb);
    execute(sb);

    // Read Operands and Issue see the old registers and units. Issue consumes
    // only a previously fetched entry, and Fetch can refill the released slot.
    if (!issue(sb) || !fetch(sb)) {
        return sb->status = SCOREBOARD_ERROR;
    }
    write_result(sb, oldest_reader);

    if (sb->halt_issued && sb->active_count == 0) {
        return sb->status = SCOREBOARD_DONE;
    }
    if (sb->cycle == INT_MAX) {
        sb->error = "simulation exceeded the cycle counter range";
        return sb->status = SCOREBOARD_ERROR;
    }
    ++sb->cycle;
    return SCOREBOARD_RUNNING;
}
