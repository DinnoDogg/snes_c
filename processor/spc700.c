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

#define EXEC_OP_IMPLIED(OPCODE, INSTR) \
    case OPCODE: \
        INSTR(cpu); \
        break

const static int direct_page_base[2] = {0x0000, 0x0100};

static uint16_t get_ya(SPC700* cpu);
static void set_ya(SPC700* cpu, uint16_t value);

static int get_dp_base(SPC700* cpu);

static bool get_flag(SPC700* cpu, SPC700_flag flag);
static void set_flag(SPC700* cpu, SPC700_flag flag, bool value);

static uint8_t read(SPC700* cpu, uint16_t address);
static void write(SPC700* cpu, uint16_t address, uint8_t data);

static uint8_t read_immediate(SPC700* cpu);

static void load_operand_byte(SPC700* cpu);

static void load_operand_word_high(SPC700* cpu);
static void load_operand_word_low(SPC700* cpu);

static void increment_address(SPC700* cpu);

static void set_nz_byte(SPC700* cpu, uint8_t value);
static void set_nz_word(SPC700* cpu, uint16_t value);

static void dummy_read_pc(SPC700* cpu);
static void dummy_read_operand(SPC700* cpu);

static void transfer_reg(SPC700* cpu, uint8_t source, uint8_t* dest);

static void end_op(SPC700* cpu);

void init_spc700(SPC700* cpu, read_callback read, write_callback write) {
    cpu->read = read;
    cpu->write = write;
}

void cycle_spc700(SPC700* cpu) {
    if (cpu->current_cycle++ == 0) {
        uint8_t opcode = read_immediate(cpu);

        switch (opcode) {
            EXEC_OP(0xE8, schedule_addr_imm, schedule_mov_a_mem, false);
            EXEC_OP(0xE6, schedule_addr_indr, schedule_mov_a_mem, false);
            EXEC_OP(0xBF, schedule_addr_indr_inc, schedule_mov_a_mem, false);
            EXEC_OP(0xE4, schedule_addr_dp, schedule_mov_a_mem, false);
            EXEC_OP(0xF4, schedule_addr_dp_x, schedule_mov_a_mem, false);
            EXEC_OP(0xE5, schedule_addr_a, schedule_mov_a_mem, false);
            EXEC_OP(0xF5, schedule_addr_a_x, schedule_mov_a_mem, false);
            EXEC_OP(0xF6, schedule_addr_a_y, schedule_mov_a_mem, false);
            EXEC_OP(0xE7, schedule_addr_dp_x_indr, schedule_mov_a_mem, false);
            EXEC_OP(0xF7, schedule_addr_dp_indr_y, schedule_mov_a_mem, false);

            EXEC_OP(0xCD, schedule_addr_imm, schedule_mov_x_mem, false);
            EXEC_OP(0xF8, schedule_addr_dp, schedule_mov_x_mem, false);
            EXEC_OP(0xF9, schedule_addr_dp_y, schedule_mov_x_mem, false);
            EXEC_OP(0xE9, schedule_addr_a, schedule_mov_x_mem, false);

            EXEC_OP(0x8D, schedule_addr_imm, schedule_mov_y_mem, false);
            EXEC_OP(0xEB, schedule_addr_dp, schedule_mov_y_mem, false);
            EXEC_OP(0xFB, schedule_addr_dp_x, schedule_mov_y_mem, false);
            EXEC_OP(0xEC, schedule_addr_a, schedule_mov_y_mem, false);

            EXEC_OP(0xC6, schedule_addr_indr, schedule_mov_mem_a, true);
            EXEC_OP(0xAF, schedule_addr_indr_inc, schedule_mov_mem_a, true);
            EXEC_OP(0xC4, schedule_addr_dp, schedule_mov_mem_a, true);
            EXEC_OP(0xD4, schedule_addr_dp_x, schedule_mov_mem_a, true);
            EXEC_OP(0xC5, schedule_addr_a, schedule_mov_mem_a, true);
            EXEC_OP(0xD5, schedule_addr_a_x, schedule_mov_mem_a, true);
            EXEC_OP(0xD6, schedule_addr_a_y, schedule_mov_mem_a, true);
            EXEC_OP(0xC7, schedule_addr_dp_x_indr, schedule_mov_mem_a, true);
            EXEC_OP(0xD7, schedule_addr_dp_indr_y, schedule_mov_mem_a, true);

            EXEC_OP(0xD8, schedule_addr_dp, schedule_mov_mem_x, true);
            EXEC_OP(0xD9, schedule_addr_dp_y, schedule_mov_mem_x, true);
            EXEC_OP(0xC9, schedule_addr_a, schedule_mov_mem_x, true);

            EXEC_OP(0xCB, schedule_addr_dp, schedule_mov_mem_y, true);
            EXEC_OP(0xDB, schedule_addr_dp_x, schedule_mov_mem_y, true);
            EXEC_OP(0xCC, schedule_addr_a, schedule_mov_mem_y, true);

            EXEC_OP_IMPLIED(0x7D, schedule_txa);
            EXEC_OP_IMPLIED(0xDD, schedule_tya);
            EXEC_OP_IMPLIED(0x5D, schedule_tax);
            EXEC_OP_IMPLIED(0xFD, schedule_tay);
            EXEC_OP_IMPLIED(0x9D, schedule_tsx);
            EXEC_OP_IMPLIED(0xBD, schedule_txs);
            EXEC_OP_IMPLIED(0xFA, schedule_mov_dp_dp);
            EXEC_OP_IMPLIED(0x8F, schedule_mov_dp_imm);

        }
    }

    else {
        cpu->cycle_lookup[cpu->current_cycle - 2](cpu);
    }
}

