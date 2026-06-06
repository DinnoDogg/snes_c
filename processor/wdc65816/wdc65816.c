#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../../include/processor/wdc65816/wdc65816.h"
#include "../../include/processor/wdc65816/instruction.h"
#include "../../include/processor/wdc65816/addressing.h"

static uint8_t read(WDC65816* cpu, uint32_t address);
static void write(WDC65816* cpu, uint32_t address, uint8_t data);

static void dummy_read(WDC65816* cpu, uint32_t address);
static void dummy_read_pc(WDC65816* cpu);

static uint8_t read_immediate(WDC65816* cpu);

static uint8_t pull_stack(WDC65816* cpu);
static void push_stack(WDC65816* cpu, uint8_t data);

static void increment_address(WDC65816* cpu);
static void decrement_address(WDC65816* cpu);

static void clamp_address_bank(WDC65816* cpu);

static void load_operand_byte(WDC65816* cpu);

static void load_operand_word_low(WDC65816* cpu);
static void load_operand_word_high(WDC65816* cpu);

static void store_result_byte(WDC65816* cpu);

static void store_result_word_low(WDC65816* cpu);
static void store_result_word_high(WDC65816* cpu);

static bool get_flag(WDC65816* cpu, WDC65816_flag flag);
static void set_flag(WDC65816* cpu, WDC65816_flag flag, bool value);

static bool page_crossed(uint32_t address_a, uint32_t address_b);

static void set_nz(WDC65816* cpu, uint16_t value);

static void end_op(WDC65816* cpu);
static void end_op_flag_i(WDC65816* cpu, bool flag_i);

static int get_highest_bit_position(WDC65816* cpu);
static int get_data_mask(WDC65816* cpu);

