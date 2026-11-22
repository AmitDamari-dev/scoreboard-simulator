#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "func_unit.h"
#include "inst_queue.h"
#include "memory.h"
#include "registers.h"

// A terminal status is returned on subsequent calls to scoreboard_step as well.
typedef enum {
    SCOREBOARD_RUNNING,
    SCOREBOARD_DONE,
    SCOREBOARD_ERROR
} ScoreboardStatus;

// The record's index is its issue order. The last three cycles are -1 until writeback.
typedef struct {
    uint32_t raw;
    int pc;
    UnitRef unit;
    int cycle_issued;
    int cycle_read_operands;
    int cycle_execute_end;
    int cycle_write_result;
} ScoreboardInstRecord;

// Observe the configured unit when busy at cycle start. The unit is borrowed for
// the call. Return false to stop the simulation before this cycle's stages run.
typedef bool (*ScoreboardUnitHook)(void *context, int cycle, const FuncUnit *unit);

// The scoreboard borrows the memory image and owns the pool and instruction records.
typedef struct {
    Memory *mem;
    Registers regs;
    InstQueue iq;
    FuncUnitPool pool;

    // Issue order also gives priority to older accesses to the same address.
    FuncUnit *active[MEM_SIZE];
    size_t active_count;
    ScoreboardInstRecord *inst_records;
    size_t inst_count;

    int pc;
    int cycle;
    bool halted_fetch;
    bool halt_issued;
    ScoreboardStatus status;
    const char *error;

    UnitRef trace_unit;
    ScoreboardUnitHook unit_hook;
    void *hook_context;
} Scoreboard;

// Start at cycle 0 using the caller's loaded image. The image must outlive the
// run. Use a fresh or freed scoreboard. On failure, error holds a diagnostic.
bool scoreboard_init(Scoreboard *sb, const Config *cfg, Memory *mem);

// Release owned storage, including after failed initialization.
void scoreboard_free(Scoreboard *sb);

// Set the observer for the busy unit named by cfg.trace_unit, or NULL to disable it.
void scoreboard_set_unit_hook(Scoreboard *sb, ScoreboardUnitHook hook, void *context);

// Run the current cycle and advance if the program is still running. Done means
// HALT has issued and all outstanding writes have finished.
ScoreboardStatus scoreboard_step(Scoreboard *sb);

#endif
