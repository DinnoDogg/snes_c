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

    SPC700* spc = (SPC700*) snes->apu;

    //int next_spc_tick = 0;
    //int accumulator = 0, master_time = 0;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long start = get_nanos(&ts);

    int frame_count = 5000;

    while (snes->scheduler->current_time < 357368 * frame_count) {
        scheduler_event* next_event = get_next_event(snes->scheduler);

        while(snes->scheduler->current_time >= next_event->timecode) {
            long id = next_event->id;
            next_event->callback(snes);
            remove_event(snes->scheduler, id);
            next_event = get_next_event(snes->scheduler);
        }

        int cycles_elapsed = cycle_cpu(snes->cpu);
        snes->scheduler->current_time += cycles_elapsed;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long end = get_nanos(&ts);

    double elapsed = (double) (end - start) / 1e+6f;
    double average = elapsed / (double) frame_count;

    print_scheduled_events(snes->scheduler);

    wdc65816_print_state((WDC65816*) snes->cpu);
    spc700_print_state((SPC700*) snes->apu);

    for (int i = 0; i < 4; i++) {
        printf("APU IO %u  IN: %02X  OUT: %02X\n", i, snes->apu->io_port[i].data_in, snes->apu->io_port[i].data_out);
    }

    printf("\n");

    for (int i = 0; i < 3; i++) {
        apu_timer* t = &snes->apu->timer[i];
        printf("APU timer %u  interval: %u  internal counter: %u  up counter: %u  event id: %lu\n", i, t->interval, t->internal_counter, t->up_counter, t->tick_event_id);
    }

    printf("elapsed ms %f  averaging %f ms per frame (%u frames) \n", elapsed, average, frame_count);

    free_cart(cart);
    free_snes(snes);

    free(rom_buffer);

    return 0;
}