#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/processor/spc700/spc700.h"
#include "../include/processor/spc700/instruction.h"
#include "../include/processor/spc700/addressing.h"

#define EXEC_OP(OPCODE, ADDR, INSTR, DUMMY) \
    case OPCODE: \
        cpu->dummy_read = DUMMY; \
        ADDR(cpu); \
        INSTR(cpu); \
        break

#define EXEC_OP_IMPL(OPCODE, INSTR, DUMMY) \
    case OPCODE: \
        cpu->dummy_read = DUMMY; \
        INSTR(cpu); \
        break

typedef uint8_t (*algorithm)(SPC700* cpu, uint8_t a, uint8_t b);

const static int direct_page_base[2] = {0x0000, 0x0100}; 

const static int cycle_table[0x100] = {
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6, 8, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 4, 6, 
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 5, 2, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3, 8, 
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 6, 6, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 4, 5, 2, 2, 4, 3, 
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5, 5, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 6, 
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 2, 4, 5, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 12, 5, 
    3, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 2, 4, 4, 
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 4, 
    3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4, 9, 
    2, 8, 4, 5, 5, 6, 6, 7, 4, 5, 5, 5, 2, 2, 6, 3, 
    2, 8, 4, 5, 3, 4, 3, 6, 2, 4, 5, 3, 4, 3, 4, 3, 
    2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 4, 2, 
};

static bool get_bit(int value, int index);

static uint16_t get_ya(SPC700* cpu);
static void set_ya(SPC700* cpu, uint16_t value);

static int get_dp_base(SPC700* cpu);

static bool get_flag(SPC700* cpu, SPC700_flag flag);
static void set_flag(SPC700* cpu, SPC700_flag flag, bool value);

static uint8_t read(SPC700* cpu, uint16_t address);
static void write(SPC700* cpu, uint16_t address, uint8_t data);

static void push_stack(SPC700* cpu, uint8_t data);
static uint8_t pop_stack(SPC700* cpu);

static uint16_t read_dp_16(SPC700* cpu, uint8_t address);
static void write_dp_16(SPC700* cpu, uint8_t address, uint16_t data);

static void store_result(SPC700* cpu);

static void load_operand(SPC700* cpu);
static void load_operand_dp_16(SPC700* cpu);
static uint8_t load_operand_mem_bit(SPC700* cpu);

static uint8_t read_immediate(SPC700* cpu);

static void set_nz_8(SPC700* cpu, uint8_t value);
static void set_nz_16(SPC700* cpu, uint16_t value);

static void transfer_reg(SPC700* cpu, uint8_t* dest, uint8_t source);

static void handle_op_indr_indr(SPC700* cpu, algorithm op);
static void handle_op_dp_dp(SPC700* cpu, algorithm op);
static void handle_op_dp_imm(SPC700* cpu, algorithm op);

static void handle_branch(SPC700* cpu, bool take_branch);

static uint8_t algorithm_adc(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);
static uint8_t algorithm_sbc(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);
static uint8_t algorithm_cmp(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);
static uint8_t algorithm_and(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);
static uint8_t algorithm_or(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);
static uint8_t algorithm_eor(SPC700* cpu, uint8_t operand_a, uint8_t operand_b);

static uint8_t algorithm_asl(SPC700* cpu, uint8_t operand);
static uint8_t algorithm_lsr(SPC700* cpu, uint8_t operand);
static uint8_t algorithm_rol(SPC700* cpu, uint8_t operand);
static uint8_t algorithm_ror(SPC700* cpu, uint8_t operand);

void init_spc700(SPC700* cpu, void* bus) {
    cpu->bus = bus;
}

bool get_bit(int value, int index) {
    return (value >> index) & 0x1;
}

uint8_t spc700_read_immediate(SPC700* cpu) {
    return read_immediate(cpu);
}

