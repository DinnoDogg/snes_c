#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/cart.h"

#include "include/color.h"

static const char* get_map_string(cart* cart);
static uint8_t* get_rom_header(uint8_t* rom_buffer, long rom_size); 

cart* new_cart(uint8_t* rom_buffer, long rom_size) {
    get_rom_header(rom_buffer, rom_size);
    return NULL;
}

void free_cart(cart* cart) {
    free(cart);
}

void print_cart_details(cart* cart) {
    const uint8_t* map_mode = get_map_string(cart);
    printf("Title: %s\n", cart->title);
    printf("Mapping mode: %s\n", map_mode);
    printf("Has ram: %u", cart->has_ram);

    if (cart->has_ram) {
        printf("(%u\n KB): %u\n", cart->ram_size);
    }

    else {
        printf("\n", cart->ram_size);
    }

    printf("rom size: %u\n", cart->rom_size);
    printf("slow rom: %u\n", cart->slow_rom);
}

const char* get_map_string(cart* cart) {
    switch (cart->map_mode) {
        case LO_ROM: return "LO ROM";
        case HI_ROM: return "HI ROM";
        case EX_HI_ROM: return "EX HI ROM";
        default: return "UNKNOWN";
    }
}

uint8_t* get_rom_header(uint8_t* rom_buffer, long rom_size) {
    uint16_t checksum = 0;

    for (long i = 0; i < rom_size; i++) {
        checksum += rom_buffer[i];
    }

    printf("size %u\n", rom_size);

    printf("Checksum: %u  %04X\n", checksum, checksum);

    return NULL;
}