static uint16_t algorithm_and(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_asl(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_cmp(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_dec(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_eor(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_inc(WDC65816* cpu, uint16_t source);

static const uint8_t highest_bit_pos[0x2] = {0xF, 0x7};
static const uint16_t max_value[0x2] = {0xFFFF, 0xFF};

static const WDC65816_op op_lookup[0x100] = {
    [0x2D] = {schedule_addr_a, schedule_and, false},
    [0x3D] = {schedule_addr_a_x, schedule_and, false},
    [0x39] = {schedule_addr_a_y, schedule_and, false},
    [0x3F] = {schedule_addr_al_x, schedule_and, false},
    [0x2F] = {schedule_addr_al, schedule_and, false},
    [0x21] = {schedule_addr_d_x_indr, schedule_and, false},
    [0x35] = {schedule_addr_d_x, schedule_and, false},
    [0x31] = {schedule_addr_d_indr_y, schedule_and, false},
    [0x32] = {schedule_addr_d_indr, schedule_and, false},
    [0x27] = {schedule_addr_dl_indr, schedule_and, false},
    [0x37] = {schedule_addr_dl_indr_y, schedule_and, false},
    [0x25] = {schedule_addr_d, schedule_and, false},
    [0x29] = {schedule_addr_imm, schedule_and, false},
    [0x23] = {schedule_addr_d_s, schedule_and, false},
    [0x33] = {schedule_addr_d_s_indr_y, schedule_and, false},

    [0x0A] = {schedule_addr_implied, schedule_asl_a, false},
    [0x0E] = {schedule_addr_a, schedule_asl, true},
    [0x06] = {schedule_addr_d, schedule_asl, true},
    [0x1E] = {schedule_addr_a_x, schedule_asl, true},
    [0x16] = {schedule_addr_d_x, schedule_asl, true},

    [0x90] = {schedule_addr_implied, schedule_bcc, false},
    [0xB0] = {schedule_addr_implied, schedule_bcs, false},
    [0xD0] = {schedule_addr_implied, schedule_bne, false},
    [0xF0] = {schedule_addr_implied, schedule_beq, false},
    [0x10] = {schedule_addr_implied, schedule_bpl, false},
    [0x30] = {schedule_addr_implied, schedule_bmi, false},
    [0x50] = {schedule_addr_implied, schedule_bvc, false},
    [0x70] = {schedule_addr_implied, schedule_bvs, false},
    [0x80] = {schedule_addr_implied, schedule_bra, false},
    [0x82] = {schedule_addr_implied, schedule_brl, false},

    [0x89] = {schedule_addr_imm, schedule_bit, false},
    [0x2C] = {schedule_addr_a, schedule_bit, false},
    [0x24] = {schedule_addr_d, schedule_bit, false},
    [0x3C] = {schedule_addr_a_x, schedule_bit, false},
    [0x34] = {schedule_addr_d_x, schedule_bit, false},

    [0x00] = {schedule_addr_implied, schedule_brk, false},
    [0x02] = {schedule_addr_implied, schedule_cop, false},

    [0x18] = {schedule_addr_implied, schedule_clc, false},
    [0x58] = {schedule_addr_implied, schedule_cli, false},
    [0xD8] = {schedule_addr_implied, schedule_cld, false},
    [0xB8] = {schedule_addr_implied, schedule_clv, false},

    [0xC9] = {schedule_addr_imm, schedule_cmp, false},
    [0xCD] = {schedule_addr_a, schedule_cmp, false},
    [0xCF] = {schedule_addr_al, schedule_cmp, false},
    [0xC5] = {schedule_addr_d, schedule_cmp, false},
    [0xD2] = {schedule_addr_d_indr, schedule_cmp, false},
    [0xC7] = {schedule_addr_dl_indr, schedule_cmp, false},
    [0xDD] = {schedule_addr_a_x, schedule_cmp, false},
    [0xDF] = {schedule_addr_al_x, schedule_cmp, false},
    [0xD9] = {schedule_addr_a_y, schedule_cmp, false},
    [0xD5] = {schedule_addr_d_x, schedule_cmp, false},
    [0xC1] = {schedule_addr_d_x_indr, schedule_cmp, false},
    [0xD1] = {schedule_addr_d_indr_y, schedule_cmp, false},
    [0xD7] = {schedule_addr_dl_indr_y, schedule_cmp, false},
    [0xC3] = {schedule_addr_d_s, schedule_cmp, false},
    [0xD3] = {schedule_addr_d_s_indr_y, schedule_cmp, false},

    [0xE0] = {schedule_addr_imm_x, schedule_cpx, false},
    [0xEC] = {schedule_addr_a, schedule_cpx, false},
    [0xE4] = {schedule_addr_d, schedule_cpx, false},

    [0xC0] = {schedule_addr_imm_x, schedule_cpy, false},
    [0xCC] = {schedule_addr_a, schedule_cpy, false},
    [0xC4] = {schedule_addr_d, schedule_cpy, false},

    [0x3A] = {schedule_addr_implied, schedule_dec_a, false},
    [0xCE] = {schedule_addr_a, schedule_dec, true},
    [0xC6] = {schedule_addr_d, schedule_dec, true},
    [0xDE] = {schedule_addr_a_x, schedule_dec, true},
    [0xD6] = {schedule_addr_d_x, schedule_dec, true},

    [0xCA] = {schedule_addr_implied, schedule_dex, false},
    [0x88] = {schedule_addr_implied, schedule_dey, false},

    [0x49] = {schedule_addr_imm, schedule_eor, false},
    [0x4D] = {schedule_addr_a, schedule_eor, false},
    [0x4F] = {schedule_addr_al, schedule_eor, false},
    [0x45] = {schedule_addr_d, schedule_eor, false},
    [0x52] = {schedule_addr_d_indr, schedule_eor, false},
    [0x47] = {schedule_addr_dl_indr, schedule_eor, false},
    [0x5D] = {schedule_addr_a_x, schedule_eor, false},
    [0x5F] = {schedule_addr_al_x, schedule_eor, false},
    [0x59] = {schedule_addr_a_y, schedule_eor, false},
    [0x55] = {schedule_addr_d_x, schedule_eor, false},
    [0x41] = {schedule_addr_d_x_indr, schedule_eor, false},
    [0x51] = {schedule_addr_d_indr_y, schedule_eor, false},
    [0x57] = {schedule_addr_dl_indr_y, schedule_eor, false},
    [0x43] = {schedule_addr_d_s, schedule_eor, false},
    [0x53] = {schedule_addr_d_s_indr_y, schedule_eor, false},

    [0x1A] = {schedule_addr_implied, schedule_inc_a, false},
    [0xEE] = {schedule_addr_a, schedule_inc, true},
    [0xE6] = {schedule_addr_d, schedule_inc, true},
    [0xFE] = {schedule_addr_a_x, schedule_inc, true},
    [0xF6] = {schedule_addr_d_x, schedule_inc, true},
    
    [0xE8] = {schedule_addr_implied, schedule_inx, false},
    [0xC8] = {schedule_addr_implied, schedule_iny, false},

    
};

void init_wdc65816(WDC65816* cpu, read_callback read, write_callback write) {
    cpu->read = read;
    cpu->write = write;
}

void cycle_wdc65816(WDC65816* cpu) {
    if (cpu->current_cycle++ == 0) {
        if (cpu->nmi_latch) {

            cpu->interrupt_vector = VECTOR_NMI;
            schedule_interrupt(cpu);
            dummy_read_pc(cpu);

            return;
        }

        if (cpu->irq_pending) {

            cpu->interrupt_vector = VECTOR_IRQ;
            schedule_interrupt(cpu);
            dummy_read_pc(cpu);

            return;
        } 

        uint8_t opcode = read_immediate(cpu);
        const WDC65816_op* op = &op_lookup[opcode];

        cpu->bank_mode = BANK_DATA;
        cpu->dummy_read = op->dummy_read;

        op->schedule_address(cpu);
        op->schedule_op(cpu);
    }

    else {
        cpu->cycle_lookup[cpu->current_cycle - 2](cpu);
    }

    cpu->nmi_latch = cpu->nmi_line && !cpu->last_nmi_line;
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

void dummy_read_pc(WDC65816* cpu) {
    cpu->read((cpu->registers.pbr << 16) | cpu->registers.pc.word);
}

uint8_t read_immediate(WDC65816* cpu) {
    return read(cpu, (cpu->registers.pbr << 16) | cpu->registers.pc.word++);
}

void increment_address(WDC65816* cpu) {
    cpu->operand_address++;
    clamp_address_bank(cpu);
}

void decrement_address(WDC65816* cpu) {
    cpu->operand_address--;
    clamp_address_bank(cpu);
}

void clamp_address_bank(WDC65816* cpu) {
    switch (cpu->bank_mode) {
        case BANK_DATA : break;
        case BANK_ZERO : cpu->operand_address &= 0xFFFF; break;
        case BANK_PROGRAM : cpu->operand_address &= 0xFFFF; cpu->operand_address |= (cpu->registers.pbr << 16); break;
    }
}

void load_operand_byte(WDC65816* cpu) {
    cpu->operand = read(cpu, cpu->operand_address);
}

void load_operand_word_low(WDC65816* cpu) {
    load_operand_byte(cpu);
    increment_address(cpu);
}

void load_operand_word_high(WDC65816* cpu) {
    uint8_t data = read(cpu, cpu->operand_address);
    cpu->operand |= (data << 8);
}

void store_result_byte(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand);
}

void store_result_word_low(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand & 0xFF);
}

void store_result_word_high(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand >> 8);
    decrement_address(cpu);
}

uint8_t pull_stack(WDC65816* cpu) {
    return read(cpu, ++cpu->registers.s.word);
}

void push_stack(WDC65816* cpu, uint8_t data) {
    write(cpu, cpu->registers.s.word--, data);
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
    return (address_a & 0xFFFF00) != (address_b & 0xFFFF00);
}

void set_nz(WDC65816* cpu, uint16_t value) {
    int highest_bit = get_highest_bit_position(cpu);
    bool n = (value >> highest_bit) & 0x1;
    set_flag(cpu, FLAG_N, n);
    set_flag(cpu, FLAG_Z, value == 0);
}

int get_highest_bit_position(WDC65816* cpu) {
    return highest_bit_pos[cpu->data_size];
}

int get_data_mask(WDC65816* cpu) {
    return max_value[cpu->data_size];
}

void end_op(WDC65816* cpu) {
    end_op_flag_i(cpu, get_flag(cpu, FLAG_I));
}

void end_op_flag_i(WDC65816* cpu, bool flag_i) {
    cpu->current_cycle = 0x00;
    cpu->cycle_lookup_index = 0x00;
    cpu->irq_pending = cpu->irq_line && !flag_i;
}

uint16_t algorithm_and(WDC65816* cpu, uint16_t source) {
    uint16_t result = source & cpu->operand;
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_eor(WDC65816* cpu, uint16_t source) {
    uint16_t result = source ^ cpu->operand;
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_asl(WDC65816* cpu, uint16_t source) {
    int highest_bit = get_highest_bit_position(cpu);
    uint16_t data_mask = get_data_mask(cpu);
    uint16_t result = source << 1;

    result &= data_mask;

    set_flag(cpu, FLAG_C, source >> highest_bit);
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_bit(WDC65816* cpu, uint16_t source) {
    uint16_t result = source & cpu->operand;
    int bit_n = get_highest_bit_position(cpu);
    int bit_v = bit_n - 1;

    set_flag(cpu, FLAG_Z, result == 0);

    if (cpu->bank_mode != BANK_PROGRAM) {
        set_flag(cpu, FLAG_N, cpu->operand >> bit_n);
        set_flag(cpu, FLAG_V, (cpu->operand >> bit_v) & 0x1);
    }

    return source;
}

uint16_t algorithm_cmp(WDC65816* cpu, uint16_t source) {
    uint16_t result = source - cpu->operand;
    set_flag(cpu, FLAG_C, source >= cpu->operand);
    set_nz(cpu, result);
    return source;
}

uint16_t algorithm_dec(WDC65816* cpu, uint16_t source) {
    uint16_t data_mask = get_data_mask(cpu);
    uint16_t result = source - 1;
    result &= data_mask;
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_inc(WDC65816* cpu, uint16_t source) {
    uint16_t data_mask = get_data_mask(cpu);
    uint16_t result = source + 1;
    result &= data_mask;
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

void schedule_addr_d_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_4;
}

void addr_d_indr_1(WDC65816* cpu) {
    cpu->indirect_address = (cpu->registers.d.high << 8) + read_immediate(cpu);

    if (cpu->registers.d.low == 0) {
        cpu->current_cycle++;
    }
} 

void addr_d_indr_2(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->indirect_address += cpu->registers.d.low;
    cpu->indirect_address &= 0xFFFF;
} 

void addr_d_indr_3(WDC65816* cpu) {
    cpu->operand_address = (cpu->registers.dbr << 16) | read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
} 

void addr_d_indr_4(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address++) << 8;
} 

void schedule_addr_d_x_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_x_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_4;
}

void addr_d_x_indr_1(WDC65816* cpu) {
    cpu->indirect_address += cpu->registers.x.word;
    cpu->indirect_address &= 0xFFFF;
} 

void schedule_addr_d(WDC65816* cpu) {
    cpu->bank_mode = BANK_ZERO;

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_2;
}

void addr_d_1(WDC65816* cpu) {
    cpu->operand_address = (cpu->registers.d.high << 8) | read_immediate(cpu);

    if (cpu->registers.d.low == 0) {
        cpu->current_cycle++;
    }
} 

void addr_d_2(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->operand_address += cpu->registers.d.low;
    cpu->operand_address &= 0xFFFF;
} 

void schedule_addr_d_x(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.x;
    schedule_addr_d_ind(cpu);
}

void schedule_addr_d_y(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.y;
    schedule_addr_d_ind(cpu);
}

void schedule_addr_d_ind(WDC65816* cpu) {
    cpu->bank_mode = BANK_ZERO;

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_ind_1;
}

void addr_d_ind_1(WDC65816* cpu) {
    cpu->operand_address += cpu->index_register->word;
    cpu->operand_address &= 0xFFFF;
} 

void schedule_addr_d_indr_y(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_y_2;
}

void addr_d_indr_y_1(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 8;

    if (!page_crossed(cpu->operand_address, cpu->operand_address + cpu->registers.y.word) && get_flag(cpu, FLAG_X) && !cpu->dummy_read) {
        cpu->current_cycle++;
    }
    
    cpu->operand_address += cpu->registers.y.low;
} 

void addr_d_indr_y_2(WDC65816* cpu) {
    dummy_read(cpu, cpu->operand_address);
    cpu->operand_address += cpu->registers.y.high << 8;
} 

void schedule_addr_dl_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_3;
}

void addr_dl_indr_1(WDC65816* cpu) {
    cpu->operand_address = read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
} 

void addr_dl_indr_2(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address++) << 8;
    cpu->indirect_address &= 0xFFFF;
} 

void addr_dl_indr_3(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 16;
} 

void schedule_addr_dl_indr_y(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dl_indr_y_1;
}

void addr_dl_indr_y_1(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 16;
    cpu->operand_address += cpu->registers.y.word;
} 

static void schedule_addr_imm(WDC65816* cpu) {
    cpu->bank_mode = BANK_PROGRAM;
    cpu->operand_address = (cpu->registers.pbr << 16) | cpu->registers.pc.word++;
    if (!get_flag(cpu, FLAG_M)) cpu->registers.pc.word++;
}

static void schedule_addr_imm_x(WDC65816* cpu) {
    cpu->bank_mode = BANK_PROGRAM;
    cpu->operand_address = (cpu->registers.pbr << 16) | cpu->registers.pc.word++;
    if (!get_flag(cpu, FLAG_X)) cpu->registers.pc.word++;
}

void schedule_addr_d_s(WDC65816* cpu) {
    cpu->bank_mode = BANK_ZERO;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_2;
}

void addr_d_s_1(WDC65816* cpu) {
    cpu->operand_address = cpu->registers.s.word + read_immediate(cpu);
    cpu->operand_address &= 0xFFFF;
}

void addr_d_s_2(WDC65816* cpu) {
    dummy_read_pc(cpu);
}

void schedule_addr_d_s_indr_y(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_4;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_5;

} 

void addr_d_s_indr_y_1(WDC65816* cpu) {
    cpu->indirect_address = cpu->registers.s.word + read_immediate(cpu);
    cpu->indirect_address &= 0xFFFF;
}

void addr_d_s_indr_y_2(WDC65816* cpu) {
    dummy_read_pc(cpu);
}

void addr_d_s_indr_y_3(WDC65816* cpu) {
    cpu->operand_address = (cpu->registers.dbr << 16) | read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
}

void addr_d_s_indr_y_4(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 8;
    cpu->operand_address += cpu->registers.y.word;
}

void addr_d_s_indr_y_5(WDC65816* cpu) {
    dummy_read(cpu, cpu->indirect_address);
}

void schedule_addr_implied(WDC65816* cpu) {
    return;
}

void schedule_alu(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_M)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_8_1;
        return;
    }  

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_16_2;
}

void alu_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    cpu->op_algorithm(cpu, cpu->registers.a.low);
    end_op(cpu);
}

