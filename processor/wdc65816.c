#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../include/processor/wdc65816/wdc65816.h"
#include "../include/processor/wdc65816/instruction.h"
#include "../include/processor/wdc65816/addressing.h"

#define EXEC_OP_M(OPCODE, ADDR, INSTR, DUMMY) \
    case OPCODE: \
        cpu->dummy_read = DUMMY; \
        ADDR(cpu); \
        get_flag(cpu, WDC_FLAG_M) ? INSTR##_8(cpu) : INSTR##_16(cpu); \
        break

#define EXEC_OP_X(OPCODE, ADDR, INSTR, DUMMY) \
    case OPCODE: \
        cpu->dummy_read = DUMMY; \
        ADDR(cpu); \
        get_flag(cpu, WDC_FLAG_X) ? INSTR##_8(cpu) : INSTR##_16(cpu); \
        break

#define EXEC_OP(OPCODE, ADDR, INSTR, DUMMY) \
    case OPCODE: \
        cpu->dummy_read = DUMMY; \
        ADDR(cpu); \
        INSTR(cpu); \
        break

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

static void adjust_address(WDC65816* cpu);

static bool get_flag(WDC65816* cpu, WDC65816_flag flag);
static void set_flag(WDC65816* cpu, WDC65816_flag flag, bool value);

static bool page_crossed(uint32_t address_a, uint32_t address_b);

static void end_op(WDC65816* cpu);
static void end_op_flag_i(WDC65816* cpu, bool flag_i);

static void handle_interrupt(WDC65816* cpu, WDC65816_vector vector);

static void load_operand_byte(WDC65816* cpu);

static void load_operand_word_high(WDC65816* cpu);
static void load_operand_word_low(WDC65816* cpu);

static void set_nz_byte(WDC65816* cpu, uint8_t value);
static void set_nz_word(WDC65816* cpu, uint16_t value);

static void compare_reg_8(WDC65816* cpu, uint8_t reg);
static void compare_reg_16(WDC65816* cpu, uint16_t reg);

static void transfer_8(WDC65816* cpu, uint8_t source, uint8_t* dest);
static void transfer_16(WDC65816* cpu, uint16_t source, uint16_t* dest);

static void schedule_reset(WDC65816* cpu);

void init_wdc65816(WDC65816* cpu, WDC65816_read_callback read, WDC65816_write_callback write) {
    cpu->read = read;
    cpu->write = write;

    cpu->current_cycle = 0;
    cpu->cycle_lookup_index = 0;
}

