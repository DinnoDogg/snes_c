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
            uint16_t addr = ppu->vram_address & 0x7FFF;
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

    switch (ppu->address_translation) {
        case 1:
            uint8_t y = (result & 0xE0) >> 5;
            uint8_t c = result & 0x1F;
            result &= 0xFF00;
            result |= (c << 3) | y;
            break; 

        case 2:
            uint8_t y = (result & 0x1C0) >> 6;
            uint8_t c = (result & 0x3E) >> 1;
            bool p = result & 1;
            result &= 0xFE00;
            result |= (c << 4) | (p << 3) | y;
            break;

        case 3:
            uint8_t y = (result & 0x380) >> 7;
            uint8_t c = (result & 0x3E) >> 1;
            uint8_t p = result & 1;

    }

    return result;
}