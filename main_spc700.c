#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <cjson/cJSON.h>

#include "include/processor/spc700/spc700.h"

#include "include/color.h"

#define SET_REGISTER(REG) cpu->registers.REG = cJSON_GetObjectItem(initial_json, #REG)->valueint
#define CHECK_REGISTER(REG) int reg_##REG##_value = cJSON_GetObjectItem(final_json, #REG)->valueint; if (cpu->registers.REG != reg_##REG##_value ) {printf(BRED "\nRegister " #REG " bad. Expected: %u, got: %u" COLOR_RESET, reg_##REG##_value, cpu->registers.REG); success = false;}

static uint8_t test_memory[0x10000];

static void init_test(SPC700* cpu, cJSON* test_json) {
    cJSON* initial_json = cJSON_GetObjectItem(test_json, "initial");
    cJSON* ram = cJSON_GetObjectItem(initial_json, "ram");

    int ram_size = cJSON_GetArraySize(ram); 

    SET_REGISTER(a);
    SET_REGISTER(x);
    SET_REGISTER(y);
    SET_REGISTER(sp);
    SET_REGISTER(psw);
    SET_REGISTER(pc);

    for (int i = 0; i < ram_size; i++) {
        cJSON* ram_entry = cJSON_GetArrayItem(ram, i);
        
        uint16_t address = cJSON_GetArrayItem(ram_entry, 0)->valueint;
        uint8_t data = cJSON_GetArrayItem(ram_entry, 1)->valueint;

        test_memory[address] = data;
    }
}

static bool end_test(SPC700* cpu, int cycle_count, cJSON* test_json) {
    cJSON* final_json = cJSON_GetObjectItem(test_json, "final");
    cJSON* cycle_list = cJSON_GetObjectItem(test_json, "cycles");
    cJSON* ram = cJSON_GetObjectItem(final_json, "ram");

    int ram_size = cJSON_GetArraySize(ram);
    int good_cycle_count = cJSON_GetArraySize(cycle_list);

    bool success = true;

    CHECK_REGISTER(a);
    CHECK_REGISTER(x);
    CHECK_REGISTER(y);
    CHECK_REGISTER(sp);
    CHECK_REGISTER(psw);
    CHECK_REGISTER(pc);


    for (int i = 0; i < ram_size; i++) {
        cJSON* ram_entry = cJSON_GetArrayItem(ram, i);
        
        uint16_t address = cJSON_GetArrayItem(ram_entry, 0)->valueint;
        uint8_t data = cJSON_GetArrayItem(ram_entry, 1)->valueint;

        if (test_memory[address] != data) {
            printf(BRED "\nMemory at address %u bad. Expected: %u, got: %u" COLOR_RESET, address, data, test_memory[address]);
            success = false;
        }
    }

    if (cycle_count != good_cycle_count) {
        printf(BRED "\nCycle count bad. Expected: %u, got: %u" COLOR_RESET, good_cycle_count, cycle_count);
        success = false;
    }

    if (!success) {
        printf("\n");
    }

    return success;
}

static uint8_t test_read(uint16_t address) {
    return test_memory[address];
}

static void test_write(uint16_t address, uint8_t data) {
    test_memory[address] = data;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(BRED "No file provided.\n" COLOR_RESET);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    
    if (file == NULL) {
        printf(BRED "Failed to open file.\n" COLOR_RESET);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);    
    char* file_buffer = malloc(file_size);
    fread(file_buffer, 1, file_size, file);

    fclose(file);

    cJSON* test_json = cJSON_Parse(file_buffer);
    int test_count = cJSON_GetArraySize(test_json);

    SPC700 test_cpu = {};
    init_spc700(&test_cpu, &test_read, &test_write);

    bool success = true;
    int i;

    printf(BYEL);

    clock_t start_time = clock();

    for (i = 0; i < test_count; i++) {
        printf("\rRunning test %s - %u/%u", argv[1], (i + 1), test_count);
        cJSON* current_test = cJSON_GetArrayItem(test_json, i);

        init_test(&test_cpu, current_test);
        int cycle_count = spc700_run_instruction(&test_cpu);
        success = end_test(&test_cpu, cycle_count, current_test);
        
        if (!success) {
            spc700_print_state(&test_cpu);
            break;
        }
    }

    clock_t end_time = clock();
    float elapsed = (float) end_time / (float) CLOCKS_PER_SEC;

    if (success) {
        printf(BGRN "\r\033[2KPassed test %s in %f seconds\n" COLOR_RESET, argv[1], elapsed);
    }

    else {
        printf(BRED "\r\033[2KFailed test %s - %u/%u\n" COLOR_RESET, argv[1], (i + 1), test_count);
    }

    free(file_buffer);
    cJSON_Delete(test_json);
    
    return 0;
}