#include <time.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/scheduler.h"

#include "include/processor/wdc65816/wdc65816.h"

static unsigned int g_seed;

// Used to seed the generator.           

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}


static long get_nanos(struct timespec* ts) {
    return (long)ts->tv_sec * 1000000000L + ts->tv_nsec;
}

static uint8_t test_read(uint32_t address) {
    return fast_rand() & 0xFF;
}

static void test_write(uint32_t address, uint8_t data) {
    return;
} 

int main() {
    snes* snes = new_snes();
    struct timespec ts;

    g_seed = time(NULL);

    int relative_time = 0, master_time = 0;

    timespec_get(&ts, TIME_UTC);
    long start = get_nanos(&ts);

    WDC65816 test_65816 = {0};
    init_wdc65816(&test_65816, &test_read, &test_write);

    while (master_time < 357268) {

        while (relative_time >= get_next_event(snes->scheduler)->timecode) {
            long id = get_next_event(snes->scheduler)->id;
            get_next_event(snes->scheduler)->callback(snes);
            remove_event(snes->scheduler, id);

            relative_time = 0;
        }

        cycle_wdc65816(&test_65816);

        relative_time += 6;
        master_time += 6;
    }

    timespec_get(&ts, TIME_UTC);
    long end = get_nanos(&ts);
    long elapsed = end - start;

    print_scheduled_events(snes->scheduler);

    printf("elapsed ms %f \n", (float) elapsed / 1e+6f );

    free_snes(snes);
}

