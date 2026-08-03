#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "processor/wdc65816/wdc65816.h"
#include "cart.h"
#include "apu.h"

typedef struct wram wram;

typedef struct cpu {
    WDC65816 base;

    struct {
        cart* cart;
        wram* wram;
        apu* apu;
    } bus;

    //dma
    uint8_t mdr;
} cpu;

cpu* new_cpu(cart* cart, wram* wram, apu* apu);
void free_cpu(cpu* cpu);

#endif