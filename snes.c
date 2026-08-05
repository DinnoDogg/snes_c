#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/snes.h"
#include "include/apu.h"
#include "include/cpu.h"
#include "include/cart.h"
#include "include/processor/spc700/spc700.h"

static void init_scheduler(snes* snes);

static void handle_apu_op(void* state);
static void schedule_apu_op(snes* snes);

snes* new_snes(cart* cart) {
    snes* result = malloc(sizeof(snes));

    result->cart = cart;

    result->apu = new_apu(result->audio_ram);
    result->scheduler = new_scheduler(SYSTEM_EVENT_COUNT);
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
    schedule_apu_op(snes);
}

void schedule_apu_op(snes* snes) {
    uint8_t opcode = spc700_read_immediate((SPC700*) snes->apu);
    int timecode = spc700_get_op_time(opcode);

    snes->apu->base.opcode = opcode;

    if (snes->apu->base.branch_taken) {
        timecode += 2;
        snes->apu->base.branch_taken = false;
    }

    timecode *= 21;

    schedule_event(snes->scheduler, timecode, &handle_apu_op);
}

void handle_apu_op(void* state) {
    snes* s = (snes*) state;
    SPC700* spc = (SPC700*) s->apu;

    spc700_run_instruction(spc, spc->opcode);
    schedule_apu_op(s);
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
