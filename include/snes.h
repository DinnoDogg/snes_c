#ifndef SNES_H
#define SNES_H

#include <stdint.h>

#include "scheduler.h"
#include "apu.h"

#define SYSTEM_EVENT_COUNT 0x10

typedef struct snes {
    apu* apu;
    scheduler* scheduler;
    uint8_t audio_ram[0x10000];
} snes;

snes* new_snes();
void free_snes(snes* snes);

#endif