#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/cart.h"

#include "include/color.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(BRED "No ROM provided.\n" COLOR_RESET);
        return 0;
    }

    FILE* rom_file = fopen(argv[1], "rb");

    if (rom_file == NULL) {
        printf(BRED "Failed tp open ROM file.\n" COLOR_RESET);
        return 1;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    uint8_t* rom_buffer = malloc(rom_size);

    fread(rom_buffer, 1, rom_size, rom_file);

    fclose(rom_file);

    new_cart(rom_buffer, rom_size);

    free(rom_buffer);
}