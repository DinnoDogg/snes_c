#ifndef SNES_H
#define SNES_H

#include <stdint.h>

#include "scheduler.h"
#include "apu.h"
#include "cpu.h"
#include "cart.h"

#define SYSTEM_EVENT_COUNT 0x10

typedef struct wram {
    uint8_t memory[0x20000];
    uint32_t io_address : 17;
} wram;

typedef struct snes {
    apu* apu;
    scheduler* scheduler;
    cart* cart;
    s_cpu* cpu;

    wram wram;
    uint8_t audio_ram[0x10000];
} snes;

snes* new_snes(cart* cart);
void free_snes(snes* snes);

static uint8_t read_wram_io(wram* wram, uint32_t address);
static void write_wram_io(wram* wram, uint32_t address, uint8_t data);

#endif