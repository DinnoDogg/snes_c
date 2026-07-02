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
static void dummy_read_sp(WDC65816* cpu);

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

static void setup_alu_a(WDC65816* cpu);
static void setup_alu_x(WDC65816* cpu);
static void setup_alu_y(WDC65816* cpu);

static void transfer_8(WDC65816* cpu, uint8_t value, uint8_t* reg);
static void transfer_16(WDC65816* cpu, uint16_t value, uint16_t* reg);

static uint16_t algorithm_and(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_asl(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_cmp(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_dec(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_eor(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_inc(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_load(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_lsr(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_or(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_rol(WDC65816* cpu, uint16_t source);
static uint16_t algorithm_ror(WDC65816* cpu, uint16_t source);

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

    [0x4C] = {schedule_addr_implied, schedule_jmp_a, false},
    [0x6C] = {schedule_addr_implied, schedule_jmp_a_indr, false},
    [0x7C] = {schedule_addr_implied, schedule_jmp_a_x_indr, false},
    [0x5C] = {schedule_addr_implied, schedule_jml, false},
    [0xDC] = {schedule_addr_implied, schedule_jml_indr, false},

    [0x20] = {schedule_addr_implied, schedule_jsr_a, false},
    [0xFC] = {schedule_addr_implied, schedule_jsr_a_x_indr, false},
    [0x22] = {schedule_addr_implied, schedule_jsl, false},

    [0xA9] = {schedule_addr_imm, schedule_lda, false},
    [0xAD] = {schedule_addr_a, schedule_lda, false},
    [0xAF] = {schedule_addr_al, schedule_lda, false},
    [0xA5] = {schedule_addr_d, schedule_lda, false},
    [0xB2] = {schedule_addr_d_indr, schedule_lda, false},
    [0xA7] = {schedule_addr_dl_indr, schedule_lda, false},
    [0xBD] = {schedule_addr_a_x, schedule_lda, false},
    [0xBF] = {schedule_addr_al_x, schedule_lda, false},
    [0xB9] = {schedule_addr_a_y, schedule_lda, false},
    [0xB5] = {schedule_addr_d_x, schedule_lda, false},
    [0xA1] = {schedule_addr_d_x_indr, schedule_lda, false},
    [0xB1] = {schedule_addr_d_indr_y, schedule_lda, false},
    [0xB7] = {schedule_addr_dl_indr_y, schedule_lda, false},
    [0xA3] = {schedule_addr_d_s, schedule_lda, false},
    [0xB3] = {schedule_addr_d_s_indr_y, schedule_lda, false},

    [0xA2] = {schedule_addr_imm_x, schedule_ldx, false},
    [0xAE] = {schedule_addr_a, schedule_ldx, false},
    [0xA6] = {schedule_addr_d, schedule_ldx, false},
    [0xBE] = {schedule_addr_a_y, schedule_ldx, false},
    [0xB6] = {schedule_addr_d_y, schedule_ldx, false},

    [0xA0] = {schedule_addr_imm_x, schedule_ldy, false},
    [0xAC] = {schedule_addr_a, schedule_ldy, false},
    [0xA4] = {schedule_addr_d, schedule_ldy, false},
    [0xBC] = {schedule_addr_a_x, schedule_ldy, false},
    [0xB4] = {schedule_addr_d_x, schedule_ldy, false},

    [0x4A] = {schedule_addr_implied, schedule_lsr_a, false},
    [0x4E] = {schedule_addr_a, schedule_lsr, true},
    [0x46] = {schedule_addr_d, schedule_lsr, true},
    [0x5E] = {schedule_addr_a_x, schedule_lsr, true},
    [0x56] = {schedule_addr_d_x, schedule_lsr, true},

    [0x54] = {schedule_addr_implied, schedule_mvn, false},
    [0x44] = {schedule_addr_implied, schedule_mvp, false},

    [0xEA] = {schedule_addr_implied, schedule_nop, false},

    [0x09] = {schedule_addr_imm, schedule_ora, false},
    [0x0D] = {schedule_addr_a, schedule_ora, false},
    [0x0F] = {schedule_addr_al, schedule_ora, false},
    [0x05] = {schedule_addr_d, schedule_ora, false},
    [0x12] = {schedule_addr_d_indr, schedule_ora, false},
    [0x07] = {schedule_addr_dl_indr, schedule_ora, false},
    [0x1D] = {schedule_addr_a_x, schedule_ora, false},
    [0x1F] = {schedule_addr_al_x, schedule_ora, false},
    [0x19] = {schedule_addr_a_y, schedule_ora, false},
    [0x15] = {schedule_addr_d_x, schedule_ora, false},
    [0x01] = {schedule_addr_d_x_indr, schedule_ora, false},
    [0x11] = {schedule_addr_d_indr_y, schedule_ora, false},
    [0x17] = {schedule_addr_dl_indr_y, schedule_ora, false},
    [0x03] = {schedule_addr_d_s, schedule_ora, false},
    [0x13] = {schedule_addr_d_s_indr_y, schedule_ora, false},

    [0xF4] = {schedule_addr_implied, schedule_pea, false},
    [0xD4] = {schedule_addr_implied, schedule_pei, false},
    [0x62] = {schedule_addr_implied, schedule_per, false},

    [0x48] = {schedule_addr_implied, schedule_pha, false},
    [0x8B] = {schedule_addr_implied, schedule_phb, false},
    [0x0B] = {schedule_addr_implied, schedule_phd, false},
    [0xDA] = {schedule_addr_implied, schedule_phx, false},
    [0x5A] = {schedule_addr_implied, schedule_phy, false},

    [0x68] = {schedule_addr_implied, schedule_pla, false},
    [0xAB] = {schedule_addr_implied, schedule_plb, false},
    [0x2B] = {schedule_addr_implied, schedule_pld, false},
    [0x28] = {schedule_addr_implied, schedule_plp, false},
    [0xFA] = {schedule_addr_implied, schedule_plx, false},
    [0x7A] = {schedule_addr_implied, schedule_ply, false},

    [0xC2] = {schedule_addr_implied, schedule_rep, false},

    [0x2A] = {schedule_addr_implied, schedule_rol_a, false},
    [0x2E] = {schedule_addr_a, schedule_rol, true},
    [0x26] = {schedule_addr_d, schedule_rol, true},
    [0x3E] = {schedule_addr_a_x, schedule_rol, true},
    [0x36] = {schedule_addr_d_x, schedule_rol, true},

    [0x6A] = {schedule_addr_implied, schedule_ror_a, false},
    [0x6E] = {schedule_addr_a, schedule_ror, true},
    [0x66] = {schedule_addr_d, schedule_ror, true},
    [0x7E] = {schedule_addr_a_x, schedule_ror, true},
    [0x76] = {schedule_addr_d_x, schedule_ror, true},

    [0x40] = {schedule_addr_implied, schedule_rti, false},
    [0x60] = {schedule_addr_implied, schedule_rts, false},
    [0x6B] = {schedule_addr_implied, schedule_rtl, false},

    [0x38] = {schedule_addr_implied, schedule_sec, false},
    [0x78] = {schedule_addr_implied, schedule_sei, false},
    [0xF8] = {schedule_addr_implied, schedule_sed, false},

    [0xE2] = {schedule_addr_implied, schedule_sep, false},

    [0x8D] = {schedule_addr_a, schedule_sta, true},
    [0x8F] = {schedule_addr_al, schedule_sta, true},
    [0x85] = {schedule_addr_d, schedule_sta, true},
    [0x92] = {schedule_addr_d_indr, schedule_sta, true},
    [0x87] = {schedule_addr_dl_indr, schedule_sta, true},
    [0x9D] = {schedule_addr_a_x, schedule_sta, true},
    [0x9F] = {schedule_addr_al_x, schedule_sta, true},
    [0x99] = {schedule_addr_a_y, schedule_sta, true},
    [0x95] = {schedule_addr_d_x, schedule_sta, true},
    [0x81] = {schedule_addr_d_x_indr, schedule_sta, true},
    [0x91] = {schedule_addr_d_indr_y, schedule_sta, true},
    [0x97] = {schedule_addr_dl_indr_y, schedule_sta, true},
    [0x83] = {schedule_addr_d_s, schedule_sta, true},
    [0x93] = {schedule_addr_d_s_indr_y, schedule_sta, true},

    [0x8E] = {schedule_addr_a, schedule_stx, true},
    [0x86] = {schedule_addr_d, schedule_stx, true},
    [0x96] = {schedule_addr_d_y, schedule_stx, true},

    [0x8C] = {schedule_addr_a, schedule_sty, true},
    [0x84] = {schedule_addr_d, schedule_sty, true},
    [0x94] = {schedule_addr_d_x, schedule_sty, true},

    [0x9C] = {schedule_addr_a, schedule_stz, true},
    [0x64] = {schedule_addr_d, schedule_stz, true},
    [0x9E] = {schedule_addr_a_x, schedule_stz, true},
    [0x74] = {schedule_addr_d_x, schedule_stz, true},

    [0xAA] = {schedule_addr_implied, schedule_tax, false},
    [0xA8] = {schedule_addr_implied, schedule_tay, false},
    [0x5B] = {schedule_addr_implied, schedule_tcd, false},
    [0x1B] = {schedule_addr_implied, schedule_tcs, false},
    [0x7B] = {schedule_addr_implied, schedule_tdc, false},

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

        else if (cpu->irq_pending) {

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
    cpu->last_nmi_line = cpu->nmi_line;
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

void dummy_read_sp(WDC65816* cpu) {
    cpu->read(cpu->registers.s.word);
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
    return highest_bit_pos[cpu->data_width_byte];
}

int get_data_mask(WDC65816* cpu) {
    return max_value[cpu->data_width_byte];
}

void end_op(WDC65816* cpu) {
    end_op_flag_i(cpu, get_flag(cpu, FLAG_I));
}

void transfer_8(WDC65816* cpu, uint8_t value, uint8_t* reg) {
    *reg = value;
    set_nz(cpu, value);
}

void transfer_16(WDC65816* cpu, uint16_t value, uint16_t* reg) {
    *reg = value;
    set_nz(cpu, value);
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

uint16_t algorithm_load(WDC65816* cpu, uint16_t source) {
    set_nz(cpu, cpu->operand);
    return cpu->operand;
}

uint16_t algorithm_lsr(WDC65816* cpu, uint16_t source) {
    uint16_t result = source >> 1;
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_Z, result == 0);
    set_flag(cpu, FLAG_C, source & 0x1);
    return result;
}

uint16_t algorithm_or(WDC65816* cpu, uint16_t source) {
    uint16_t result = source | cpu->operand;
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_rol(WDC65816* cpu, uint16_t source) {
    uint16_t data_mask = get_data_mask(cpu);
    int bit_c = get_highest_bit_position(cpu);
    uint16_t result = (source << 1) | get_flag(cpu, FLAG_C);
    result &= data_mask;
    set_flag(cpu, FLAG_C, source >> bit_c);
    set_nz(cpu, result);
    return result;
}

uint16_t algorithm_ror(WDC65816* cpu, uint16_t source) {
    int bit_c = get_highest_bit_position(cpu);
    uint16_t result = (source >> 1) | (get_flag(cpu, FLAG_C) << bit_c);
    set_flag(cpu, FLAG_C, source & 0x1);
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
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
}

void addr_d_s_1(WDC65816* cpu) {
    cpu->operand_address = cpu->registers.s.word + read_immediate(cpu);
    cpu->operand_address &= 0xFFFF;
}

void schedule_addr_d_s_indr_y(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_d_s_indr_y_4;

} 

void addr_d_s_indr_y_1(WDC65816* cpu) {
    cpu->indirect_address = cpu->registers.s.word + read_immediate(cpu);
    cpu->indirect_address &= 0xFFFF;
}

void addr_d_s_indr_y_2(WDC65816* cpu) {
    cpu->operand_address = (cpu->registers.dbr << 16) | read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
}

void addr_d_s_indr_y_3(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 8;
    cpu->operand_address += cpu->registers.y.word;
}

void addr_d_s_indr_y_4(WDC65816* cpu) {
    dummy_read(cpu, cpu->indirect_address);
}

void schedule_addr_implied(WDC65816* cpu) {
    return;
}

void setup_alu_a(WDC65816* cpu) {
    cpu->operand_register = &cpu->registers.a;
    cpu->data_width_byte = get_flag(cpu, FLAG_M);
}

void setup_alu_x(WDC65816* cpu) {
    cpu->operand_register = &cpu->registers.x;
    cpu->data_width_byte = get_flag(cpu, FLAG_X);
}

void setup_alu_y(WDC65816* cpu) {
    cpu->operand_register = &cpu->registers.y;
    cpu->data_width_byte = get_flag(cpu, FLAG_X);
}

void schedule_alu_memory(WDC65816* cpu) {
    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_8_1;
        return;
    }  
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_16_2;
}

void alu_memory_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    cpu->op_algorithm(cpu, cpu->operand_register->low);
    end_op(cpu);
}

void alu_memory_16_1(WDC65816* cpu) {
    load_operand_word_low(cpu);
}

void alu_memory_16_2(WDC65816* cpu) {
    load_operand_word_high(cpu);
    cpu->op_algorithm(cpu, cpu->operand_register->word);
    end_op(cpu);
}

void schedule_alu_memory_writeback(WDC65816* cpu) {
    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_writeback_8_1;
        return;
    }  
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_memory_writeback_16_1;
}

void alu_memory_writeback_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    alu_8_1(cpu);
}

void alu_memory_writeback_16_1(WDC65816* cpu) {
    load_operand_word_high(cpu);
    alu_16_1(cpu);
}

void schedule_alu_implicit(WDC65816* cpu) {
    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_8_1;
        return;
    }  
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &alu_16_1;
}

void alu_8_1(WDC65816* cpu) {
    cpu->operand_register->low = cpu->op_algorithm(cpu, cpu->operand_register->low);
    end_op(cpu);
}

void alu_16_1(WDC65816* cpu) {
    cpu->operand_register->word = cpu->op_algorithm(cpu, cpu->operand_register->word);
    end_op(cpu);
}

void schedule_rmw(WDC65816* cpu) {
    cpu->data_width_byte = get_flag(cpu, FLAG_M);

    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_1;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_2;
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_8_3;
        return;
    }  

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
    setup_alu_a(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_asl(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_asl;
    schedule_rmw(cpu);
}

void schedule_asl_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_asl;
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
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
    setup_alu_a(cpu);
    schedule_alu_memory(cpu);
}

void schedule_cmp(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    setup_alu_a(cpu);
    schedule_alu_memory(cpu);
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
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_cpx(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    setup_alu_x(cpu);
    schedule_alu_memory(cpu);
}

void schedule_cpy(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_cmp;
    setup_alu_y(cpu);
    schedule_alu_memory(cpu);
}

void schedule_dex(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    setup_alu_x(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_dey(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_dec;
    setup_alu_y(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_eor(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_eor;
    setup_alu_a(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_inc(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    schedule_rmw(cpu);
}

void schedule_inc_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_inx(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    setup_alu_x(cpu);
    schedule_alu_implicit(cpu);

}

void schedule_iny(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_inc;
    setup_alu_y(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_jmp_a(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_2;
}

void jmp_a_1(WDC65816* cpu) {
    cpu->operand_address = read_immediate(cpu);
}

void jmp_a_2(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
    cpu->registers.pc.word = cpu->operand_address;
    end_op(cpu);
}

void schedule_jmp_a_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_4;

}

void jmp_a_indr_1(WDC65816* cpu) {
    cpu->indirect_address = read_immediate(cpu);
}

void jmp_a_indr_2(WDC65816* cpu) {
    cpu->indirect_address |= read_immediate(cpu) << 8;
}

void jmp_a_indr_3(WDC65816* cpu) {
    cpu->operand_address = read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
}

void jmp_a_indr_4(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address++) << 8;
    cpu->indirect_address &= 0xFFFF;
    cpu->registers.pc.word = cpu->operand_address;
    end_op(cpu);
}
void schedule_jmp_a_x_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_x_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_x_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_4;
}

void jmp_a_x_indr_1(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->indirect_address += cpu->registers.x.word;
    cpu->indirect_address &= 0xFFFF;
    cpu->indirect_address |= cpu->registers.pbr << 16;
}

void jmp_a_x_indr_2(WDC65816* cpu) {
    cpu->operand_address = read(cpu, cpu->indirect_address++);
    cpu->indirect_address &= 0xFFFF;
    cpu->indirect_address |= cpu->registers.pbr << 16;
}

void schedule_jml(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jml_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jml_2;
}

void jml_1(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
}

void jml_2(WDC65816* cpu) {
    cpu->registers.pbr = read_immediate(cpu);
    cpu->registers.pc.word = cpu->operand_address;
    end_op(cpu);
}
void schedule_jml_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jml_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jml_indr_2;
}

void jml_indr_1(WDC65816* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address++) << 8;
    cpu->indirect_address &= 0xFFFF;
}

void jml_indr_2(WDC65816* cpu) {
    cpu->registers.pbr = read(cpu, cpu->indirect_address);
    cpu->registers.pc.word = cpu->operand_address;
    end_op(cpu);
}

void schedule_jsr_a(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_3;
}

void jsr_a_1(WDC65816* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
}

void jsr_a_2(WDC65816* cpu) {
    cpu->registers.pc.word--;
    push_stack(cpu, cpu->registers.pc.high);
}

void jsr_a_3(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pc.low);
    cpu->registers.pc.word = cpu->operand_address;
    end_op(cpu);
}

void schedule_jsr_a_x_indr(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_x_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_x_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_x_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_x_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_indr_4;
}

void jsr_a_x_indr_1(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pc.high);
}

void jsr_a_x_indr_2(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pc.low);
}

void schedule_jsl(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jmp_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jml_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsl_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_sp;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsl_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &jsr_a_3;
}

void jsl_1(WDC65816* cpu) {
    push_stack(cpu, cpu->registers.pbr);
}

void jsl_2(WDC65816* cpu) {
    cpu->registers.pbr = read_immediate(cpu);
}

void schedule_lda(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_load;
    setup_alu_a(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_ldx(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_load;
    setup_alu_x(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_ldy(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_load;
    setup_alu_y(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_lsr(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_lsr;
    schedule_rmw(cpu);
}

void schedule_lsr_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_lsr;
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_block_move(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_4;
} 

void block_move_1(WDC65816* cpu) {
    cpu->registers.dbr = read_immediate(cpu);
}

void block_move_2(WDC65816* cpu) {
    cpu->operand_address = read_immediate(cpu) << 16;
}

void block_move_3(WDC65816* cpu) {
    cpu->operand = read(cpu, cpu->operand_address | cpu->registers.x.word);
}

void block_move_4(WDC65816* cpu) {
    uint32_t address = (cpu->registers.dbr << 16) | cpu->registers.y.word;
    write(cpu, address, cpu->operand);

}

void block_move_5(WDC65816* cpu) {
    uint32_t address = (cpu->registers.dbr << 16) | cpu->registers.y.word;
    dummy_read(cpu, address);

    if (--cpu->registers.a.word != 0xFFFF) {
        cpu->registers.pc.word -= 3;
    }

    end_op(cpu);
}

void schedule_mvn(WDC65816* cpu) {
    schedule_block_move(cpu);
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mvn_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_5;
}

void mvn_1(WDC65816* cpu) {
    uint32_t address = (cpu->registers.dbr << 16) | cpu->registers.y.word;
    dummy_read(cpu, address);
    cpu->registers.x.word++;
    cpu->registers.y.word++;

    if (get_flag(cpu, FLAG_X)) {
        cpu->registers.x.word &= 0xFF;
        cpu->registers.y.word &= 0xFF;    
    }   
}

void schedule_mvp(WDC65816* cpu) {
    schedule_block_move(cpu);
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mvp_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &block_move_5;
}

void mvp_1(WDC65816* cpu) {
    uint32_t address = (cpu->registers.dbr << 16) | cpu->registers.y.word;
    dummy_read(cpu, address);
    cpu->registers.x.word--;
    cpu->registers.y.word--;

    if (get_flag(cpu, FLAG_X)) {
        cpu->registers.x.word &= 0xFF;
        cpu->registers.y.word &= 0xFF;    
    }   
}

void schedule_nop(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &nop_1;
}

void nop_1(WDC65816* cpu) {
    dummy_read_pc(cpu);
    end_op(cpu);
}

void schedule_ora(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_or;
    setup_alu_a(cpu);
    schedule_alu_memory_writeback(cpu);
}

void schedule_push_op_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &push_op_8_1;
}

void push_op_8_1(WDC65816* cpu) {
    push_stack(cpu, cpu->operand & 0xFF);
    end_op(cpu);
}

void schedule_push_op_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &push_op_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &push_op_16_2;
}

void push_op_16_1(WDC65816* cpu) {
    push_stack(cpu, cpu->operand >> 8);
}

void push_op_16_2(WDC65816* cpu) {
    push_stack(cpu, cpu->operand & 0xFF);
    end_op(cpu);
}

void schedule_pea(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pea_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pea_2;
    schedule_push_op_16(cpu);
}

void pea_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void pea_2(WDC65816* cpu) {
    cpu->operand |= read_immediate(cpu) << 8;
}

void schedule_pei(WDC65816* cpu) {
    schedule_addr_d(cpu);
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pei_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pei_2;
    schedule_push_op_16(cpu);
}

void pei_1(WDC65816* cpu) {
    load_operand_word_low(cpu);
}

void pei_2(WDC65816* cpu) {
    load_operand_word_high(cpu);
}

void schedule_per(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &per_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &per_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &per_3;
    schedule_push_op_16(cpu);
}

void per_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void per_2(WDC65816* cpu) {
    cpu->operand |= read_immediate(cpu) << 8;
}

void per_3(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->operand += cpu->registers.pc.word;
}

void schedule_pha(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;

    if (get_flag(cpu, FLAG_M)) {
        schedule_push_op_8(cpu);
        return;
    }
    schedule_push_op_16(cpu);
}

void schedule_phb(WDC65816* cpu) {
    cpu->operand = cpu->registers.dbr;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_phd(WDC65816* cpu) {
    cpu->operand = cpu->registers.d.word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_16(cpu);
}

void schedule_phk(WDC65816* cpu) {
    cpu->operand = cpu->registers.pbr;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_php(WDC65816* cpu) {
    cpu->operand = cpu->registers.p;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_push_index(WDC65816* cpu) {
    cpu->operand = cpu->index_register->word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;

    if (get_flag(cpu, FLAG_X)) {
        schedule_push_op_8(cpu);
        return;
    }
    schedule_push_op_16(cpu);
}

void schedule_phx(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.x;
    schedule_push_index(cpu);
}

void schedule_phy(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.y;
    schedule_push_index(cpu);
}

void schedule_pla(WDC65816* cpu) {
    cpu->data_width_byte = get_flag(cpu, FLAG_M);
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    
    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_8_1;
        return;
    }

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_16_2;
}

void pla_8_1(WDC65816* cpu) {
    cpu->registers.a.low = pull_stack(cpu);
    set_nz(cpu, cpu->registers.a.low);
    end_op(cpu);
}

void pla_16_1(WDC65816* cpu) {
    cpu->registers.a.low = pull_stack(cpu);
}

void pla_16_2(WDC65816* cpu) {
    cpu->registers.a.high = pull_stack(cpu);
    set_nz(cpu, cpu->registers.a.word);
    end_op(cpu);
}

void schedule_plb(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plb_1;
    cpu->data_width_byte = true;
}

void plb_1(WDC65816* cpu) {
    cpu->registers.dbr = pull_stack(cpu);
    set_nz(cpu, cpu->registers.dbr);
    end_op(cpu);
}

void schedule_pld(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pld_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pld_2;
    cpu->data_width_byte = false;
}

void pld_1(WDC65816* cpu) {
    cpu->registers.d.low = pull_stack(cpu);
}

void pld_2(WDC65816* cpu) {
    cpu->registers.d.high = pull_stack(cpu);
    set_nz(cpu, cpu->registers.d.word);
    end_op(cpu);
}

void schedule_plp(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plp_1;
}

void plp_1(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, FLAG_I);
    cpu->registers.p = pull_stack(cpu);

    if (get_flag(cpu, FLAG_X)) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
    }
    end_op_flag_i(cpu, flag_i);
}

void schedule_pull_index(WDC65816* cpu) {
    cpu->data_width_byte = get_flag(cpu, FLAG_X);
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    
    if (cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pull_index_8_1;
        return;
    }

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pull_index_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pull_index_16_2;
}

void pull_index_8_1(WDC65816* cpu) {
    cpu->index_register->low = pull_stack(cpu);
    set_nz(cpu, cpu->index_register->low);
    end_op(cpu);
}

void pull_index_16_1(WDC65816* cpu) {
    cpu->index_register->low = pull_stack(cpu);
}

void pull_index_16_2(WDC65816* cpu) {
    cpu->index_register->high = pull_stack(cpu);
    set_nz(cpu, cpu->index_register->word);
    end_op(cpu);
}

void schedule_plx(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.x;
    schedule_pull_index(cpu);
}

void schedule_ply(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.y;
    schedule_pull_index(cpu);
}

void schedule_rep(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rep_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rep_2;
}

void rep_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void rep_2(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, FLAG_I);
    cpu->registers.p &= ~cpu->operand;
    end_op_flag_i(cpu, flag_i);
}

void schedule_rol(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_rol;
    schedule_rmw(cpu);
}

void schedule_rol_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_rol;
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_ror(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_ror;
    schedule_rmw(cpu);
}

void schedule_ror_a(WDC65816* cpu) {
    cpu->op_algorithm = &algorithm_ror;
    setup_alu_a(cpu);
    schedule_alu_implicit(cpu);
}

void schedule_rts(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rts_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rts_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rts_3;

}

void rts_1(WDC65816* cpu) {
    cpu->registers.pc.low = pull_stack(cpu);
}

void rts_2(WDC65816* cpu) {
    cpu->registers.pc.high = pull_stack(cpu);
    cpu->registers.pc.word++;
}

void rts_3(WDC65816* cpu) {
    dummy_read_sp(cpu);
    end_op(cpu);
}

void schedule_rtl(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rts_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rts_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rtl_1;
}

void rtl_1(WDC65816* cpu) {
    cpu->registers.pbr = pull_stack(cpu);
    end_op(cpu);
}


void schedule_rti(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rti_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rti_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rti_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rti_4;
}

void rti_1(WDC65816* cpu) {
    cpu->registers.p = pull_stack(cpu);
    if (get_flag(cpu, FLAG_X)) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
    }
}

void rti_2(WDC65816* cpu) {
    cpu->registers.pc.low = pull_stack(cpu);
}

void rti_3(WDC65816* cpu) {
    cpu->registers.pc.high = pull_stack(cpu);
}

void rti_4(WDC65816* cpu) {
    cpu->registers.pbr = pull_stack(cpu);
    end_op(cpu);
}

void schedule_flag_set(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &flag_set_1;
}

void flag_set_1(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, FLAG_I); 
    set_flag(cpu, cpu->operation_flag, true);
    end_op_flag_i(cpu, flag_i);
}

void schedule_sec(WDC65816* cpu) {
    cpu->operation_flag = FLAG_C;
    schedule_flag_set(cpu);
}

void schedule_sed(WDC65816* cpu) {
    cpu->operation_flag = FLAG_D;
    schedule_flag_set(cpu);
}

void schedule_sei(WDC65816* cpu) {
    cpu->operation_flag = FLAG_I;
    schedule_flag_set(cpu);
}

void schedule_sep(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sep_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sep_2;
}

void sep_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void sep_2(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, FLAG_I);
    cpu->registers.p |= cpu->operand;

    if (get_flag(cpu, FLAG_X)) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
    }
    end_op_flag_i(cpu, flag_i);
}

void schedule_store_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_8_1;
}

void schedule_store_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_16_2;
}

void store_8_1(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand & 0xFF);
    end_op(cpu);
}

void store_16_1(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand & 0xFF);
    increment_address(cpu);
}

void store_16_2(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand >> 8);
    end_op(cpu);
}

void schedule_sta(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.word;
    if (get_flag(cpu, FLAG_M)) {
        schedule_store_8(cpu);
        return;
    }
    schedule_store_16(cpu);
}

void schedule_store_indirect(WDC65816* cpu) {
    if (get_flag(cpu, FLAG_X)) {
        schedule_store_8(cpu);
        return;
    }
    schedule_store_16(cpu);
}

void schedule_stx(WDC65816* cpu) {
    cpu->operand = cpu->registers.x.word;
    schedule_store_indirect(cpu);
}

void schedule_sty(WDC65816* cpu) {
    cpu->operand = cpu->registers.y.word;
    schedule_store_indirect(cpu);
}

void schedule_stz(WDC65816* cpu) {
    cpu->operand = 0x00;
    if (get_flag(cpu, FLAG_M)) {
        schedule_store_8(cpu);
        return;
    }
    schedule_store_16(cpu);
}

void schedule_ta_index(WDC65816* cpu) {
    cpu->data_width_byte = get_flag(cpu, FLAG_X);

    if(cpu->data_width_byte) {
        cpu->cycle_lookup[cpu->cycle_lookup_index++] = &ta_index_8_1;
        return;
    }

    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &ta_index_16_1;
}

void ta_index_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.a.low, &cpu->index_register->low);
    end_op(cpu);
}

void ta_index_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.a.word, &cpu->index_register->word);
    end_op(cpu);
}

void schedule_tax(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.x;
    schedule_ta_index(cpu);
}

void schedule_tay(WDC65816* cpu) {
    cpu->index_register = &cpu->registers.y;
    schedule_ta_index(cpu);
}

void schedule_tcd(WDC65816* cpu) {
    cpu->data_width_byte = false;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tcd_1;
}

void tcd_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.a.word, &cpu->registers.d.word);
    end_op(cpu);
}

void schedule_tcs(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tcs_1;
}

void tcs_1(WDC65816* cpu) {
    cpu->registers.s.word = cpu->registers.a.word;
    end_op(cpu);
}

void schedule_tdc(WDC65816* cpu) {
    cpu->data_width_byte = false;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tdc_1;
}

void tdc_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.d.word, &cpu->registers.a.word);
    end_op(cpu);
}