void cycle_wdc65816(WDC65816* cpu) {
    if (cpu->current_cycle++ == 0) {
        cpu->bank_mode = BANK_DATA;

        if (cpu->nmi_latch) {
            handle_interrupt(cpu, VECTOR_NMI);
            return;
        }

        if (cpu->irq_pending) {
            handle_interrupt(cpu, VECTOR_IRQ);
            return;
        } 

        if (cpu->wai) {
            dummy_read_pc(cpu);
            cpu->current_cycle = 0;
            return;
        }

        uint8_t opcode = read_immediate(cpu);

        //printf("65816 opcode %02X\n", opcode);
        
        switch (opcode) {
            EXEC_OP_M(0x2D, schedule_addr_a, schedule_and, false);
            EXEC_OP_M(0x3D, schedule_addr_a_x, schedule_and, false);
            EXEC_OP_M(0x39, schedule_addr_a_y, schedule_and, false);
            EXEC_OP_M(0x3F, schedule_addr_al_x, schedule_and, false);
            EXEC_OP_M(0x2F, schedule_addr_al, schedule_and, false);
            EXEC_OP_M(0x21, schedule_addr_d_x_indr, schedule_and, false);
            EXEC_OP_M(0x35, schedule_addr_d_x, schedule_and, false);
            EXEC_OP_M(0x31, schedule_addr_d_indr_y, schedule_and, false);
            EXEC_OP_M(0x32, schedule_addr_d_indr, schedule_and, false);
            EXEC_OP_M(0x27, schedule_addr_dl_indr, schedule_and, false);
            EXEC_OP_M(0x37, schedule_addr_dl_indr_y, schedule_and, false);
            EXEC_OP_M(0x25, schedule_addr_d, schedule_and, false);
            EXEC_OP_M(0x29, schedule_addr_imm, schedule_and, false);
            EXEC_OP_M(0x23, schedule_addr_d_s, schedule_and, false);
            EXEC_OP_M(0x33, schedule_addr_d_s_indr_y, schedule_and, false);

            EXEC_OP_M(0x0A, schedule_addr_implied, schedule_asl_a, false);
            EXEC_OP_M(0x0E, schedule_addr_a, schedule_asl_rmw, true);
            EXEC_OP_M(0x06, schedule_addr_d, schedule_asl_rmw, true);
            EXEC_OP_M(0x1E, schedule_addr_a_x, schedule_asl_rmw, true);
            EXEC_OP_M(0x16, schedule_addr_d_x, schedule_asl_rmw, true);

            EXEC_OP(0x90, schedule_addr_implied, schedule_bcc, false);
            EXEC_OP(0xB0, schedule_addr_implied, schedule_bcs, false);
            EXEC_OP(0xD0, schedule_addr_implied, schedule_bne, false);
            EXEC_OP(0xF0, schedule_addr_implied, schedule_beq, false);
            EXEC_OP(0x10, schedule_addr_implied, schedule_bpl, false);
            EXEC_OP(0x30, schedule_addr_implied, schedule_bmi, false);
            EXEC_OP(0x50, schedule_addr_implied, schedule_bvc, false);
            EXEC_OP(0x70, schedule_addr_implied, schedule_bvs, false);
            EXEC_OP(0x80, schedule_addr_implied, schedule_bra, false);
            EXEC_OP(0x82, schedule_addr_implied, schedule_brl, false);

            EXEC_OP_M(0x89, schedule_addr_imm, schedule_bit, false);
            EXEC_OP_M(0x2C, schedule_addr_a, schedule_bit, false);
            EXEC_OP_M(0x24, schedule_addr_d, schedule_bit, false);
            EXEC_OP_M(0x3C, schedule_addr_a_x, schedule_bit, false);
            EXEC_OP_M(0x34, schedule_addr_d_x, schedule_bit, false);

            EXEC_OP(0x00, schedule_addr_implied, schedule_brk, false);
            EXEC_OP(0x02, schedule_addr_implied, schedule_cop, false);

            EXEC_OP(0x18, schedule_addr_implied, schedule_clc, false);
            EXEC_OP(0x58, schedule_addr_implied, schedule_cli, false);
            EXEC_OP(0xD8, schedule_addr_implied, schedule_cld, false);
            EXEC_OP(0xB8, schedule_addr_implied, schedule_clv, false);

            EXEC_OP_M(0xC9, schedule_addr_imm, schedule_cmp, false);
            EXEC_OP_M(0xCD, schedule_addr_a, schedule_cmp, false);
            EXEC_OP_M(0xCF, schedule_addr_al, schedule_cmp, false);
            EXEC_OP_M(0xC5, schedule_addr_d, schedule_cmp, false);
            EXEC_OP_M(0xD2, schedule_addr_d_indr, schedule_cmp, false);
            EXEC_OP_M(0xC7, schedule_addr_dl_indr, schedule_cmp, false);
            EXEC_OP_M(0xDD, schedule_addr_a_x, schedule_cmp, false);
            EXEC_OP_M(0xDF, schedule_addr_al_x, schedule_cmp, false);
            EXEC_OP_M(0xD9, schedule_addr_a_y, schedule_cmp, false);
            EXEC_OP_M(0xD5, schedule_addr_d_x, schedule_cmp, false);
            EXEC_OP_M(0xC1, schedule_addr_d_x_indr, schedule_cmp, false);
            EXEC_OP_M(0xD1, schedule_addr_d_indr_y, schedule_cmp, false);
            EXEC_OP_M(0xD7, schedule_addr_dl_indr_y, schedule_cmp, false);
            EXEC_OP_M(0xC3, schedule_addr_d_s, schedule_cmp, false);
            EXEC_OP_M(0xD3, schedule_addr_d_s_indr_y, schedule_cmp, false);

            EXEC_OP_X(0xE0, schedule_addr_imm_x, schedule_cpx, false);
            EXEC_OP_X(0xEC, schedule_addr_a, schedule_cpx, false);
            EXEC_OP_X(0xE4, schedule_addr_d, schedule_cpx, false);

            EXEC_OP_X(0xC0, schedule_addr_imm_x, schedule_cpy, false);
            EXEC_OP_X(0xCC, schedule_addr_a, schedule_cpy, false);
            EXEC_OP_X(0xC4, schedule_addr_d, schedule_cpy, false);

            EXEC_OP_M(0x3A, schedule_addr_implied, schedule_dec_a, true);
            EXEC_OP_M(0xCE, schedule_addr_a, schedule_dec_rmw, true);
            EXEC_OP_M(0xC6, schedule_addr_d, schedule_dec_rmw, true);
            EXEC_OP_M(0xDE, schedule_addr_a_x, schedule_dec_rmw, true);
            EXEC_OP_M(0xD6, schedule_addr_d_x, schedule_dec_rmw, true);

            EXEC_OP_X(0xCA, schedule_addr_implied, schedule_dex, false);
            EXEC_OP_X(0x88, schedule_addr_implied, schedule_dey, false);

            EXEC_OP_M(0x49, schedule_addr_imm, schedule_eor, false);
            EXEC_OP_M(0x4D, schedule_addr_a, schedule_eor, false);
            EXEC_OP_M(0x4F, schedule_addr_al, schedule_eor, false);
            EXEC_OP_M(0x45, schedule_addr_d, schedule_eor, false);
            EXEC_OP_M(0x52, schedule_addr_d_indr, schedule_eor, false);
            EXEC_OP_M(0x47, schedule_addr_dl_indr, schedule_eor, false);
            EXEC_OP_M(0x5D, schedule_addr_a_x, schedule_eor, false);
            EXEC_OP_M(0x5F, schedule_addr_al_x, schedule_eor, false);
            EXEC_OP_M(0x59, schedule_addr_a_y, schedule_eor, false);
            EXEC_OP_M(0x55, schedule_addr_d_x, schedule_eor, false);
            EXEC_OP_M(0x41, schedule_addr_d_x_indr, schedule_eor, false);
            EXEC_OP_M(0x51, schedule_addr_d_indr_y, schedule_eor, false);
            EXEC_OP_M(0x57, schedule_addr_dl_indr_y, schedule_eor, false);
            EXEC_OP_M(0x43, schedule_addr_d_s, schedule_eor, false);
            EXEC_OP_M(0x53, schedule_addr_d_s_indr_y, schedule_eor, false);
            
            EXEC_OP_M(0x1A, schedule_addr_implied, schedule_inc_a, false);
            EXEC_OP_M(0xEE, schedule_addr_a, schedule_inc_rmw, true);
            EXEC_OP_M(0xE6, schedule_addr_d, schedule_inc_rmw, true);
            EXEC_OP_M(0xFE, schedule_addr_a_x, schedule_inc_rmw, true);
            EXEC_OP_M(0xF6, schedule_addr_d_x, schedule_inc_rmw, true);

            EXEC_OP_X(0xE8, schedule_addr_implied, schedule_inx, false);
            EXEC_OP_X(0xC8, schedule_addr_implied, schedule_iny, false);

            EXEC_OP(0x4C, schedule_addr_implied, schedule_jmp_a, false);
            EXEC_OP(0x6C, schedule_addr_implied, schedule_jmp_a_indr, false);
            EXEC_OP(0x7C, schedule_addr_implied, schedule_jmp_a_x_indr, false);
            
            EXEC_OP(0x5C, schedule_addr_implied, schedule_jml, false);
            EXEC_OP(0xDC, schedule_addr_implied, schedule_jml_indr, false);

            EXEC_OP(0x20, schedule_addr_implied, schedule_jsr_a, false);
            EXEC_OP(0xFC, schedule_addr_implied, schedule_jsr_a_x_indr, false);

            EXEC_OP(0x22, schedule_addr_implied, schedule_jsl, false);

            EXEC_OP_M(0xA9, schedule_addr_imm, schedule_lda, false);
            EXEC_OP_M(0xAD, schedule_addr_a, schedule_lda, false);
            EXEC_OP_M(0xAF, schedule_addr_al, schedule_lda, false);
            EXEC_OP_M(0xA5, schedule_addr_d, schedule_lda, false);
            EXEC_OP_M(0xB2, schedule_addr_d_indr, schedule_lda, false);
            EXEC_OP_M(0xA7, schedule_addr_dl_indr, schedule_lda, false);
            EXEC_OP_M(0xBD, schedule_addr_a_x, schedule_lda, false);
            EXEC_OP_M(0xBF, schedule_addr_al_x, schedule_lda, false);
            EXEC_OP_M(0xB9, schedule_addr_a_y, schedule_lda, false);
            EXEC_OP_M(0xB5, schedule_addr_d_x, schedule_lda, false);
            EXEC_OP_M(0xA1, schedule_addr_d_x_indr, schedule_lda, false);
            EXEC_OP_M(0xB1, schedule_addr_d_indr_y, schedule_lda, false);
            EXEC_OP_M(0xB7, schedule_addr_dl_indr_y, schedule_lda, false);
            EXEC_OP_M(0xA3, schedule_addr_d_s, schedule_lda, false);
            EXEC_OP_M(0xB3, schedule_addr_d_s_indr_y, schedule_lda, false);

            EXEC_OP_X(0xA2, schedule_addr_imm_x, schedule_ldx, false);
            EXEC_OP_X(0xAE, schedule_addr_a, schedule_ldx, false);
            EXEC_OP_X(0xA6, schedule_addr_d, schedule_ldx, false);
            EXEC_OP_X(0xBE, schedule_addr_a_y, schedule_ldx, false);
            EXEC_OP_X(0xB6, schedule_addr_d_y, schedule_ldx, false);

            EXEC_OP_X(0xA0, schedule_addr_imm_x, schedule_ldy, false);
            EXEC_OP_X(0xAC, schedule_addr_a, schedule_ldy, false);
            EXEC_OP_X(0xA4, schedule_addr_d, schedule_ldy, false);
            EXEC_OP_X(0xBC, schedule_addr_a_x, schedule_ldy, false);
            EXEC_OP_X(0xB4, schedule_addr_d_x, schedule_ldy, false);

            EXEC_OP_M(0x4A, schedule_addr_implied, schedule_lsr_a, false);
            EXEC_OP_M(0x4E, schedule_addr_a, schedule_lsr_rmw, true);
            EXEC_OP_M(0x46, schedule_addr_d, schedule_lsr_rmw, true);
            EXEC_OP_M(0x5E, schedule_addr_a_x, schedule_lsr_rmw, true);
            EXEC_OP_M(0x56, schedule_addr_d_x, schedule_lsr_rmw, true);

            EXEC_OP(0x54, schedule_addr_implied, schedule_mvn, false);
            EXEC_OP(0x44, schedule_addr_implied, schedule_mvp, false);

            EXEC_OP(0xEA, schedule_addr_implied, schedule_nop, false);

            EXEC_OP_M(0x09, schedule_addr_imm, schedule_ora, false);
            EXEC_OP_M(0x0D, schedule_addr_a, schedule_ora, false);
            EXEC_OP_M(0x0F, schedule_addr_al, schedule_ora, false);
            EXEC_OP_M(0x05, schedule_addr_d, schedule_ora, false);
            EXEC_OP_M(0x12, schedule_addr_d_indr, schedule_ora, false);
            EXEC_OP_M(0x07, schedule_addr_dl_indr, schedule_ora, false);
            EXEC_OP_M(0x1D, schedule_addr_a_x, schedule_ora, false);
            EXEC_OP_M(0x1F, schedule_addr_al_x, schedule_ora, false);
            EXEC_OP_M(0x19, schedule_addr_a_y, schedule_ora, false);
            EXEC_OP_M(0x15, schedule_addr_d_x, schedule_ora, false);
            EXEC_OP_M(0x01, schedule_addr_d_x_indr, schedule_ora, false);
            EXEC_OP_M(0x11, schedule_addr_d_indr_y, schedule_ora, false);
            EXEC_OP_M(0x17, schedule_addr_dl_indr_y, schedule_ora, false);
            EXEC_OP_M(0x03, schedule_addr_d_s, schedule_ora, false);
            EXEC_OP_M(0x13, schedule_addr_d_s_indr_y, schedule_ora, false);

            EXEC_OP(0xF4, schedule_addr_implied, schedule_pea, false);
            EXEC_OP(0xD4, schedule_addr_implied, schedule_pei, false);
            EXEC_OP(0x62, schedule_addr_implied, schedule_per, false);

            EXEC_OP_M(0x48, schedule_addr_implied, schedule_pha, false);
            EXEC_OP(0x8B, schedule_addr_implied, schedule_phb, false);
            EXEC_OP(0x0B, schedule_addr_implied, schedule_phd, false);
            EXEC_OP(0x4B, schedule_addr_implied, schedule_phk, false);
            EXEC_OP(0x08, schedule_addr_implied, schedule_php, false);
            EXEC_OP_X(0xDA, schedule_addr_implied, schedule_phx, false);
            EXEC_OP_X(0x5A, schedule_addr_implied, schedule_phy, false);

            EXEC_OP_M(0x68, schedule_addr_implied, schedule_pla, false);
            EXEC_OP(0xAB, schedule_addr_implied, schedule_plb, false);
            EXEC_OP(0x2B, schedule_addr_implied, schedule_pld, false);
            EXEC_OP(0x28, schedule_addr_implied, schedule_plp, false);
            EXEC_OP_X(0xFA, schedule_addr_implied, schedule_plx, false);
            EXEC_OP_X(0x7A, schedule_addr_implied, schedule_ply, false);

            EXEC_OP(0xC2, schedule_addr_implied, schedule_rep, false);

            EXEC_OP_M(0x2A, schedule_addr_implied, schedule_rol_a, false);
            EXEC_OP_M(0x2E, schedule_addr_a, schedule_rol_rmw, true);
            EXEC_OP_M(0x26, schedule_addr_d, schedule_rol_rmw, true);
            EXEC_OP_M(0x3E, schedule_addr_a_x, schedule_rol_rmw, true);
            EXEC_OP_M(0x36, schedule_addr_d_x, schedule_rol_rmw, true);

            EXEC_OP_M(0x6A, schedule_addr_implied, schedule_ror_a, false);
            EXEC_OP_M(0x6E, schedule_addr_a, schedule_ror_rmw, true);
            EXEC_OP_M(0x66, schedule_addr_d, schedule_ror_rmw, true);
            EXEC_OP_M(0x7E, schedule_addr_a_x, schedule_ror_rmw, true);
            EXEC_OP_M(0x76, schedule_addr_d_x, schedule_ror_rmw, true);

            EXEC_OP(0x40, schedule_addr_implied, schedule_rti, false);
            EXEC_OP(0x60, schedule_addr_implied, schedule_rts, false);
            EXEC_OP(0x6B, schedule_addr_implied, schedule_rtl, false);

            EXEC_OP(0x38, schedule_addr_implied, schedule_sec, false);
            EXEC_OP(0x78, schedule_addr_implied, schedule_sei, false);
            EXEC_OP(0xF8, schedule_addr_implied, schedule_sed, false);

            EXEC_OP(0xE2, schedule_addr_implied, schedule_sep, false);

            EXEC_OP_M(0x8D, schedule_addr_a, schedule_sta, true);
            EXEC_OP_M(0x8F, schedule_addr_al, schedule_sta, true);
            EXEC_OP_M(0x85, schedule_addr_d, schedule_sta, true);
            EXEC_OP_M(0x92, schedule_addr_d_indr, schedule_sta, true);
            EXEC_OP_M(0x87, schedule_addr_dl_indr, schedule_sta, true);
            EXEC_OP_M(0x9D, schedule_addr_a_x, schedule_sta, true);
            EXEC_OP_M(0x9F, schedule_addr_al_x, schedule_sta, true);
            EXEC_OP_M(0x99, schedule_addr_a_y, schedule_sta, true);
            EXEC_OP_M(0x95, schedule_addr_d_x, schedule_sta, true);
            EXEC_OP_M(0x81, schedule_addr_d_x_indr, schedule_sta, true);
            EXEC_OP_M(0x91, schedule_addr_d_indr_y, schedule_sta, true);
            EXEC_OP_M(0x97, schedule_addr_dl_indr_y, schedule_sta, true);
            EXEC_OP_M(0x83, schedule_addr_d_s, schedule_sta, true);
            EXEC_OP_M(0x93, schedule_addr_d_s_indr_y, schedule_sta, true);

            EXEC_OP_X(0x8E, schedule_addr_a, schedule_stx, true);
            EXEC_OP_X(0x86, schedule_addr_d, schedule_stx, true);
            EXEC_OP_X(0x96, schedule_addr_d_y, schedule_stx, true);

            EXEC_OP_X(0x8C, schedule_addr_a, schedule_sty, true);
            EXEC_OP_X(0x84, schedule_addr_d, schedule_sty, true);
            EXEC_OP_X(0x94, schedule_addr_d_x, schedule_sty, true);

            EXEC_OP_M(0x9C, schedule_addr_a, schedule_stz, true);
            EXEC_OP_M(0x64, schedule_addr_d, schedule_stz, true);
            EXEC_OP_M(0x9E, schedule_addr_a_x, schedule_stz, true);
            EXEC_OP_M(0x74, schedule_addr_d_x, schedule_stz, true);

            EXEC_OP_X(0xAA, schedule_addr_implied, schedule_tax, false);
            EXEC_OP_X(0xA8, schedule_addr_implied, schedule_tay, false);
            EXEC_OP(0x5B, schedule_addr_implied, schedule_tcd, false);
            EXEC_OP(0x1B, schedule_addr_implied, schedule_tcs, false);
            EXEC_OP(0x7B, schedule_addr_implied, schedule_tdc, false);
            EXEC_OP(0x3B, schedule_addr_implied, schedule_tsc, false);
            EXEC_OP_X(0xBA, schedule_addr_implied, schedule_tsx, false);
            EXEC_OP_M(0x8A, schedule_addr_implied, schedule_txa, false);
            EXEC_OP(0x9A, schedule_addr_implied, schedule_txs, false);
            EXEC_OP_X(0x9B, schedule_addr_implied, schedule_txy, false);
            EXEC_OP_M(0x98, schedule_addr_implied, schedule_tya, false);
            EXEC_OP_X(0xBB, schedule_addr_implied, schedule_tyx, false);

            EXEC_OP_M(0x1C, schedule_addr_a, schedule_trb_rmw, true);
            EXEC_OP_M(0x14, schedule_addr_d, schedule_trb_rmw, true);

            EXEC_OP_M(0x0C, schedule_addr_a, schedule_tsb_rmw, true);
            EXEC_OP_M(0x04, schedule_addr_d, schedule_tsb_rmw, true);

            EXEC_OP(0xCB, schedule_addr_implied, schedule_wai, false);

            EXEC_OP(0x42, schedule_addr_implied, schedule_wdm, false);

            EXEC_OP(0xEB, schedule_addr_implied, schedule_xba, false);

            EXEC_OP(0xFB, schedule_addr_implied, schedule_xce, false);

            EXEC_OP_M(0x69, schedule_addr_imm, schedule_adc, false);
            EXEC_OP_M(0x6D, schedule_addr_a, schedule_adc, false);
            EXEC_OP_M(0x6F, schedule_addr_al, schedule_adc, false);
            EXEC_OP_M(0x65, schedule_addr_d, schedule_adc, false);
            EXEC_OP_M(0x72, schedule_addr_d_indr, schedule_adc, false);
            EXEC_OP_M(0x67, schedule_addr_dl_indr, schedule_adc, false);
            EXEC_OP_M(0x7D, schedule_addr_a_x, schedule_adc, false);
            EXEC_OP_M(0x7F, schedule_addr_al_x, schedule_adc, false);
            EXEC_OP_M(0x79, schedule_addr_a_y, schedule_adc, false);
            EXEC_OP_M(0x75, schedule_addr_d_x, schedule_adc, false);
            EXEC_OP_M(0x61, schedule_addr_d_x_indr, schedule_adc, false);
            EXEC_OP_M(0x71, schedule_addr_d_indr_y, schedule_adc, false);
            EXEC_OP_M(0x77, schedule_addr_dl_indr_y, schedule_adc, false);
            EXEC_OP_M(0x63, schedule_addr_d_s, schedule_adc, false);
            EXEC_OP_M(0x73, schedule_addr_d_s_indr_y, schedule_adc, false);

            EXEC_OP_M(0xE9, schedule_addr_imm, schedule_sbc, false);
            EXEC_OP_M(0xED, schedule_addr_a, schedule_sbc, false);
            EXEC_OP_M(0xEF, schedule_addr_al, schedule_sbc, false);
            EXEC_OP_M(0xE5, schedule_addr_d, schedule_sbc, false);
            EXEC_OP_M(0xF2, schedule_addr_d_indr, schedule_sbc, false);
            EXEC_OP_M(0xE7, schedule_addr_dl_indr, schedule_sbc, false);
            EXEC_OP_M(0xFD, schedule_addr_a_x, schedule_sbc, false);
            EXEC_OP_M(0xFF, schedule_addr_al_x, schedule_sbc, false);
            EXEC_OP_M(0xF9, schedule_addr_a_y, schedule_sbc, false);
            EXEC_OP_M(0xF5, schedule_addr_d_x, schedule_sbc, false);
            EXEC_OP_M(0xE1, schedule_addr_d_x_indr, schedule_sbc, false);
            EXEC_OP_M(0xF1, schedule_addr_d_indr_y, schedule_sbc, false);
            EXEC_OP_M(0xF7, schedule_addr_dl_indr_y, schedule_sbc, false);
            EXEC_OP_M(0xE3, schedule_addr_d_s, schedule_sbc, false);
            EXEC_OP_M(0xF3, schedule_addr_d_s_indr_y, schedule_sbc, false);
        }
    }

    else {
        cpu->cycle_lookup[cpu->current_cycle - 2](cpu);
    }

    cpu->nmi_latch = cpu->nmi_line && !cpu->last_nmi_line;
    cpu->last_nmi_line = cpu->nmi_line;

    return;
}

