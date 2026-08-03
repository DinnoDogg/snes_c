#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/snes.h"
#include "include/cart.h"
#include "include/apu.h"
#include "include/cpu.h"
#include "include/processor/wdc65816/wdc65816.h"

static uint8_t read_cpu_bus(void* c, uint32_t address, bool valid_access);
static void write_cpu_bus(void* c, uint32_t address, uint8_t data);

static unsigned int g_seed;

// Used to seed the generator.           

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}


cpu* new_cpu(cart* cart, wram* wram, apu* apu) {
    cpu* result = malloc(sizeof(cpu));

    init_wdc65816((WDC65816*) result, &read_cpu_bus, &write_cpu_bus);

    result->bus.wram = wram;
    result->bus.apu = apu;
    result->bus.cart = cart;

    return result;
}

void free_cpu(cpu* cpu) {
    free(cpu);
}

uint8_t read_cpu_bus(void* c, uint32_t address, bool valid_access) {
    cpu* cpu_ptr = (cpu*) c;
    
    uint8_t result = fast_rand() & 0xFF;

    if (valid_access) {
        cpu_ptr->mdr = result;
    }

    return result;
}

void write_cpu_bus(void* c, uint32_t address, uint8_t data) {
    cpu* cpu_ptr = (cpu*) c;
    cpu_ptr->mdr = data;
}
