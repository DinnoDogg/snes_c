#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "include/scheduler.h"

static void remove_and_sift(scheduler* scheduler, unsigned int root);
static bool push(scheduler* scheduler, scheduler_event event);

scheduler* new_scheduler(unsigned int size) {
    scheduler* result = calloc(1, sizeof(scheduler));
    result->list = calloc(size, sizeof(scheduler_event));

    result->size = size;
    result->id_counter = 1;

    return result;
}

void free_scheduler(scheduler* scheduler) {
    if (scheduler->list) {
        free(scheduler->list);
    }

    free(scheduler);
}

scheduler_event scheduler_peek(scheduler* scheduler) {
    return scheduler->list[0];
}

bool scheduler_push(scheduler* scheduler, uint32_t relative_time, scheduler_callback callback) {
    scheduler_event event = {
        scheduler->id_counter++, scheduler->current_time + relative_time, callback
    };

    bool success = push(scheduler, event);
    return success;
}

bool scheduler_push_set_id(scheduler* scheduler, uint64_t* id, uint32_t relative_time, scheduler_callback callback) {
    *id = scheduler->id_counter++;
    
    scheduler_event event = {
        *id, scheduler->current_time + relative_time, callback
    };

    bool success = push(scheduler, event);
    return success;
}

scheduler_event scheduler_pop(scheduler* scheduler) {
    scheduler_event result = scheduler->list[0];
    remove_and_sift(scheduler, 0);
    return result;
}

void scheduler_remove_by_id(scheduler* scheduler, uint64_t id) {
    int index = -1;

    for (int i = 0; i < scheduler->back; i++) {
        if (scheduler->list[i].id != id) {
            continue;
        }

        index = i;
        break;
    }

    if (index == -1) {
        return;
    }

    remove_and_sift(scheduler, index);
}

void remove_and_sift(scheduler* scheduler, unsigned int root) {
    if (scheduler->back == 0) {
        return;
    }

    unsigned int back = --scheduler->back;
    scheduler->list[root] = scheduler->list[back];

    while (true) {
        unsigned int l_child = (root << 1) + 1, r_child = l_child + 1;
        unsigned int smallest = root;

        if (l_child < back && scheduler->list[l_child].time < scheduler->list[smallest].time) {
            smallest = l_child;
        }

        if (r_child < back && scheduler->list[r_child].time < scheduler->list[smallest].time) {
            smallest = r_child;
        }

        if (smallest == root) {
            break;
        } 

        scheduler_event e = scheduler->list[root];

        scheduler->list[root] = scheduler->list[smallest];
        scheduler->list[smallest] = e;

        root = smallest;
    }

    scheduler->list[back].time = 0;
    scheduler->list[back].id = 0;
    scheduler->list[back].callback = NULL;
}

bool push(scheduler* scheduler, scheduler_event event) {
    if (scheduler->back >= scheduler->size) {
        return false;
    }

    unsigned int back = scheduler->back++;
    scheduler->list[back] = event;

    while (back) {
        unsigned int parent = (back - 1) >> 1;

        if (scheduler->list[back].time > scheduler->list[parent].time) {
            break;
        }

        scheduler_event e = scheduler->list[back];

        scheduler->list[back] = scheduler->list[parent];
        scheduler->list[parent] = e;
        back = parent;
    }

    return true;
}