void schedule_reset(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &reset_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &reset_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &reset_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &reset_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &reset_3;
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

    printf("N: %u\n", get_flag(cpu, WDC_FLAG_N));
    printf("V: %u\n", get_flag(cpu, WDC_FLAG_V));
    printf("M: %u\n", get_flag(cpu, WDC_FLAG_M));
    printf("X: %u\n", get_flag(cpu, WDC_FLAG_X));
    printf("D: %u\n", get_flag(cpu, WDC_FLAG_D));
    printf("I: %u\n", get_flag(cpu, WDC_FLAG_I));
    printf("Z: %u\n", get_flag(cpu, WDC_FLAG_Z));
    printf("C: %u\n\n", get_flag(cpu, WDC_FLAG_C));

    printf("Operand address: %u\n\n", cpu->operand_address);
}

void handle_interrupt(WDC65816* cpu, WDC65816_vector vector) {
    cpu->interrupt_vector = vector;
    cpu->wai = false;
    schedule_interrupt(cpu);
    dummy_read_pc(cpu);
}

int wdc65816_run_instruction(WDC65816* cpu) {
    int cycle_count = 0;

    do {
        cycle_wdc65816(cpu);
        cycle_count++;
    } while (cpu->current_cycle != 0);

    return cycle_count;
}

