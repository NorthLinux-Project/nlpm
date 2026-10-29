#define _POSIX_C_SOURCE 200809L
#include "hash_map.h"
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME        16777619u
#define MAX_LOAD_FACTOR  0.70f

static uint32_t fnv1a_hash(const char *key) {
    uint32_t hash = FNV_OFFSET_BASIS;
    while (*key) {
        hash ^= (unsigned char)(*key++);
        hash *= FNV_PRIME;
    }
    return hash;
}

static size_t next_power_of_two(size_t n) {
    size_t cap = 16;
    while (cap < n) {
        cap <<= 1;
    }
    return cap;
}

HashMap hash_map_create(size_t initial_capacity) {
    HashMap map = {0};
    map.capacity = next_power_of_two(initial_capacity);
    map.slots = (HashSlot *)calloc(map.capacity, sizeof(HashSlot));
    return map;
}

void hash_map_destroy(HashMap *map) {
    if (map && map->slots) {
        for (size_t i = 0; i < map->capacity; ++i) {
            if (map->slots[i].state == SLOT_OCCUPIED) {
                free(map->slots[i].key);
            }
        }
        free(map->slots);
        map->slots = NULL;
        map->capacity = 0;
        map->count = 0;
        map->tombstones = 0;
    }
}

static bool insert_internal(HashSlot *slots, size_t capacity, char *key, uintptr_t value) {
    uint32_t hash = fnv1a_hash(key);
    size_t mask = capacity - 1;
    size_t index = hash & mask;
    size_t tombstone_index = (size_t)-1;

    for (;;) {
        SlotState state = slots[index].state;

        if (state == SLOT_EMPTY) {
            size_t target = (tombstone_index != (size_t)-1) ? tombstone_index : index;
            slots[target].key = key;
            slots[target].value = value;
            slots[target].state = SLOT_OCCUPIED;
            return true;
        }

        if (state == SLOT_TOMBSTONE && tombstone_index == (size_t)-1) {
            tombstone_index = index;
        } else if (state == SLOT_OCCUPIED && strcmp(slots[index].key, key) == 0) {
            slots[index].value = value;
            free(key);
            return false;
        }

        index = (index + 1) & mask;
    }
}

static void resize_map(HashMap *map, size_t new_capacity) {
    HashSlot *new_slots = (HashSlot *)calloc(new_capacity, sizeof(HashSlot));
    if (!new_slots) return;

    for (size_t i = 0; i < map->capacity; ++i) {
        if (map->slots[i].state == SLOT_OCCUPIED) {
            insert_internal(new_slots, new_capacity, map->slots[i].key, map->slots[i].value);
        }
    }

    free(map->slots);
    map->slots = new_slots;
    map->capacity = new_capacity;
    map->tombstones = 0;
}

bool hash_map_set(HashMap *map, const char *key, uintptr_t value) {
    if (!map || !map->slots || !key) return false;

    if ((float)(map->count + map->tombstones + 1) / (float)map->capacity >= MAX_LOAD_FACTOR) {
        resize_map(map, map->capacity * 2);
    }

    char *key_copy = strdup(key);
    if (!key_copy) return false;

    uint32_t hash = fnv1a_hash(key);
    size_t mask = map->capacity - 1;
    size_t index = hash & mask;
    size_t tombstone_index = (size_t)-1;

    for (;;) {
        SlotState state = map->slots[index].state;

        if (state == SLOT_EMPTY) {
            size_t target = (tombstone_index != (size_t)-1) ? tombstone_index : index;
            if (tombstone_index != (size_t)-1) {
                map->tombstones--;
            }
            map->slots[target].key = key_copy;
            map->slots[target].value = value;
            map->slots[target].state = SLOT_OCCUPIED;
            map->count++;
            return true;
        }

        if (state == SLOT_TOMBSTONE && tombstone_index == (size_t)-1) {
            tombstone_index = index;
        } else if (state == SLOT_OCCUPIED && strcmp(map->slots[index].key, key) == 0) {
            map->slots[index].value = value;
            free(key_copy);
            return false;
        }

        index = (index + 1) & mask;
    }
}

bool hash_map_get(const HashMap *map, const char *key, uintptr_t *out_val) {
    if (!map || !map->slots || !key) return false;

    uint32_t hash = fnv1a_hash(key);
    size_t mask = map->capacity - 1;
    size_t index = hash & mask;
    size_t start = index;

    while (map->slots[index].state != SLOT_EMPTY) {
        if (map->slots[index].state == SLOT_OCCUPIED && strcmp(map->slots[index].key, key) == 0) {
            if (out_val) {
                *out_val = map->slots[index].value;
            }
            return true;
        }
        index = (index + 1) & mask;
        if (index == start) break;
    }

    return false;
}

bool hash_map_delete(HashMap *map, const char *key) {
    if (!map || !map->slots || !key) return false;

    uint32_t hash = fnv1a_hash(key);
    size_t mask = map->capacity - 1;
    size_t index = hash & mask;
    size_t start = index;

    while (map->slots[index].state != SLOT_EMPTY) {
        if (map->slots[index].state == SLOT_OCCUPIED && strcmp(map->slots[index].key, key) == 0) {
            free(map->slots[index].key);
            map->slots[index].key = NULL;
            map->slots[index].state = SLOT_TOMBSTONE;
            map->count--;
            map->tombstones++;
            return true;
        }
        index = (index + 1) & mask;
        if (index == start) break;
    }

    return false;
}
