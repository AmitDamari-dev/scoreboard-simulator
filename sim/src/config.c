/*
 * config.c
 *
 * Parse required settings and validate the selected trace unit.
 */

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unit name prefix and the two configuration keys for its type. */
typedef struct {
    const char *name;
    const char *count_key;
    const char *delay_key;
} UnitInfo;

static const UnitInfo unit_info[UNIT_TYPE_COUNT] = {
    [UNIT_ADD] = { "ADD", "add_nr_units", "add_delay" },
    [UNIT_SUB] = { "SUB", "sub_nr_units", "sub_delay" },
    [UNIT_MUL] = { "MUL", "mul_nr_units", "mul_delay" },
    [UNIT_DIV] = { "DIV", "div_nr_units", "div_delay" },
    [UNIT_LD] = { "LD", "ld_nr_units", "ld_delay" },
    [UNIT_ST] = { "ST", "st_nr_units", "st_delay" }
};

/* Outcomes of reading one config line, including a final line without a newline. */
typedef enum {
    LINE_OK,
    LINE_EOF,
    LINE_ERROR
} LineStatus;

/* Grow the reusable buffer so spacing and comments have no fixed length limit. */
static LineStatus read_line(FILE *file, char **line, size_t *capacity, const char **error)
{
    size_t used = 0;

    for (;;) {
        int ch = fgetc(file);
        if (ch == EOF && ferror(file)) {
            *error = "cannot read configuration";
            return LINE_ERROR;
        }
        if (ch == EOF && used == 0) {
            return LINE_EOF;
        }
        if (ch == '\0') {
            *error = "unexpected NUL byte";
            return LINE_ERROR;
        }
        if (used == *capacity) {
            if (*capacity > SIZE_MAX / 2) {
                *error = "configuration line is too long";
                return LINE_ERROR;
            }
            size_t new_capacity = *capacity == 0 ? 128u : *capacity * 2;
            char *expanded = realloc(*line, new_capacity);
            if (expanded == NULL) {
                *error = "out of memory reading configuration";
                return LINE_ERROR;
            }
            *line = expanded;
            *capacity = new_capacity;
        }
        if (ch == '\n' || ch == EOF) {
            (*line)[used] = '\0';
            return LINE_OK;
        }
        (*line)[used++] = (char)ch;
    }
}

/* Strip leading and trailing whitespace from a mutable string. */
static char *trim(char *text)
{
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        --end;
    }
    *end = '\0';
    return text;
}

/* Accept one base-10 integer in [minimum, INT_MAX]. */
static bool parse_int(const char *text, int minimum, int *value)
{
    char *end;
    errno = 0;
    long number = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE || number < minimum || number > INT_MAX) {
        return false;
    }
    *value = (int)number;
    return true;
}

bool config_parse_unit_name(const char *name, UnitRef *unit)
{
    if (name == NULL || unit == NULL) {
        return false;
    }
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        const char *prefix = unit_info[type].name;
        size_t prefix_length = strlen(prefix);
        if (strncmp(name, prefix, prefix_length) == 0) {
            const char *suffix = name + prefix_length;
            int index;
            if (*suffix < '0' || *suffix > '9' || !parse_int(suffix, 0, &index)) {
                return false;
            }
            unit->type = (UnitType)type;
            unit->index = index;
            return true;
        }
    }
    return false;
}

bool config_format_unit_name(UnitRef unit, char *buffer, size_t buffer_size)
{
    if (unit.type < UNIT_ADD || unit.type >= UNIT_TYPE_COUNT || unit.index < 0 ||
        buffer == NULL || buffer_size == 0) {
        return false;
    }
    int written = snprintf(buffer, buffer_size, "%s%d", unit_info[unit.type].name, unit.index);
    return written >= 0 && (size_t)written < buffer_size;
}

