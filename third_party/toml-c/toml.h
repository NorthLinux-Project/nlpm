#ifndef TOML_H
#define TOML_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOML_STRING,
    TOML_INT,
    TOML_BOOL,
    TOML_ARRAY,
    TOML_TABLE
} TomlType;

typedef struct TomlValue TomlValue;
typedef struct TomlTable TomlTable;
typedef struct TomlArray TomlArray;

struct TomlArray {
    TomlValue *items;
    size_t count;
};

struct TomlValue {
    TomlType type;
    union {
        char *str;
        int64_t int_val;
        bool bool_val;
        TomlArray *array;
        TomlTable *table;
    };
};

typedef struct {
    char *key;
    TomlValue value;
} TomlEntry;

struct TomlTable {
    TomlEntry *entries;
    size_t count;
};

TomlTable *toml_parse(const char *src, char *err_buf, size_t err_size);
TomlTable *toml_parse_file(const char *path, char *err_buf, size_t err_size);
void toml_free(TomlTable *table);

const TomlValue *toml_get(const TomlTable *table, const char *key);
const char *toml_get_string(const TomlTable *table, const char *key, const char *default_val);
int64_t toml_get_int(const TomlTable *table, const char *key, int64_t default_val);
bool toml_get_bool(const TomlTable *table, const char *key, bool default_val);
const TomlTable *toml_get_table(const TomlTable *table, const char *key);
const TomlArray *toml_get_array(const TomlTable *table, const char *key);

#ifdef __cplusplus
}
#endif

#endif