void wdc65816_reset(WDC65816* cpu) {
    cpu->registers.d.word = 0;
    cpu->registers.dbr = 0;
    cpu->registers.pbr = 0;

    cpu->registers.x.high = 0;
    cpu->registers.y.high = 0;

    set_flag(cpu, WDC_FLAG_M, true);
    set_flag(cpu, WDC_FLAG_X, true);
    set_flag(cpu, WDC_FLAG_D, false);
    set_flag(cpu, WDC_FLAG_I, true);
    set_flag(cpu, WDC_FLAG_C, true);

    cpu->emulation_mode = true;
    cpu->current_cycle = 1;

    cpu->wai = false;

    cpu->irq_pending = false;

    cpu->nmi_line = false;
    cpu->last_nmi_line = false;

    cpu->irq_line = false;
    cpu->nmi_latch = false;

    schedule_reset(cpu);
}

uint8_t read(WDC65816* cpu, uint32_t address) {
    return cpu->read(cpu, address, true);
}

void write(WDC65816* cpu, uint32_t address, uint8_t data) {
    cpu->write(cpu, address, data);
}

void dummy_read(WDC65816* cpu, uint32_t address) {
    cpu->read(cpu, address, false);
}

void dummy_read_pc(WDC65816* cpu) {
    dummy_read(cpu, (cpu->registers.pbr << 16) | cpu->registers.pc.word);
}

void dummy_read_sp(WDC65816* cpu) {
    dummy_read(cpu, cpu->registers.s.word);
}

uint8_t read_immediate(WDC65816* cpu) {
    return read(cpu, (cpu->registers.pbr << 16) | cpu->registers.pc.word++);
}

void increment_address(WDC65816* cpu) {
    cpu->operand_address++;
    adjust_address(cpu);
}

void decrement_address(WDC65816* cpu) {
    cpu->operand_address--;
    adjust_address(cpu);
}

void adjust_address(WDC65816* cpu) {
    switch (cpu->bank_mode) {
        case BANK_DATA : break;
        case BANK_ZERO : cpu->operand_address &= 0xFFFF; break;
        case BANK_PROGRAM : cpu->operand_address &= 0xFFFF; cpu->operand_address |= (cpu->registers.pbr << 16); break;
    }
}

void set_nz_byte(WDC65816* cpu, uint8_t value) {
    set_flag(cpu, WDC_FLAG_Z, value == 0);
    set_flag(cpu, WDC_FLAG_N, value >> 7);
}

void set_nz_word(WDC65816* cpu, uint16_t value) {
    set_flag(cpu, WDC_FLAG_Z, value == 0);
    set_flag(cpu, WDC_FLAG_N, value >> 15);
}

void compare_reg_8(WDC65816* cpu, uint8_t reg) {
    uint8_t result = reg - cpu->operand;
    set_flag(cpu, WDC_FLAG_C, reg >= cpu->operand);
    set_nz_byte(cpu, result);
}

void compare_reg_16(WDC65816* cpu, uint16_t reg) {
    uint16_t result = reg - cpu->operand;
    set_flag(cpu, WDC_FLAG_C, reg >= cpu->operand);
    set_nz_word(cpu, result);
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

        if (flag == WDC_FLAG_X) {
            cpu->registers.x.high = 0x00;
            cpu->registers.y.high = 0x00;
        }

        return;
    }

    cpu->registers.p &= ~(1 << flag);
}

void transfer_8(WDC65816* cpu, uint8_t source, uint8_t* dest) {
    *dest = source;
    set_nz_byte(cpu, source);
}

void transfer_16(WDC65816* cpu, uint16_t source, uint16_t* dest) {
    *dest = source;
    set_nz_word(cpu, source);
}

bool page_crossed(uint32_t address_a, uint32_t address_b) {
    return (address_a & 0xFFFF00) != (address_b & 0xFFFF00);
}

void end_op(WDC65816* cpu) {
    end_op_flag_i(cpu, get_flag(cpu, WDC_FLAG_I));
}

