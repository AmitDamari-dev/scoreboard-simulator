#include "isa.h"

typedef struct {
    const char *name;
    UnitType unit_type;
} OpcodeInfo;

static const OpcodeInfo opcode_info[OP_COUNT] = {
    [OP_LD] = { "LD", UNIT_LD },
    [OP_ST] = { "ST", UNIT_ST },
    [OP_ADD] = { "ADD", UNIT_ADD },
    [OP_SUB] = { "SUB", UNIT_SUB },
    [OP_MULT] = { "MULT", UNIT_MUL },
    [OP_DIV] = { "DIV", UNIT_DIV },
    [OP_HALT] = { "HALT", UNIT_NONE }
};

static bool opcode_is_valid(Opcode opcode)
{
    return opcode >= OP_LD && opcode < OP_COUNT;
}

Instruction isa_decode(uint32_t raw)
{
    Instruction instruction;

    instruction.raw = raw;
    instruction.opcode = (Opcode)((raw >> 24) & 0x0fu);
    instruction.dst = (uint8_t)((raw >> 20) & 0x0fu);
    instruction.src0 = (uint8_t)((raw >> 16) & 0x0fu);
    instruction.src1 = (uint8_t)((raw >> 12) & 0x0fu);
    instruction.imm = (uint16_t)(raw & 0x0fffu);

    return instruction;
}

bool isa_is_valid(const Instruction *instruction)
{
    return (instruction->raw >> 28) == 0u && opcode_is_valid(instruction->opcode);
}

const char *isa_opcode_name(Opcode opcode)
{
    return opcode_is_valid(opcode) ? opcode_info[opcode].name : "INVALID";
}

UnitType isa_unit_type_for_opcode(Opcode opcode)
{
    return opcode_is_valid(opcode) ? opcode_info[opcode].unit_type : UNIT_NONE;
}
