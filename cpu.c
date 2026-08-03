#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/cart.h"
#include "include/apu.h"
#include "include/cpu.h"
#include "include/processor/wdc65816/wdc65816.h"

static uint8_t read_cpu_bus(void* cpu, uint32_t address, bool valid_access);
static void write_cpu_bus(void* cpu, uint32_t address, uint8_t data);

static uint8_t read_bus_b(s_cpu* cpu, uint8_t address);
static void write_bus_b(s_cpu* cpu, uint8_t address, uint8_t data);

static uint8_t read_internal_io(s_cpu* cpu, uint16_t address);
static void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data);

s_cpu* new_cpu(cart* cart, wram* wram, apu* apu) {
    s_cpu* result = malloc(sizeof(s_cpu));

    init_wdc65816((WDC65816*) result, &read_cpu_bus, &write_cpu_bus);

    result->bus.wram = wram;
    result->bus.apu = apu;
    result->bus.cart = cart;

    wdc65816_reset((WDC65816*) result);

    return result;
}

void free_cpu(s_cpu* cpu) {
    free(cpu);
}

uint8_t read_cpu_bus(void* cpu, uint32_t address, bool valid_access) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    uint8_t result = cpu_ptr->mdr;

    uint8_t bank = address >> 0x10;
    uint8_t offset = (address >> 0x8) & 0xFF;

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {

        if (offset < 0x20) {
            result = cpu_ptr->bus.wram->memory[address & 0x1FFF];
        }

        else if (offset == 0x21) {
            result = read_bus_b(cpu_ptr, address & 0xFF);
        }

        else if (offset > 0x3F && offset < 0x44) {
            result = read_internal_io(cpu_ptr, address & 0x3FF);
        }

        else if (offset > 0x7F) {
            //read cart
        }
    }

    else if (bank == 0x7E || bank == 0x7F) {
        result = cpu_ptr->bus.wram->memory[address & 0x1FFFF];
    }

    else {
        //read cart
    }

    if (valid_access) {
        cpu_ptr->mdr = result;
    }

    return result;
}

void write_cpu_bus(void* cpu, uint32_t address, uint8_t data) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    cpu_ptr->mdr = data;

    uint8_t bank = address >> 0x10;
    uint8_t offset = (address >> 0x8) & 0xFF;

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {

        if (offset < 0x20) {
            cpu_ptr->bus.wram->memory[address & 0x1FFF] = data;
            return;
        }

        if (offset == 0x21) {
            write_bus_b(cpu_ptr, address & 0xFF, data);
            return;
        }

        if (offset > 0x3F && offset < 0x44) {
            write_internal_io(cpu_ptr, address & 0x3FF, data);
            return;
        }

        if (offset > 0x7F) {
            //write cart
            return;
        }
    }

    if (bank == 0x7E || bank == 0x7F) {
        cpu_ptr->bus.wram->memory[address & 0x1FFFF] = data;
        return;
    }

    //write cart
}

uint8_t read_bus_b(s_cpu* cpu, uint8_t address) {

}

void write_bus_b(s_cpu* cpu, uint8_t address, uint8_t data) {

}

uint8_t read_internal_io(s_cpu* cpu, uint16_t address) {
    return 0;
}

void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data) {
    
}