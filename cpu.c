#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/cart.h"
#include "include/apu.h"
#include "include/cpu.h"
#include "include/processor/wdc65816/wdc65816.h"

static bool get_bit(int data, int index);

static uint8_t read_cpu_bus(void* cpu, uint32_t address, bool valid_access);
static void write_cpu_bus(void* cpu, uint32_t address, uint8_t data);

static uint8_t read_bus_b(s_cpu* cpu, uint8_t address);
static void write_bus_b(s_cpu* cpu, uint8_t address, uint8_t data);

static uint8_t read_internal_io(s_cpu* cpu, uint16_t address);
static void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data);

static cpu_waitstate_speed get_waitstates(s_cpu* cpu, uint32_t address);

static const int GLOBAL_CPU_VERSION = 2;

bool get_bit(int data, int index) {
    return (data >> index) & 0x1;
}

s_cpu* new_cpu(cart* cart, wram* wram, apu* apu) {
    s_cpu* result = malloc(sizeof(s_cpu));

    init_wdc65816((WDC65816*) result, &read_cpu_bus, &write_cpu_bus);

    result->bus.wram = wram;
    result->bus.apu = apu;
    result->bus.cart = cart;

    reset_cpu(result);

    return result;
}

void free_cpu(s_cpu* cpu) {
    free(cpu);
}

void reset_cpu(s_cpu* cpu) {
    wdc65816_reset((WDC65816*) cpu);

    cpu->vblank_nmi_enable = false;
    cpu->vblank_flag = false;
}

int cycle_cpu(s_cpu* cpu) {
    cpu->waitstate_count = 0;
    cycle_wdc65816((WDC65816*) cpu);
    return CLOCK_DIVISOR_CPU + cpu->waitstate_count;
}

uint8_t read_cpu_bus(void* cpu, uint32_t address, bool valid_access) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    cpu_ptr->waitstate_count += get_waitstates(cpu_ptr, address);

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
            result = read_cart(cpu_ptr->bus.cart, address, cpu_ptr->mdr); 
        }
    }

    else if (bank == 0x7E || bank == 0x7F) {
        result = cpu_ptr->bus.wram->memory[address & 0x1FFFF];
    }

    else {
        result = read_cart(cpu_ptr->bus.cart, address, cpu_ptr->mdr); 
    }

    if (valid_access) {
        cpu_ptr->mdr = result;
    }

    return result;
}

void write_cpu_bus(void* cpu, uint32_t address, uint8_t data) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    cpu_ptr->waitstate_count += get_waitstates(cpu_ptr, address);

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
            write_cart(cpu_ptr->bus.cart, address, data);
            return;
        }
    }

    if (bank == 0x7E || bank == 0x7F) {
        cpu_ptr->bus.wram->memory[address & 0x1FFFF] = data;
        return;
    }

    write_cart(cpu_ptr->bus.cart, address, data);
}

uint8_t read_bus_b(s_cpu* cpu, uint8_t address) {
    uint8_t result = cpu->mdr;
    
    if (address < 0x40) {
        //read ppu
    }

    else if (address < 0x80) {
        result = read_apu_io(cpu->bus.apu, address & 0x3);
    }

    else if (address == 0x80) {
        result = read_wram_io(cpu->bus.wram);
    }

    return result;
}

void write_bus_b(s_cpu* cpu, uint8_t address, uint8_t data) {
    //printf("b bus write\n");

    if (address < 0x40) {
        //write ppu
        return;
    }

    if (address < 0x80) {
        write_apu_io(cpu->bus.apu, address & 0x3, data);
        return;
    }

    if (address < 0x84) {
        write_wram_io(cpu->bus.wram, address & 0x3, data);
        return;
    }
}

