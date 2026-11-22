/*
 * memory.c
 *
 * Load and dump memory images, access words, and convert between word and float bits.
 */

#include "memory.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(float) == sizeof(uint32_t), "Simulator floats must occupy one memory word");

void mem_init(Memory *memory)
{
    memset(memory->data, 0, sizeof(memory->data));
}

/* Convert one hex digit to its value, or return -1 for any other byte. */
static int hex_digit(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

/* Read exactly eight hex digits and consume LF, CRLF, or final EOF. */
static bool read_hex_word(FILE *file, int ch, uint32_t *word)
{
    uint32_t value = 0;
    for (int i = 0; i < 8; ++i) {
        int digit = hex_digit(ch);
        if (digit < 0) {
            return false;
        }
        value = (value << 4) | (uint32_t)digit;
        ch = fgetc(file);
    }

    if (ch == '\r') {
        if (fgetc(file) != '\n') {
            return false;
        }
    } else if (ch != '\n' && ch != EOF) {
        return false;
    }
    *word = value;
    return true;
}

bool mem_load(Memory *memory, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "%s: cannot open memory input\n", path);
        return false;
    }

    Memory loaded;
    mem_init(&loaded);
    unsigned int address = 0;
    bool ok = true;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (address == MEM_SIZE) {
            fprintf(stderr, "%s: memory image exceeds %d words\n", path, MEM_SIZE);
            ok = false;
            break;
        }
        if (!read_hex_word(file, ch, &loaded.data[address])) {
            if (!ferror(file)) {
                fprintf(stderr, "%s:%u: expected a line of eight hex digits\n", path, address + 1u);
            }
            ok = false;
            break;
        }
        ++address;
    }
    if (ferror(file)) {
        fprintf(stderr, "%s: cannot read memory input\n", path);
        ok = false;
    }
    if (fclose(file) != 0 && ok) {
        fprintf(stderr, "%s: cannot close memory input\n", path);
        ok = false;
    }
    if (ok) {
        *memory = loaded;
    }
    return ok;
}

bool mem_dump(const Memory *memory, const char *path)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "%s: cannot open memory output\n", path);
        return false;
    }

    bool ok = true;
    for (int address = 0; address < MEM_SIZE; ++address) {
        if (fprintf(file, "%08" PRIX32 "\n", memory->data[address]) < 0) {
            ok = false;
            break;
        }
    }
    if (fclose(file) != 0) {
        ok = false;
    }
    if (!ok) {
        fprintf(stderr, "%s: cannot write memory image\n", path);
    }
    return ok;
}

bool mem_read_word(const Memory *memory, uint32_t address, uint32_t *word)
{
    if (address >= MEM_SIZE || word == NULL) {
        return false;
    }
    *word = memory->data[address];
    return true;
}

bool mem_write_word(Memory *memory, uint32_t address, uint32_t word)
{
    if (address >= MEM_SIZE) {
        return false;
    }
    memory->data[address] = word;
    return true;
}

float mem_word_to_float(uint32_t word)
{
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}

uint32_t mem_float_to_word(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}
