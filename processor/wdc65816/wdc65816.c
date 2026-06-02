#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../../include/processor/wdc65816/wdc65816.h"
#include "../../include/processor/wdc65816/instruction.h"
#include "../../include/processor/wdc65816/addressing.h"

static uint8_t read(WDC65816* cpu, uint32_t address);
static void write(WDC65816* cpu, uint32_t address, uint8_t data);

static void dummy_read(WDC65816* cpu, uint32_t address);
static uint8_t read_immediate(WDC65816* cpu);

static void increment_address(WDC65816* cpu);

static void fetch_operand(WDC65816* cpu);

static void fetch_operand_byte(WDC65816* cpu);

static void fetch_operand_word_low(WDC65816* cpu);
static void fetch_operand_word_high(WDC65816* cpu);

static bool get_flag(WDC65816* cpu, WDC65816_flag flag);
static void set_flag(WDC65816* cpu, WDC65816_flag flag, bool value);

static bool page_crossed(uint32_t address_a, uint32_t address_b);

static void set_nz(WDC65816* cpu, uint16_t value);

static void end_op(WDC65816* cpu, bool poll_irq);

static void handle_alu_8(WDC65816* cpu);
static void handle_alu_16(WDC65816* cpu);

static uint16_t algorithm_and(WDC65816* cpu, uint16_t source);

static const WDC65816_op op_lookup[0x100] = {
    [0x2D] = {schedule_addr_a, schedule_and, false},
    [0x3D] = {schedule_addr_a_x, schedule_and, false},
    [0x39] = {schedule_addr_a_y, schedule_and, false},
    [0x3F] = {schedule_addr_al_x, schedule_and, false},
    [0x2F] = {schedule_addr_al, schedule_and, false},
};

void init_wdc65816(WDC65816* cpu, read_callback read, write_callback write) {
    cpu->read = read;
    cpu->write = write;
}

void cycle_wdc65816(WDC65816* cpu) {
    if (cpu->current_cycle++ == 0) {
        uint8_t opcode = 0x00;

        if (cpu->irq_pending || cpu->nmi_latch) {
            //interrupt
        }

        else {
            opcode = read_immediate(cpu);
            //printf("Suck %u\n", opcode);
        }
        
        const WDC65816_op* op = &op_lookup[opcode];

        cpu->bank_mode = BANK_DATA;

        op->schedule_address(cpu);
        op->schedule_op(cpu);
    }

    else {
        //printf("Zog\n");
        cpu->cycle_lookup[cpu->current_cycle - 2](cpu);
    }
}

void wdc65816_print_state(WDC65816* cpu) {
    printf("A.W: %u\n", cpu->registers.a.word);
    printf("A.H: %u\n", cpu->registers.a.high);
    printf("A.L: %u\n\n", cpu->registers.a.low);

    printf("X.W: %u\n", cpu->registers.x.word);
    printf("X.H: %u\n", cpu->registers.x.high);
    printf("X.L: %u\n\n", cpu->registers.x.low);

    printf("Y.W: %u\n", cpu->registers.y.word);
    printf("Y.H: %u\n", cpu->registers.y.high);
    printf("Y.L: %u\n\n", cpu->registers.y.low);

    printf("S.W: %u\n", cpu->registers.s.word);
    printf("S.H: %u\n", cpu->registers.s.high);
    printf("S.L: %u\n\n", cpu->registers.s.low);

    printf("D.W: %u\n", cpu->registers.d.word);
    printf("D.H: %u\n", cpu->registers.d.high);
    printf("D.L: %u\n\n", cpu->registers.d.low);

    printf("PC.W: %u\n", cpu->registers.pc.word);
    printf("PC.H: %u\n", cpu->registers.pc.high);
    printf("PC.L: %u\n\n", cpu->registers.pc.low);

    printf("PBR: %u\n", cpu->registers.pbr);
    printf("DBR: %u\n\n", cpu->registers.dbr);

    printf("P: %u\n\n", cpu->registers.p);

    printf("N: %u\n", get_flag(cpu, FLAG_N));
    printf("V: %u\n", get_flag(cpu, FLAG_V));
    printf("M: %u\n", get_flag(cpu, FLAG_M));
    printf("X: %u\n", get_flag(cpu, FLAG_X));
    printf("D: %u\n", get_flag(cpu, FLAG_D));
    printf("I: %u\n", get_flag(cpu, FLAG_I));
    printf("Z: %u\n", get_flag(cpu, FLAG_Z));
    printf("C: %u\n\n", get_flag(cpu, FLAG_C));

    printf("Operand address: %u\n\n", cpu->operand_address);
}

int wdc65816_run_instruction(WDC65816* cpu) {
    int cycle_count = 0;

    do {
        cycle_wdc65816(cpu);
        cycle_count++;
    } while (cpu->current_cycle != 0);

    return cycle_count;
}

uint8_t read(WDC65816* cpu, uint32_t address) {
    //set open bus
    return cpu->read(address);
}