void alu_16_1(WDC65816* cpu) {
    load_operand_word_low(cpu);
}

void alu_16_2(WDC65816* cpu) {
    load_operand_word_high(cpu);
    cpu->op_algorithm(cpu, cpu->registers.a.word);
    end_op(cpu);
}

void schedule_alu_writeback(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_M)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_writeback_8_1;
        return;
    }  

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_writeback_16_1;
}

void alu_writeback_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    cpu->registers.a.low = cpu->op_algorithm(cpu, cpu->registers.a.low);
    end_op(cpu);
}

void alu_writeback_16_1(WDC65816* cpu) {
    load_operand_word_high(cpu);
    cpu->registers.a.word = cpu->op_algorithm(cpu, cpu->registers.a.word);
    end_op(cpu);
}

void schedule_alu_index_reg(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_X)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_index_8_1;
        return;
    }  

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_index_16_1;
}

void alu_index_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    cpu->op_algorithm(cpu, cpu->index_register->low);
    end_op(cpu);
}

void alu_index_16_1(WDC65816* cpu) {
    load_operand_word_high(cpu);
    cpu->op_algorithm(cpu, cpu->index_register->word);
    end_op(cpu);
}

void schedule_rmw(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_M)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_1;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_2;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_3;
        return;
    }  

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_16_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_16_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_16_4;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_16_5;
}

