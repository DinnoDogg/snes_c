#ifndef CART_H
#define CART_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cart cart;

typedef uint8_t (*cart_read_callback)(cart* cart, uint32_t address, uint8_t open_bus);
typedef void (*cart_write_callback)(cart* cart, uint32_t address, uint8_t data);

typedef enum cart_map {
    LO_ROM, HI_ROM, EX_HI_ROM = 0x5
} cart_map;

typedef enum coprocessor_type {
    DSP, SUPERFX, OBC1, SA1, S_DD1, S_RTC, SUPER_GAMEBOY, CUSTOM, NO_COPROCESSOR
} coprocessor_type;

struct cart {
    cart_read_callback read;
    cart_write_callback write;

    uint8_t* rom;
    uint8_t* ram;

    int rom_size, ram_size;

    bool has_ram, has_battery;

    coprocessor_type coprocessor_type;
};

cart* new_cart(uint8_t* rom_buffer);
void free_cart(cart* cart);

uint8_t read_cart(cart* cart, uint32_t address, uint8_t open_bus);
void write_cart(cart* cart, uint32_t address, uint8_t data);

#endif