#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/ppu.h"

typedef void (*ppu_draw_callback)(ppu* ppu);

typedef enum bg_priority : uint8_t {
    BG_1_LOW, BG_1_HIGH, BG_2_LOW, BG_2_HIGH,
    BG_3_LOW, BG_3_HIGH, BG_4_LOW, BG_4_HIGH
} bg_priority;

typedef enum ppu_bit_depth : uint8_t {
    TWO_BPP, FOUR_BPP, EIGHT_BPP, EIGHT_BPP_DIRECT
} ppu_bit_depth;

static const bg_priority priority_table[9][8] = {
    {BG_1_HIGH},
    {BG_1_HIGH, BG_2_HIGH, BG_1_LOW, BG_2_LOW, BG_3_HIGH, BG_3_LOW},
    {BG_1_HIGH},
    {BG_1_HIGH},
    {BG_1_HIGH},
    {BG_1_HIGH},
    {BG_1_HIGH},
    {BG_1_HIGH},
    {BG_3_HIGH, BG_1_HIGH, BG_2_HIGH, BG_1_LOW, BG_2_LOW, BG_3_LOW},
};

static const ppu_bit_depth bit_depth_table[8][4] = {
    {TWO_BPP, TWO_BPP, TWO_BPP, TWO_BPP},
    {FOUR_BPP, FOUR_BPP, TWO_BPP}
};

static const uint8_t words_per_char[4] = {
    8, 16, 32
};

static const uint8_t bg_layer_count[8] = {
    8, 6
};

typedef struct ppu_character {
    uint16_t index, base_address;
    uint8_t palette_index;

    bool size, flip_x, flip_y; 
    ppu_bit_depth bit_depth;
} ppu_character;

// S3 1H 2H S2 1L 2L S1 3H    S0 3L


// static void draw_bg_1(ppu* ppu);
// static void draw_bg_2(ppu* ppu);
// static void draw_bg_3(ppu* ppu);
// static void draw_bg_4(ppu* ppu);

// static const ppu_draw_callback draw_bg[4] = {
//     &draw_bg_1, &draw_bg_2, &draw_bg_3, &draw_bg_4 
// };

// static const ascending_bg_priority[]

static bool get_bit(unsigned int data, unsigned int index);

static void increment_vram_address(ppu* ppu);
static uint16_t get_vram_address(ppu* ppu);

static void increment_current_y(ppu* ppu);

static void draw_background(ppu* ppu, uint8_t bg);
static void put_pixel(ppu* ppu, uint8_t x, uint8_t y, uint32_t color);

static uint32_t get_top_pixel(ppu* ppu, uint16_t x, uint16_t y);
static uint32_t get_chr_pixel(ppu* ppu, ppu_character chr, uint8_t x, uint8_t y);



static uint32_t rgb15_to_rgb24(uint32_t color);

static const uint8_t increment_amount[4] = {1, 32, 128, 128};

ppu* new_ppu(uint16_t* vram) {
    ppu* result = calloc(1, sizeof(ppu));
    result->vram = vram;
    return result;
}

void free_ppu(ppu* ppu) {
    free(ppu);
}

bool get_bit(unsigned int data, unsigned int index) {
    return (data >> index) & 0x1;
}

uint8_t read_ppu_io(ppu* ppu, uint8_t address) {
    uint8_t result = ppu->open_bus;
    
    switch (address) {
        
    }
 
    return result;
}