void spc700_run_instruction(SPC700* cpu, uint8_t opcode) {
    cpu->opcode = opcode;
    cpu->cycle_count = spc700_get_op_time(opcode);

    //printf("SPC Opcode: %02X\n", opcode);

    switch (opcode) {
        EXEC_OP(0xE8, addr_imm, mov_a_mem, false);
        EXEC_OP(0xE6, addr_indr, mov_a_mem, false);
        EXEC_OP(0xBF, addr_indr_inx, mov_a_mem, false);
        EXEC_OP(0xE4, addr_dp, mov_a_mem, false);
        EXEC_OP(0xF4, addr_dp_x, mov_a_mem, false);
        EXEC_OP(0xE5, addr_a, mov_a_mem, false);
        EXEC_OP(0xF5, addr_a_x, mov_a_mem, false);
        EXEC_OP(0xF6, addr_a_y, mov_a_mem, false);
        EXEC_OP(0xE7, addr_dp_x_indr, mov_a_mem, false);
        EXEC_OP(0xF7, addr_dp_indr_y, mov_a_mem, false);

        EXEC_OP(0xCD, addr_imm, mov_x_mem, false);
        EXEC_OP(0xF8, addr_dp, mov_x_mem, false);
        EXEC_OP(0xF9, addr_dp_y, mov_x_mem, false);
        EXEC_OP(0xE9, addr_a, mov_x_mem, false);

        EXEC_OP(0x8D, addr_imm, mov_y_mem, false);
        EXEC_OP(0xEB, addr_dp, mov_y_mem, false);
        EXEC_OP(0xFB, addr_dp_x, mov_y_mem, false);
        EXEC_OP(0xEC, addr_a, mov_y_mem, false);

        EXEC_OP(0xC6, addr_indr, mov_mem_a, false);
        EXEC_OP(0xAF, addr_indr_inx, mov_mem_a, false);
        EXEC_OP(0xC4, addr_dp, mov_mem_a, false);
        EXEC_OP(0xD4, addr_dp_x, mov_mem_a, false);
        EXEC_OP(0xC5, addr_a, mov_mem_a, false);
        EXEC_OP(0xD5, addr_a_x, mov_mem_a, false);
        EXEC_OP(0xD6, addr_a_y, mov_mem_a, false);
        EXEC_OP(0xC7, addr_dp_x_indr, mov_mem_a, false);
        EXEC_OP(0xD7, addr_dp_indr_y, mov_mem_a, false);

        EXEC_OP(0xD8, addr_dp, mov_mem_x, false);
        EXEC_OP(0xD9, addr_dp_y, mov_mem_x, false);
        EXEC_OP(0xC9, addr_a, mov_mem_x, false);

        EXEC_OP(0xCB, addr_dp, mov_mem_y, false);
        EXEC_OP(0xDB, addr_dp_x, mov_mem_y, false);
        EXEC_OP(0xCC, addr_a, mov_mem_y, false);

        EXEC_OP_IMPL(0x7D, mov_a_x, false);
        EXEC_OP_IMPL(0xDD, mov_a_y, false);
        EXEC_OP_IMPL(0x5D, mov_x_a, false);
        EXEC_OP_IMPL(0xFD, mov_y_a, false);
        EXEC_OP_IMPL(0x9D, mov_x_sp, false);
        EXEC_OP_IMPL(0xBD, mov_sp_x, false);
        EXEC_OP_IMPL(0xFA, mov_dp_dp, false);
        EXEC_OP_IMPL(0x8F, mov_dp_imm, false);

        EXEC_OP(0x88, addr_imm, adc_a, false);
        EXEC_OP(0x86, addr_indr, adc_a, false);
        EXEC_OP(0x84, addr_dp, adc_a, false);
        EXEC_OP(0x94, addr_dp_x, adc_a, false);
        EXEC_OP(0x85, addr_a, adc_a, false);
        EXEC_OP(0x95, addr_a_x, adc_a, false);
        EXEC_OP(0x96, addr_a_y, adc_a, false);
        EXEC_OP(0x87, addr_dp_x_indr, adc_a, false);
        EXEC_OP(0x97, addr_dp_indr_y, adc_a, false);
        EXEC_OP_IMPL(0x99, adc_indr_indr, false);
        EXEC_OP_IMPL(0x89, adc_dp_dp, false);
        EXEC_OP_IMPL(0x98, adc_dp_imm, false);

        EXEC_OP(0xA8, addr_imm, sbc_a, false);
        EXEC_OP(0xA6, addr_indr, sbc_a, false);
        EXEC_OP(0xA4, addr_dp, sbc_a, false);
        EXEC_OP(0xB4, addr_dp_x, sbc_a, false);
        EXEC_OP(0xA5, addr_a, sbc_a, false);
        EXEC_OP(0xB5, addr_a_x, sbc_a, false);
        EXEC_OP(0xB6, addr_a_y, sbc_a, false);
        EXEC_OP(0xA7, addr_dp_x_indr, sbc_a, false);
        EXEC_OP(0xB7, addr_dp_indr_y, sbc_a, false);
        EXEC_OP_IMPL(0xB9, sbc_indr_indr, false);
        EXEC_OP_IMPL(0xA9, sbc_dp_dp, false);
        EXEC_OP_IMPL(0xB8, sbc_dp_imm, false);

        EXEC_OP(0x68, addr_imm, cmp_a, false);
        EXEC_OP(0x66, addr_indr, cmp_a, false);
        EXEC_OP(0x64, addr_dp, cmp_a, false);
        EXEC_OP(0x74, addr_dp_x, cmp_a, false);
        EXEC_OP(0x65, addr_a, cmp_a, false);
        EXEC_OP(0x75, addr_a_x, cmp_a, false);
        EXEC_OP(0x76, addr_a_y, cmp_a, false);
        EXEC_OP(0x67, addr_dp_x_indr, cmp_a, false);
        EXEC_OP(0x77, addr_dp_indr_y, cmp_a, false);
        EXEC_OP_IMPL(0x79, cmp_indr_indr, true);
        EXEC_OP_IMPL(0x69, cmp_dp_dp, true);
        EXEC_OP_IMPL(0x78, cmp_dp_imm, true);

        EXEC_OP(0xC8, addr_imm, cmp_x, false);
        EXEC_OP(0x3E, addr_dp, cmp_x, false);
        EXEC_OP(0x1E, addr_a, cmp_x, false);

        EXEC_OP(0xAD, addr_imm, cmp_y, false);
        EXEC_OP(0x7E, addr_dp, cmp_y, false);
        EXEC_OP(0x5E, addr_a, cmp_y, false);

        EXEC_OP(0x28, addr_imm, and_a, false);
        EXEC_OP(0x26, addr_indr, and_a, false);
        EXEC_OP(0x24, addr_dp, and_a, false);
        EXEC_OP(0x34, addr_dp_x, and_a, false);
        EXEC_OP(0x25, addr_a, and_a, false);
        EXEC_OP(0x35, addr_a_x, and_a, false);
        EXEC_OP(0x36, addr_a_y, and_a, false);
        EXEC_OP(0x27, addr_dp_x_indr, and_a, false);
        EXEC_OP(0x37, addr_dp_indr_y, and_a, false);
        EXEC_OP_IMPL(0x39, and_indr_indr, false);
        EXEC_OP_IMPL(0x29, and_dp_dp, false);
        EXEC_OP_IMPL(0x38, and_dp_imm, false);

        EXEC_OP(0x08, addr_imm, or_a, false);
        EXEC_OP(0x06, addr_indr, or_a, false);
        EXEC_OP(0x04, addr_dp, or_a, false);
        EXEC_OP(0x14, addr_dp_x, or_a, false);
        EXEC_OP(0x05, addr_a, or_a, false);
        EXEC_OP(0x15, addr_a_x, or_a, false);
        EXEC_OP(0x16, addr_a_y, or_a, false);
        EXEC_OP(0x07, addr_dp_x_indr, or_a, false);
        EXEC_OP(0x17, addr_dp_indr_y, or_a, false);
        EXEC_OP_IMPL(0x19, or_indr_indr, false);
        EXEC_OP_IMPL(0x09, or_dp_dp, false);
        EXEC_OP_IMPL(0x18, or_dp_imm, false);

        EXEC_OP(0x48, addr_imm, eor_a, false);
        EXEC_OP(0x46, addr_indr, eor_a, false);
        EXEC_OP(0x44, addr_dp, eor_a, false);
        EXEC_OP(0x54, addr_dp_x, eor_a, false);
        EXEC_OP(0x45, addr_a, eor_a, false);
        EXEC_OP(0x55, addr_a_x, eor_a, false);
        EXEC_OP(0x56, addr_a_y, eor_a, false);
        EXEC_OP(0x47, addr_dp_x_indr, eor_a, false);
        EXEC_OP(0x57, addr_dp_indr_y, eor_a, false);
        EXEC_OP_IMPL(0x59, eor_indr_indr, false);
        EXEC_OP_IMPL(0x49, eor_dp_dp, false);
        EXEC_OP_IMPL(0x58, eor_dp_imm, false);

        EXEC_OP_IMPL(0xBC, inc_a, false);
        EXEC_OP(0xAB, addr_dp, inc_mem, false);
        EXEC_OP(0xBB, addr_dp_x, inc_mem, false);
        EXEC_OP(0xAC, addr_a, inc_mem, false);

        EXEC_OP_IMPL(0x3D, inc_x, false);
        EXEC_OP_IMPL(0xFC, inc_y, false);

        EXEC_OP_IMPL(0x9C, dec_a, false);
        EXEC_OP(0x8B, addr_dp, dec_mem, false);
        EXEC_OP(0x9B, addr_dp_x, dec_mem, false);
        EXEC_OP(0x8C, addr_a, dec_mem, false);

        EXEC_OP_IMPL(0x1D, dec_x, false);
        EXEC_OP_IMPL(0xDC, dec_y, false);

        EXEC_OP_IMPL(0x1C, asl_a, false);
        EXEC_OP(0x0B, addr_dp, asl_mem, false);
        EXEC_OP(0x1B, addr_dp_x, asl_mem, false);
        EXEC_OP(0x0C, addr_a, asl_mem, false);

        EXEC_OP_IMPL(0x5C, lsr_a, false);
        EXEC_OP(0x4B, addr_dp, lsr_mem, false);
        EXEC_OP(0x5B, addr_dp_x, lsr_mem, false);
        EXEC_OP(0x4C, addr_a, lsr_mem, false);

        EXEC_OP_IMPL(0x3C, rol_a, false);
        EXEC_OP(0x2B, addr_dp, rol_mem, false);
        EXEC_OP(0x3B, addr_dp_x, rol_mem, false);
        EXEC_OP(0x2C, addr_a, rol_mem, false);

        EXEC_OP_IMPL(0x7C, ror_a, false);
        EXEC_OP(0x6B, addr_dp, ror_mem, false);
        EXEC_OP(0x7B, addr_dp_x, ror_mem, false);
        EXEC_OP(0x6C, addr_a, ror_mem, false);

        EXEC_OP_IMPL(0x9F, xcn_a, false);

        EXEC_OP(0xBA, addr_dp, movw_ya_dp, false);
        EXEC_OP(0xDA, addr_dp, movw_dp_ya, false);

        EXEC_OP(0x3A, addr_dp, incw_dp, false);
        EXEC_OP(0x1A, addr_dp, decw_dp, false);

        EXEC_OP(0x7A, addr_dp, addw_dp, false);
        EXEC_OP(0x9A, addr_dp, subw_dp, false);
        EXEC_OP(0x5A, addr_dp, cmpw_dp, false);

        EXEC_OP_IMPL(0xCF, mul_ya, false);
        EXEC_OP_IMPL(0x9E, div_ya, false);

        EXEC_OP_IMPL(0xDF, daa, false);
        EXEC_OP_IMPL(0xBE, das, false);

        EXEC_OP_IMPL(0x2F, bra, false);
        EXEC_OP_IMPL(0xF0, beq, false);
        EXEC_OP_IMPL(0xD0, bne, false);
        EXEC_OP_IMPL(0xB0, bcs, false);
        EXEC_OP_IMPL(0x90, bcc, false);
        EXEC_OP_IMPL(0x70, bvs, false);
        EXEC_OP_IMPL(0x50, bvc, false);
        EXEC_OP_IMPL(0x30, bmi, false);
        EXEC_OP_IMPL(0x10, bpl, false);

        EXEC_OP(0x03, addr_dp, bbs, false);
        EXEC_OP(0x23, addr_dp, bbs, false);
        EXEC_OP(0x43, addr_dp, bbs, false);
        EXEC_OP(0x63, addr_dp, bbs, false);
        EXEC_OP(0x83, addr_dp, bbs, false);
        EXEC_OP(0xA3, addr_dp, bbs, false);
        EXEC_OP(0xC3, addr_dp, bbs, false);
        EXEC_OP(0xE3, addr_dp, bbs, false);

        EXEC_OP(0x13, addr_dp, bbc, false);
        EXEC_OP(0x33, addr_dp, bbc, false);
        EXEC_OP(0x53, addr_dp, bbc, false);
        EXEC_OP(0x73, addr_dp, bbc, false);
        EXEC_OP(0x93, addr_dp, bbc, false);
        EXEC_OP(0xB3, addr_dp, bbc, false);
        EXEC_OP(0xD3, addr_dp, bbc, false);
        EXEC_OP(0xF3, addr_dp, bbc, false);

        EXEC_OP(0x2E, addr_dp, cbne, false);
        EXEC_OP(0xDE, addr_dp_x, cbne, false);

        EXEC_OP(0x6E, addr_dp, dbnz_mem, false);
        EXEC_OP_IMPL(0xFE, dbnz_y, false);

        EXEC_OP(0x5F, addr_a, jmp, false);
        EXEC_OP(0x1F, addr_a_x_indr, jmp, false);

        EXEC_OP(0x3F, addr_a, call, false);
        EXEC_OP_IMPL(0x4F, pcall, false);

        EXEC_OP_IMPL(0x01, tcall, false);
        EXEC_OP_IMPL(0x11, tcall, false);
        EXEC_OP_IMPL(0x21, tcall, false);
        EXEC_OP_IMPL(0x31, tcall, false);
        EXEC_OP_IMPL(0x41, tcall, false);
        EXEC_OP_IMPL(0x51, tcall, false);
        EXEC_OP_IMPL(0x61, tcall, false);
        EXEC_OP_IMPL(0x71, tcall, false);
        EXEC_OP_IMPL(0x81, tcall, false);
        EXEC_OP_IMPL(0x91, tcall, false);
        EXEC_OP_IMPL(0xA1, tcall, false);
        EXEC_OP_IMPL(0xB1, tcall, false);
        EXEC_OP_IMPL(0xC1, tcall, false);
        EXEC_OP_IMPL(0xD1, tcall, false);
        EXEC_OP_IMPL(0xE1, tcall, false);
        EXEC_OP_IMPL(0xF1, tcall, false);

        EXEC_OP_IMPL(0x0F, brk, false);

        EXEC_OP_IMPL(0x6F, ret, false);
        EXEC_OP_IMPL(0x7F, reti, false);

        EXEC_OP_IMPL(0x2D, push_a, false);
        EXEC_OP_IMPL(0x4D, push_x, false);
        EXEC_OP_IMPL(0x6D, push_y, false);
        EXEC_OP_IMPL(0x0D, push_psw, false);

        EXEC_OP_IMPL(0xAE, pop_a, false);
        EXEC_OP_IMPL(0xCE, pop_x, false);
        EXEC_OP_IMPL(0xEE, pop_y, false);
        EXEC_OP_IMPL(0x8E, pop_psw, false);

        EXEC_OP(0x02, addr_dp, set_mem_bit, false);
        EXEC_OP(0x22, addr_dp, set_mem_bit, false);
        EXEC_OP(0x42, addr_dp, set_mem_bit, false);
        EXEC_OP(0x62, addr_dp, set_mem_bit, false);
        EXEC_OP(0x82, addr_dp, set_mem_bit, false);
        EXEC_OP(0xA2, addr_dp, set_mem_bit, false);
        EXEC_OP(0xC2, addr_dp, set_mem_bit, false);
        EXEC_OP(0xE2, addr_dp, set_mem_bit, false);

        EXEC_OP(0x12, addr_dp, clr_mem_bit, false);
        EXEC_OP(0x32, addr_dp, clr_mem_bit, false);
        EXEC_OP(0x52, addr_dp, clr_mem_bit, false);
        EXEC_OP(0x72, addr_dp, clr_mem_bit, false);
        EXEC_OP(0x92, addr_dp, clr_mem_bit, false);
        EXEC_OP(0xB2, addr_dp, clr_mem_bit, false);
        EXEC_OP(0xD2, addr_dp, clr_mem_bit, false);
        EXEC_OP(0xF2, addr_dp, clr_mem_bit, false);

        EXEC_OP(0x0E, addr_a, tset1, false);
        EXEC_OP(0x4E, addr_a, tclr1, false);

        EXEC_OP(0x4A, addr_a, and1, false);
        EXEC_OP(0x6A, addr_a, nand1, false);

        EXEC_OP(0x0A, addr_a, or1, false);
        EXEC_OP(0x2A, addr_a, nor1, false);

        EXEC_OP(0x8A, addr_a, eor1, false);

        EXEC_OP(0xEA, addr_a, not1, false);

        EXEC_OP(0xAA, addr_a, mov1_c_mem, false);
        EXEC_OP(0xCA, addr_a, mov1_mem_c, false);

        EXEC_OP_IMPL(0x60, clcr, false);
        EXEC_OP_IMPL(0x80, setc, false);
        EXEC_OP_IMPL(0xED, notc, false);
        EXEC_OP_IMPL(0xE0, clrv, false);
        EXEC_OP_IMPL(0x20, clrp, false);
        EXEC_OP_IMPL(0x40, sep, false);
        EXEC_OP_IMPL(0xA0, ei, false);
        EXEC_OP_IMPL(0xC0, di, false);

        default: nop(cpu);
    }
}

