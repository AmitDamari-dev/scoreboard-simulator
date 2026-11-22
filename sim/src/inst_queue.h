/*
 * inst_queue.h
 *
 * Fixed-size FIFO between Fetch and Issue.
 */

#ifndef INST_QUEUE_H
#define INST_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#include "isa.h"

/* A fetched word, its decoded fields, and its word address in instruction memory. */
typedef struct {
    uint32_t raw;
    Instruction inst;
    int pc;
} InstQueueEntry;

/* A ring buffer whose head and count identify the occupied entries. */
typedef struct {
    InstQueueEntry entries[INST_QUEUE_SIZE];
    size_t head;
    size_t count;
} InstQueue;

/* Initialize an empty queue. */
void iq_init(InstQueue *queue);

/* Return whether the queue has no entries. */
bool iq_is_empty(const InstQueue *queue);

/* Return whether all INST_QUEUE_SIZE entries are occupied. */
bool iq_is_full(const InstQueue *queue);

/* Return the number of queued instructions. */
size_t iq_count(const InstQueue *queue);

/* Append a fetched word and cache its decode. Return false if the queue is full. */
bool iq_push(InstQueue *queue, uint32_t raw, int pc);

/* Borrow the head until removal or reinitialization. Return NULL for an empty queue. */
const InstQueueEntry *iq_peek(const InstQueue *queue);

/* Remove the head, optionally copying it to entry. Return false if the queue is empty. */
bool iq_pop(InstQueue *queue, InstQueueEntry *entry);

#endif /* INST_QUEUE_H */