void write_ppu_io(ppu* ppu, uint8_t address, uint8_t data) {
    ppu->open_bus = data;
    address &= 0x3F;

    switch (address) {
        case 0x00: //INIDISP
            ppu->brightness = data & 0xF;
            ppu->force_blank = get_bit(data, 7);
            //handle force blank
            break;

        case 0x05: //BGSEL
            ppu->bg_mode = data & 0x7;
            ppu->mode_1_bg_3_priority = get_bit(data, 3);

            for (int i = 0; i < 4; i++) {
                ppu->background[i].char_size = get_bit(data, i + 4);
            }

            break;

        case 0x06: //MOSAIC
            for (int i = 0; i < 4; i++) {
                ppu->background[i].mosaic = get_bit(data, i);
            }

            ppu->mosaic_size = (data >> 4) & 0xF;
            break;

        case 0x07: case 0x08: case 0x09: case 0x0A: { //BGnSC
            uint8_t bg = (address - 7) & 0x3;
            uint16_t addr = data >> 2;

            ppu->background[bg].horizontal_map = get_bit(data, 0);
            ppu->background[bg].vertical_map = get_bit(data, 1);
            ppu->background[bg].tilemap_address = addr << 10;

            break;
        } 

        case 0x0B: { //BG12NBA
            uint16_t addr_1 = data & 0xF;
            uint16_t addr_2 = data >> 4;

            ppu->background[0].tileset_address = addr_1 << 12;
            ppu->background[1].tileset_address = addr_2 << 12;
            break;
        }

        case 0x0C: { //BG34NBA
            uint16_t addr_3 = data & 0xF;
            uint16_t addr_4 = data >> 4;

            ppu->background[2].tileset_address = addr_3 << 12;
            ppu->background[3].tileset_address = addr_4 << 12;
            break;
        }

        case 0x0D: case 0x0F: case 0x11: case 0x13: { //BGnHOFS
            uint8_t bg = (address >> 1) - 6;
            ppu->background[bg].horizontal_offset = ((data << 8) | (ppu->v_h_offset_latch & ~0x7) | (ppu->h_offset_latch & 0x7)) & 0x3FF;

            ppu->v_h_offset_latch = data;
            ppu->h_offset_latch = data;
            break;
        }

        case 0xE: case 0x10: case 0x12: case 0x14: { //BGnVOFS
            uint8_t bg = (address >> 1) - 7;
            ppu->background[bg].vertical_offset = ((data << 8) | ppu->v_h_offset_latch) & 0x3FF;
            ppu->v_h_offset_latch = data;
            break;
        }

        case 0x2C: //TM
            for (int i = 0; i < 4; i++) {
                ppu->background[i].enabled = get_bit(data, i); 
            }    

            ppu->obj_enable = get_bit(data, 4);
            break;

        case 0x15: //VMAIN
            ppu->increment_amount = data & 0x3;
            ppu->address_translation = (data >> 2) & 0x3;
            ppu->increment_mode = get_bit(data, 7);
            break;

        case 0x16: {//VMADDL
            ppu->vram_address &= 0xFF00;
            ppu->vram_address |= data;

            uint16_t addr = get_vram_address(ppu);
            ppu->vram_read_latch = ppu->vram[addr];

            break;
        }

        case 0x17: {//VMADDH
            ppu->vram_address &= 0xFF;
            ppu->vram_address |= data << 8;

            uint16_t addr = get_vram_address(ppu);
            ppu->vram_read_latch = ppu->vram[addr];

            break;
        }

        case 0x18: { //VMDATAL 
            uint16_t addr = get_vram_address(ppu);
            ppu->vram[addr] = (ppu->vram[addr] & 0xFF00) | data;

            if (!ppu->increment_mode) {
                increment_vram_address(ppu);
            }
            break;
        }

        case 0x19: { //VMDATAH
            uint16_t addr = get_vram_address(ppu);
            ppu->vram[addr] = (ppu->vram[addr] & 0xFF) | data << 8;

            if (ppu->increment_mode) {
                increment_vram_address(ppu);
            }
            break;
        }

        case 0x21: //CGADD
            ppu->cgram_address = data;
            ppu->cgram_access = false;
            break;

        case 0x22: { //CGDATA
            bool access = ppu->cgram_access;
            ppu->cgram_access = !ppu->cgram_access;

            if (!access) {
                ppu->cgram_buffer = data;
                break;
            }

            ppu->cgram[ppu->cgram_address++] = (data << 8) | ppu->cgram_buffer;
            break;
        }   

        case 0x23: //W12SEL
            break;

    }
}

void increment_vram_address(ppu* ppu) {
    uint8_t amount = ppu->increment_amount;
    ppu->vram_address += increment_amount[amount];
}