void rmw_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    increment_address(cpu);
}

void rmw_8_2(WDC65816* cpu) {
    dummy_read(cpu, cpu->operand_address + 1);
    cpu->operand = cpu->op_algorithm(cpu, cpu->operand);
}

void rmw_8_3(WDC65816* cpu) {
    decrement_address(cpu);
    store_result_byte(cpu);
    end_op(cpu);
}

void rmw_16_1(WDC65816* cpu) {
    load_operand_word_low(cpu);
}

void rmw_16_2(WDC65816* cpu) {
    load_operand_word_high(cpu);
}

void rmw_16_3(WDC65816* cpu) {
    dummy_read(cpu, cpu->operand_address);
    cpu->operand = cpu->op_algorithm(cpu, cpu->operand);
}

void rmw_16_4(WDC65816* cpu) {
    store_result_word_high(cpu);
}

void rmw_16_5(WDC65816* cpu) {
    store_result_word_low(cpu);
    end_op(cpu);
}

void schedule_op_accumulator(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_M)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &accumulator_8_1;
        return;
    }

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &accumulator_16_1;
    return;
}

void schedule_op_index_reg(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_X)) {
        cpu->data_size = SIZE_BYTE;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &index_reg_8_1;
        return;
    }

    cpu->data_size = SIZE_WORD;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &index_reg_16_1;
    return;
}

