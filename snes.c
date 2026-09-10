#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/snes.h"
#include "include/apu.h"
#include "include/cpu.h"
#include "include/cart.h"
#include "include/processor/spc700/spc700.h"

static void init_scheduler(snes* snes);

static void schedule_apu_op(snes* snes, int timecode);
static void handle_apu_op(void* state);

static void schedule_vblank(snes* snes);
static void schedule_hblank(snes* snes);

static void vblank_start(void* state);
static void vblank_end(void* state);

static void hblank_start(void* state);
static void hblank_end(void* state);

snes* new_snes(cart* cart) {
    snes* result = calloc(1, sizeof(snes));

    result->cart = cart;

    result->scheduler = new_scheduler(SYSTEM_EVENT_COUNT);

    result->apu = new_apu(result->audio_ram, result->scheduler);
    result->cpu = new_cpu(result->cart, result->scheduler, &result->wram, result->apu, &result->cycle_count);

    init_scheduler(result);
    
    return result;
}

void free_snes(snes* snes) {
    free_apu(snes->apu);
    free_scheduler(snes->scheduler);
    free_cpu(snes->cpu);
    free(snes);
}

void init_scheduler(snes* snes) {
    handle_apu_op(snes);

    //schedule vblank - assume interlace and overscan disabled. FIX LATER!!!!
    schedule_vblank(snes);
    // schedule_hblank(snes);
}

uint8_t read_wram_io(wram* wram) {
    return wram->memory[wram->io_address++];
}

void write_wram_io(wram* wram, uint8_t address, uint8_t data) {
    address &= 0x3;
    
    switch (address) {
        case 0: 
            wram->memory[wram->io_address++] = data; 
            break;

        case 1:
            wram->io_address &= ~0xFF;
            wram->io_address |= data;
            break;

        case 2:
            wram->io_address &= ~0xFF00;
            wram->io_address |= data << 8;
            break;
        
        case 3:
            wram->io_address &= 0xFFFF;
            wram->io_address |= (data & 0x1) << 0x10;
            break;
    }
}

void snes_run_for(snes* snes, uint64_t cycle_count) {
    uint64_t target = snes->scheduler->current_time + cycle_count;
    scheduler_event* next_event = &snes->scheduler->event_list[0];

    snes->cycle_count += cycle_count;

    while (next_event->timecode <= target) {
        long id = next_event->id;
        snes->scheduler->current_time = next_event->timecode;

        next_event->callback(snes);

        remove_event(snes->scheduler, id);
        next_event = &snes->scheduler->event_list[0];

        if (snes->scheduler->current_time > target) {
            target = snes->scheduler->current_time;
        }
    }

    snes->scheduler->current_time = target;
}

//snes scheduler functions

void handle_apu_op(void* state) {
    snes* snes_ptr = (snes*) state;
    SPC700* spc = (SPC700*) snes_ptr->apu;

    spc700_run_instruction(spc);

    int next_time = spc->cycle_count * CLOCK_DIVISOR_APU;

    schedule_event(snes_ptr->scheduler, next_time, &handle_apu_op);
}

void schedule_vblank(snes* snes) {
    long start_time = 225 * 1364; //225 lines till next vblank; ASSUME OVERSCAN OFF FIX LATER !!!!!
    schedule_event(snes->scheduler, start_time, &vblank_start);
}

void schedule_hblank(snes* snes) {
    int start_time = 274 * 4; 
    schedule_event(snes->scheduler, start_time, &hblank_start);
}

void vblank_start(void* state) {
    snes* snes_ptr = (snes*) state;
    snes_ptr->cpu->vblank_flag = true;
    snes_ptr->cpu->nmi_flag = true;

    long end_time = 37 * 1364; //37 lines of vblank, 1364 cycles per vblank line; ASSUME OVERSCAN OFF FIX LATER !!!!!

    printf("V BLONK!!!\n");

    schedule_event(snes_ptr->scheduler, end_time, &vblank_end);
}

void vblank_end(void* state) {
    snes* snes_ptr = (snes*) state;
    snes_ptr->cpu->vblank_flag = false;
    snes_ptr->cpu->nmi_flag = false;
    
    snes_ptr->cycle_count = 0;

    printf("bye vblon\n");

    schedule_vblank(snes_ptr);
}

void hblank_start(void* state) {
    snes* snes_ptr = (snes*) state;
    //handle hdraw

    int end_time = 67 * 4;
    int h_pos = (snes_ptr->cycle_count >> 2) % 342;

    printf("h blank at %u\n", h_pos);

    schedule_event(snes_ptr->scheduler, end_time, &hblank_end);
}

void hblank_end(void* state) {
    snes* snes_ptr = (snes*) state;
    schedule_hblank(snes_ptr);
}