void end_op_flag_i(WDC65816* cpu, bool flag_i) {
    cpu->current_cycle = 0x00;
    cpu->cycle_lookup_index = 0x00;
    cpu->irq_pending = cpu->irq_line && !flag_i;
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
    
    if (!page_crossed(cpu->operand_address, cpu->operand_address + cpu->index_register->word) && get_flag(cpu, WDC_FLAG_X) && !cpu->dummy_read) {
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

    if (!page_crossed(cpu->operand_address, cpu->operand_address + cpu->registers.y.word) && get_flag(cpu, WDC_FLAG_X) && !cpu->dummy_read) {
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

void schedule_addr_imm(WDC65816* cpu) {
    cpu->bank_mode = BANK_PROGRAM;
    cpu->operand_address = (cpu->registers.pbr << 16) | cpu->registers.pc.word++;
    if (!get_flag(cpu, WDC_FLAG_M)) cpu->registers.pc.word++;
}

void schedule_addr_imm_x(WDC65816* cpu) {
    cpu->bank_mode = BANK_PROGRAM;
    cpu->operand_address = (cpu->registers.pbr << 16) | cpu->registers.pc.word++;
    if (!get_flag(cpu, WDC_FLAG_X)) cpu->registers.pc.word++;
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

void schedule_load_mem_op_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_mem_op_8_1;
}

void load_mem_op_8_1(WDC65816* cpu) {
    load_operand_byte(cpu);
    cpu->op_callback(cpu);
    end_op(cpu);
}

void schedule_load_mem_op_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_operand_word_low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_mem_op_16_1;
}

void load_mem_op_16_1(WDC65816* cpu) {
    load_operand_word_high(cpu);
    cpu->op_callback(cpu);
    end_op(cpu);
}

void schedule_rmw_a_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_a_8_1;
}

void rmw_a_8_1(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.low;
    cpu->op_callback(cpu);
    cpu->registers.a.low = cpu->operand;
    end_op(cpu);
}

void schedule_rmw_a_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_a_16_1;
}

void rmw_a_16_1(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.word;
    cpu->op_callback(cpu);
    cpu->registers.a.word = cpu->operand;
    end_op(cpu);
}

void schedule_rmw_mem_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_operand_word_low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_mem_8_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_mem_8_2;
}

void rmw_mem_8_1(WDC65816* cpu) {
    dummy_read(cpu, cpu->operand_address);
    cpu->op_callback(cpu);
}

void rmw_mem_8_2(WDC65816* cpu) {
    decrement_address(cpu);
    write(cpu, cpu->operand_address, cpu->operand);
    end_op(cpu);
}

void schedule_rmw_mem_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_operand_word_low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_operand_word_high;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_mem_8_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_mem_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rmw_mem_16_2;
}

void rmw_mem_16_1(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand >> 8);
    decrement_address(cpu);
}

void rmw_mem_16_2(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand & 0xFF);
    end_op(cpu);
}

void schedule_and_8(WDC65816* cpu) {
    cpu->op_callback = &op_and_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_and_16(WDC65816* cpu) {
    cpu->op_callback = &op_and_16;
    schedule_load_mem_op_16(cpu);
}

void op_and_8(WDC65816* cpu) {
    cpu->registers.a.low &= cpu->operand;
    set_nz_byte(cpu, cpu->registers.a.low);
} 

void op_and_16(WDC65816* cpu) {
    cpu->registers.a.word &= cpu->operand;
    set_nz_word(cpu, cpu->registers.a.word);
} 

void schedule_asl_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_asl_8;
    schedule_rmw_a_8(cpu);
} 

void schedule_asl_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_asl_16;
    schedule_rmw_a_16(cpu);
}

void schedule_asl_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_asl_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_asl_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_asl_16;
    schedule_rmw_mem_16(cpu);
}

void op_asl_8(WDC65816* cpu) {
    bool c = cpu->operand >> 0x7;
    cpu->operand <<= 1;
    cpu->operand &= 0xFF;
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_byte(cpu, cpu->operand);
}

void op_asl_16(WDC65816* cpu) {
    bool c = cpu->operand >> 0xF;
    cpu->operand <<= 1;
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_word(cpu, cpu->operand);
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
    cpu->take_branch = !get_flag(cpu, WDC_FLAG_C);
    schedule_branch(cpu);
}

void schedule_bcs(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, WDC_FLAG_C);
    schedule_branch(cpu);
}

void schedule_bne(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, WDC_FLAG_Z);
    schedule_branch(cpu);
}

void schedule_beq(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, WDC_FLAG_Z);
    schedule_branch(cpu);
}

void schedule_bpl(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, WDC_FLAG_N);
    schedule_branch(cpu);
}

void schedule_bmi(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, WDC_FLAG_N);
    schedule_branch(cpu);
}

void schedule_bvc(WDC65816* cpu) {
    cpu->take_branch = !get_flag(cpu, WDC_FLAG_V);
    schedule_branch(cpu);
}

void schedule_bvs(WDC65816* cpu) {
    cpu->take_branch = get_flag(cpu, WDC_FLAG_V);
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

void schedule_bit_8(WDC65816* cpu) {
    cpu->op_callback = &op_bit_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_bit_16(WDC65816* cpu) {
    cpu->op_callback = &op_bit_16;
    schedule_load_mem_op_16(cpu);
}

void op_bit_8(WDC65816* cpu) {
    uint8_t result = cpu->registers.a.low & cpu->operand;
    set_flag(cpu, WDC_FLAG_Z, result == 0);

    if (cpu->bank_mode != BANK_PROGRAM) {
        set_flag(cpu, WDC_FLAG_N, cpu->operand >> 7);
        set_flag(cpu, WDC_FLAG_V, (cpu->operand >> 6) & 0x1);
    }
} 

void op_bit_16(WDC65816* cpu) {
    uint16_t result = cpu->registers.a.word & cpu->operand;
    set_flag(cpu, WDC_FLAG_Z, result == 0);

    if (cpu->bank_mode != BANK_PROGRAM) {
        set_flag(cpu, WDC_FLAG_N, cpu->operand >> 15);
        set_flag(cpu, WDC_FLAG_V, (cpu->operand >> 14) & 0x1);
    }
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
    dummy_read_pc(cpu);
    if (cpu->interrupt_vector == VECTOR_COP || cpu->interrupt_vector == VECTOR_BRK) cpu->registers.pc.word++;
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
    set_flag(cpu, WDC_FLAG_D, false);
    set_flag(cpu, WDC_FLAG_I, true);
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

void schedule_clc(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &clc_1;
}

void clc_1(WDC65816* cpu) {
    set_flag(cpu, WDC_FLAG_C, false);
    end_op(cpu);
}

void schedule_cld(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &cld_1;
}

void cld_1(WDC65816* cpu) {
    set_flag(cpu, WDC_FLAG_D, false);
    end_op(cpu);
}

void schedule_cli(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &cli_1;
}

void cli_1(WDC65816* cpu) {
    bool i = get_flag(cpu, WDC_FLAG_I);
    set_flag(cpu, WDC_FLAG_I, false);
    end_op_flag_i(cpu, i);
}

void schedule_clv(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &clv_1;
}

void clv_1(WDC65816* cpu) {
    set_flag(cpu, WDC_FLAG_V, false);
    end_op(cpu);
}

void schedule_cmp_8(WDC65816* cpu) {
    cpu->op_callback = &op_cmp_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_cmp_16(WDC65816* cpu) {
    cpu->op_callback = &op_cmp_16;
    schedule_load_mem_op_16(cpu);
}

void op_cmp_8(WDC65816* cpu) {
    compare_reg_8(cpu, cpu->registers.a.low);
} 

void op_cmp_16(WDC65816* cpu) {
    compare_reg_16(cpu, cpu->registers.a.word);
} 

void schedule_cpx_8(WDC65816* cpu) {
    cpu->op_callback = &op_cpx_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_cpx_16(WDC65816* cpu) {
    cpu->op_callback = &op_cpx_16;
    schedule_load_mem_op_16(cpu);
}

void op_cpx_8(WDC65816* cpu) {
    compare_reg_8(cpu, cpu->registers.x.low);
} 

void op_cpx_16(WDC65816* cpu) {
    compare_reg_16(cpu, cpu->registers.x.word);
} 

void schedule_cpy_8(WDC65816* cpu) {
    cpu->op_callback = &op_cpy_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_cpy_16(WDC65816* cpu) {
    cpu->op_callback = &op_cpy_16;
    schedule_load_mem_op_16(cpu);
}

void op_cpy_8(WDC65816* cpu) {
    compare_reg_8(cpu, cpu->registers.y.low);
} 

void op_cpy_16(WDC65816* cpu) {
    compare_reg_16(cpu, cpu->registers.y.word);
} 

void schedule_dec_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_dec_8;
    schedule_rmw_a_8(cpu);
} 

void schedule_dec_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_dec_16;
    schedule_rmw_a_16(cpu);
}

void schedule_dec_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_dec_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_dec_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_dec_16;
    schedule_rmw_mem_16(cpu);
}

void op_dec_8(WDC65816* cpu) {
    set_nz_byte(cpu, --cpu->operand);
}

void op_dec_16(WDC65816* cpu) {
    set_nz_word(cpu, --cpu->operand);
}

void schedule_dex_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dex_8_1;
}

void schedule_dex_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dex_16_1;
}

void dex_8_1(WDC65816* cpu) {
    set_nz_byte(cpu, --cpu->registers.x.low);
    end_op(cpu);
}

void dex_16_1(WDC65816* cpu) {
    set_nz_word(cpu, --cpu->registers.x.word);
    end_op(cpu);
}

void schedule_dey_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dey_8_1;
}