void index_reg_8_1(WDC65816* cpu) {
    cpu->index_register->low = cpu->op_algorithm(cpu, cpu->index_register->low);
    end_op(cpu);
}

void index_reg_16_1(WDC65816* cpu) {
    cpu->index_register->word = cpu->op_algorithm(cpu, cpu->index_register->word);
    end_op(cpu);
}

void accumulator_8_1(WDC65816* cpu) {
    cpu->registers.a.low = cpu->op_algorithm(cpu, cpu->registers.a.low);
    end_op(cpu);
}

void accumulator_16_1(WDC65816* cpu) {
    cpu->registers.a.word = cpu->op_algorithm(cpu, cpu->registers.a.word);
    end_op(cpu);
}

void schedule_and(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_and;
    schedule_alu_writeback(cpu);
}

void schedule_asl(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_asl;
    schedule_rmw(cpu);
}

void schedule_asl_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_asl;
    schedule_op_accumulator(cpu);
}

void schedule_branch(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &branch_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &branch_2;
}

void branch_1(WDC65816* cpu) {
    cpu->branch_offset = (int8_t) read_immediate(cpu);

    if (!cpu->take_branch) {
        end_op(cpu);
    }
}

void branch_2(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->registers.pc.word += cpu->branch_offset; 
    end_op(cpu);
}

