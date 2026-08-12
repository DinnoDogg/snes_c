#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/processor/spc700/spc700.h"

#include "include/apu.h"
#include "include/snes.h"
#include "include/scheduler.h"

typedef void (*timer_tick_callback)(void* state);

static void timer_0_tick(void* state);
static void timer_1_tick(void* state);
static void timer_2_tick(void* state);

static void tick_timer(apu* apu, int timer_number);

static bool get_bit(int value, int index);

static int get_master_cycles_per_timer_tick(int timer_number);

static uint8_t read_internal_io(apu* apu, uint8_t address);
static void write_internal_io(apu* apu, uint8_t address, uint8_t data);

static uint8_t read_apu_bus(void* cpu, uint16_t address);
static void write_apu_bus(void* cpu, uint16_t address, uint8_t data);

static const int master_cycles_per_timer_tick[0x3] = {336, 336, 2684};
static const timer_tick_callback tick_callback[0x3] = {&timer_0_tick, &timer_1_tick, &timer_2_tick};

static const uint8_t ipl_rom[0x40] = {
    0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0, 0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
    0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4, 0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
    0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB, 0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
    0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD, 0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
};

bool get_bit(int value, int index) {
    return (value >> index) & 0x1;
}

apu* new_apu(uint8_t* audio_ram, scheduler* scheduler) {
    apu* result = calloc(1, sizeof(apu));

    result->bus.audio_ram = audio_ram;
    //dsp

    result->scheduler = scheduler;

    init_spc700((SPC700*) result, &read_apu_bus, &write_apu_bus);
    reset_apu(result);

    return result;
}

void free_apu(apu* apu) {
    free(apu);
}

void reset_apu(apu* apu) {
    spc700_reset((SPC700*) apu);

    apu->ipl_enable = true;

    for (int i = 0; i < 3; i++) {
        apu->timer[i].interval = 0;
        apu->timer[i].internal_counter = 0;
        apu->timer[i].up_counter = 0;
        apu->timer[i].enabled = false;
    }

    for (int i = 0; i < 4; i++) {
        apu->io_port[i].data_in = 0;
        apu->io_port[i].data_out = 0;
    }
}

int get_master_cycles_per_timer_tick(int timer_number) {
    return master_cycles_per_timer_tick[timer_number];
}

// uint8_t read_apu_bus(void* cpu, uint16_t address) {
//     apu* apu_ptr = (apu*) cpu;
    
//     if (address < 0xF0 || (address > 0xFF && address < 0xFFC0) || address == 0x00F8 || address == 0x00F9) {
//         return apu_ptr->bus.audio_ram[address];
//     }

//     if (address > 0xFFBF) {
//         return (apu_ptr->ipl_enable) ? ipl_rom[address & 0x3F] : apu_ptr->bus.audio_ram[address];
//     }

//     return read_internal_io(apu_ptr, address);
// }

// void write_apu_bus(void* cpu, uint16_t address, uint8_t data){
//     apu* apu_ptr = (apu*) cpu;

//     if (address < 0xF0 || address > 0xFF || address == 0x00F8 || address == 0x00F9) {
//         apu_ptr->bus.audio_ram[address] = data;
//         return;
//     }

//     write_internal_io(apu_ptr, address, data);
// }

uint8_t read_apu_bus(void* cpu, uint16_t address) {
    apu* apu_ptr = (apu*) cpu;
    
    if (address < 0xF0) {
        return apu_ptr->bus.audio_ram[address];
    }

    if (address < 0x100) {
        return read_internal_io(apu_ptr, address);
    }

    if (address < 0xFFC0) {
        return apu_ptr->bus.audio_ram[address];
    }

    return (apu_ptr->ipl_enable) ? ipl_rom[address & 0x3F] : apu_ptr->bus.audio_ram[address];
}