void spc700_print_state(SPC700* cpu) {
    printf("A: %u\n", cpu->registers.a);
    printf("X: %u\n", cpu->registers.x);
    printf("Y: %u\n", cpu->registers.y);
    printf("SP: %u\n", cpu->registers.sp);
    printf("PC: %u\n", cpu->registers.pc);
    printf("YA: %u\n\n", get_ya(cpu));

    printf("PSW: %u\n\n", cpu->registers.psw);

    printf("N: %u\n", get_flag(cpu, FLAG_N));
    printf("V: %u\n", get_flag(cpu, FLAG_V));
    printf("P: %u\n", get_flag(cpu, FLAG_P));
    printf("H: %u\n", get_flag(cpu, FLAG_H));
    printf("I: %u\n", get_flag(cpu, FLAG_I));
    printf("Z: %u\n", get_flag(cpu, FLAG_Z));
    printf("C: %u\n\n", get_flag(cpu, FLAG_C));

    printf("Operand address: %u\n\n", cpu->operand_address);
    printf("Operand: %u\n\n", cpu->operand);
    //printf("Indirect address: %u\n\n", cpu->indirect_address);
}

uint16_t get_ya(SPC700* cpu) {
    return (cpu->registers.y << 8) | cpu->registers.a;
}

void set_ya(SPC700* cpu, uint16_t value) {
    cpu->registers.y = value >> 8;
    cpu->registers.a = value & 0xFF;
}

