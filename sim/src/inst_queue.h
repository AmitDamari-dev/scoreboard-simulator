// Fixed-size FIFO instruction queue between Fetch and Issue.

#ifndef INST_QUEUE_H
#define INST_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#include "isa.h"

typedef struct {
    uint32_t raw;
    Instruction inst;
    int pc;
} InstQueueEntry;

typedef struct {
    InstQueueEntry entries[INST_QUEUE_SIZE];
    size_t head;
    size_t count;
} InstQueue;

void iq_init(InstQueue *queue);

bool iq_is_empty(const InstQueue *queue);

bool iq_is_full(const InstQueue *queue);

size_t iq_count(const InstQueue *queue);

bool iq_push(InstQueue *queue, uint32_t raw, int pc);

// Returns a pointer valid until the next pop or re-init, or NULL when empty.
const InstQueueEntry *iq_peek(const InstQueue *queue);

// Pop the head into entry if non-NULL.
bool iq_pop(InstQueue *queue, InstQueueEntry *entry);

#endif
