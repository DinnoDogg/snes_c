#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "include/scheduler.h"

static long schedule(scheduler* scheduler, int timecode, event_callback callback);

scheduler* new_scheduler(int length) {
    scheduler* result = calloc(sizeof(scheduler), 1);
    result->event_list = calloc(sizeof(scheduler_event), length);
    result->list_size = length;
    return result;
}

void free_scheduler(scheduler* scheduler) {
    free(scheduler->event_list);
    free(scheduler);
}

scheduler_event* get_next_event(scheduler* scheduler) {
    return &scheduler->event_list[0];
}

bool schedule_event(scheduler* scheduler, int timecode, event_callback callback) {
    if (scheduler->next_free_event == scheduler->list_size) {
        return false;
    }

    schedule(scheduler, timecode, callback);
    return true;
}

bool schedule_event_set_id(scheduler* scheduler, long* event_id, int timecode, event_callback callback) {
    if (scheduler->next_free_event == scheduler->list_size) {
        return false;
    }

    *event_id = schedule(scheduler, timecode, callback);
    return true;
}

void remove_event(scheduler* scheduler, int id) {
    if (scheduler->next_free_event == 0) {
        return;
    }

    bool id_found = false;
    int index;

    for (int i = 0; i < scheduler->next_free_event; i++) {
        if (scheduler->event_list[i].id == id) {
            id_found = true;
            index = i;
            break;
        }
    }

    if (!id_found) {
        return;
    }

    int end = --scheduler->next_free_event;
    
    while (index < scheduler->next_free_event) {
        scheduler->event_list[index] = scheduler->event_list[index+1];  
        index++;
    } 

    scheduler->event_list[end].timecode = 0;
    scheduler->event_list[end].callback = NULL;
    scheduler->event_list[end].id = 0;
}

void print_scheduled_events(scheduler* scheduler) {
    for (int i = 0; i < scheduler->list_size; i++) {
        printf("Index: %X, ID: %lu, Timecode: %lu\n", i, scheduler->event_list[i].id, scheduler->event_list[i].timecode);
    }
}

long schedule(scheduler* scheduler, int timecode, event_callback callback) {
    int i = scheduler->next_free_event;
    int id = scheduler->id_counter++;
    
    uint64_t master_timecode = scheduler->current_time + timecode;

    scheduler->event_list[i].timecode = master_timecode;
    scheduler->event_list[i].callback = callback;
    scheduler->event_list[i].id = id;

    while (i > 0 && scheduler->event_list[i-1].timecode > scheduler->event_list[i].timecode) {
        scheduler_event e = scheduler->event_list[i];

        scheduler->event_list[i] = scheduler->event_list[i-1];
        scheduler->event_list[i-1] = e;

        i--;
    }

    scheduler->next_free_event++;
    return id;
}