void schedule_dey_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dey_16_1;
}

void dey_8_1(WDC65816* cpu) {
    set_nz_byte(cpu, --cpu->registers.y.low);
    end_op(cpu);
}

void dey_16_1(WDC65816* cpu) {
    set_nz_word(cpu, --cpu->registers.y.word);
    end_op(cpu);
}

void schedule_eor_8(WDC65816* cpu) {
    cpu->op_callback = &op_eor_8;
    schedule_load_mem_op_8(cpu);
}

void schedule_eor_16(WDC65816* cpu) {
    cpu->op_callback = &op_eor_16;
    schedule_load_mem_op_16(cpu);
}

void op_eor_8(WDC65816* cpu) {
    cpu->registers.a.low ^= cpu->operand;
    set_nz_byte(cpu, cpu->registers.a.low);
}

void op_eor_16(WDC65816* cpu) {
    cpu->registers.a.word ^= cpu->operand;
    set_nz_word(cpu, cpu->registers.a.word);
}

void schedule_inc_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_inc_8;
    schedule_rmw_a_8(cpu);
} 

void schedule_inc_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_inc_16;
    schedule_rmw_a_16(cpu);
}

void schedule_inc_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_inc_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_inc_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_inc_16;
    schedule_rmw_mem_16(cpu);
}

void op_inc_8(WDC65816* cpu) {
    set_nz_byte(cpu, ++cpu->operand);
}

void op_inc_16(WDC65816* cpu) {
    set_nz_word(cpu, ++cpu->operand);
}

void schedule_inx_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &inx_8_1;
}

void schedule_inx_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &inx_16_1;
}

void inx_8_1(WDC65816* cpu) {
    set_nz_byte(cpu, ++cpu->registers.x.low);
    end_op(cpu);
}

void inx_16_1(WDC65816* cpu) {
    set_nz_word(cpu, ++cpu->registers.x.word);
    end_op(cpu);
}

void schedule_iny_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &iny_8_1;
}

void schedule_iny_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &iny_16_1;
}

void iny_8_1(WDC65816* cpu) {
    set_nz_byte(cpu, ++cpu->registers.y.low);
    end_op(cpu);
}

void iny_16_1(WDC65816* cpu) {
    set_nz_word(cpu, ++cpu->registers.y.word);
    end_op(cpu);
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

void schedule_lda_8(WDC65816* cpu) {
    cpu->op_callback = &op_lda_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_lda_16(WDC65816* cpu) {
    cpu->op_callback = &op_lda_16;
    schedule_load_mem_op_16(cpu);
}

void op_lda_8(WDC65816* cpu) {
    cpu->registers.a.low = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
} 

void op_lda_16(WDC65816* cpu) {
    cpu->registers.a.word = cpu->operand;
    set_nz_word(cpu, cpu->operand);
} 

void schedule_ldx_8(WDC65816* cpu) {
    cpu->op_callback = &op_ldx_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_ldx_16(WDC65816* cpu) {
    cpu->op_callback = &op_ldx_16;
    schedule_load_mem_op_16(cpu);
}

void op_ldx_8(WDC65816* cpu) {
    cpu->registers.x.low = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
} 

void op_ldx_16(WDC65816* cpu) {
    cpu->registers.x.word = cpu->operand;
    set_nz_word(cpu, cpu->operand);
} 

void schedule_ldy_8(WDC65816* cpu) {
    cpu->op_callback = &op_ldy_8;
    schedule_load_mem_op_8(cpu);
} 

void schedule_ldy_16(WDC65816* cpu) {
    cpu->op_callback = &op_ldy_16;
    schedule_load_mem_op_16(cpu);
}

void op_ldy_8(WDC65816* cpu) {
    cpu->registers.y.low = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
} 

void op_ldy_16(WDC65816* cpu) {
    cpu->registers.y.word = cpu->operand;
    set_nz_word(cpu, cpu->operand);
} 

void schedule_lsr_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_lsr;
    schedule_rmw_a_8(cpu);
} 

void schedule_lsr_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_lsr;
    schedule_rmw_a_16(cpu);
}

void schedule_lsr_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_lsr;
    schedule_rmw_mem_8(cpu);
} 

void schedule_lsr_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_lsr;
    schedule_rmw_mem_16(cpu);
}

void op_lsr(WDC65816* cpu) {
    bool c = cpu->operand & 0x1;
    cpu->operand >>= 1;
    set_flag(cpu, WDC_FLAG_N, false);
    set_flag(cpu, WDC_FLAG_C, c);
    set_flag(cpu, WDC_FLAG_Z, cpu->operand == 0);
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

    if (get_flag(cpu, WDC_FLAG_X)) {
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

    if (get_flag(cpu, WDC_FLAG_X)) {
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

void schedule_ora_8(WDC65816* cpu) {
    cpu->op_callback = &op_ora_8;
    schedule_load_mem_op_8(cpu);
}

void schedule_ora_16(WDC65816* cpu) {
    cpu->op_callback = &op_ora_16;
    schedule_load_mem_op_16(cpu);
}

void op_ora_8(WDC65816* cpu) {
    cpu->registers.a.low |= cpu->operand;
    set_nz_byte(cpu, cpu->registers.a.low);
}

void op_ora_16(WDC65816* cpu) {
    cpu->registers.a.word |= cpu->operand;
    set_nz_word(cpu, cpu->registers.a.word);
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

void schedule_pha_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_pha_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
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

void schedule_phx_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.x.low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_phx_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.x.word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_16(cpu);
}

void schedule_phy_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.y.low;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_8(cpu);
}

void schedule_phy_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.y.word;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    schedule_push_op_16(cpu);
}

void schedule_pla_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_8_1;
}

void schedule_pla_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pla_16_2;
}

void pla_8_1(WDC65816* cpu) {
    cpu->registers.a.low = pull_stack(cpu);
    set_nz_byte(cpu, cpu->registers.a.low);
    end_op(cpu);
}

void pla_16_1(WDC65816* cpu) {
    cpu->registers.a.low = pull_stack(cpu);
}

void pla_16_2(WDC65816* cpu) {
    cpu->registers.a.high = pull_stack(cpu);
    set_nz_word(cpu, cpu->registers.a.word);
    end_op(cpu);
}

void schedule_plb(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plb_1;
}

void plb_1(WDC65816* cpu) {
    cpu->registers.dbr = pull_stack(cpu);
    set_nz_byte(cpu, cpu->registers.dbr);
    end_op(cpu);
}

void schedule_pld(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pld_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &pld_2;
}

void pld_1(WDC65816* cpu) {
    cpu->registers.d.low = pull_stack(cpu);
}

void pld_2(WDC65816* cpu) {
    cpu->registers.d.high = pull_stack(cpu);
    set_nz_word(cpu, cpu->registers.d.word);
    end_op(cpu);
}

void schedule_plp(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plp_1;
}

void plp_1(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, WDC_FLAG_I);
    cpu->registers.p = pull_stack(cpu);

    if (get_flag(cpu, WDC_FLAG_X)) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
    }
    end_op_flag_i(cpu, flag_i);
}

void schedule_plx_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plx_8_1;
}

void schedule_plx_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plx_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &plx_16_2;
}

void plx_8_1(WDC65816* cpu) {
    cpu->registers.x.low = pull_stack(cpu);
    set_nz_byte(cpu, cpu->registers.x.low);
    end_op(cpu);
}

void plx_16_1(WDC65816* cpu) {
    cpu->registers.x.low = pull_stack(cpu);
}

