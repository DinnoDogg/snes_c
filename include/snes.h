#ifndef SNES_H
#define SNES_H

#include "include/scheduler.h"

typedef struct apu apu;

typedef struct snes {
    apu apu;

    uint8_t audio_ram[0x10000];

    scheduler* scheduler;
} snes;

#endif