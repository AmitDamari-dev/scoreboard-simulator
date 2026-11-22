/*
 * inst_queue.c
 *
 * Store fetched words in a fixed-size FIFO for Issue.
 */

#include "inst_queue.h"

void iq_init(InstQueue *queue)
{
    *queue = (InstQueue){ 0 };
}

bool iq_is_empty(const InstQueue *queue)
{
    return queue->count == 0;
}

bool iq_is_full(const InstQueue *queue)
{
    return queue->count == INST_QUEUE_SIZE;
}

size_t iq_count(const InstQueue *queue)
{
    return queue->count;
}

bool iq_push(InstQueue *queue, uint32_t raw, int pc)
{
    if (iq_is_full(queue)) {
        return false;
    }

    size_t tail = (queue->head + queue->count) % INST_QUEUE_SIZE;
    queue->entries[tail] = (InstQueueEntry){ .raw = raw, .inst = isa_decode(raw), .pc = pc };
    ++queue->count;
    return true;
}

const InstQueueEntry *iq_peek(const InstQueue *queue)
{
    return iq_is_empty(queue) ? NULL : &queue->entries[queue->head];
}

bool iq_pop(InstQueue *queue, InstQueueEntry *entry)
{
    const InstQueueEntry *head = iq_peek(queue);
    if (head == NULL) {
        return false;
    }
    if (entry != NULL) {
        *entry = *head;
    }
    queue->head = (queue->head + 1) % INST_QUEUE_SIZE;
    --queue->count;
    return true;
}