void plx_16_2(WDC65816* cpu) {
    cpu->registers.x.high = pull_stack(cpu);
    set_nz_word(cpu, cpu->registers.x.word);
    end_op(cpu);
}

void schedule_ply_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &ply_8_1;
}

void schedule_ply_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &ply_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &ply_16_2;
}

void ply_8_1(WDC65816* cpu) {
    cpu->registers.y.low = pull_stack(cpu);
    set_nz_byte(cpu, cpu->registers.y.low);
    end_op(cpu);
}

void ply_16_1(WDC65816* cpu) {
    cpu->registers.y.low = pull_stack(cpu);
}

void ply_16_2(WDC65816* cpu) {
    cpu->registers.y.high = pull_stack(cpu);
    set_nz_word(cpu, cpu->registers.y.word);
    end_op(cpu);
}

void schedule_rep(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rep_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &rep_2;
}

void rep_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void rep_2(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, WDC_FLAG_I);
    cpu->registers.p &= ~cpu->operand;
    end_op_flag_i(cpu, flag_i);
}

void schedule_rol_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_rol_8;
    schedule_rmw_a_8(cpu);
} 

void schedule_rol_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_rol_16;
    schedule_rmw_a_16(cpu);
}

void schedule_rol_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_rol_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_rol_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_rol_16;
    schedule_rmw_mem_16(cpu);
}

void op_rol_8(WDC65816* cpu) {
    bool c = cpu->operand >> 0x7;
    cpu->operand = (cpu->operand << 1) | get_flag(cpu, WDC_FLAG_C);
    cpu->operand &= 0xFF;
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_byte(cpu, cpu->operand);
}

void op_rol_16(WDC65816* cpu) {
    bool c = cpu->operand >> 0xF;
    cpu->operand = (cpu->operand << 1) | get_flag(cpu, WDC_FLAG_C);
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_word(cpu, cpu->operand);
}


void schedule_ror_a_8(WDC65816* cpu) {
    cpu->op_callback = &op_ror_8;
    schedule_rmw_a_8(cpu);
} 

void schedule_ror_a_16(WDC65816* cpu) {
    cpu->op_callback = &op_ror_16;
    schedule_rmw_a_16(cpu);
}

void schedule_ror_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_ror_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_ror_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_ror_16;
    schedule_rmw_mem_16(cpu);
}

void op_ror_8(WDC65816* cpu) {
    bool c = cpu->operand & 0x1;
    cpu->operand = (cpu->operand >> 1) | get_flag(cpu, WDC_FLAG_C) << 0x7;
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_byte(cpu, cpu->operand);
}

void op_ror_16(WDC65816* cpu) {
    bool c = cpu->operand & 0x1;
    cpu->operand = (cpu->operand >> 1) | get_flag(cpu, WDC_FLAG_C) << 0xF;
    set_flag(cpu, WDC_FLAG_C, c);
    set_nz_word(cpu, cpu->operand);
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
    if (get_flag(cpu, WDC_FLAG_X)) {
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

void schedule_sec(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sec_1;
}

void sec_1(WDC65816* cpu) {
    set_flag(cpu, WDC_FLAG_C, true);
    end_op(cpu);
}

void schedule_sed(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sed_1;
}

void sed_1(WDC65816* cpu) {
    set_flag(cpu, WDC_FLAG_D, true);
    end_op(cpu);
}

void schedule_sei(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sei_1;
}

void sei_1(WDC65816* cpu) {
    bool i = get_flag(cpu, WDC_FLAG_I);
    set_flag(cpu, WDC_FLAG_I, true);
    end_op_flag_i(cpu, i);
}

void schedule_sep(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sep_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &sep_2;
}

void sep_1(WDC65816* cpu) {
    cpu->operand = read_immediate(cpu);
}

void sep_2(WDC65816* cpu) {
    bool flag_i = get_flag(cpu, WDC_FLAG_I);
    cpu->registers.p |= cpu->operand;

    if (get_flag(cpu, WDC_FLAG_X)) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
    }
    end_op_flag_i(cpu, flag_i);
}

void schedule_store_op_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_op_8_1;
}

void schedule_store_op_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_op_16_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &store_op_16_2;
}

void store_op_8_1(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand);
    end_op(cpu);
}

void store_op_16_1(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand & 0xFF);
    increment_address(cpu);
}

void store_op_16_2(WDC65816* cpu) {
    write(cpu, cpu->operand_address, cpu->operand >> 8);
    increment_address(cpu);
    end_op(cpu);
}

void schedule_sta_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.low;
    schedule_store_op_8(cpu);
}

void schedule_sta_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.a.word;
    schedule_store_op_16(cpu);
}

void schedule_stz_8(WDC65816* cpu) {
    cpu->operand = 0x00;
    schedule_store_op_8(cpu);
}

void schedule_stz_16(WDC65816* cpu) {
    cpu->operand = 0x00;
    schedule_store_op_16(cpu);
}

void schedule_stx_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.x.low;
    schedule_store_op_8(cpu);
}

void schedule_stx_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.x.word;
    schedule_store_op_16(cpu);
}

void schedule_sty_8(WDC65816* cpu) {
    cpu->operand = cpu->registers.y.low;
    schedule_store_op_8(cpu);
}

void schedule_sty_16(WDC65816* cpu) {
    cpu->operand = cpu->registers.y.word;
    schedule_store_op_16(cpu);
}

void schedule_tax_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tax_8_1;
}

void schedule_tax_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tax_16_1;
}

void tax_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.a.low, &cpu->registers.x.low);
    end_op(cpu);

}

void tax_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.a.word, &cpu->registers.x.word);
    end_op(cpu);
}

void schedule_tay_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tay_8_1;
}

void schedule_tay_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tay_16_1;
}

void tay_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.a.low, &cpu->registers.y.low);
    end_op(cpu);

}

void tay_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.a.word, &cpu->registers.y.word);
    end_op(cpu);
}

void schedule_tcd(WDC65816* cpu) {
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
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tdc_1;
}

void tdc_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.d.word, &cpu->registers.a.word);
    end_op(cpu);
}

void schedule_tsc(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tsc_1;
}

void tsc_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.s.word, &cpu->registers.a.word);
    end_op(cpu);
}

void schedule_tsx_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tsx_8_1;
}

void schedule_tsx_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tsx_16_1;
}

void tsx_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.s.low, &cpu->registers.x.low);
    end_op(cpu);
}

void tsx_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.s.word, &cpu->registers.x.word);
    end_op(cpu);
}

void schedule_txa_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txa_8_1;
}

void schedule_txa_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txa_16_1;
}

void txa_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.x.low, &cpu->registers.a.low);
    end_op(cpu);
}

void txa_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.x.word, &cpu->registers.a.word);
    end_op(cpu);
}

void schedule_txs(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txs_1;
}

void txs_1(WDC65816* cpu) {
    cpu->registers.s.word = cpu->registers.x.word;
    end_op(cpu);
}

void schedule_txy_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txy_8_1;
}

void schedule_txy_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txy_16_1;
}

void txy_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.x.low, &cpu->registers.y.low);
    end_op(cpu);
}

void txy_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.x.word, &cpu->registers.y.word);
    end_op(cpu);
}

void schedule_tya_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tya_8_1;
}

void schedule_tya_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tya_16_1;
}

void tya_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.y.low, &cpu->registers.a.low);
    end_op(cpu);
}

void tya_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.y.word, &cpu->registers.a.word);
    end_op(cpu);
}

void schedule_tyx_8(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tyx_8_1;
}

void schedule_tyx_16(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tyx_16_1;
}

void tyx_8_1(WDC65816* cpu) {
    transfer_8(cpu, cpu->registers.y.low, &cpu->registers.x.low);
    end_op(cpu);
}

void tyx_16_1(WDC65816* cpu) {
    transfer_16(cpu, cpu->registers.y.word, &cpu->registers.x.word);
    end_op(cpu);
}

void schedule_trb_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_trb_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_trb_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_trb_16;
    schedule_rmw_mem_16(cpu);
}

void op_trb_8(WDC65816* cpu) {
    bool z = (cpu->operand & cpu->registers.a.low) == 0;
    cpu->operand &= (~cpu->registers.a.low);
    set_flag(cpu, WDC_FLAG_Z, z);
}

