#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

#include "processor/spc700/spc700.h"

typedef struct apu apu;

typedef struct apu_bus {
    SPC700_bus base;
    uint8_t* audio_ram;
    apu* apu;
    //dsp
} apu_bus;

typedef struct apu_io_port {
    uint8_t data_in, data_out;
} apu_io_port;

typedef struct apu_timer {
    uint8_t count : 4;
    uint8_t interval;
    long event_id;
} apu_timer;

struct apu {
    SPC700 base;

    apu_io_port io_port[0x4];
    apu_timer timer[0x3];

    uint8_t dsp_address;

    bool ipl_enable;
};

apu* new_apu(uint8_t* audio_ram);
void free_apu(apu* apu);

void reset_apu(apu* apu);

#endif