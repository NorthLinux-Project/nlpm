#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SLOT_EMPTY = 0,
    SLOT_OCCUPIED,
    SLOT_TOMBSTONE
} SlotState;

typedef struct {
    char *key;
    uintptr_t value;
    SlotState state;
} HashSlot;

typedef struct {
    HashSlot *slots;
    size_t capacity;
    size_t count;
    size_t tombstones;
} HashMap;

HashMap hash_map_create(size_t initial_capacity);
void hash_map_destroy(HashMap *map);
bool hash_map_set(HashMap *map, const char *key, uintptr_t value);
bool hash_map_get(const HashMap *map, const char *key, uintptr_t *out_val);
bool hash_map_delete(HashMap *map, const char *key);

#endif