void schedule_bcc(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, FLAG_C);
    schedule_branch(cpu);
}

void schedule_bcs(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, FLAG_C);
    schedule_branch(cpu);
}

void schedule_bne(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, FLAG_Z);
    schedule_branch(cpu);
}

void schedule_beq(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, FLAG_Z);
    schedule_branch(cpu);
}

void schedule_bpl(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, FLAG_N);
    schedule_branch(cpu);
}

void schedule_bmi(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, FLAG_N);
    schedule_branch(cpu);
}

void schedule_bvc(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, FLAG_V);
    schedule_branch(cpu);
}

void schedule_bvs(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, FLAG_V);
    schedule_branch(cpu);
}

void schedule_bra(WDC65816* cpu) {
    cpu->take_branch = true;
    schedule_branch(cpu);
}

void schedule_brl(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &brl_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &brl_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &branch_2;
}

void brl_1(WDC65816* cpu) {
    cpu->branch_offset = read_immediate(cpu);
}

void brl_2(WDC65816* cpu) {
    cpu->branch_offset |= read_immediate(cpu) << 8;
}

void schedule_bit(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_bit;
    schedule_alu(cpu);
}

void schedule_cmp(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    schedule_alu(cpu);
}

void schedule_interrupt(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_4;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_5;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_6;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &interrupt_7;
}

