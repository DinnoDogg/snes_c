#include <stdint.h>
#include <stdlib.h>

#include "include/ppu.h"

static void increment_vram_address(ppu* ppu);
static uint16_t get_vram_address(ppu* ppu);

ppu* new_ppu(uint16_t* vram) {
    ppu* result = calloc(1, sizeof(ppu));
    result->vram = vram;
    return result;
}

void free_ppu(ppu* ppu) {
    free(ppu);
}

uint8_t read_ppu_io(ppu* ppu, uint8_t address) {
    uint8_t result = ppu->open_bus;
    
    switch (address) {
        
    }
 
    return result;
}

void write_ppu_io(ppu* ppu, uint8_t address, uint8_t data) {
    switch (address) {
        case 0x16: //VMADDL
            ppu->vram_address &= 0xFF00;
            ppu->vram_address |= data;
            break;

        case 0x17: //VMADDH
            ppu->vram_address &= 0xFF;
            ppu->vram_address |= data << 8;
            break;

        case 0x18: //VMDATAL
            uint16_t addr = ppu->vram_address & 0x7FFF;
            addr <<= 1;
            ppu->vram[addr] = data;

            if (ppu->increment_low) {
                increment_vram_address(ppu);
            }

            break;

        case 0x19: //VMDATAH
            // uint16_t addr = ppu->vram_address & 0x7FFF;
            addr <<= 1;
            ppu->vram[addr + 1] = data;

            if (!ppu->increment_low) {
                increment_vram_address(ppu);
            }

            break;

    }
}

void increment_vram_address(ppu* ppu) {
    return;
}

uint16_t get_vram_address(ppu* ppu) {
    uint16_t result = ppu->vram_address;

    if (!ppu->address_translation) {
        return result;
    }

    uint8_t bit_count = 7 + ppu->address_translation;

    uint16_t mask = (1 << bit_count) - 1;
    uint16_t lower = result & mask;

    uint8_t shift = bit_count - 3;

    uint16_t shifted = ((lower << 3) | (lower >> shift)) & mask;
    result = (result & ~mask) | shifted;

    return result;
}