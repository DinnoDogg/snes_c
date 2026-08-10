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

snes* new_snes(cart* cart) {
    snes* result = malloc(sizeof(snes));

    result->cart = cart;

    result->scheduler = new_scheduler(SYSTEM_EVENT_COUNT);

    result->apu = new_apu(result->audio_ram, result->scheduler);
    result->cpu = new_cpu(result->cart, &result->wram, result->apu);

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

//snes scheduler functions

void handle_apu_op(void* state) {
    snes* snes_ptr = (snes*) state;
    SPC700* spc = (SPC700*) snes_ptr->apu;

    spc700_run_instruction(spc);

    int next_time = spc->cycle_count * CLOCK_DIVISOR_APU;

    schedule_event(snes_ptr->scheduler, next_time, &handle_apu_op);
}

