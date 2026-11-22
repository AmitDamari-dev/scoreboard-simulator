// Simulator configuration loaded from cfg.txt.

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

typedef struct {
    int nr_units[UNIT_TYPE_COUNT];
    int delay[UNIT_TYPE_COUNT];
    UnitRef trace_unit;
} Config;

// Load all 13 required keys. Leaves config untouched on parse or validation error.
bool config_load(Config *config, const char *path);

// Parse a unit name like "MUL2" into type + zero-based index.
bool config_parse_unit_name(const char *name, UnitRef *unit);

bool config_format_unit_name(UnitRef unit, char *buffer, size_t buffer_size);

#endif
