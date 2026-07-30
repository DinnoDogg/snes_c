#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/snes.h"
#include "include/apu.h"
#include "include/processor/spc700/spc700.h"

static void init_scheduler(snes* snes);

static void handle_apu_op(void* state);
static void schedule_apu_op(snes* snes, int cycle_offset);

snes* new_snes() {
    snes* result = malloc(sizeof(snes));

    result->apu = new_apu(result->audio_ram);
    result->scheduler = new_scheduler(SYSTEM_EVENT_COUNT);

    init_scheduler(result);
    
    return result;
}

void free_snes(snes* snes) {
    free_apu(snes->apu);
    free_scheduler(snes->scheduler);
    free(snes);
}

void init_scheduler(snes* snes) {
    schedule_apu_op(snes, 0);
}

void schedule_apu_op(snes* snes, int cycle_offset) {
    uint8_t opcode = spc700_read_immediate((SPC700*) snes->apu);
    int timecode = spc700_get_op_time(opcode) + cycle_offset;

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
    schedule_apu_op(s, 0);
}