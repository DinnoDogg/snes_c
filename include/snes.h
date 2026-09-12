#ifndef SNES_H
#define SNES_H

#include <stdint.h>

#include "scheduler.h"
#include "apu.h"
#include "cpu.h"
#include "cart.h"
#include "ppu.h"

#define SYSTEM_EVENT_COUNT 0x10

typedef enum snes_clock_divisor {
    CLOCK_DIVISOR_CPU = 6,
    CLOCK_DIVISOR_APU = 21
} snes_clock_divisor;

typedef struct wram {
    uint8_t memory[0x20000];
    uint32_t io_address : 17;
} wram;

typedef struct snes {
    scheduler* scheduler;
    apu* apu;
    cart* cart;
    s_cpu* cpu;
    ppu* ppu;

    wram wram;

    uint8_t audio_ram[0x10000];
    uint16_t vram[0x8000];

    int cycle_count;
} snes;

snes* new_snes(cart* cart);
void free_snes(snes* snes);

uint8_t read_wram_io(wram* wram);
void write_wram_io(wram* wram, uint8_t address, uint8_t data);

void snes_run_for(snes* snes, uint64_t cycle_count);

#endif