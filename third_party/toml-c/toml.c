#define _POSIX_C_SOURCE 200809L
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void toml_value_free(TomlValue *val);

static TomlTable *toml_table_new(void) {
    TomlTable *t = (TomlTable *)calloc(1, sizeof(TomlTable));
    return t;
}

static TomlArray *toml_array_new(void) {
    TomlArray *a = (TomlArray *)calloc(1, sizeof(TomlArray));
    return a;
}

static void toml_table_add(TomlTable *table, const char *key, TomlValue val) {
    for (size_t i = 0; i < table->count; ++i) {
        if (strcmp(table->entries[i].key, key) == 0) {
            toml_value_free(&table->entries[i].value);
            table->entries[i].value = val;
            return;
        }
    }
    table->entries = (TomlEntry *)realloc(table->entries, (table->count + 1) * sizeof(TomlEntry));
    table->entries[table->count].key = strdup(key);
    table->entries[table->count].value = val;
    table->count++;
}

static void toml_array_add(TomlArray *arr, TomlValue val) {
    arr->items = (TomlValue *)realloc(arr->items, (arr->count + 1) * sizeof(TomlValue));
    arr->items[arr->count++] = val;
}

static void toml_value_free(TomlValue *val) {
    if (!val) return;
    if (val->type == TOML_STRING) {
        free(val->str);
    } else if (val->type == TOML_ARRAY && val->array) {
        for (size_t i = 0; i < val->array->count; ++i) {
            toml_value_free(&val->array->items[i]);
        }
        free(val->array->items);
        free(val->array);
    } else if (val->type == TOML_TABLE && val->table) {
        toml_free(val->table);
    }
    memset(val, 0, sizeof(*val));
}

void toml_free(TomlTable *table) {
    if (!table) return;
    for (size_t i = 0; i < table->count; ++i) {
        free(table->entries[i].key);
        toml_value_free(&table->entries[i].value);
    }
    free(table->entries);
    free(table);
}

const TomlValue *toml_get(const TomlTable *table, const char *key) {
    if (!table || !key) return NULL;
    for (size_t i = 0; i < table->count; ++i) {
        if (strcmp(table->entries[i].key, key) == 0) {
            return &table->entries[i].value;
        }
    }
    return NULL;
}

const char *toml_get_string(const TomlTable *table, const char *key, const char *default_val) {
    const TomlValue *v = toml_get(table, key);
    if (v && v->type == TOML_STRING) {
        return v->str;
    }
    return default_val;
}

int64_t toml_get_int(const TomlTable *table, const char *key, int64_t default_val) {
    const TomlValue *v = toml_get(table, key);
    if (v && v->type == TOML_INT) {
        return v->int_val;
    }
    return default_val;
}

bool toml_get_bool(const TomlTable *table, const char *key, bool default_val) {
    const TomlValue *v = toml_get(table, key);
    if (v && v->type == TOML_BOOL) {
        return v->bool_val;
    }
    return default_val;
}

const TomlTable *toml_get_table(const TomlTable *table, const char *key) {
    const TomlValue *v = toml_get(table, key);
    if (v && v->type == TOML_TABLE) {
        return v->table;
    }
    return NULL;
}

const TomlArray *toml_get_array(const TomlTable *table, const char *key) {
    const TomlValue *v = toml_get(table, key);
    if (v && v->type == TOML_ARRAY) {
        return v->array;
    }
    return NULL;
}

static const char *skip_whitespace_and_comments(const char *p) {
    while (*p) {
        if (isspace((unsigned char)*p)) {
            p++;
        } else if (*p == '#') {
            while (*p && *p != '\n') p++;
        } else {
            break;
        }
    }
    return p;
}

static const char *parse_string(const char *p, char **out_str) {
    if (*p != '"') return NULL;
    p++;
    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)malloc(cap);

    while (*p && *p != '"') {
        char c = *p++;
        if (c == '\\') {
            if (!*p) { free(buf); return NULL; }
            char esc = *p++;
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                default: c = esc; break;
            }
        }
        if (len + 2 >= cap) {
            cap *= 2;
            buf = (char *)realloc(buf, cap);
        }
        buf[len++] = c;
    }

    if (*p != '"') {
        free(buf);
        return NULL;
    }
    p++;
    buf[len] = '\0';
    *out_str = buf;
    return p;
}

static const char *parse_value(const char *p, TomlValue *out_val);

static const char *parse_array(const char *p, TomlArray **out_arr) {
    if (*p != '[') return NULL;
    p++;
    TomlArray *arr = toml_array_new();

    for (;;) {
        p = skip_whitespace_and_comments(p);
        if (*p == ']') {
            p++;
            break;
        }
        if (*p == '\0') {
            for (size_t i = 0; i < arr->count; ++i) toml_value_free(&arr->items[i]);
            free(arr->items);
            free(arr);
            return NULL;
        }

        TomlValue elem = {0};
        p = parse_value(p, &elem);
        if (!p) {
            for (size_t i = 0; i < arr->count; ++i) toml_value_free(&arr->items[i]);
            free(arr->items);
            free(arr);
            return NULL;
        }
        toml_array_add(arr, elem);

        p = skip_whitespace_and_comments(p);
        if (*p == ',') {
            p++;
        } else if (*p == ']') {
            p++;
            break;
        } else {
            for (size_t i = 0; i < arr->count; ++i) toml_value_free(&arr->items[i]);
            free(arr->items);
            free(arr);
            return NULL;
        }
    }

    *out_arr = arr;
    return p;
}

