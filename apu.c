#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "include/apu.h"
#include "include/processor/spc700/spc700.h"

static bool get_bit(int value, int index);

static uint8_t read_internal_io(apu* apu, uint8_t address);
static void write_internal_io(apu* apu, uint8_t address, uint8_t data);

static uint8_t read_apu_bus(void* bus, uint16_t address);
static void write_apu_bus(void* bus, uint16_t address, uint8_t data);

static const uint8_t ipl_rom[0x40] = {
    0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0, 0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
    0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4, 0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
    0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB, 0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
    0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD, 0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
};

bool get_bit(int value, int index) {
    return (value >> index) & 0x1;
}

apu* new_apu(uint8_t* audio_ram) {
    apu* result = malloc(sizeof(apu));
    apu_bus* bus = malloc(sizeof(apu_bus));

    bus->audio_ram = audio_ram;
    bus->apu = result;

    bus->base.read = &read_apu_bus;
    bus->base.write = &write_apu_bus;

    init_spc700((SPC700*) result, bus);
    reset_apu(result);

    return result;
}

void free_apu(apu* apu) {
    free(apu->base.bus);
    free(apu);
}

void reset_apu(apu* apu) {
    apu->base.registers.pc = 0xFFC0;
    apu->base.registers.sp = 0xFF;

    apu->base.registers.psw = 0x00;
    apu->base.registers.x = 0x00;
    apu->base.registers.y = 0x00;
    apu->base.registers.a = 0x00;

    apu->ipl_enable = true;
    
    apu->timer[0].count = 0;
    apu->timer[1].count = 0;
    apu->timer[2].count = 0;

    apu->timer[0].interval = 0;
    apu->timer[1].interval = 0;
    apu->timer[2].interval = 0;
}

uint8_t read_apu_bus(void* bus, uint16_t address) {
    apu_bus* b = (apu_bus*) bus;
    
    if (address < 0xF0 || (address > 0xFF && address < 0xFFC0) || address == 0x00F8 || address == 0x00F9) {
        return b->audio_ram[address];
    }

    if (address > 0xFFBF) {
        return (b->apu->ipl_enable) ? ipl_rom[address & 0x3F] : b->audio_ram[address];
    }

    return read_internal_io(b->apu, address);
}

void write_apu_bus(void* bus, uint16_t address, uint8_t data){
    apu_bus* b = (apu_bus*) bus;

    if (address < 0xF0 || address > 0xFF || address == 0x00F8 || address == 0x00F9) {
        b->audio_ram[address] = data;
        return;
    }

    write_internal_io(b->apu, address, data);
}

uint8_t read_internal_io(apu* apu, uint8_t address) {
    uint8_t result = 0x00;
    address &= 0xF;

    switch (address) {
        case 0x2: 
            result = apu->dsp_address & 0x7F;
            break;
        
        case 0x3: 
            break; //dsp read

        case 0x4: case 0x5: case 0x6: case 0x7: 
            uint8_t port = address &= 0x3;
            result = apu->io_port[port].data_in;
            break;

        case 0xD: case 0xE: case 0xF:
            uint8_t timer = (address - 1) & 0x3;
            result = apu->timer[timer].count;
            apu->timer[timer].count = 0;
            break;
    }

    return result;
}

void write_internal_io(apu* apu, uint8_t address, uint8_t data) {
    address &= 0xF;

    switch (address) {
        case 0x0: 
            break; //test register

        case 0x1: 
            bool timer_0_enable = get_bit(data, 0);
            bool timer_1_enable = get_bit(data, 1);
            bool timer_2_enable = get_bit(data, 2);

            bool clear_ports_0_1 = get_bit(data, 4);
            bool clear_ports_2_3 = get_bit(data, 5);

            bool ipl_enable = get_bit(data, 5);

            //handle timer scheduling

            if (clear_ports_0_1) {
                apu->io_port[0].data_in = 0;
                apu->io_port[1].data_in = 0;
            }

            if (clear_ports_2_3) {
                apu->io_port[2].data_in = 0;
                apu->io_port[3].data_in = 0;
            }

            apu->timer[0].count = 0;
            apu->timer[1].count = 0;
            apu->timer[2].count = 0;

            apu->ipl_enable = ipl_enable;
            break;

        case 0x4: case 0x5: case 0x6: case 0x7:
            uint8_t port = address &= 0x3;
            apu->io_port[port].data_out = data;
            break;

        case 0xA: case 0xB: case 0xC:
            uint8_t timer = (address + 2) & 0x3;
            apu->timer[timer].interval = data;
            break;
    }
}
