/*
 * memory.h
 *
 * Main memory: 4096 words x 32 bits (word addressed).
 * Loads memin.txt (may be shorter than 4096 lines; the rest are zero)
 * and dumps memout.txt (8 hex digits per line).
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

#include "common.h"

/* Unified instruction and data memory, indexed by word address. */
typedef struct {
    uint32_t data[MEM_SIZE];
} Memory;

/* Set every word to zero. */
void mem_init(Memory *memory);

/* Load an image with omitted trailing words set to zero. Leave memory intact on failure. */
bool mem_load(Memory *memory, const char *path);

/* Write all MEM_SIZE words as eight uppercase hex digits per line. */
bool mem_dump(const Memory *memory, const char *path);

/* Read a word. Return false for an invalid address or missing output pointer. */
bool mem_read_word(const Memory *memory, uint32_t address, uint32_t *word);

/* Write a word. Return false and leave memory intact for an invalid address. */
bool mem_write_word(Memory *memory, uint32_t address, uint32_t word);

/* Interpret a memory word as the bit representation of a float. */
float mem_word_to_float(uint32_t word);

/* Return the bit representation of a float for storage in a memory word. */
uint32_t mem_float_to_word(float value);

#endif /* MEMORY_H */
