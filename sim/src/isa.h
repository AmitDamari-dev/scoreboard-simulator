/*
 * isa.h
 *
 * Instruction set and decoding.
 * Format: [31:28]=0 [27:24]=OPCODE [23:20]=DST [19:16]=SRC0 [15:12]=SRC1 [11:0]=IMM
 * Opcodes: LD=0 ST=1 ADD=2 SUB=3 MULT=4 DIV=5 HALT=6.
 */

#ifndef ISA_H
#define ISA_H

#include <stdbool.h>

#include "common.h"

/* Opcode numbers as encoded in bits 27:24. */
typedef enum {
    OP_LD = 0,
    OP_ST = 1,
    OP_ADD = 2,
    OP_SUB = 3,
    OP_MULT = 4,
    OP_DIV = 5,
    OP_HALT = 6,
    OP_COUNT
} Opcode;

/* Original instruction word and its decoded fields, including unused fields. */
typedef struct {
    uint32_t raw;
    Opcode opcode;
    uint8_t dst;
    uint8_t src0;
    uint8_t src1;
    uint16_t imm;
} Instruction;

/* Decode all fields, even if the word has a reserved opcode or top nibble. */
Instruction isa_decode(uint32_t raw);

/* Check that a decoded instruction has a zero top nibble and a defined opcode. */
bool isa_is_valid(const Instruction *instruction);

/* Return the opcode mnemonic, or "INVALID" for a reserved opcode. */
const char *isa_opcode_name(Opcode opcode);

/* Return the execution unit type, or UNIT_NONE for HALT or a reserved opcode. */
UnitType isa_unit_type_for_opcode(Opcode opcode);

#endif /* ISA_H */