int spc700_run_instruction(SPC700* cpu) {
    int cycle_count = 0;

    do {
        cycle_spc700(cpu);
        cycle_count++;
    } while (cpu->current_cycle != 0);

    return cycle_count;
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
    printf("Indirect address: %u\n\n", cpu->indirect_address);
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
    return (cpu->registers.psw >> flag) & 0x1;
}

void set_flag(SPC700* cpu, SPC700_flag flag, bool value) {
    if (value) {
        cpu->registers.psw |= 1 << flag;
        return;
    }
    cpu->registers.psw &= ~(1 << flag);
}

uint8_t read(SPC700* cpu, uint16_t address) {
    return cpu->read(address);
}

void write(SPC700* cpu, uint16_t address, uint8_t data) {
    cpu->write(address, data);
}

uint8_t read_immediate(SPC700* cpu) {
    return read(cpu, cpu->registers.pc++);
}

void load_operand_byte(SPC700* cpu) {
    cpu->operand = read(cpu, cpu->operand_address);
}

void load_operand_word_low(SPC700* cpu) {
    cpu->operand = read(cpu, cpu->operand_address);
    increment_address(cpu);
}

void load_operand_word_high(SPC700* cpu) {
    cpu->operand |= read(cpu, cpu->operand_address) << 8;
}

void dummy_read_pc(SPC700* cpu) {
    read(cpu, cpu->registers.pc);
}

void dummy_read_operand(SPC700* cpu) {
    read(cpu, cpu->operand_address);
}

void transfer_reg(SPC700* cpu, uint8_t source, uint8_t* dest) {
    *dest = source;
    set_nz_byte(cpu, source);
}

void increment_address(SPC700* cpu) {
    cpu->operand_address++;
    
    /*if (cpu->direct_page) {
        cpu->operand_address &= 0xFF;
        cpu->operand_address |= get_dp_base(cpu);
    }*/
}

void end_op(SPC700* cpu) {
    cpu->current_cycle = 0;
    cpu->cycle_lookup_index = 0;
}

void set_nz_byte(SPC700* cpu, uint8_t value) {
    set_flag(cpu, FLAG_N, value >> 0x7);
    set_flag(cpu, FLAG_Z, value == 0);
}

void set_nz_word(SPC700* cpu, uint16_t value) {
    set_flag(cpu, FLAG_N, value >> 0xF);
    set_flag(cpu, FLAG_Z, value == 0);
}

void schedule_addr_imm(SPC700* cpu) {
    cpu->operand_address = cpu->registers.pc++;
}

