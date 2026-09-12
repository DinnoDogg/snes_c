#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct ppu {
    uint16_t* vram;

    uint8_t open_bus;

    struct {
        uint8_t high, low;
    } vram_read_latch;

    uint16_t vram_address;
    bool increment_low;
    uint8_t address_translation;
} ppu;

ppu* new_ppu(uint16_t* vram);
void free_ppu(ppu* ppu);

uint8_t read_ppu_io(ppu* ppu, uint8_t address);
void write_ppu_io(ppu* ppu, uint8_t address, uint8_t data);

#endif