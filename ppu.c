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
    uint8_t low = ppu->vram_address & 0xFF;
    uint16_t high = ppu->vram_address & 0xFF00;

    switch (ppu->address_translation) {
        case 1:
            

    }

    return high | low;
}