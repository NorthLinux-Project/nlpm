#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdbool.h>

#define DEFAULT_CONFIG_PATH "/etc/nlpm/nlpm.toml"
#define DEFAULT_ROOT_DIR    "/"
#define DEFAULT_CACHE_DIR   "/var/cache/nlpm"
#define DEFAULT_DB_DIR      "/var/lib/nlpm"
#define DEFAULT_PARALLEL    4

typedef struct {
    char *name;
    char *url;
    bool enabled;
} NlpmRepoConfig;

typedef struct {
    char *rootdir;
    char *cachedir;
    char *dbdir;
    int parallel;
    NlpmRepoConfig *repos;
    size_t repo_count;
} NlpmConfig;

bool config_load(const char *path, NlpmConfig *out_cfg);
bool config_load_string(const char *content, NlpmConfig *out_cfg);
void config_free(NlpmConfig *cfg);

#endif