void op_trb_16(WDC65816* cpu) {
    bool z = (cpu->operand & cpu->registers.a.word) == 0;
    cpu->operand &= (~cpu->registers.a.word);
    set_flag(cpu, WDC_FLAG_Z, z);
}

void schedule_tsb_rmw_8(WDC65816* cpu) {
    cpu->op_callback = &op_tsb_8;
    schedule_rmw_mem_8(cpu);
} 

void schedule_tsb_rmw_16(WDC65816* cpu) {
    cpu->op_callback = &op_tsb_16;
    schedule_rmw_mem_16(cpu);
}

void op_tsb_8(WDC65816* cpu) {
    bool z = (cpu->operand & cpu->registers.a.low) == 0;
    cpu->operand |= cpu->registers.a.low;
    set_flag(cpu, WDC_FLAG_Z, z);
}

void op_tsb_16(WDC65816* cpu) {
    bool z = (cpu->operand & cpu->registers.a.word) == 0;
    cpu->operand |= cpu->registers.a.word;
    set_flag(cpu, WDC_FLAG_Z, z);
}

void schedule_wai(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &wai_1;
}

void wai_1(WDC65816* cpu) {
    cpu->wai = true;
    dummy_read_pc(cpu);
    end_op(cpu);
}

void schedule_wdm(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &wdm_1;
}

void wdm_1(WDC65816* cpu) {
    dummy_read_pc(cpu);
    cpu->registers.pc.word++;
    end_op(cpu);
}

void schedule_xba(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &xba_1;
}

void xba_1(WDC65816* cpu) {
    uint8_t high = cpu->registers.a.high, low = cpu->registers.a.low;
    cpu->registers.a.high = low;
    cpu->registers.a.low = high;
    set_nz_byte(cpu, high);
    end_op(cpu);
}

void schedule_xce(WDC65816* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &xce_1;
}

void xce_1(WDC65816* cpu) {
    bool e = cpu->emulation_mode;
    bool c = get_flag(cpu, WDC_FLAG_C);
    cpu->emulation_mode = c;
    set_flag(cpu, WDC_FLAG_C, e);

    if (c) {
        cpu->registers.x.high = 0x00;
        cpu->registers.y.high = 0x00;
        cpu->registers.s.high = 0x01;
        set_flag(cpu, WDC_FLAG_X, true);
        set_flag(cpu, WDC_FLAG_M, true);
    }

    end_op(cpu);
}

void schedule_adc_8(WDC65816* cpu) {
    cpu->op_callback = &op_adc_8;
    schedule_load_mem_op_8(cpu);
}

void schedule_adc_16(WDC65816* cpu) {
    cpu->op_callback = &op_adc_16;
    schedule_load_mem_op_16(cpu);
}

void op_adc_8(WDC65816* cpu) {
    int result = 0x00;

    if (get_flag(cpu, WDC_FLAG_D)) {
        result = (cpu->registers.a.low & 0x0F) + (cpu->operand & 0x0F) + get_flag(cpu, WDC_FLAG_C);
        if (result > 0x9) result = ((result + 0x06) & 0x0F) + 0x10;
        
        result = (cpu->registers.a.low & 0xF0) + (cpu->operand & 0xF0) + result;
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.low) & (result ^ cpu->operand) & 0x80);
        if (result > 0x9F) result += 0x60;
    } 

    else {
        result = cpu->registers.a.low + cpu->operand + get_flag(cpu, WDC_FLAG_C);
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.low) & (result ^ cpu->operand) & 0x80);
    }

    set_flag(cpu, WDC_FLAG_C, result > 0xFF);
    set_nz_byte(cpu, result);

    cpu->registers.a.low = result & 0xFF;
}

void op_adc_16(WDC65816* cpu) {
    int result = 0x0000;

    if (get_flag(cpu, WDC_FLAG_D)) {
        result = (cpu->registers.a.word & 0x0F) + (cpu->operand & 0x0F) + get_flag(cpu, WDC_FLAG_C);
        if (result > 0x9) result = ((result + 0x06) & 0x0F) + 0x10;

        result = (cpu->registers.a.word & 0xF0) + (cpu->operand & 0xF0) + result;
        if (result > 0x9F) result = ((result + 0x060) & 0x0FF) + 0x100;

        result = (cpu->registers.a.word & 0xF00) + (cpu->operand & 0xF00) + result;
        if (result > 0x9FF) result = ((result + 0x0600) & 0x0FFF) + 0x1000;

        result = (cpu->registers.a.word & 0xF000) + (cpu->operand & 0xF000) + result;
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.word) & (result ^ cpu->operand) & 0x8000);
        if (result > 0x9FFF) result += 0x6000;
    }

    else {
        result = cpu->registers.a.word + cpu->operand + get_flag(cpu, WDC_FLAG_C);
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.word) & (result ^ cpu->operand) & 0x8000);
    }

    set_flag(cpu, WDC_FLAG_C, result > 0xFFFF);
    set_nz_word(cpu, result);

    cpu->registers.a.word = result & 0xFFFF;
}

void schedule_sbc_8(WDC65816* cpu) {
    cpu->op_callback = &op_sbc_8;
    schedule_load_mem_op_8(cpu);
}

void schedule_sbc_16(WDC65816* cpu) {
    cpu->op_callback = &op_sbc_16;
    schedule_load_mem_op_16(cpu);
}

void op_sbc_8(WDC65816* cpu) {
    int result = 0x00;
    
    if (get_flag(cpu, WDC_FLAG_D)) {
        result = (cpu->registers.a.low & 0x0F) - (cpu->operand & 0x0F) - !get_flag(cpu, WDC_FLAG_C);
        if (result < 0) result = ((result - 0x06) & 0x0F) - 0x10;
        result = (cpu->registers.a.low & 0xF0) - (cpu->operand & 0xF0) + result;
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.low) & (result ^ ~cpu->operand) & 0x80);
        if (result < 0) result -= 0x60;
    }

    else {
        result = cpu->registers.a.low - cpu->operand - !get_flag(cpu, WDC_FLAG_C);
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.low) & (result ^ ~cpu->operand) & 0x80);
    }

    set_flag(cpu, WDC_FLAG_C, !(result < 0x00));
    set_nz_byte(cpu, result);

    cpu->registers.a.low = result & 0xFF;
}

void op_sbc_16(WDC65816* cpu) {
    int result = 0x0000;

    if (get_flag(cpu, WDC_FLAG_D)) {
        result = (cpu->registers.a.word & 0x0F) - (cpu->operand & 0x0F) - !get_flag(cpu, WDC_FLAG_C);
        if (result < 0) result = ((result - 0x06) & 0x0F) - 0x10;

        result = (cpu->registers.a.word & 0xF0) - (cpu->operand & 0xF0) + result;
        if (result < 0) result = ((result - 0x060) & 0x0FF) - 0x100;

        result = (cpu->registers.a.word & 0xF00) - (cpu->operand & 0xF00) + result;
        if (result < 0) result = ((result - 0x0600) & 0x0FFF) - 0x1000;

        result = (cpu->registers.a.word & 0xF000) - (cpu->operand & 0xF000) + result;

        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.word) & (result ^ ~cpu->operand) & 0x8000);

        if (result < 0) result -= 0x6000;
    }

    else {
        result = cpu->registers.a.word - cpu->operand - !get_flag(cpu, WDC_FLAG_C);
        set_flag(cpu, WDC_FLAG_V, (result ^ cpu->registers.a.word) & (result ^ ~cpu->operand) & 0x8000);
    }

    set_flag(cpu, WDC_FLAG_C, !(result < 0x0000));
    set_nz_word(cpu, result);

    cpu->registers.a.word = result & 0xFFFF;
}

void reset_1(WDC65816* cpu) {
    read(cpu, cpu->registers.s.word--);
}

void reset_2(WDC65816* cpu) {
    cpu->registers.pc.word = read(cpu, VECTOR_RESET);
}

void reset_3(WDC65816* cpu) {
    cpu->registers.pc.word |= read(cpu, VECTOR_RESET + 1) << 8;
    end_op(cpu);
}

#undef EXEC_OP
#undef EXEC_OP_M
#undef EXEC_OP_X