void write(WDC65816* cpu, uint32_t address, uint8_t data) {
    cpu->write(address, data);
}

void dummy_read(WDC65816* cpu, uint32_t address) {
    cpu->read(address);
}

uint8_t read_immediate(WDC65816* cpu) {
    return read(cpu, (cpu->registers.pbr << 16) | cpu->registers.pc.word++);
}

void increment_address(WDC65816* cpu) {
    cpu->operand_address++;

    switch (cpu->bank_mode) {
        case BANK_DATA : break;
        case BANK_ZERO : cpu->operand_address &= 0xFFFF; break;
        case BANK_PROGRAM : cpu->operand_address &= 0xFFFF; cpu->operand_address |= (cpu->registers.pbr << 16); break;
    }
}

void fetch_operand(WDC65816* cpu) {
    cpu->fetch_operand(cpu);
}

void fetch_operand_byte(WDC65816* cpu) {
    cpu->operand = read(cpu, cpu->operand_address);
}

void fetch_operand_word_low(WDC65816* cpu) {
    fetch_operand_byte(cpu);
    increment_address(cpu);
    cpu->fetch_operand = &fetch_operand_word_high;
}

void fetch_operand_word_high(WDC65816* cpu) {
    uint8_t data = read(cpu, cpu->operand_address);
    cpu->operand |= (data << 8);
}

bool get_flag(WDC65816* cpu, WDC65816_flag flag) {
    return (cpu->registers.p >> flag) & 0x1;
}

void set_flag(WDC65816* cpu, WDC65816_flag flag, bool value) {
    if (value) {
        cpu->registers.p |= 1 << flag;

        if (flag == FLAG_X) {
            cpu->registers.x.high = 0x00;
            cpu->registers.y.high = 0x00;
        }

        return;
    }

    cpu->registers.p &= ~(1 << flag);
}

bool page_crossed(uint32_t address_a, uint32_t address_b) {
    return (address_a & 0xFF00) != (address_b & 0xFF00);
}

void set_nz(WDC65816* cpu, uint16_t value) {
    bool n = (value >> cpu->highest_bit) & 0x1;
    set_flag(cpu, FLAG_N, n);
    set_flag(cpu, FLAG_Z, value == 0);
}

void end_op(WDC65816* cpu, bool poll_irq) {
    cpu->current_cycle = 0x00;
    cpu->cycle_lookup_index = 0x00;

    if (poll_irq) {
        cpu->irq_pending = cpu->irq_line;
    }
}

uint16_t algorithm_and(WDC65816* cpu, uint16_t source) {
    uint16_t result = source & cpu->operand;
    set_nz(cpu, result);
    return result;
}


void schedule_addr_a(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_2;
}

void addr_a_1(WDC65816* cpu) {
    cpu->operand_address = (cpu->registers.dbr << 16) | read_immediate(cpu);
}

void addr_a_2(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
}

void schedule_addr_a_x(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.x;
    schedule_addr_a_ind(cpu);
}

void schedule_addr_a_y(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.y;
    schedule_addr_a_ind(cpu);
}

void schedule_addr_a_ind(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_ind_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_ind_2;

}

void addr_a_ind_1(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
    
    if (!page_crossed(cpu->operand_address, cpu->operand_address + cpu->index_register->word) && get_flag(cpu, FLAG_X) && !cpu->dummy_read) {
        cpu->current_cycle++;
    }
    
    cpu->operand_address += cpu->index_register->low;
}

void addr_a_ind_2(WDC65816* cpu) {
    dummy_read(cpu, cpu->operand_address);
    cpu->operand_address += cpu->index_register->high << 8;
}

void schedule_addr_al_x(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_al_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_al_x_1;

}

void addr_al_x_1(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 16;
    cpu->operand_address += cpu->registers.x.word;
}

void schedule_addr_al(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_al_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_al_2;
}

void addr_al_1(WDC65816* cpu) {
    cpu->operand_address = read_immediate(cpu);
}

void addr_al_2(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 16;
}


void schedule_alu(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_M)) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &handle_alu_8;
        cpu->fetch_operand = &fetch_operand_byte;
        cpu->highest_bit = HIGHEST_BYTE_BIT;
        return;
    }  

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &handle_alu_16;
    cpu->fetch_operand = &fetch_operand_word_low;
    cpu->highest_bit = HIGHEST_WORD_BIT;
}

void alu_1(WDC65816* cpu) {
    fetch_operand(cpu);
}

void handle_alu_8(WDC65816* cpu) {
    fetch_operand(cpu);
    cpu->registers.a.low = cpu->op_algorithm(cpu, cpu->registers.a.low);
    end_op(cpu, true);
}

void handle_alu_16(WDC65816* cpu) {
    fetch_operand(cpu);
    cpu->registers.a.word = cpu->op_algorithm(cpu, cpu->registers.a.word);
    end_op(cpu, true);
}

void schedule_and(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_and;
    schedule_alu(cpu);
}