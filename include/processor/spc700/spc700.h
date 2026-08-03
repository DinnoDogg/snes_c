#ifndef SPC700_H
#define SPC700_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*SPC700_read_callback)(void* cpu, uint16_t address);
typedef void (*SPC700_write_callback)(void* cpu, uint16_t address, uint8_t data);

typedef enum SPC700_flag {
    SPC_FLAG_C, SPC_FLAG_Z, SPC_FLAG_I, SPC_FLAG_H, SPC_FLAG_B, SPC_FLAG_P, SPC_FLAG_V, SPC_FLAG_N
} SPC700_flag;

typedef struct SPC700 {
    struct {
        uint8_t a, x, y, sp, psw;
        uint16_t pc;
    } registers;

    SPC700_read_callback read;
    SPC700_write_callback write;

    uint16_t operand_address;
    uint16_t operand;

    uint8_t opcode;

    int cycle_count;

    bool dummy_read;
    bool branch_taken;
} SPC700;

void init_spc700(SPC700* cpu, SPC700_read_callback read, SPC700_write_callback write);

uint8_t spc700_read_immediate(SPC700* cpu);

void spc700_run_instruction(SPC700* cpu, uint8_t opcode);

int spc700_get_op_time(uint8_t opcode);
void spc700_print_state(SPC700* cpu);

#endif