int get_dp_base(SPC700* cpu) {
    bool p = get_flag(cpu, FLAG_P);
    return direct_page_base[p];
}

bool get_flag(SPC700* cpu, SPC700_flag flag) {
    return get_bit(cpu->registers.psw, flag);
}

void set_flag(SPC700* cpu, SPC700_flag flag, bool value) {
    if (value) {
        cpu->registers.psw |= 1 << flag;
        return;
    }
    cpu->registers.psw &= ~(1 << flag);
}

uint8_t read(SPC700* cpu, uint16_t address) {
    return cpu->bus->read(cpu->bus, address);
}

void write(SPC700* cpu, uint16_t address, uint8_t data) {
    cpu->bus->write(cpu->bus, address, data);
}

uint8_t read_immediate(SPC700* cpu) {
    return read(cpu, cpu->registers.pc++);
}

void push_stack(SPC700* cpu, uint8_t data) {
    uint16_t addr = cpu->registers.sp-- | 0x100;
    write(cpu, addr, data);
}

uint8_t pop_stack(SPC700* cpu) {
    uint16_t addr = ++cpu->registers.sp | 0x100;
    return read(cpu, addr);
}

uint16_t read_dp_16(SPC700* cpu, uint8_t address) {
    uint16_t dp = address | get_dp_base(cpu);
    uint16_t result = read(cpu, dp++);
    result |= read(cpu, (dp & 0xFF) | get_dp_base(cpu)) << 8;
    return result;
}