uint16_t get_vram_address(ppu* ppu) {
    uint16_t result = ppu->vram_address & 0x7FFF;

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

void ppu_h_draw(ppu* ppu) {
    uint16_t current_y = ppu->current_y;
    increment_current_y(ppu);

    if (current_y > 223) {
        return;
    }

    for (int x = 0; x < SNES_SCREEN_WIDTH; x++) {
        //handle sprites
        uint32_t c = get_top_pixel(ppu, x, current_y);
        put_pixel(ppu, x, current_y, c);

    }
}

uint32_t get_top_pixel(ppu* ppu, uint16_t x, uint16_t y) {
    uint32_t result = 0; //backdrop color

    uint8_t layer_count = bg_layer_count[ppu->bg_mode];
    uint8_t table_index = ppu->bg_mode;

    if (ppu->bg_mode == 1 && ppu->mode_1_bg_3_priority) {
        table_index = 8;
    }

    // handle layer count later
    for (int i = 0; i < layer_count; i++) {
        bg_priority priority = priority_table[table_index][i];

        uint8_t bg = priority >> 1;

        if (!ppu->background[bg].enabled) {
            continue;
        }

        bool high_priority = priority & 0x1;

        uint16_t scrolled_x = (ppu->background[bg].horizontal_offset + x) & 0x1FF;
        uint16_t scrolled_y = (ppu->background[bg].vertical_offset + y) & 0x1FF;

        uint16_t tile_x = scrolled_x >> 3;
        uint16_t tile_y = scrolled_y >> 3;

        uint16_t tilemap_addr = ppu->background[bg].tilemap_address;

        bool horizontal_map = ppu->background[bg].horizontal_map;
        bool vertical_map = ppu->background[bg].vertical_map;

        if (ppu->background[bg].horizontal_map) {
            if (tile_x > 31) {
                tilemap_addr += 0x400;
            }
        }

        if (ppu->background[bg].vertical_map) {
            if (tile_y > 31) {
                tilemap_addr += 0x400;
                if (horizontal_map) tilemap_addr += 0x400;
            }
        }

        tilemap_addr |= ((tile_y & 0x1F) * 32) + (tile_x & 0x1F);

        uint16_t tilemap_data = ppu->vram[tilemap_addr];
        bool tile_priority = get_bit(tilemap_data, 13);

        uint16_t char_index = tilemap_data & 0x3FF;
        uint8_t palette_index = (tilemap_data >> 10) & 0x7;

        ppu_character chr = {
            char_index, ppu->background[bg].tileset_address,
            palette_index, 
            ppu->background[bg].char_size,
            get_bit(tilemap_data, 14), get_bit(tilemap_data, 15),
            bit_depth_table[ppu->bg_mode][bg] 
        };

        result = get_chr_pixel(ppu, chr, scrolled_x, scrolled_y);

        if (tile_priority != high_priority || !result) {
            continue;
        }

        break;
    }

    if (!result) {
        result = ppu->cgram[0]; 
    }

    return rgb15_to_rgb24(result);
}

uint32_t get_chr_pixel(ppu* ppu, ppu_character chr, uint8_t x, uint8_t y) {
    uint8_t word_count = words_per_char[chr.bit_depth];
    uint16_t data_address = chr.base_address + (chr.index * word_count);

    uint32_t result = 0;

    //HANDLE 16x16 TILES!!!

    y &= 0x7;
    x &= 0x7;

    if (!chr.flip_x) {
        x = 7 - x;
    }

    if (chr.flip_y) {
        y = 7 - y;
    }

    switch (chr.bit_depth) {
        case TWO_BPP: {
            data_address += y;
            uint16_t word = ppu->vram[data_address];

            uint8_t plane_high = word >> 8;
            uint8_t plane_low = word & 0xFF;

            uint8_t color_index = get_bit(plane_high, x) << 1;
            color_index |= get_bit(plane_low, x);

            if (!color_index) {
                break;
            }

            uint8_t cgram_addr = (chr.palette_index << 2) + color_index;
            result = 0xFF000000 | ppu->cgram[cgram_addr];
            break;
        }

        case FOUR_BPP: {
            data_address += (y);
            uint16_t low = ppu->vram[data_address];
            uint16_t high = ppu->vram[data_address + 8];

            uint8_t plane[4] = {
                high >> 8, high & 0xFF,
                low >> 8, low & 0xFF
            };

            uint16_t color_index = 0;

            for (int i = 0; i < 4; i++) {
                color_index |= get_bit(plane[i], x) << (3 - i);
            }

            if (!color_index) {
                break;
            }

            uint8_t cgram_addr = (chr.palette_index * 16) + color_index;
            result = 0xFF000000 | ppu->cgram[cgram_addr];
            break;
        }
    }

    return result;
}

void draw_background(ppu* ppu, uint8_t bg) {

}


void put_pixel(ppu* ppu, uint8_t x, uint8_t y, uint32_t color) {
    ppu->framebuffer[y * SNES_SCREEN_WIDTH + x] = color;
}

uint32_t rgb15_to_rgb24(uint32_t color) {
    uint16_t r = (color % 32) * 8; 
    uint16_t g = ((color / 32) % 32) * 8; 
    uint16_t b = ((color / 1024) % 32) * 8; 

    return (r << 16) | (g << 8) | b; 
}

void increment_current_y(ppu* ppu) {
    ppu->current_y++;

    if (ppu->current_y > 261) {
        ppu->current_y = 0;
    }
}