static const char *parse_value(const char *p, TomlValue *out_val) {
    p = skip_whitespace_and_comments(p);
    memset(out_val, 0, sizeof(*out_val));

    if (*p == '"') {
        out_val->type = TOML_STRING;
        return parse_string(p, &out_val->str);
    }

    if (*p == '[') {
        out_val->type = TOML_ARRAY;
        return parse_array(p, &out_val->array);
    }

    if (strncmp(p, "true", 4) == 0 && !isalnum((unsigned char)p[4])) {
        out_val->type = TOML_BOOL;
        out_val->bool_val = true;
        return p + 4;
    }

    if (strncmp(p, "false", 5) == 0 && !isalnum((unsigned char)p[5])) {
        out_val->type = TOML_BOOL;
        out_val->bool_val = false;
        return p + 5;
    }

    if (*p == '-' || *p == '+' || isdigit((unsigned char)*p)) {
        char *endptr = NULL;
        long long val = strtoll(p, &endptr, 10);
        if (endptr && endptr != p) {
            out_val->type = TOML_INT;
            out_val->int_val = (int64_t)val;
            return endptr;
        }
    }

    return NULL;
}

static const char *parse_key(const char *p, char **out_key) {
    p = skip_whitespace_and_comments(p);
    const char *start = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) {
        p++;
    }
    if (p == start) return NULL;
    size_t len = (size_t)(p - start);
    char *k = (char *)malloc(len + 1);
    memcpy(k, start, len);
    k[len] = '\0';
    *out_key = k;
    return p;
}

TomlTable *toml_parse(const char *src, char *err_buf, size_t err_size) {
    if (!src) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Null source");
        return NULL;
    }

    TomlTable *root = toml_table_new();
    TomlTable *current = root;

    const char *p = src;
    while (*p) {
        p = skip_whitespace_and_comments(p);
        if (!*p) break;

        if (*p == '[') {
            if (p[1] == '[') {
                p += 2;
                char *table_name = NULL;
                p = parse_key(p, &table_name);
                if (!p || p[0] != ']' || p[1] != ']') {
                    if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Malformed array table header");
                    free(table_name);
                    toml_free(root);
                    return NULL;
                }
                p += 2;

                const TomlValue *existing = toml_get(root, table_name);
                TomlArray *arr = NULL;
                if (!existing) {
                    arr = toml_array_new();
                    TomlValue av = {0};
                    av.type = TOML_ARRAY;
                    av.array = arr;
                    toml_table_add(root, table_name, av);
                } else if (existing->type == TOML_ARRAY) {
                    arr = existing->array;
                } else {
                    if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Key collision on array table: %s", table_name);
                    free(table_name);
                    toml_free(root);
                    return NULL;
                }

                TomlTable *new_entry = toml_table_new();
                TomlValue tv = {0};
                tv.type = TOML_TABLE;
                tv.table = new_entry;
                toml_array_add(arr, tv);
                current = new_entry;
                free(table_name);
                continue;
            } else {
                p++;
                char *table_name = NULL;
                p = parse_key(p, &table_name);
                if (!p || *p != ']') {
                    if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Malformed table header");
                    free(table_name);
                    toml_free(root);
                    return NULL;
                }
                p++;

                const TomlValue *existing = toml_get(root, table_name);
                if (existing && existing->type == TOML_TABLE) {
                    current = existing->table;
                } else {
                    TomlTable *new_sub = toml_table_new();
                    TomlValue tv = {0};
                    tv.type = TOML_TABLE;
                    tv.table = new_sub;
                    toml_table_add(root, table_name, tv);
                    current = new_sub;
                }
                free(table_name);
                continue;
            }
        }

        char *key = NULL;
        p = parse_key(p, &key);
        if (!p) {
            if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Expected key");
            toml_free(root);
            return NULL;
        }

        p = skip_whitespace_and_comments(p);
        if (*p != '=') {
            if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Expected '=' after key '%s'", key);
            free(key);
            toml_free(root);
            return NULL;
        }
        p++;

        TomlValue val = {0};
        p = parse_value(p, &val);
        if (!p) {
            if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to parse value for key '%s'", key);
            free(key);
            toml_free(root);
            return NULL;
        }

        toml_table_add(current, key, val);
        free(key);
    }

    return root;
}

TomlTable *toml_parse_file(const char *path, char *err_buf, size_t err_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to open file: %s", path);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz < 0) {
        fclose(fp);
        return NULL;
    }

    char *buf = (char *)malloc(sz + 1);
    size_t read_bytes = fread(buf, 1, sz, fp);
    fclose(fp);
    buf[read_bytes] = '\0';

    TomlTable *table = toml_parse(buf, err_buf, err_size);
    free(buf);
    return table;
}