/* Assign a numeric setting once. A negative field marks a missing value. */
static const char *set_number(int *field, const char *value, int minimum)
{
    if (*field >= 0) {
        return "duplicate parameter";
    }
    if (!parse_int(value, minimum, field)) {
        return minimum == 0 ? "expected a non-negative integer in range" :
                              "expected a positive integer in range";
    }
    return NULL;
}

/* Assign a known key and return an error message if its value is invalid. */
static const char *set_parameter(Config *config, const char *key, char *value)
{
    if (strcmp(key, "trace_unit") == 0) {
        if (config->trace_unit.type != UNIT_NONE) {
            return "duplicate parameter";
        }
        size_t length = strlen(value);
        if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
            value[length - 1] = '\0';
            ++value;
        }
        if (!config_parse_unit_name(value, &config->trace_unit)) {
            return "expected a unit name such as ADD0 or MUL2";
        }
        return NULL;
    }
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        if (strcmp(key, unit_info[type].count_key) == 0) {
            return set_number(&config->nr_units[type], value, 0);
        }
        if (strcmp(key, unit_info[type].delay_key) == 0) {
            return set_number(&config->delay[type], value, 1);
        }
    }
    return "unknown parameter";
}

/* Remove comments and parse a single assignment, if present. */
static bool parse_line(Config *config, char *line, const char *path, size_t line_number)
{
    char *comment = strchr(line, '#');
    if (comment != NULL) {
        *comment = '\0';
    }
    char *key = trim(line);
    if (*key == '\0') {
        return true;
    }
    char *equals = strchr(key, '=');
    if (equals == NULL || equals == key) {
        fprintf(stderr, "%s:%zu: expected parameter = value\n", path, line_number);
        return false;
    }
    *equals = '\0';
    key = trim(key);
    const char *error = set_parameter(config, key, trim(equals + 1));
    if (error != NULL) {
        fprintf(stderr, "%s:%zu: %s: %s\n", path, line_number, key, error);
        return false;
    }
    return true;
}

/* Require every key and check the selected index after all counts are known. */
static bool validate_config(const Config *config, const char *path)
{
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        if (config->nr_units[type] < 0) {
            fprintf(stderr, "%s: missing required parameter '%s'\n", path, unit_info[type].count_key);
            return false;
        }
        if (config->delay[type] < 1) {
            fprintf(stderr, "%s: missing required parameter '%s'\n", path, unit_info[type].delay_key);
            return false;
        }
    }
    UnitRef trace = config->trace_unit;
    if (trace.type == UNIT_NONE) {
        fprintf(stderr, "%s: missing required parameter 'trace_unit'\n", path);
        return false;
    }
    if (trace.index >= config->nr_units[trace.type]) {
        fprintf(stderr, "%s: trace_unit %s%d is outside the configured unit count (%d)\n",
                path, unit_info[trace.type].name, trace.index, config->nr_units[trace.type]);
        return false;
    }
    return true;
}

bool config_load(Config *config, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "%s: cannot open configuration\n", path);
        return false;
    }

    Config parsed;
    for (int type = UNIT_ADD; type < UNIT_TYPE_COUNT; ++type) {
        parsed.nr_units[type] = -1;
        parsed.delay[type] = -1;
    }
    parsed.trace_unit = UNIT_REF_NONE;

    char *line = NULL;
    size_t capacity = 0;
    bool ok = true;
    for (size_t line_number = 1; ; ++line_number) {
        const char *error = NULL;
        LineStatus status = read_line(file, &line, &capacity, &error);
        if (status == LINE_EOF) {
            break;
        }
        if (status == LINE_ERROR) {
            fprintf(stderr, "%s:%zu: %s\n", path, line_number, error);
            ok = false;
            break;
        }
        if (!parse_line(&parsed, line, path, line_number)) {
            ok = false;
            break;
        }
    }
    free(line);
    if (fclose(file) != 0 && ok) {
        fprintf(stderr, "%s: cannot close configuration input\n", path);
        ok = false;
    }
    if (!ok || !validate_config(&parsed, path)) {
        return false;
    }
    *config = parsed;
    return true;
}
