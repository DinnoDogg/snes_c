#ifndef WDC65816_H
#define WDC65816_H

#include <stdint.h>
#include <stdbool.h>

typedef struct WDC65816 WDC65816;

typedef uint8_t (*read_callback)(uint32_t address);
typedef void (*write_callback)(uint32_t address, uint8_t data);

typedef void (*WDC65816_cycle_handler)(WDC65816* cpu);

typedef void (*WDC65816_address_scheduler)(WDC65816* cpu);
typedef void (*WDC65816_op_scheduler)(WDC65816* cpu);

typedef uint16_t (*WDC65816_algorithm_callback)(WDC65816* cpu, uint16_t source);

typedef struct WDC65816_op {
    WDC65816_address_scheduler schedule_address;
    WDC65816_op_scheduler schedule_op;
    
    bool dummy_read;
} WDC65816_op;

typedef enum WDC65816_data_size {
    SIZE_WORD, SIZE_BYTE 
} WDC65816_data_size;

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

    WDC65816_algorithm_callback op_algorithm;
    WDC65816_data_size data_size;

    WDC65816_vector interrupt_vector;
    WDC65816_flag operation_flag;

    uint32_t operand_address : 24;
    uint32_t indirect_address : 24; 
    uint16_t operand;

    bool take_branch;
    int16_t branch_offset;
    
    uint8_t current_cycle;
    uint8_t cycle_lookup_index;

    read_callback read;
    write_callback write;

    bool dummy_read;

    bool nmi_line, last_nmi_line, nmi_latch;
    bool irq_line;

    bool irq_pending;
};

void init_wdc65816(WDC65816* cpu, read_callback read, write_callback write);
void cycle_wdc65816(WDC65816* cpu);

int wdc65816_run_instruction(WDC65816* cpu);

void wdc65816_print_state(WDC65816* cpu);

#endif