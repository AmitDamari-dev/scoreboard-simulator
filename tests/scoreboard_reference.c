// Build from the repository root in an x64 Native Tools Command Prompt.
// cl /std:c11 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /I sim\src ^
//    tests\scoreboard_reference.c sim\src\config.c sim\src\func_unit.c sim\src\isa.c ^
//    sim\src\inst_queue.c sim\src\memory.c sim\src\registers.c sim\src\scoreboard.c
// Run scoreboard_reference.exe with the example directory as its sole argument.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "func_unit.h"
#include "memory.h"
#include "scoreboard.h"

#define PATH_CAPACITY 1024
#define LINE_CAPACITY 1024

typedef struct {
    FILE *expected;
    int failed;
    size_t rows;
} UnitTraceCheck;

static int make_path(char *path, size_t capacity, const char *folder, const char *name)
{
    size_t length = strlen(folder);
    const char *separator = (length != 0U && folder[length - 1U] != '\\' &&
                             folder[length - 1U] != '/') ? "\\" : "";
    int written;

    written = snprintf(path, capacity, "%s%s%s", folder, separator, name);
    return written >= 0 && (size_t)written < capacity;
}

// Preserve spaces because the trace rows are compared verbatim.
static int read_row(FILE *file, char *line, size_t capacity)
{
    size_t length;

    if (fgets(line, (int)capacity, file) == NULL) {
        if (ferror(file)) {
            fprintf(stderr, "could not read a reference row\n");
            return -1;
        }
        return 0;
    }
    length = strlen(line);
    if (length != 0U && line[length - 1U] == '\n') {
        line[--length] = '\0';
    }
    if (length != 0U && line[length - 1U] == '\r') {
        line[--length] = '\0';
    }
    if (length == capacity - 1U) {
        fprintf(stderr, "reference line exceeds %u bytes\n", (unsigned int)(capacity - 1U));
        return -1;
    }
    return 1;
}

static int format_unit(UnitRef ref, char *name, size_t capacity)
{
    int written;

    if (ref.type == UNIT_NONE) {
        written = snprintf(name, capacity, "-");
        return written >= 0 && (size_t)written < capacity;
    }
    return config_format_unit_name(ref, name, capacity) ? 1 : 0;
}

static int format_register(int reg, char *name, size_t capacity)
{
    int written;

    if (reg < 0) {
        written = snprintf(name, capacity, "-");
        return written >= 0 && (size_t)written < capacity;
    }
    written = snprintf(name, capacity, "F%d", reg);
    return written >= 0 && (size_t)written < capacity;
}

static bool check_unit_trace(void *context, int cycle, const FuncUnit *unit)
{
    UnitTraceCheck *check = (UnitTraceCheck *)context;
    char expected[LINE_CAPACITY];
    char actual[LINE_CAPACITY];
    char unit_name[32];
    char fi[16];
    char fj[16];
    char fk[16];
    char qj[32];
    char qk[32];
    int result;
    int written;

    if (check == NULL || unit == NULL) {
        return false;
    }
    result = read_row(check->expected, expected, sizeof(expected));
    if (result != 1) {
        fprintf(stderr, "traceunit: missing expected row %u\n", (unsigned int)(check->rows + 1U));
        check->failed = 1;
        return false;
    }
    if (!format_unit(unit->ref, unit_name, sizeof(unit_name)) ||
        !format_register(unit->fi, fi, sizeof(fi)) ||
        !format_register(unit->fj, fj, sizeof(fj)) ||
        !format_register(unit->fk, fk, sizeof(fk)) ||
        !format_unit(unit->qj, qj, sizeof(qj)) ||
        !format_unit(unit->qk, qk, sizeof(qk))) {
        fprintf(stderr, "traceunit: could not format row %u\n", (unsigned int)(check->rows + 1U));
        check->failed = 1;
        return false;
    }
    written = snprintf(actual, sizeof(actual), "%d %s %s %s %s %s %s %s %s", cycle,
                       unit_name, fi, fj, fk, qj, qk, unit->rj ? "Yes" : "No",
                       unit->rk ? "Yes" : "No");
    if (written < 0 || (size_t)written >= sizeof(actual)) {
        fprintf(stderr, "traceunit: could not format row %u\n", (unsigned int)(check->rows + 1U));
        check->failed = 1;
        return false;
    }
    ++check->rows;
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "traceunit row %u mismatch\n  expected: %s\n  actual:   %s\n",
                (unsigned int)check->rows, expected, actual);
        check->failed = 1;
        return false;
    }
    return true;
}

