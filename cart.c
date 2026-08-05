#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/cart.h"

#include "include/color.h"

typedef struct cart_chipset {
    bool has_battery, has_coprocessor, has_ram;
    coprocessor_type cp_type;
} cart_chipset;

static bool get_bit(int data, int index);

static const char* get_coprocessor_name(coprocessor_type type);

static cart_chipset get_chipset(uint8_t chipset);

static cart* new_lorom_cart(uint8_t* rom_buffer);
static cart* new_hirom_cart(uint8_t* rom_buffer);

static uint8_t read_lorom_cart(cart* cart, uint32_t address, uint8_t open_bus);
static void write_lorom_cart(cart* cart, uint32_t address, uint8_t data);

static uint8_t read_hirom_cart(cart* cart, uint32_t address, uint8_t open_bus);
static void write_hirom_cart(cart* cart, uint32_t address, uint8_t data);

bool get_bit(int data, int index) {
    return (data >> index) & 0x1;
}

cart* new_cart(uint8_t* rom_buffer) {
    //identify header
    //temporarily assume power of 2 sized lo rom cart
    return new_lorom_cart(rom_buffer);
}

void free_cart(cart* cart) {
    if (cart->has_ram) {
        free(cart->ram);
    }

    free(cart->rom);
    free(cart);
}

uint8_t read_cart(cart* cart, uint32_t address, uint8_t open_bus) {
    return cart->read(cart, address, open_bus);
}

void write_cart(cart* cart, uint32_t address, uint8_t data) {
    cart->write(cart, address, data);
}

const char* get_coprocessor_name(coprocessor_type type) {
    switch (type) {
        case DSP: return "DSP";
        case SUPERFX: return "SUPER FX";
        case OBC1: return "OBC 1";
        case SA1: return "SA-1";
        case S_DD1: return "S-DD 1";
        case S_RTC: return "S-RTC";
        case SUPER_GAMEBOY: return "SUPER GAMEBOY";
        case CUSTOM: return "CUSTOM";
        case NO_COPROCESSOR: "NO COPROCESSOR";
    }
}

cart_chipset get_chipset(uint8_t chipset) {
    cart_chipset result;

    switch (chipset & 0xF) {
        case 0:
            result.has_battery = false;
            result.has_ram = false;
            result.has_coprocessor = false;
            break;

        case 1:
            result.has_battery = false;
            result.has_ram = true;
            result.has_coprocessor = false;
            break;

        case 2:
            result.has_battery = true;
            result.has_ram = true;
            result.has_coprocessor = false;
            break;

        case 3:
            result.has_battery = false;
            result.has_ram = false;
            result.has_coprocessor = true;
            break;

        case 4:
            result.has_battery = false;
            result.has_ram = true;
            result.has_coprocessor = true;
            break;

        case 5:
            result.has_battery = true;
            result.has_ram = true;
            result.has_coprocessor = true;
            break;
            
        case 6:
            result.has_battery = true;
            result.has_ram = false;
            result.has_coprocessor = true;
            break;
    }

    result.cp_type = result.has_coprocessor ? (chipset >> 4) : NO_COPROCESSOR;

    return result;
}

cart* new_lorom_cart(uint8_t* rom_buffer) {
    cart* result;

    uint8_t* header = rom_buffer + 0x7FC0;

    char title[22];

    uint8_t map_mode = header[21];
    uint8_t chip = header[22];
    int rom_size = header[23];
    int ram_size = header[24];
    bool fast_rom = get_bit(map_mode, 4);

    rom_size = 1 << rom_size;

    cart_chipset chipset = get_chipset(chip);

    if (chipset.has_coprocessor) {
        printf(BRED "Coprocessors unsupported. Has: %s\n" COLOR_RESET, get_coprocessor_name(chipset.cp_type));
        return NULL;
    }

    result = malloc(sizeof(cart));

    if (chipset.has_ram && ram_size) {
        ram_size = 1 << ram_size;

        printf("RAM size: %u Kib\n", ram_size);
        printf("RAM battery backing: %u\n", chipset.has_battery);

        ram_size <<= 0xA;
        result->ram = calloc(ram_size, 1); 
    }

    printf("ROM size: %u Kib\n", rom_size);

    rom_size <<= 0xA;

    result->rom = malloc(rom_size);
    memcpy(result->rom, rom_buffer, rom_size);

    result->ram_size = ram_size;
    result->rom_size = rom_size;
    result->coprocessor_type = chipset.cp_type;
    result->has_battery = chipset.has_battery;
    result->has_ram = chipset.has_ram;

    memcpy(title, header, 21);
    title[21] = '\0';

    printf("Rom title: %s\n", title);

    result->read = &read_lorom_cart;
    result->write = &write_lorom_cart;

    return result;
}

uint8_t read_lorom_cart(cart* cart, uint32_t address, uint8_t open_bus) {
    uint8_t bank = address >> 0x10;
    uint8_t offset = (address >> 0x8) & 0xFF;

    if (offset < 0x80) {
        if (cart->has_ram && (bank > 0x6F || bank > 0xEF)) {
            address &= (cart->ram_size - 1);
            return cart->ram[address];
        }

        return open_bus;
    }

    uint32_t rom_address = address & 0x7FFF;
    rom_address |= ((address & 0x7F0000) >> 1);
    rom_address &= (cart->rom_size - 1); //assume power of 2... DANGEROUS WIP :(

    return cart->rom[rom_address];
}

void write_lorom_cart(cart* cart, uint32_t address, uint8_t data) {
    uint8_t bank = address >> 0x10;
    uint8_t offset = (address >> 0x8) & 0xFF;

    if (offset < 0x80) {
        if (cart->has_ram && (bank > 0x6F || bank > 0xEF)) {
            address &= (cart->ram_size - 1);
            cart->ram[address] = data;
        }
    }
}
