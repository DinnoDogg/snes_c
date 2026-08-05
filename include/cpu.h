#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "processor/wdc65816/wdc65816.h"
#include "cart.h"
#include "apu.h"

typedef struct wram wram;

typedef enum cpu_waitstate_speed {
    CPU_WAITSTATE_FAST = 0, CPU_WAITSTATE_SLOW = 2, CPU_WAITSTATE_X_SLOW = 6
} cpu_waitstate_speed;

typedef struct s_cpu {
    WDC65816 base;

    int waitstate_count;

    struct {
        cart* cart;
        wram* wram;
        apu* apu;
        //joypad
    } bus;

    uint8_t mdr;

    bool nmi_flag;
    bool irq_flag;

    bool vblank_flag;
    bool hblank_flag;
    bool auto_joypad_busy;

    uint16_t quotient;
    uint16_t product_and_remainder;

    bool vblank_nmi_enable;

    uint8_t multiplicand, multiplier;

    uint16_t dividend;
    uint8_t divisor;

    uint16_t h_timer_count, v_timer_count;

    bool memory_2_region_speed;

    //dma
} s_cpu;

s_cpu* new_cpu(cart* cart, wram* wram, apu* apu);
void free_cpu(s_cpu* cpu);

int cycle_cpu(s_cpu* cpu);

void reset_cpu(s_cpu* cpu);

#endif