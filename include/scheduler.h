#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef void (*event_callback)(void* state);

typedef struct scheduler_event {
    uint64_t timecode;
    long id;
    event_callback callback;
} scheduler_event;

typedef struct scheduler {
    int list_size, next_free_event;
    long id_counter;
    uint64_t current_time;
    scheduler_event* event_list;
} scheduler;

scheduler* new_scheduler(int length);
void free_scheduler(scheduler* scheduler);

scheduler_event* get_next_event(scheduler* scheduler);
void remove_event(scheduler* scheduler, int id);

bool schedule_event(scheduler* scheduler, int timecode, event_callback callback);
bool schedule_event_set_id(scheduler* scheduler, long* event_id, int timecode, event_callback callback);

void print_scheduled_events(scheduler* scheduler);

#endif