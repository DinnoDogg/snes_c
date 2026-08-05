#include <time.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/scheduler.h"

#include "include/processor/wdc65816/wdc65816.h"

static long get_nanos(struct timespec* ts) {
    return (long)ts->tv_sec * 1000000000L + ts->tv_nsec;
}


int main() {
    snes* snes = new_snes(NULL);
    struct timespec ts;

    long frame_begin = 0;

    while (true) {
        int relative_time = 0, master_time = 0;

        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        long start = get_nanos(&ts);

        while (master_time < 357268) {
            while (relative_time >= get_next_event(snes->scheduler)->timecode) {
                long id = get_next_event(snes->scheduler)->id;
                get_next_event(snes->scheduler)->callback(snes);
                remove_event(snes->scheduler, id);

                relative_time = 0;
            }

            cycle_wdc65816((WDC65816*) snes->cpu);

            relative_time += 6;
            master_time += 6;
        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        long end = get_nanos(&ts);
        long elapsed = end - start;

        printf("elapsed ms %f \n", (float) elapsed / 1e+6f );

        while (true) {
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
            
            if ((float) (get_nanos(&ts) - frame_begin) >= 1.666667e+7) {
                frame_begin = get_nanos(&ts);
                break;
            } 
        }
    }

    print_scheduled_events(snes->scheduler);

    free_snes(snes);
}

