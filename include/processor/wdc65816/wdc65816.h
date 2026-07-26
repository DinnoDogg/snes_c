#ifndef WDC65816_H
#define WDC65816_H

#include <stdint.h>
#include <stdbool.h>

typedef struct WDC65816 WDC65816;

typedef uint8_t (*read_callback)(uint32_t address);
typedef void (*write_callback)(uint32_t address, uint8_t data);

typedef void (*WDC65816_cycle_handler)(WDC65816* cpu);
typedef void (*WDC65816_op_callback)(WDC65816* cpu);

typedef enum WDC65816_flag {
    FLAG_C, FLAG_Z, FLAG_I, FLAG_D, FLAG_X, FLAG_M, FLAG_V, FLAG_N
} WDC65816_flag;

typedef enum WDC65816_bank {
    BANK_PROGRAM, BANK_DATA, BANK_ZERO
} WDC65816_bank;

typedef enum WDC65816_vector {
    VECTOR_IRQ = 0xFFEE, VECTOR_NMI = 0xFFEA, VECTOR_BRK = 0xFFE6, VECTOR_COP = 0xFFE4
} WDC65816_vector;

typedef union register_pair {
    struct {
        uint8_t low;
        uint8_t high;
    };

    uint16_t word;
} register_pair;

struct WDC65816 {
    struct {
        register_pair a, x, y, s, pc, d;
        uint8_t pbr, dbr, p;
    } registers;

    WDC65816_bank bank_mode;

    WDC65816_cycle_handler cycle_lookup[0x8];

    register_pair* index_register;

    WDC65816_op_callback op_callback;

    WDC65816_vector interrupt_vector;

    uint32_t operand_address : 24;
    uint32_t indirect_address : 24; 
    uint16_t operand;

    bool take_branch;
    int16_t branch_offset;

    int current_cycle;
    int cycle_lookup_index;

    int master_cycles_elapsed;

    read_callback read;
    write_callback write;

    bool dummy_read;

    bool nmi_line, last_nmi_line, nmi_latch;
    bool irq_line;

    bool irq_pending;
    bool wai;

    bool emulation_mode;
};

void init_wdc65816(WDC65816* cpu, read_callback read, write_callback write);
void cycle_wdc65816(WDC65816* cpu);

int wdc65816_run_instruction(WDC65816* cpu);

void wdc65816_print_state(WDC65816* cpu);

#endif