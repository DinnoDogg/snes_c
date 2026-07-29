#ifndef SPC700_H
#define SPC700_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*read_callback)(void* bus, uint16_t address);
typedef void (*write_callback)(void* bus, uint16_t address, uint8_t data);

typedef enum SPC700_flag {
    FLAG_C, FLAG_Z, FLAG_I, FLAG_H, FLAG_B, FLAG_P, FLAG_V, FLAG_N
} SPC700_flag;

typedef struct SPC700_bus {
    read_callback read;
    write_callback write;
} SPC700_bus;

typedef struct SPC700 {
    struct {
        uint8_t a, x, y, sp, psw;
        uint16_t pc;
    } registers;

    SPC700_bus* bus;

    uint16_t operand_address;
    uint16_t operand;

    uint8_t opcode;

    int cycle_count;

    bool dummy_read;
    bool branch_taken;
} SPC700;

void init_spc700(SPC700* cpu, void* bus);

int spc700_run_immediate(SPC700* cpu);
uint8_t spc700_run_immediate_get_next_op(SPC700* cpu);

uint8_t spc700_read_immediate(SPC700* cpu);

void spc700_run_instruction(SPC700* cpu, uint8_t opcode);

int spc700_get_op_time(uint8_t opcode);
void spc700_print_state(SPC700* cpu);

#endif