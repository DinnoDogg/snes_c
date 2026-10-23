#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*scheduler_callback)(void* state);

typedef struct scheduler_event {
    uint64_t id, time;
    scheduler_callback callback;
} scheduler_event;

typedef struct scheduler {
    unsigned int size, back;
    uint64_t id_counter, current_time;
    scheduler_event* list;
} scheduler;

scheduler* new_scheduler(unsigned int size);
void free_scheduler(scheduler* scheduler);

bool scheduler_push(scheduler* scheduler, uint32_t relative_time, scheduler_callback callback);

scheduler_event scheduler_pop(scheduler* scheduler);
scheduler_event scheduler_peek(scheduler* scheduler);

bool scheduler_push_set_id(scheduler* scheduler, uint64_t* id, uint32_t relative_time, scheduler_callback callback);
void scheduler_remove_by_id(scheduler* scheduler, uint64_t id);

#endif