static int check_no_extra_rows(FILE *file, const char *label)
{
    char line[LINE_CAPACITY];
    int result = read_row(file, line, sizeof(line));

    if (result == 0) {
        return 1;
    }
    if (result > 0) {
        fprintf(stderr, "%s: extra expected row: %s\n", label, line);
    }
    return 0;
}

static int check_memory(const Memory *actual, const char *path)
{
    Memory expected;
    if (!mem_load(&expected, path)) {
        fprintf(stderr, "could not load expected memory: %s\n", path);
        return 0;
    }
    for (size_t index = 0; index < MEM_SIZE; ++index) {
        if (actual->data[index] != expected.data[index]) {
            fprintf(stderr, "memory word %u mismatch: expected %08x, actual %08x\n",
                    (unsigned int)index, (unsigned int)expected.data[index],
                    (unsigned int)actual->data[index]);
            return 0;
        }
    }
    return 1;
}

static int check_registers(const Registers *registers, const char *path)
{
    FILE *expected = fopen(path, "rb");
    if (expected == NULL) {
        fprintf(stderr, "could not open expected registers: %s\n", path);
        return 0;
    }

    char reference[LINE_CAPACITY];
    char actual[64];
    for (int index = 0; index < NUM_REGS; ++index) {
        if (read_row(expected, reference, sizeof(reference)) != 1) {
            fprintf(stderr, "regout: missing or invalid row for F%d\n", index);
            fclose(expected);
            return 0;
        }
        int written = snprintf(actual, sizeof(actual), "%f", registers->val[index]);
        if (written < 0 || (size_t)written >= sizeof(actual)) {
            fprintf(stderr, "regout: could not format F%d\n", index);
            fclose(expected);
            return 0;
        }
        // Six decimal places could hide a change to the example's exact register values.
        char *end;
        float value = strtof(reference, &end);
        if (end == reference || *end != '\0' ||
            mem_float_to_word(value) != mem_float_to_word(registers->val[index]) ||
            strcmp(reference, actual) != 0) {
            fprintf(stderr, "register F%d mismatch: expected %s, actual %.9g\n", index,
                    reference, (double)registers->val[index]);
            fclose(expected);
            return 0;
        }
    }
    if (!check_no_extra_rows(expected, "regout")) {
        fclose(expected);
        return 0;
    }
    fclose(expected);
    return 1;
}