void write_dp_16(SPC700* cpu, uint8_t address, uint16_t data) {
    uint16_t dp = address | get_dp_base(cpu);
    write(cpu, dp++, data & 0xFF);
    write(cpu, (dp & 0xFF) | get_dp_base(cpu), data >> 8);
}

void load_operand(SPC700* cpu) {
    cpu->operand = read(cpu, cpu->operand_address);
}

void load_operand_dp_16(SPC700* cpu) {
    cpu->operand = read_dp_16(cpu, cpu->operand_address);
}

uint8_t load_operand_mem_bit(SPC700* cpu) {
    uint8_t index = cpu->operand_address >> 0xD;
    cpu->operand_address &= 0x1FFF;
    load_operand(cpu);
    return index;
}

void store_result(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->operand);
}

void transfer_reg(SPC700* cpu, uint8_t* dest, uint8_t source) {
    *dest = source;
    set_nz_8(cpu, source);
}

void handle_branch(SPC700* cpu, bool take_branch) {
    int8_t offset = (int8_t) read_immediate(cpu);
    
    if (take_branch) {
        cpu->registers.pc += offset;
        cpu->cycle_count += 2;
    }

    cpu->branch_taken = take_branch;
}

uint8_t algorithm_adc(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    bool h = (operand_a & 0xF) + (operand_b & 0xF) + get_flag(cpu, FLAG_C) > 0xF;
    int result = operand_a + operand_b + get_flag(cpu, FLAG_C);
    set_flag(cpu, FLAG_H, h);
    set_flag(cpu, FLAG_C, result > 0xFF);
    set_flag(cpu, FLAG_V, (result ^ operand_a) & (result ^ operand_b) & 0x80);
    set_nz_8(cpu, result);
    return result & 0xFF;
}

uint8_t algorithm_sbc(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    operand_b = ~operand_b;
    return algorithm_adc(cpu, operand_a, operand_b);
}

uint8_t algorithm_cmp(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    operand_b = ~operand_b;
    int result = operand_a + operand_b + 1;
    set_flag(cpu, FLAG_C, result > 0xFF);
    set_nz_8(cpu, result);
    return result & 0xFF;
}

uint8_t algorithm_and(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    uint8_t result = operand_a & operand_b;
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_or(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    uint8_t result = operand_a | operand_b;
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_eor(SPC700* cpu, uint8_t operand_a, uint8_t operand_b) {
    uint8_t result = operand_a ^ operand_b;
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_asl(SPC700* cpu, uint8_t operand) {
    uint8_t result = operand << 1;
    bool c = operand >> 7;
    set_flag(cpu, FLAG_C, c);
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_lsr(SPC700* cpu, uint8_t operand) {
    uint8_t result = operand >> 1;
    bool c = operand & 0x1;
    set_flag(cpu, FLAG_C, c);
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_rol(SPC700* cpu, uint8_t operand) {
    uint8_t result = (operand << 1) | get_flag(cpu, FLAG_C);
    bool c = operand >> 7;
    set_flag(cpu, FLAG_C, c);
    set_nz_8(cpu, result);
    return result;
}

uint8_t algorithm_ror(SPC700* cpu, uint8_t operand) {
    uint8_t result = (operand >> 1) | get_flag(cpu, FLAG_C) << 7;
    bool c = operand & 0x1;
    set_flag(cpu, FLAG_C, c);
    set_nz_8(cpu, result);
    return result;
}

void set_nz_8(SPC700* cpu, uint8_t value) {
    set_flag(cpu, FLAG_N, value >> 0x7);
    set_flag(cpu, FLAG_Z, value == 0);
}

void set_nz_16(SPC700* cpu, uint16_t value) {
    set_flag(cpu, FLAG_N, value >> 0xF);
    set_flag(cpu, FLAG_Z, value == 0);
}

int spc700_get_op_time(uint8_t opcode) {
    return cycle_table[opcode];
}

void handle_op_indr_indr(SPC700* cpu, algorithm op) {
    uint16_t addr_x = cpu->registers.x | get_dp_base(cpu), addr_y = cpu->registers.y | get_dp_base(cpu);
    uint8_t source = read(cpu, addr_y), dest = read(cpu, addr_x);
    uint8_t result = op(cpu, dest, source);
    (cpu->dummy_read) ? read(cpu, dest) : write(cpu, addr_x, result);
}

void handle_op_dp_dp(SPC700* cpu, algorithm op) {
    uint16_t addr_s = read_immediate(cpu) | get_dp_base(cpu), addr_d = read_immediate(cpu) | get_dp_base(cpu);
    uint8_t source = read(cpu, addr_s), dest = read(cpu, addr_d);
    uint8_t result = op(cpu, dest, source);
    (cpu->dummy_read) ? read(cpu, dest) : write(cpu, addr_d, result);
}

void handle_op_dp_imm(SPC700* cpu, algorithm op) {
    uint8_t source = read_immediate(cpu);
    uint16_t addr_d = read_immediate(cpu) | get_dp_base(cpu);
    uint8_t dest = read(cpu, addr_d);
    uint8_t result = op(cpu, dest, source);
    (cpu->dummy_read) ? read(cpu, dest) : write(cpu, addr_d, result);
}

uint8_t get_mem_bit(SPC700* cpu) {
    uint16_t addr = cpu->operand_address & 0x1FFF;
    uint8_t index = cpu->operand_address >> 0xA;
    cpu->operand_address = addr;
    return index;
}

void addr_imm(SPC700* cpu) {
    cpu->operand_address = cpu->registers.pc++;
}

void addr_indr(SPC700* cpu) {
    cpu->operand_address = cpu->registers.x | get_dp_base(cpu);
}

void addr_indr_inx(SPC700* cpu) {
    cpu->operand_address = cpu->registers.x++ | get_dp_base(cpu);
}

void addr_dp(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) | get_dp_base(cpu);
}

void addr_dp_x(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) + cpu->registers.x;
    cpu->operand_address = (cpu->operand_address & 0xFF) | get_dp_base(cpu);
}

void addr_dp_y(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) + cpu->registers.y;
    cpu->operand_address = (cpu->operand_address & 0xFF) | get_dp_base(cpu);
}

void addr_a(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu);
    cpu->operand_address |= read_immediate(cpu) << 8;
}

void addr_a_x(SPC700* cpu) {
    addr_a(cpu);
    cpu->operand_address += cpu->registers.x;
}

void addr_a_y(SPC700* cpu) {
    addr_a(cpu);
    cpu->operand_address += cpu->registers.y;
}

void addr_dp_x_indr(SPC700* cpu) {
    uint16_t indr_low = ((read_immediate(cpu) + cpu->registers.x) & 0xFF) | get_dp_base(cpu);
    uint16_t indr_high = ((indr_low + 1) & 0xFF) | get_dp_base(cpu);
    cpu->operand_address = (read(cpu, indr_high) << 8) | read(cpu, indr_low);
}

void addr_dp_indr_y(SPC700* cpu) {
    uint16_t indr_low = read_immediate(cpu) | get_dp_base(cpu);
    uint16_t indr_high = ((indr_low + 1) & 0xFF) | get_dp_base(cpu);
    cpu->operand_address = (read(cpu, indr_high) << 8) | read(cpu, indr_low);
    cpu->operand_address += cpu->registers.y;
}

void addr_a_x_indr(SPC700* cpu) {
    uint16_t indr = read_immediate(cpu);
    indr |= read_immediate(cpu) << 8;
    indr += cpu->registers.x;
    cpu->operand_address = read(cpu, indr++);
    cpu->operand_address |= read(cpu, indr) << 8;
}

void mov_a_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = cpu->operand;
    set_nz_8(cpu, cpu->operand);
}