void schedule_addr_indr(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_indr_1(SPC700* cpu) {
    cpu->operand_address = cpu->registers.x | get_dp_base(cpu);
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_indr_inc(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_indr_inc_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
}

void addr_indr_inc_1(SPC700* cpu) {
    cpu->operand_address = cpu->registers.x++ | get_dp_base(cpu);
    dummy_read_pc(cpu);
}

void schedule_addr_dp(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_dp_1(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) | get_dp_base(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_dp_x(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_x_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_x_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_dp_x_1(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) + cpu->registers.x;
    cpu->operand_address = (cpu->operand_address & 0xFF) | get_dp_base(cpu);
}

void addr_dp_x_2(SPC700* cpu) {
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_dp_y(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_y_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_dp_y_2(SPC700* cpu) {
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void addr_dp_y_1(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) + cpu->registers.y;
    cpu->operand_address = (cpu->operand_address & 0xFF) | get_dp_base(cpu);
}

void schedule_addr_a(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_a_1(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu);
}

void addr_a_2(SPC700* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_a_x(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_x_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_x_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_a_x_1(SPC700* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
    cpu->operand_address += cpu->registers.x;
}

void addr_a_x_2(SPC700* cpu) {
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_a_y(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_a_y_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
}

void addr_a_y_1(SPC700* cpu) {
    cpu->operand_address |= read_immediate(cpu) << 8;
    cpu->operand_address += cpu->registers.y;
}

void addr_a_y_2(SPC700* cpu) {
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_dp_x_indr(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_x_indr_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_pc;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_x_indr_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_x_indr_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_dp_x_indr_1(SPC700* cpu) {
    cpu->indirect_address = read_immediate(cpu) + cpu->registers.x;
    cpu->indirect_address = (cpu->indirect_address & 0xFF) | get_dp_base(cpu);
}

void addr_dp_x_indr_2(SPC700* cpu) {
    cpu->operand_address = read(cpu, cpu->indirect_address++);
    cpu->indirect_address = (cpu->indirect_address & 0xFF) | get_dp_base(cpu);
}

void addr_dp_x_indr_3(SPC700* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 8;
    if (!cpu->dummy_read) cpu->current_cycle++;
}

void schedule_addr_dp_indr_y(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_indr_y_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_indr_y_2;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_indr_y_3;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &addr_dp_indr_y_4;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
}

void addr_dp_indr_y_1(SPC700* cpu) {
    cpu->indirect_address = read_immediate(cpu) | get_dp_base(cpu);
}

void addr_dp_indr_y_2(SPC700* cpu) {
    cpu->operand_address = read(cpu, cpu->indirect_address++);
    cpu->indirect_address = (cpu->indirect_address & 0xFF) | get_dp_base(cpu);
}

void addr_dp_indr_y_3(SPC700* cpu) {
    cpu->operand_address |= read(cpu, cpu->indirect_address) << 8;
    cpu->operand_address += cpu->registers.y;
}

void addr_dp_indr_y_4(SPC700* cpu) {
    dummy_read_pc(cpu);
    if (!cpu->dummy_read) cpu->current_cycle++;
}


void schedule_mov_a_mem(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_a_mem_1;
}

void mov_a_mem_1(SPC700* cpu) {
    load_operand_byte(cpu);
    cpu->registers.a = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
    end_op(cpu);
}

void schedule_mov_x_mem(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_x_mem_1;
}

void mov_x_mem_1(SPC700* cpu) {
    load_operand_byte(cpu);
    cpu->registers.x = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
    end_op(cpu);
}

void schedule_mov_y_mem(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_y_mem_1;
}

void mov_y_mem_1(SPC700* cpu) {
    load_operand_byte(cpu);
    cpu->registers.y = cpu->operand;
    set_nz_byte(cpu, cpu->operand);
    end_op(cpu);
}

void schedule_mov_mem_a(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_mem_a_1;
}

void mov_mem_a_1(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.a);
    end_op(cpu);
}

void schedule_mov_mem_x(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_mem_x_1;
}

void mov_mem_x_1(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.x);
    end_op(cpu);
}

void schedule_mov_mem_y(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_mem_y_1;
}

void mov_mem_y_1(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->registers.y);
    end_op(cpu);
}

void schedule_txa(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txa_1;
}

void txa_1(SPC700* cpu) {
    transfer_reg(cpu, cpu->registers.x, &cpu->registers.a);
    end_op(cpu);
}

void schedule_tya(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tya_1;
}

void tya_1(SPC700* cpu) {
    transfer_reg(cpu, cpu->registers.y, &cpu->registers.a);
    end_op(cpu);
}

void schedule_tax(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tax_1;
}

void tax_1(SPC700* cpu) {
    transfer_reg(cpu, cpu->registers.a, &cpu->registers.x);
    end_op(cpu);
}

void schedule_tay(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tay_1;
}

void tay_1(SPC700* cpu) {
    transfer_reg(cpu, cpu->registers.a, &cpu->registers.y);
    end_op(cpu);
}

void schedule_tsx(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &tsx_1;
}

void tsx_1(SPC700* cpu) {
    transfer_reg(cpu, cpu->registers.sp, &cpu->registers.x);
    end_op(cpu);
}

void schedule_txs(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &txs_1;
}

void txs_1(SPC700* cpu) {
    cpu->registers.sp = cpu->registers.x;
    end_op(cpu);
}

void schedule_mov_dp_dp(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_dp_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &load_operand_byte;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_dp_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_dp_2;
}

void mov_dp_dp_1(SPC700* cpu) {
    cpu->operand_address = read_immediate(cpu) | get_dp_base(cpu);
}

void mov_dp_dp_2(SPC700* cpu) {
    write(cpu, cpu->operand_address, cpu->operand);
    end_op(cpu);
}

void schedule_mov_dp_imm(SPC700* cpu) {
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_imm_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_dp_1;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &dummy_read_operand;
    cpu->cycle_lookup[cpu->cycle_lookup_index++] = &mov_dp_dp_2;
}

void mov_dp_imm_1(SPC700* cpu) {
    cpu->operand = read_immediate(cpu);
}
