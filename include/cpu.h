#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "processor/wdc65816/wdc65816.h"
#include "cart.h"
#include "apu.h"
#include "scheduler.h"

typedef struct wram wram;

typedef enum cpu_waitstate_speed {
    CPU_WAITSTATE_FAST = 0, CPU_WAITSTATE_SLOW = 2, CPU_WAITSTATE_X_SLOW = 6
} cpu_waitstate_speed;

typedef enum dma_address_mode {
    DMA_ADDRESS_INCREMENT, DMA_ADDRESS_CONSTANT, DMA_ADDRESS_DECREMENT 
} dma_address_mode;

typedef enum dma_transfer_direction {
    DMA_DIRECTION_A_B, DMA_DIRECTION_B_A
} dma_transfer_direction;

typedef struct cpu_dma_channel {
    uint8_t bus_b_address;
    
    struct {
        uint8_t bank;
        uint16_t offset;
    } bus_a_address;

    dma_transfer_direction direction;
    dma_address_mode address_a_mode;

    uint32_t count;
    uint8_t pattern;

    bool enabled;

    uint8_t unused_byte;
} cpu_dma_channel;

typedef struct s_cpu {
    WDC65816 base;
    scheduler* scheduler;

    int waitstate_count;

    struct {
        cart* cart;
        wram* wram;
        apu* apu;
        //joypad
    } bus;

    cpu_dma_channel dma_channel[0x8];

    uint8_t mdr;

    bool dma_active;
    uint8_t active_dma_channel;

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
} s_cpu;

s_cpu* new_cpu(cart* cart, scheduler* scheduler, wram* wram, apu* apu);
void free_cpu(s_cpu* cpu);

int cycle_cpu(s_cpu* cpu);

void reset_cpu(s_cpu* cpu);

#endif