void mov_x_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.x = cpu->operand;
    set_nz_8(cpu, cpu->operand);
}

void mov_y_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.y = cpu->operand;
    set_nz_8(cpu, cpu->operand);
}

void mov_mem_a(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.a);
}

void mov_mem_x(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.x);
}

void mov_mem_y(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.y);
}

void mov_a_x(SPC700* cpu) {
    transfer_reg(cpu, &cpu->registers.a, cpu->registers.x);
}

void mov_a_y(SPC700* cpu) {
    transfer_reg(cpu, &cpu->registers.a, cpu->registers.y);
}

void mov_x_a(SPC700* cpu) {
    transfer_reg(cpu, &cpu->registers.x, cpu->registers.a);
}

void mov_y_a(SPC700* cpu) {
    transfer_reg(cpu, &cpu->registers.y, cpu->registers.a);
}

void mov_x_sp(SPC700* cpu) {
    transfer_reg(cpu, &cpu->registers.x, cpu->registers.sp);
}

void mov_sp_x(SPC700* cpu) {
    cpu->registers.sp = cpu->registers.x;
}

void mov_dp_dp(SPC700* cpu) {
    uint16_t addr_s = read_immediate(cpu) | get_dp_base(cpu), addr_d = read_immediate(cpu) | get_dp_base(cpu);
    uint8_t source = read(cpu, addr_s);
    write(cpu, addr_d, source);
}

void mov_dp_imm(SPC700* cpu) {
    uint8_t source = read_immediate(cpu);
    uint16_t addr_d = read_immediate(cpu) | get_dp_base(cpu);
    write(cpu, addr_d, source);
}

void adc_a(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = algorithm_adc(cpu, cpu->registers.a, cpu->operand);
}

void adc_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_adc);
}

void adc_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_adc);
}

void adc_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_adc);
}

void sbc_a(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = algorithm_sbc(cpu, cpu->registers.a, cpu->operand);
}

void sbc_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_sbc);
}

void sbc_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_sbc);
}

void sbc_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_sbc);
}

void cmp_a(SPC700* cpu) {
    load_operand(cpu);
    algorithm_cmp(cpu, cpu->registers.a, cpu->operand);
}

void cmp_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_cmp);
}

void cmp_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_cmp);
}

void cmp_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_cmp);
}

void cmp_x(SPC700* cpu) {
    load_operand(cpu);
    algorithm_cmp(cpu, cpu->registers.x, cpu->operand);
}

void cmp_y(SPC700* cpu) {
    load_operand(cpu);
    algorithm_cmp(cpu, cpu->registers.y, cpu->operand);
}

void and_a(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = algorithm_and(cpu, cpu->registers.a, cpu->operand);
}

void and_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_and);
}

void and_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_and);
}

void and_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_and);
}

void or_a(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = algorithm_or(cpu, cpu->registers.a, cpu->operand);
}

void or_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_or);
}

void or_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_or);
}

void or_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_or);
}

void eor_a(SPC700* cpu) {
    load_operand(cpu);
    cpu->registers.a = algorithm_eor(cpu, cpu->registers.a, cpu->operand);
}

void eor_indr_indr(SPC700* cpu) {
    handle_op_indr_indr(cpu, &algorithm_eor);
}

void eor_dp_dp(SPC700* cpu) {
    handle_op_dp_dp(cpu, &algorithm_eor);
}

void eor_dp_imm(SPC700* cpu) {
    handle_op_dp_imm(cpu, &algorithm_eor);
}

void inc_a(SPC700* cpu) {
    set_nz_8(cpu, ++cpu->registers.a);
}

void inc_mem(SPC700* cpu) {
    load_operand(cpu);
    set_nz_8(cpu, ++cpu->operand);
    store_result(cpu);
}

void dec_a(SPC700* cpu) {
    set_nz_8(cpu, --cpu->registers.a);
}

void dec_mem(SPC700* cpu) {
    load_operand(cpu);
    set_nz_8(cpu, --cpu->operand);
    store_result(cpu);
}

void inc_x(SPC700* cpu) {
    set_nz_8(cpu, ++cpu->registers.x);
}

void inc_y(SPC700* cpu) {
    set_nz_8(cpu, ++cpu->registers.y);
}

void dec_x(SPC700* cpu) {
    set_nz_8(cpu, --cpu->registers.x);
}

void dec_y(SPC700* cpu) {
    set_nz_8(cpu, --cpu->registers.y);
}

void asl_a(SPC700* cpu) {
    cpu->registers.a = algorithm_asl(cpu, cpu->registers.a);
}

