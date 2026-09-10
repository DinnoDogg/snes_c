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

static uint8_t read_bus_a(void* cpu, uint32_t address);
static void write_bus_a(void* cpu, uint32_t address, uint8_t data);

static uint8_t read_bus_b(s_cpu* cpu, uint8_t address);
static void write_bus_b(s_cpu* cpu, uint8_t address, uint8_t data);

static uint8_t read_internal_io(s_cpu* cpu, uint16_t address);
static void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data);

static uint8_t read_dma_io(s_cpu* cpu, uint16_t address);
static void write_dma_io(s_cpu* cpu, uint16_t address, uint8_t data);

static cpu_waitstate_speed get_waitstates(s_cpu* cpu, uint32_t address);

static void schedule_timer_irq(s_cpu* cpu);

static int get_dma_pattern_increment(uint32_t bytes_transfered, uint8_t pattern);
static void handle_dma(void* state);

static void handle_timer_irq(void* state);

static const int GLOBAL_CPU_VERSION = 2;

bool get_bit(int data, int index) {
    return (data >> index) & 0x1;
}

s_cpu* new_cpu(cart* cart, scheduler* scheduler, wram* wram, apu* apu, int* cycle_count) {
    s_cpu* result = calloc(1, sizeof(s_cpu));

    init_wdc65816((WDC65816*) result, &read_cpu_bus, &write_cpu_bus);

    result->scheduler = scheduler;

    result->bus.wram = wram;
    result->bus.apu = apu;
    result->bus.cart = cart;

    result->cycle_count = cycle_count;

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
    cpu->memory_2_region_speed = false;

    for (int i = 0; i < 8; i++) {
        cpu->dma_channel[i].bus_b_address = 0xFF;
        cpu->dma_channel[i].bus_a_address.offset = 0xFFFF;
        cpu->dma_channel[i].bus_a_address.bank = 0xFF;
        cpu->dma_channel[i].count = 0xFFFF;
        cpu->dma_channel[i].unused_byte = 0xFF;
        cpu->dma_channel[i].enabled = false;
    }
}

int cycle_cpu(s_cpu* cpu) {
    cpu->waitstate_count = 0;
    cycle_wdc65816((WDC65816*) cpu);
    
    wdc65816_set_nmi_line((WDC65816*) cpu, cpu->nmi_flag && cpu->vblank_nmi_enable);
    wdc65816_set_irq_line((WDC65816*) cpu, cpu->irq_flag);

    return CLOCK_DIVISOR_CPU + cpu->waitstate_count;
}

uint8_t read_cpu_bus(void* cpu, uint32_t address, bool valid_access) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    uint8_t result = cpu_ptr->mdr;

    uint8_t bank = address >> 0x10;
    uint16_t offset = address & 0xFFFF;

    if (!valid_access) {
        return result;
    }

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {
        
        if (offset > 0x20FF && offset < 0x2200) {
            result = read_bus_b(cpu, address & 0xFF);
        }

        else if (offset > 0x3FFF && offset < 0x4400) {
            result = read_internal_io(cpu, address & 0x3FF);
        }

        else {
            result = read_bus_a(cpu, address);
        }
    }

    else {
        result = read_bus_a(cpu, address);
    }

    cpu_ptr->mdr = result;
    cpu_ptr->waitstate_count += get_waitstates(cpu_ptr, address);

    return result;
}

void write_cpu_bus(void* cpu, uint32_t address, uint8_t data) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    cpu_ptr->waitstate_count += get_waitstates(cpu_ptr, address);

    cpu_ptr->mdr = data;

    uint8_t bank = address >> 0x10;
    uint16_t offset = address & 0xFFFF;

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {
        
        if (offset > 0x20FF && offset < 0x2200) {
            write_bus_b(cpu, address & 0xFF, data);
            return;
        }

        if (offset > 0x3FFF && offset < 0x4400) {
            write_internal_io(cpu, address & 0x3FF, data);
            return;
        }

        else {
            write_bus_a(cpu, address, data);
            return;
        }
    }

    write_bus_a(cpu, address, data);
}

uint8_t read_bus_a(void* cpu, uint32_t address) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    uint8_t result = cpu_ptr->mdr;

    uint8_t bank = address >> 0x10;
    uint16_t offset = address & 0xFFFF;

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {
        if (offset < 0x2000) {
            result = cpu_ptr->bus.wram->memory[address & 0x1FFF];
        }

        else if (offset > 0x7FFF) {
            result = read_cart(cpu_ptr->bus.cart, address, cpu_ptr->mdr);
        }
    }

    else if (bank == 0x7E || bank == 0x7F) {
        result = cpu_ptr->bus.wram->memory[address & 0x1FFFF];
    }

    else {
        result = read_cart(cpu_ptr->bus.cart, address, cpu_ptr->mdr);
    }

    return result;
}

