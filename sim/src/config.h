/*
 * config.h
 *
 * Configuration (cfg.txt): number of units and delay per unit type
 * (add/sub/mul/div/ld/st) and the trace_unit name (e.g. "MUL2").
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

/* Configured unit counts and latencies indexed by UnitType, and the unit to trace. */
typedef struct {
    int nr_units[UNIT_TYPE_COUNT];
    int delay[UNIT_TYPE_COUNT];
    UnitRef trace_unit;
} Config;

/* Load all required settings. On failure, report the error and leave config unchanged. */
bool config_load(Config *config, const char *path);

/* Parse an uppercase name and decimal index such as MUL2. Return false for invalid names. */
bool config_parse_unit_name(const char *name, UnitRef *unit);

/* Write a unit name into buffer. Return false for an invalid unit or insufficient space. */
bool config_format_unit_name(UnitRef unit, char *buffer, size_t buffer_size);

#endif /* CONFIG_H */
