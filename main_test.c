#include <time.h>
#include <stdlib.h>

#include "include/apu.h"
#include "include/scheduler.h"

uint8_t audio_ram[0x10000];

typedef struct test_snes {
    scheduler* s;
    apu* apu;
} test_snes;

static void test_spc_run_op(void* testy); 
static void test_schedule_spc(void* testy);

void test_schedule_spc(void* testy) {
    test_snes* snes_test = (test_snes*) testy;
    SPC700* test_spc = (SPC700*) snes_test->apu;

    uint8_t opcode = spc700_read_immediate(test_spc);
    int exec_in = spc700_get_op_time(opcode) - 1;

    test_spc->opcode = opcode;
    exec_in *= 21;

    schedule_event(snes_test->s, exec_in, &test_spc_run_op);
}

void test_spc_run_op(void* testy) {
    test_snes* snes_test = (test_snes*) testy;
    SPC700* test_spc = (SPC700*) snes_test->apu;

    spc700_run_instruction(test_spc, test_spc->opcode);

    test_schedule_spc(testy);
}

int main() {
    test_snes testy = {
        new_scheduler(0x100),
        new_apu(audio_ram)
    };

    test_schedule_spc(&testy);

    int t = 0;
    int master_cycles = 0;

    clock_t start = clock();

    while (master_cycles < 400000) {
        while (t < get_next_event(testy.s)->timecode) {
            t += 6;
            master_cycles += 6;
        }

        //printf("Sussy Tee %u\n\n", t);

        get_next_event(testy.s)->callback(&testy);
        remove_event(testy.s, get_next_event(testy.s)->id);
        t = 0;
    }

    clock_t end = clock();
    double elapsed = (float) (end - start) / (float) CLOCKS_PER_SEC;

    print_scheduled_events(testy.s);

    printf("Elapsed: %f sec, %f ms \n", elapsed, elapsed * 1000.0f);

    free_scheduler(testy.s);
    free_apu(testy.apu);

    return 0;
}