static int check_instruction_trace(const Scoreboard *sb, const char *path)
{
    FILE *expected = fopen(path, "rb");
    if (expected == NULL) {
        fprintf(stderr, "could not open expected instruction trace: %s\n", path);
        return 0;
    }

    char reference[LINE_CAPACITY];
    char actual[LINE_CAPACITY];
    char unit_name[32];
    for (size_t index = 0; index < sb->inst_count; ++index) {
        const ScoreboardInstRecord *record = &sb->inst_records[index];
        if (read_row(expected, reference, sizeof(reference)) != 1) {
            fprintf(stderr, "traceinst: missing or invalid row %u\n", (unsigned int)(index + 1u));
            fclose(expected);
            return 0;
        }
        if (!format_unit(record->unit, unit_name, sizeof(unit_name))) {
            fprintf(stderr, "traceinst: could not format row %u\n", (unsigned int)(index + 1u));
            fclose(expected);
            return 0;
        }
        int written = snprintf(actual, sizeof(actual), "%08x %d %s %d %d %d %d",
                               (unsigned int)record->raw, record->pc, unit_name,
                               record->cycle_issued, record->cycle_read_operands,
                               record->cycle_execute_end, record->cycle_write_result);
        if (written < 0 || (size_t)written >= sizeof(actual)) {
            fprintf(stderr, "traceinst: could not format row %u\n", (unsigned int)(index + 1u));
            fclose(expected);
            return 0;
        }
        if (strcmp(reference, actual) != 0) {
            fprintf(stderr, "traceinst row %u mismatch\n  expected: %s\n  actual:   %s\n",
                    (unsigned int)(index + 1u), reference, actual);
            fclose(expected);
            return 0;
        }
    }
    if (!check_no_extra_rows(expected, "traceinst")) {
        fclose(expected);
        return 0;
    }
    fclose(expected);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <reference-folder>\n", argv[0]);
        return 2;
    }

    const char *folder = argv[1];
    char cfg_path[PATH_CAPACITY];
    char memin_path[PATH_CAPACITY];
    char memout_path[PATH_CAPACITY];
    char regout_path[PATH_CAPACITY];
    char traceinst_path[PATH_CAPACITY];
    char traceunit_path[PATH_CAPACITY];
    if (!make_path(cfg_path, sizeof(cfg_path), folder, "cfg.txt") ||
        !make_path(memin_path, sizeof(memin_path), folder, "memin.txt") ||
        !make_path(memout_path, sizeof(memout_path), folder, "memout.txt") ||
        !make_path(regout_path, sizeof(regout_path), folder, "regout.txt") ||
        !make_path(traceinst_path, sizeof(traceinst_path), folder, "traceinst.txt") ||
        !make_path(traceunit_path, sizeof(traceunit_path), folder, "traceunit.txt")) {
        fprintf(stderr, "reference path is too long\n");
        return 2;
    }

    Config config;
    Memory memory;
    if (!config_load(&config, cfg_path) || !mem_load(&memory, memin_path)) {
        return 1;
    }

    UnitTraceCheck unit_check = { .expected = fopen(traceunit_path, "rb") };
    if (unit_check.expected == NULL) {
        fprintf(stderr, "could not open expected unit trace: %s\n", traceunit_path);
        return 1;
    }

    Scoreboard scoreboard;
    if (!scoreboard_init(&scoreboard, &config, &memory)) {
        fprintf(stderr, "scoreboard initialization failed%s%s\n", scoreboard.error != NULL ? ": " : "",
                scoreboard.error != NULL ? scoreboard.error : "");
        scoreboard_free(&scoreboard);
        fclose(unit_check.expected);
        return 1;
    }

    scoreboard_set_unit_hook(&scoreboard, check_unit_trace, &unit_check);
    ScoreboardStatus status;
    do {
        status = scoreboard_step(&scoreboard);
    } while (status == SCOREBOARD_RUNNING && !unit_check.failed);

    int ok = 1;
    if (status != SCOREBOARD_DONE) {
        fprintf(stderr, "scoreboard failed%s%s\n", scoreboard.error != NULL ? ": " : "",
                scoreboard.error != NULL ? scoreboard.error : "");
        ok = 0;
    }
    if (unit_check.failed || !check_no_extra_rows(unit_check.expected, "traceunit")) {
        ok = 0;
    }
    if (ok && !check_memory(&memory, memout_path)) {
        ok = 0;
    }
    if (ok && !check_registers(&scoreboard.regs, regout_path)) {
        ok = 0;
    }
    if (ok && !check_instruction_trace(&scoreboard, traceinst_path)) {
        ok = 0;
    }
    scoreboard_free(&scoreboard);
    fclose(unit_check.expected);
    if (ok) {
        printf("reference check passed\n");
        return 0;
    }
    return 1;
}
