#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "include/snes.h"
#include "include/cart.h"

#include "include/color.h"

static long get_nanos(struct timespec* ts) {
    return (long)ts->tv_sec * 1000000000L + ts->tv_nsec;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(BRED "No ROM provided.\n" COLOR_RESET);
        return 0;
    }

    FILE* rom_file = fopen(argv[1], "rb");

    if (rom_file == NULL) {
        printf(BRED "Failed tp open ROM file.\n" COLOR_RESET);
        return 0;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    uint8_t* rom_buffer = malloc(rom_size);

    fread(rom_buffer, 1, rom_size, rom_file);

    fclose(rom_file);

    cart* cart = new_cart(rom_buffer);

    if (cart == NULL) {
        printf(BRED "Failed to start emulator.\n" COLOR_RESET);
        free(rom_buffer);
        return 0;
    }

    snes* snes = new_snes(cart);

    struct timespec ts;

    int relative_time = 0, master_time = 0;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long start = get_nanos(&ts);

    while (master_time < 357268 * 5) {
        while (relative_time >= get_next_event(snes->scheduler)->timecode) {
            long id = get_next_event(snes->scheduler)->id;
            get_next_event(snes->scheduler)->callback(snes);
            remove_event(snes->scheduler, id);

            relative_time = 0;
        }

        int cycles_elapsed = cycle_cpu(snes->cpu);

        relative_time += cycles_elapsed;
        master_time += cycles_elapsed;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long end = get_nanos(&ts);
    long elapsed = end - start;

    print_scheduled_events(snes->scheduler);

    wdc65816_print_state((WDC65816*) snes->cpu);
    spc700_print_state((SPC700*) snes->apu);

    //printf("elapsed ms %f \n", (float) elapsed / 1e+6f);

    free_cart(cart);
    free_snes(snes);

    free(rom_buffer);

    return 0;
}