void write_bus_a(void* cpu, uint32_t address, uint8_t data) {
    s_cpu* cpu_ptr = (s_cpu*) cpu;

    uint8_t bank = address >> 0x10;
    uint16_t offset = address & 0xFFFF;

    if (bank < 0x40 || (bank > 0x7F && bank < 0xC0)) {
        if (offset < 0x2000) {
            cpu_ptr->bus.wram->memory[address & 0x1FFF] = data;
            return;
        }

        if (offset > 0x7FFF) {
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
        //printf("apu port %u read %02X PC = %04X fussy = %06X\n", address & 0x3, cpu->bus.apu->io_port[address & 0x3].data_out, cpu->base.registers.pc.word, address);
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
    if (address > 0x2FF) {
        return read_dma_io(cpu, address);
    }

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

    return result;
}

void write_internal_io(s_cpu* cpu, uint16_t address, uint8_t data) {
    if (address > 0x2FF) {
        write_dma_io(cpu, address, data);
        return;
    }

    switch (address) {
        case 0x016: //JOYWR
            //write joypad
            break;

        case 0x200: //NMITIMEN
            bool nmi_enable = get_bit(data, 7);
            bool auto_joypad_enable = get_bit(data, 0);
            uint8_t h_v_irq_mode = (data & 0x30) >> 4;

            cpu->irq_flag = h_v_irq_mode;

            if (!h_v_irq_mode) {
                cpu->irq_flag = false;

                if (cpu->timer_irq_enabled) {
                    remove_event(cpu->scheduler, cpu->irq_event_id);
                    cpu->timer_irq_enabled = false;
                }
            }

            else {
                //schedule_timer_irq(cpu);
            }

            cpu->vblank_nmi_enable = nmi_enable;
            //schedule auto joypad
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
            if (data == 0) {
                break;
            }

            long next_time = 0;

            for (int i = 8; i > 0; i--) {
                int channel_number = i - 1;

                cpu_dma_channel* channel = &cpu->dma_channel[channel_number];
                bool channel_enable = get_bit(data, channel_number);

                channel->enabled = channel_enable;
                
                if (!channel_enable) {
                    continue;
                }

                if (channel->count == 0) {
                    channel->count = 0x10000;
                }

                schedule_event(cpu->scheduler, next_time, &handle_dma);

                next_time += 16; //initialization and end INACCURATE
                next_time += channel->count * 8; //8 cycles per byte;

                cpu->active_dma_channel = channel_number;

                printf("\nChannel %u\n", channel_number);
                printf("Address A: %lu\n", (channel->bus_a_address.bank << 16) | channel->bus_a_address.offset);
                printf("Address B: %u\n", channel->bus_b_address);
                printf("Direction: %s\n", channel->direction ? "B -> A" : "A -> B");
                printf("Address mode: ");

                switch (channel->address_a_mode) {
                    case DMA_ADDRESS_INCREMENT: printf("Increment\n"); break;
                    case DMA_ADDRESS_DECREMENT: printf("Decrement\n"); break;
                    default: printf("Constant\n"); break;
                }

                printf("Count %u\n", channel->count);
                printf("Pattern %u\n\n", channel->pattern);

            }

            break;

        case 0x20C: //HDMAEN
            //hdma
            break;

        case 0x20D: //MEMSEL
            bool speed = get_bit(data, 0);
            cpu->memory_2_region_speed = speed;
            break; 
    }
}

uint8_t read_dma_io(s_cpu* cpu, uint16_t address) {
    uint8_t channel_number = (address & 0xF0) >> 4;
    uint8_t port = address & 0xF;

    uint8_t result = cpu->mdr;

    cpu_dma_channel* channel = &cpu->dma_channel[channel_number];

    switch (port) {
        case 0x0: //DMAPx
            result &= 0x20;
            result |= (channel->direction << 0x7) /*| (channel->hdma indirect << 6)*/ | (channel->address_a_mode << 0x3) | channel->pattern;
            break;

        case 0x1: //BBADx
            result = channel->bus_b_address;
            break;

        case 0x2: //A1TxL
            result = channel->bus_a_address.offset &= 0xFF;
            break;

        case 0x3: //A1TxH
            result = channel->bus_a_address.offset >> 0x8;
            break;

        case 0x4: //A1Bx
            result = channel->bus_a_address.bank;
            break;

        case 0x5: //DASxL
            result = channel->count & 0xFF;
            break;

        case 0x6: //DASxH
            result = channel->count >> 0x8;
            break;

        case 0x7: //DASBx
            //hdma
            break;

        case 0x8: //A2AxL
            //hdma
            break;

        case 0x9: //A2AxH
            //hdma
            break;

        case 0xA: //NTRLx
            //hdma
            break;

        case 0xB: case 0xF: //UNUSEDx
            result = channel->unused_byte;
            break;
    }

    return result;
}

void write_dma_io(s_cpu* cpu, uint16_t address, uint8_t data) {
    uint8_t channel_number = (address & 0xF0) >> 4;
    uint8_t port = address & 0xF;

    cpu_dma_channel* channel = &cpu->dma_channel[channel_number];

    switch (port) {
        case 0x0: //DMAPx
            bool transfer_direction = get_bit(data, 7);
            uint8_t address_a_mode = (data >> 0x3) & 0x3;
            uint8_t pattern = data & 0x7;
            
            //hdma 

            channel->direction = transfer_direction;
            channel->address_a_mode = address_a_mode;
            channel->pattern = pattern;
            break;

        case 0x1: //BBADx
            channel->bus_b_address = data;
            break;

        case 0x2: //A1TxL
            channel->bus_a_address.offset &= 0xFF00;
            channel->bus_a_address.offset |= data;
            break;

        case 0x3: //A1TxH
            channel->bus_a_address.offset &= 0xFF;
            channel->bus_a_address.offset |= data << 8;
            break;

        case 0x4: //A1Bx
            channel->bus_a_address.bank = data;
            break;

        case 0x5: //DASxL
            channel->count &= 0xFF00;
            channel->count |= data;
            break;

        case 0x6: //DASxH
            channel->count &= 0xFF;
            channel->count |= data << 8;
            break;

        case 0x7: //DASBx
            //hdma
            break;

        case 0x8: //A2AxL
            //hdma
            break;

        case 0x9: //A2AxH
            //hdma
            break;

        case 0xA: //NTRLx
            //hdma
            break;

        case 0xB: case 0xF: //UNUSEDx
            channel->unused_byte = data;
            break;
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

void schedule_timer_irq(s_cpu* cpu) {
    if (!cpu->timer_irq_enabled) {
        return;
    }

    /*remove_event(cpu->scheduler, cpu->irq_event_id);

    int h_pos = (*cpu->cycle_count >> 2) % 342;
    int v_pos = *cpu->cycle_count / 1364;

    int interrupt_time;

    switch (h_v_irq_mode) {
        case 1:
            interrupt_time = (cpu->h_timer_count - h_pos) * 4;
            break;

        case 2:
            interrupt_time = (cpu->v_timer_count - v_pos) * 1364;
            interrupt_time -= h_pos * 4;
            break;

        case 3:
            interrupt_time = (cpu->v_timer_count - v_pos) * 1364;
            interrupt_time += cpu->h_timer_count * 4;

            interrupt_time -= h_pos*4;
            break;
    }

    if (interrupt_time < 0) {
        interrupt_time += 357368;
    }

    schedule_event_set_id(cpu->scheduler, &cpu->irq_event_id, interrupt_time, &handle_timer_irq);*/
}

int get_dma_pattern_increment(uint32_t bytes_transfered, uint8_t pattern) {
    pattern &= 0x7;

    switch (pattern) {
        default: return 0;
        case 1: case 5: return bytes_transfered & 0x1;
        case 3: case 7: return (bytes_transfered >> 0x1) & 0x1;
        case 4: return bytes_transfered & 0x3;
    }
}

void handle_dma(void* state) {
    snes* snes_ptr = (snes*) state;
    s_cpu* cpu = snes_ptr->cpu;

    cpu_dma_channel* channel;

    while (true) {
        if (cpu->dma_channel[cpu->active_dma_channel].enabled) {
            channel = &cpu->dma_channel[cpu->active_dma_channel];
            break;
        }

        cpu->active_dma_channel++;

        if (cpu->active_dma_channel >= 8) {
            return;
        }
    }

    uint32_t transfered = 0;
    int address_a_increment = 0;

    if (channel->address_a_mode == DMA_ADDRESS_INCREMENT) {
        address_a_increment = 1;
    }

    else if (channel->address_a_mode == DMA_ADDRESS_DECREMENT) {
        address_a_increment = -1;
    }

    while (channel->count != 0) {
        uint8_t data;

        uint32_t addr_a = (channel->bus_a_address.bank << 16) | channel->bus_a_address.offset;
        uint8_t addr_b = channel->bus_b_address + get_dma_pattern_increment(transfered, channel->pattern); 

        if (channel->direction == DMA_DIRECTION_A_B) {
            data = read_bus_a(cpu, addr_a);
            write_bus_b(cpu, addr_b, data);
        }

        else {
            data = read_bus_b(cpu, addr_b);
            write_bus_a(cpu, addr_a, data);
        }

        channel->bus_a_address.offset += address_a_increment;

        transfered++;
        channel->count--;
    }

    //cpu->scheduler->current_time += transfered * 8; //fix later
    channel->enabled = false;

    //printf("sussy %u sussy %llu\n", transfered * 8, cpu->scheduler->current_time);
    //print_scheduled_events(cpu->scheduler);

    snes_run_for(snes_ptr, (transfered * 8) + 16);
}

void handle_timer_irq(void* state) {
    snes* snes_ptr = (snes*) state;
    snes_ptr->cpu->irq_flag = true;
}