uint8_t read_internal_io(s_cpu* cpu, uint16_t address) {
    uint8_t result = cpu->mdr;

    switch (address) {
        case 0x016: //JOYA
            //joypad a
            //clock joypad
            result = 0;
            break;

        case 0x017: //JOYB
            //joypad b
            //clock joypad
            result = 0;
            break;

        case 0x210: //RDNMI
            result &= 0x70;
            result |= GLOBAL_CPU_VERSION | cpu->nmi_flag << 7;
            cpu->nmi_flag = false;
            break;

        case 0x211: //TIMEUP
            result &= 0x7F;
            result |= cpu->irq_flag << 7;
            cpu->irq_flag = false;
            break;

        case 0x212: //HVBJOY
            result &= 0x3E;
            result |= (cpu->vblank_flag << 7) | (cpu->hblank_flag << 6) | cpu->auto_joypad_busy;
            break;

        case 0x213: //RDIO
            //joypad io
            result = 0;
            break;

        case 0x214: //RDDIVL
            result = cpu->quotient & 0xFF;
            break;

        case 0x215: //RDDIVH
            result = cpu->quotient >> 8;
            break;

        case 0x216: //RDMPYL
            result = cpu->product_and_remainder & 0xFF;
            break;

        case 0x217: //RDMPYH
            result = cpu->product_and_remainder >> 8;
            break;

        //218-21F joypad data
        case 0x218: case 0x219:
        case 0x21A: case 0x21B:
        case 0x21C: case 0x21D:
        case 0x21E: case 0x21F:
            result = 0;
            break;
    }
}

void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data) {
    //printf("internal write\n");

    switch (address) {
        case 0x016: //JOYWR
            //write joypad
            break;

        case 0x200: //NMITIMEN
            bool nmi_enable = get_bit(data, 7);
            bool auto_joypad_enable = get_bit(data, 0);
            uint8_t h_v_irq_mode = (data & 0x30) >> 4;

            if (h_v_irq_mode == 0) {
                cpu->irq_flag = false;
            }

            cpu->vblank_nmi_enable = nmi_enable;

            //schedule auto joypad
            //schedule irq
            break;

        case 0x201: //WRIO
            //joypad io
            break;

        case 0x202: //WRMPYA
            cpu->multiplicand = data;
            break;
        
        case 0x203: //WRMPYB
            cpu->multiplier = data;
            //schedule multiplication
            break;

        case 0x204: //WRDIVL
            cpu->dividend &= 0xFF00;
            cpu->dividend |= data;
            break;

        case 0x205: //WRDIVH
            cpu->dividend &= 0xFF;
            cpu->dividend |= data << 8;
            break;

        case 0x206: //WRDIVB
            cpu->divisor = data;
            ///schedule division
            break;

        case 0x207: //HTIMEL
            cpu->h_timer_count &= 0xFF00;
            cpu->h_timer_count |= data;
            //reschedule timer
            break;
            
        case 0x208: //HTIMEH
            cpu->h_timer_count &= 0xFF;
            cpu->h_timer_count |= data << 8;
            //reschedule timer
            break;

        case 0x209: //VTIMEL
            cpu->v_timer_count &= 0xFF00;
            cpu->v_timer_count |= data;
            //reschedule timer
            break;

        case 0x20A: //VTIMEH
            cpu->v_timer_count &= 0xFF;
            cpu->v_timer_count |= data << 8;
            //reschedule timer
            break;

        case 0x20B: //MDMAEN
            //dma
            break;

        case 0x20C: //HDMAEN
            //hdma
            break;

        case 0x20D: //MEMSEL
            bool speed = get_bit(data, 0);
            cpu->memory_2_region_speed = speed;
            break; 

        //dma
    }
}

cpu_waitstate_speed get_waitstates(s_cpu* cpu, uint32_t address) {
    uint8_t bank = address >> 0x10;
    uint8_t offset = (address >> 0x8) & 0xFF;

    if (bank < 0x40) {
        if (offset < 0x20 || offset > 0x5F)  {
            return CPU_WAITSTATE_SLOW;
        }

        if (offset > 0x3F && offset < 0x42) {
            return CPU_WAITSTATE_X_SLOW;
        }

        return CPU_WAITSTATE_FAST;
    }

    if (bank < 0x80) {
        return CPU_WAITSTATE_SLOW;
    }

    if (bank < 0xC0) {
        if (offset == 0x80) {
            return cpu->memory_2_region_speed ? CPU_WAITSTATE_FAST : CPU_WAITSTATE_SLOW;
        }

        if (offset < 0x20 || (offset > 0x5F && offset < 0x80))  {
            return CPU_WAITSTATE_SLOW;
        }

        if (offset > 0x3F && offset < 0x42) {
            return CPU_WAITSTATE_X_SLOW;
        }

        return CPU_WAITSTATE_FAST;
    }

    return cpu->memory_2_region_speed ? CPU_WAITSTATE_FAST : CPU_WAITSTATE_SLOW;
}