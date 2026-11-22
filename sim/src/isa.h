// Instruction set definitions and 32-bit word decoding.
// Layout: [31:28]=0 [27:24]=OPCODE [23:20]=DST [19:16]=SRC0 [15:12]=SRC1 [11:0]=IMM

#ifndef ISA_H
#define ISA_H

#include <stdbool.h>

#include "common.h"

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

// Preserves the raw word for traceinst.txt alongside all unpacked fields.
typedef struct {
    uint32_t raw;
    Opcode opcode;
    uint8_t dst;
    uint8_t src0;
    uint8_t src1;
    uint16_t imm;
} Instruction;

Instruction isa_decode(uint32_t raw);

// Verify the top nibble is zero and the opcode is in range.
bool isa_is_valid(const Instruction *instruction);

const char *isa_opcode_name(Opcode opcode);

// Maps HALT and invalid opcodes to UNIT_NONE.
UnitType isa_unit_type_for_opcode(Opcode opcode);

#endif