void asl_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->operand = algorithm_asl(cpu, cpu->operand);
    store_result(cpu);
}

void lsr_a(SPC700* cpu) {
    cpu->registers.a = algorithm_lsr(cpu, cpu->registers.a);
}

void lsr_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->operand = algorithm_lsr(cpu, cpu->operand);
    store_result(cpu);
}

void rol_a(SPC700* cpu) {
    cpu->registers.a = algorithm_rol(cpu, cpu->registers.a);
}

void rol_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->operand = algorithm_rol(cpu, cpu->operand);
    store_result(cpu);
}

void ror_a(SPC700* cpu) {
    cpu->registers.a = algorithm_ror(cpu, cpu->registers.a);
}

void ror_mem(SPC700* cpu) {
    load_operand(cpu);
    cpu->operand = algorithm_ror(cpu, cpu->operand);
    store_result(cpu);
}

void xcn_a(SPC700* cpu) {
    uint8_t high = cpu->registers.a >> 0x4, low = cpu->registers.a & 0xF;
    cpu->registers.a = (low << 4) | high;
    set_nz_8(cpu, cpu->registers.a);
}

void movw_ya_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    set_ya(cpu, cpu->operand);
    set_nz_16(cpu, cpu->operand);
}

void movw_dp_ya(SPC700* cpu) {
    uint16_t ya = get_ya(cpu);
    write_dp_16(cpu, cpu->operand_address, ya);
}

void incw_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    set_nz_16(cpu, ++cpu->operand);
    write_dp_16(cpu, cpu->operand_address, cpu->operand);
}

void decw_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    set_nz_16(cpu, --cpu->operand);
    write_dp_16(cpu, cpu->operand_address, cpu->operand);
}

void addw_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    uint16_t ya = get_ya(cpu); 
    bool h = (ya & 0xFFF) + (cpu->operand & 0xFFF) > 0xFFF;
    int result = get_ya(cpu) + cpu->operand;
    set_flag(cpu, FLAG_H, h);
    set_flag(cpu, FLAG_V, (result ^ ya) & (result ^ cpu->operand) & 0x8000);
    set_flag(cpu, FLAG_C, result > 0xFFFF);
    set_nz_16(cpu, result);
    set_ya(cpu, result & 0xFFFF);
}

void subw_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    cpu->operand = ~cpu->operand;

    uint16_t ya = get_ya(cpu); 
    bool h = (ya & 0xFFF) + (cpu->operand & 0xFFF) > 0xFFF;
    int result = get_ya(cpu) + cpu->operand + 1;
    set_flag(cpu, FLAG_H, h);
    set_flag(cpu, FLAG_V, (result ^ ya) & (result ^ cpu->operand) & 0x8000);
    set_flag(cpu, FLAG_C, result > 0xFFFF);
    set_nz_16(cpu, result);
    set_ya(cpu, result & 0xFFFF);
}

void cmpw_dp(SPC700* cpu) {
    load_operand_dp_16(cpu);
    cpu->operand = ~cpu->operand;
    int result = get_ya(cpu) + cpu->operand + 1;
    set_flag(cpu, FLAG_C, result > 0xFFFF);
    set_nz_16(cpu, result);
}

void mul_ya(SPC700* cpu) {
    uint16_t result = cpu->registers.a * cpu->registers.y;
    set_ya(cpu, result);
    set_nz_8(cpu, cpu->registers.y);
}

void div_ya(SPC700* cpu) {
    int yva = get_ya(cpu);
    int x = (cpu->registers.x << 9) & 0x1FFFF;

    set_flag(cpu, FLAG_H, (cpu->registers.x & 0xF) <= (cpu->registers.y & 0xF));

    for (int i = 0; i < 9; i++) {
        bool c = yva >> 0x10;
        yva = (yva << 1) | c;
        yva &= 0x1FFFF;
        if (yva >= x) yva = yva ^ 0x1;
        if (yva & 0x1) yva -= x;
        yva &= 0x1FFFF;
    }

    uint8_t a = yva & 0xFF;
    uint8_t y = (yva >> 0x9) & 0xFF;
    bool v = (yva >> 8) & 0x1;

    cpu->registers.a = a;
    cpu->registers.y = y;
    set_flag(cpu, FLAG_V, v);
    set_nz_8(cpu, a);
}

void daa(SPC700* cpu) {
    if (get_flag(cpu, FLAG_C) || cpu->registers.a > 0x99) {
        cpu->registers.a += 0x60;
        set_flag(cpu, FLAG_C, true);
    }

    if (get_flag(cpu, FLAG_H) || (cpu->registers.a & 0xF) > 0x09) {
        cpu->registers.a += 0x06;
    }

    set_nz_8(cpu, cpu->registers.a);
}

void das(SPC700* cpu) {
    if (!get_flag(cpu, FLAG_C) || cpu->registers.a > 0x99) {
        cpu->registers.a -= 0x60;
        set_flag(cpu, FLAG_C, false);
    }

    if (!get_flag(cpu, FLAG_H) || (cpu->registers.a & 0xF) > 0x09) {
        cpu->registers.a -= 0x06;
    }

    set_nz_8(cpu, cpu->registers.a);
}

void bra(SPC700* cpu) {
    handle_branch(cpu, true);
}

void beq(SPC700* cpu) {
    handle_branch(cpu, get_flag(cpu, FLAG_Z));
}

void bne(SPC700* cpu) {
    handle_branch(cpu, !get_flag(cpu, FLAG_Z));
}

void bcs(SPC700* cpu) {
    handle_branch(cpu, get_flag(cpu, FLAG_C));
}

void bcc(SPC700* cpu) {
    handle_branch(cpu, !get_flag(cpu, FLAG_C));
}

void bvs(SPC700* cpu) {
    handle_branch(cpu, get_flag(cpu, FLAG_V));
}

void bvc(SPC700* cpu) {
    handle_branch(cpu, !get_flag(cpu, FLAG_V));
}

void bmi(SPC700* cpu) {
    handle_branch(cpu, get_flag(cpu, FLAG_N));
}

void bpl(SPC700* cpu) {
    handle_branch(cpu, !get_flag(cpu, FLAG_N));
}