void interrupt_1(WDC65816* cpu) {
    if (cpu->interrupt_vector == VECTOR_COP || cpu->interrupt_vector == VECTOR_BRK) {
        read_immediate(cpu);
        return;
    }

    dummy_read_pc(cpu);
}

void interrupt_2(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pbr);
}

void interrupt_3(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pc.high);
}

void interrupt_4(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pc.low);
}

void interrupt_5(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.p);
    set_flag(cpu, FLAG_D, false);
    set_flag(cpu, FLAG_I, true);
}

void interrupt_6(WDC65816* cpu) {
    cpu->registers.pbr = 0x00;
    cpu->registers.pc.word = read(cpu, cpu->interrupt_vector);
}

void interrupt_7(WDC65816* cpu) {
    cpu->registers.pc.word |= read(cpu, cpu->interrupt_vector + 1) << 8;
    end_op(cpu);
}

void schedule_brk(WDC65816* cpu) {
    cpu->interrupt_vector = VECTOR_BRK;
    schedule_interrupt(cpu);
}

void schedule_cop(WDC65816* cpu) {
    cpu->interrupt_vector = VECTOR_COP;
    schedule_interrupt(cpu);
}

void schedule_flag_clear(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &flag_clear_1;
}

void flag_clear_1(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, FLAG_I); 
    set_flag(cpu, cpu->operation_flag, false);
    end_op_flag_i(cpu, flag_i);
}

void schedule_clc(WDC65816* cpu) {
    cpu->operation_flag = FLAG_C;
    schedule_flag_clear(cpu);
}

void schedule_clv(WDC65816* cpu) {
    cpu->operation_flag = FLAG_V;
    schedule_flag_clear(cpu);
}

void schedule_cli(WDC65816* cpu) {
    cpu->operation_flag = FLAG_I;
    schedule_flag_clear(cpu);
}

void schedule_cld(WDC65816* cpu) {
    cpu->operation_flag = FLAG_D;
    schedule_flag_clear(cpu);
}

void schedule_dec(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    schedule_rmw(cpu);
}

void schedule_dec_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    schedule_op_accumulator(cpu);
}

void schedule_cpx(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    cpu->index_register = &cpu->registers.x;
    schedule_alu_index_reg(cpu);
}

void schedule_cpy(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    cpu->index_register = &cpu->registers.y;
    schedule_alu_index_reg(cpu);
}

void schedule_dex(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    cpu->index_register = &cpu->registers.x;
    schedule_op_index_reg(cpu);
}

void schedule_dey(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    cpu->index_register = &cpu->registers.y;
    schedule_op_index_reg(cpu);
}

void schedule_eor(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_eor;
    schedule_alu_writeback(cpu);
}

void schedule_inc(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    schedule_rmw(cpu);
}

void schedule_inc_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    schedule_op_accumulator(cpu);
}

void schedule_inx(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    cpu->index_register = &cpu->registers.x;
    schedule_op_index_reg(cpu);
}

void schedule_iny(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    cpu->index_register = &cpu->registers.y;
    schedule_op_index_reg(cpu);
}

