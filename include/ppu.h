#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

#define SNES_SCREEN_WIDTH 256
#define SNES_SCREEN_HEIGHT 224

typedef struct ppu_background {
    uint16_t tilemap_address, tileset_address;
    bool mosaic, char_size;
    bool horizontal_map, vertical_map;

    uint16_t horizontal_offset, vertical_offset;
    bool enabled;
} ppu_background;

typedef struct ppu_window {
    uint16_t left, right;
    bool enable_bg[4];
    bool invert_bg[4];
} ppu_window;

// typedef struct ppu_pixel {
//     uint32_t color_data;
    
// } ppu_pixel;

typedef struct ppu {
    uint16_t* vram;
    uint16_t cgram[0x100];

    uint32_t framebuffer[SNES_SCREEN_HEIGHT * SNES_SCREEN_WIDTH];
    uint16_t current_y;

    uint8_t cgram_address, cgram_buffer;
    bool cgram_access;
    
    ppu_background background[4];
    ppu_window window[2];

    uint8_t open_bus;
    uint16_t vram_address, vram_read_latch;

    bool increment_mode;
    uint8_t address_translation, increment_amount;

    bool force_blank;
    unsigned int blank_start, blank_end;
    uint8_t brightness;

    uint8_t bg_mode, mosaic_size;
    bool mode_1_bg_3_priority;

    uint16_t h_offset_latch, v_h_offset_latch;
    bool obj_enable;
} ppu;

ppu* new_ppu(uint16_t* vram);
void free_ppu(ppu* ppu);

uint8_t read_ppu_io(ppu* ppu, uint8_t address);
void write_ppu_io(ppu* ppu, uint8_t address, uint8_t data);

void ppu_h_draw(ppu* ppu);

#endif