void bbs(SPC700* cpu) {
    load_operand(cpu);
    
    int index = cpu->opcode >> 5;
    bool bit = (cpu->operand >> index) & 0x1;
    handle_branch(cpu, bit);
}

void bbc(SPC700* cpu) {
    load_operand(cpu);
    
    int index = cpu->opcode >> 5;
    bool bit = (cpu->operand >> index) & 0x1;
    handle_branch(cpu, !bit);
}

void cbne(SPC700* cpu) {
    load_operand(cpu);
    cpu->operand = ~cpu->operand;
    uint8_t result = cpu->registers.a + cpu->operand + 1;
    bool equal = (result == 0);
    handle_branch(cpu, !equal);
}

void dbnz_mem(SPC700* cpu) {
    load_operand(cpu);
    bool zero = (--cpu->operand == 0);
    store_result(cpu);
    handle_branch(cpu, !zero);
}

void dbnz_y(SPC700* cpu) {
    bool zero = (--cpu->registers.y == 0);
    handle_branch(cpu, !zero);
}

void jmp(SPC700* cpu) {
    cpu->registers.pc = cpu->operand_address;
}

void call(SPC700* cpu) {
    push_stack(cpu, cpu->registers.pc >> 8);
    push_stack(cpu, cpu->registers.pc & 0xFF);
    cpu->registers.pc = cpu->operand_address;
}

void pcall(SPC700* cpu) {
    cpu->operand_address = 0xFF00 | read_immediate(cpu);
    call(cpu);
}

void tcall(SPC700* cpu) {
    uint8_t vector = cpu->opcode >> 0x4;
    uint16_t addr = 0xFF00 | (0xDE - (vector << 1));
    cpu->operand_address = read(cpu, addr++);
    cpu->operand_address |= read(cpu, addr) << 8;
    call(cpu);
}

void brk(SPC700* cpu) {
    push_stack(cpu, cpu->registers.pc >> 8);
    push_stack(cpu, cpu->registers.pc & 0xFF);
    push_stack(cpu, cpu->registers.psw);
    set_flag(cpu, FLAG_B, true);
    set_flag(cpu, FLAG_I, false);
    cpu->registers.pc = read(cpu, 0xFFDE);
    cpu->registers.pc |= read(cpu, 0xFFDF) << 8;
}

void ret(SPC700* cpu) {
    cpu->registers.pc = pop_stack(cpu);
    cpu->registers.pc |= pop_stack(cpu) << 8;
}

void reti(SPC700* cpu) {
    cpu->registers.psw = pop_stack(cpu);
    ret(cpu);
}

void push_a(SPC700* cpu) {
    push_stack(cpu, cpu->registers.a);
}

void push_x(SPC700* cpu) {
    push_stack(cpu, cpu->registers.x);
}

void push_y(SPC700* cpu) {
    push_stack(cpu, cpu->registers.y);
}

void push_psw(SPC700* cpu) {
    push_stack(cpu, cpu->registers.psw);
}

void pop_a(SPC700* cpu) {
    cpu->registers.a = pop_stack(cpu);
}

void pop_x(SPC700* cpu) {
    cpu->registers.x = pop_stack(cpu);
}

void pop_y(SPC700* cpu) {
    cpu->registers.y = pop_stack(cpu);
}

void pop_psw(SPC700* cpu) {
    cpu->registers.psw = pop_stack(cpu);
}

void set_mem_bit(SPC700* cpu) {
    load_operand(cpu);
    uint8_t bit = cpu->opcode >> 0x5;
    cpu->operand |= 1 << bit;
    store_result(cpu);
}

void clr_mem_bit(SPC700* cpu) {
    load_operand(cpu);
    uint8_t bit = cpu->opcode >> 0x5;
    cpu->operand &= ~(1 << bit);
    store_result(cpu);
}

void tset1(SPC700* cpu) {
    load_operand(cpu);
    uint8_t result = cpu->registers.a - cpu->operand;
    cpu->operand |= cpu->registers.a;
    set_nz_8(cpu, result);
    store_result(cpu);
}

void tclr1(SPC700* cpu) {
    load_operand(cpu);
    uint8_t result = cpu->registers.a - cpu->operand;
    cpu->operand &= ~cpu->registers.a;
    set_nz_8(cpu, result);
    store_result(cpu);
}

void and1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool result = get_flag(cpu, FLAG_C) && get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, result);
}

void nand1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool result = get_flag(cpu, FLAG_C) && !get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, result);
}

void or1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool result = get_flag(cpu, FLAG_C) || get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, result);
}

void nor1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool result = get_flag(cpu, FLAG_C) || !get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, result);
}

void eor1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool result = get_flag(cpu, FLAG_C) ^ get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, result);
}

void not1(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    cpu->operand ^= 1 << index;
    store_result(cpu);
}

void mov1_c_mem(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    bool b = get_bit(cpu->operand, index);
    set_flag(cpu, FLAG_C, b);
}

void mov1_mem_c(SPC700* cpu) {
    uint8_t index = load_operand_mem_bit(cpu);
    if (get_flag(cpu, FLAG_C)) {
        cpu->operand |= 1 << index;
    }
    else {
        cpu->operand &= ~(1 << index);
    }
    
    store_result(cpu);
}

void clcr(SPC700* cpu) {
    set_flag(cpu, FLAG_C, false);
}

void setc(SPC700* cpu) {
    set_flag(cpu, FLAG_C, true);
}

void notc(SPC700* cpu) {
    bool c = get_flag(cpu, FLAG_C);
    set_flag(cpu, FLAG_C, !c);
}

void clrv(SPC700* cpu) {
    set_flag(cpu, FLAG_V, false);
    set_flag(cpu, FLAG_H, false);
}

void clrp(SPC700* cpu) {
    set_flag(cpu, FLAG_P, false);
}

void sep(SPC700* cpu) {
    set_flag(cpu, FLAG_P, true);
}

void ei(SPC700* cpu) {
    set_flag(cpu, FLAG_I, true);
}

void di(SPC700* cpu) {
    set_flag(cpu, FLAG_I, false);
}

void nop(SPC700* cpu) {
    return;
}

#undef EXEC_OP
#undef EXEC_OP_IMPL