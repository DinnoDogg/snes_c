#ifndef SPC700_H
#define SPC700_H

#include <stdint.h>
#include <stdbool.h>

typedef struct SPC700 SPC700;

typedef uint8_t (*read_callback)(uint16_t address);
typedef void (*write_callback)(uint16_t address, uint8_t data);

typedef enum SPC700_flag {
    FLAG_C, FLAG_Z, FLAG_I, FLAG_H, FLAG_B, FLAG_P, FLAG_V, FLAG_N
} SPC700_flag;

struct SPC700 {
    struct {
        uint8_t a, x, y, sp, psw;
        uint16_t pc;
    } registers;

    read_callback read;
    write_callback write;

    uint16_t operand_address;
    uint16_t operand;

    uint8_t opcode;

    int cycle_count;

    bool dummy_read;
};

void init_spc700(SPC700* cpu, read_callback read, write_callback write);

int spc700_run_instruction(SPC700* cpu);

void spc700_print_state(SPC700* cpu);

#endif