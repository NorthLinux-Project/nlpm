#ifndef STRETCHY_BUFFER_H
#define STRETCHY_BUFFER_H

#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t capacity;
    size_t count;
} BufferHeader;

#define sb__header(b)     ((BufferHeader *)((char *)(b) - sizeof(BufferHeader)))
#define sb__need_grow(b, n) ((b) == NULL || sb__header(b)->count + (n) > sb__header(b)->capacity)
#define sb__maybe_grow(b, n) (sb__need_grow((b), (n)) ? (sb__grow((void **)&(b), (n), sizeof(*(b)))) : 0)

#define sb_count(b)       ((b) ? sb__header(b)->count : 0)
#define sb_capacity(b)    ((b) ? sb__header(b)->capacity : 0)
#define sb_push(b, v)     (sb__maybe_grow((b), 1), (b)[sb__header(b)->count++] = (v))
#define sb_pop(b)         ((b) && sb__header(b)->count > 0 ? (b)[--sb__header(b)->count] : 0)
#define sb_last(b)        ((b)[sb__header(b)->count - 1])
#define sb_free(b)        ((b) ? (free(sb__header(b)), (b) = NULL) : 0)

static inline int sb__grow(void **buf, size_t increment, size_t item_size) {
    size_t cur_cap = *buf ? sb__header(*buf)->capacity : 0;
    size_t cur_count = *buf ? sb__header(*buf)->count : 0;
    size_t min_cap = cur_count + increment;
    size_t new_cap = cur_cap ? cur_cap * 2 : 8;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }

    size_t total_size = sizeof(BufferHeader) + new_cap * item_size;
    BufferHeader *header;

    if (*buf) {
        header = (BufferHeader *)realloc(sb__header(*buf), total_size);
    } else {
        header = (BufferHeader *)malloc(total_size);
        if (header) {
            header->count = 0;
        }
    }

    if (!header) {
        return 0;
    }

    header->capacity = new_cap;
    *buf = (char *)header + sizeof(BufferHeader);
    return 1;
}

#endif