void write_apu_bus(void* cpu, uint16_t address, uint8_t data) {
    apu* apu_ptr = (apu*) cpu;
    
    if (address < 0xF0) {
        apu_ptr->bus.audio_ram[address] = data;
        return;
    }

    if (address < 0x100) {
        write_internal_io(apu_ptr, address, data);
        return;
    }

    apu_ptr->bus.audio_ram[address] = data;
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
            uint8_t port = address & 0x3;
            result = apu->io_port[port].data_in;
            break;

        case 0x8: case 0x9:
            result = apu->bus.audio_ram[0xF0 | address];
            break;

        case 0xD: case 0xE: case 0xF:
            uint8_t timer = (address - 1) & 0x3;
            result = apu->timer[timer].up_counter;
            apu->timer[timer].up_counter = 0;
            break;
    }

    return result;
}

void write_internal_io(apu* apu, uint8_t address, uint8_t data) {
    address &= 0xF;

    //printf("spc700 internal write\n");

    switch (address) {
        case 0x0: 
            break; //test register

        case 0x1: 
            bool ipl_enable = get_bit(data, 7);
            bool clear_ports_0_1 = get_bit(data, 4);
            bool clear_ports_2_3 = get_bit(data, 5);

            //printf("controlle writte %02Xn\n", data);

            if (clear_ports_0_1) {
                apu->io_port[0].data_in = 0;
                apu->io_port[1].data_in = 0;
            }

            if (clear_ports_2_3) {
                apu->io_port[2].data_in = 0;
                apu->io_port[3].data_in = 0;
            }

            for (int i = 0; i < 3; i++) {
                apu_timer* timer = &apu->timer[i];
                bool timer_enable = get_bit(data, i); 

                timer->up_counter = 0;
                timer->internal_counter = 0;

                if (timer->enabled) {
                    remove_event(apu->scheduler, timer->tick_event_id);
                }

                if (timer_enable) {
                    int next_time = master_cycles_per_timer_tick[i];
                    timer_tick_callback callback = tick_callback[i];

                    schedule_event_set_id(apu->scheduler, &timer->tick_event_id, next_time, callback);
                }

                timer->enabled = timer_enable;
            }

            apu->ipl_enable = ipl_enable;
            break;

            bool schedule_event_set_id(scheduler* scheduler, long* event_id, int timecode, event_callback callback);

        case 0x4: case 0x5: case 0x6: case 0x7:
            uint8_t port = address & 0x3;
            apu->io_port[port].data_out = data;
            break;

        case 0x8: case 0x9:
            apu->bus.audio_ram[0xF0 | address] = data;
            break;

        case 0xA: case 0xB: case 0xC:
            uint8_t timer = (address + 2) & 0x3;
            apu->timer[timer].interval = data;
            break;
    }
}

uint8_t read_apu_io(apu* apu, uint8_t address) {
    uint8_t port = address & 0x3;
    return apu->io_port[port].data_out;
}

void write_apu_io(apu* apu, uint8_t address, uint8_t data) {
    uint8_t port = address & 0x3;
    apu->io_port[port].data_in = data;
    //printf("apu port %u write %02X\n", port, data);
}

void timer_0_tick(void* state) {
    snes* snes_ptr = (snes*) state;
    tick_timer(snes_ptr->apu, 0);
    //printf("I am a sussy sigma hehehe\n");
}

void timer_1_tick(void* state) {
    snes* snes_ptr = (snes*) state;
    tick_timer(snes_ptr->apu, 1);
}

void timer_2_tick(void* state) {
    snes* snes_ptr = (snes*) state;
    tick_timer(snes_ptr->apu, 2);
}

void tick_timer(apu* apu, int timer_number) {
    apu_timer* timer = &apu->timer[timer_number];
    timer_tick_callback callback = tick_callback[timer_number];

    //printf("I am a sussy rizzler tick tick %u\n", timer_number);

    //print_scheduled_events(apu->scheduler);

    int next_time = master_cycles_per_timer_tick[timer_number];

    if (++timer->internal_counter == timer->interval) {
        timer->internal_counter = 0;
        timer->up_counter++;
    }

    schedule_event_set_id(apu->scheduler, &timer->tick_event_id, next_time, callback);
}