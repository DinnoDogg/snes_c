#ifndef CART_H
#define CART_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*read_callback)(uint32_t address);
typedef void (*write_callback)(uint32_t address, uint8_t data);

typedef enum cart_map {
    LO_ROM, HI_ROM, EX_HI_ROM = 0x5
} cart_map;

typedef struct cart {
    char title[0x16];
    bool has_ram, has_battery, slow_rom;

    int rom_size, ram_size;

    cart_map map_mode;

    uint8_t* rom;
    uint8_t* ram;

    read_callback read;
    write_callback write;
} cart;

cart* new_cart(uint8_t* rom_buffer, long rom_size);
void free_cart(cart* cart);

void print_cart_details(cart* cart);

#endif