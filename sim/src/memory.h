// Word-addressed 4096 x 32-bit main memory and IEEE-754 bit conversions.

#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

#include "common.h"

typedef struct {
    uint32_t data[MEM_SIZE];
} Memory;

void mem_init(Memory *memory);

// Load memin.txt (shorter files are zero-padded). Leaves memory untouched on failure.
bool mem_load(Memory *memory, const char *path);

bool mem_dump(const Memory *memory, const char *path);

bool mem_read_word(const Memory *memory, uint32_t address, uint32_t *word);

bool mem_write_word(Memory *memory, uint32_t address, uint32_t word);

// Reinterpret raw IEEE-754 bits without numeric conversion.
float mem_word_to_float(uint32_t word);

uint32_t mem_float_to_word(float value);

#endif
