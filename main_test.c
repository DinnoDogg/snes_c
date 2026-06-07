#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "include/processor/wdc65816/wdc65816.h"

uint8_t test_read(uint32_t address) {
    return 0xE4;
}

void test_write(uint32_t address, uint8_t data) {
    return;
}

int main() {
    WDC65816 test_cpu = {};

    init_wdc65816(&test_cpu, &test_read, &test_write);

    clock_t start_time = clock();

    for (int i = 0; i < 50000; i++) {
        wdc65816_run_instruction(&test_cpu);
    }

    clock_t end_time = clock();
    float elapsed = (float) end_time / (float) CLOCKS_PER_SEC;

    printf("Elapsed: %f\n", elapsed);

    return 0;
}
