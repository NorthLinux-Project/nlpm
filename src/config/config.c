#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "toml.h"
#include <stdlib.h>
#include <string.h>

static void set_defaults(NlpmConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->rootdir = strdup(DEFAULT_ROOT_DIR);
    cfg->cachedir = strdup(DEFAULT_CACHE_DIR);
    cfg->dbdir = strdup(DEFAULT_DB_DIR);
    cfg->parallel = DEFAULT_PARALLEL;
    cfg->repos = NULL;
    cfg->repo_count = 0;
}

static bool parse_config_table(TomlTable *root, NlpmConfig *out_cfg) {
    set_defaults(out_cfg);

    const TomlTable *general = toml_get_table(root, "general");
    if (general) {
        const char *rootdir = toml_get_string(general, "rootdir", NULL);
        if (rootdir) {
            free(out_cfg->rootdir);
            out_cfg->rootdir = strdup(rootdir);
        }

        const char *cachedir = toml_get_string(general, "cachedir", NULL);
        if (cachedir) {
            free(out_cfg->cachedir);
            out_cfg->cachedir = strdup(cachedir);
        }

        const char *dbdir = toml_get_string(general, "dbdir", NULL);
        if (dbdir) {
            free(out_cfg->dbdir);
            out_cfg->dbdir = strdup(dbdir);
        }

        out_cfg->parallel = (int)toml_get_int(general, "parallel", DEFAULT_PARALLEL);
        if (out_cfg->parallel <= 0) {
            out_cfg->parallel = 1;
        }
    }

    const TomlArray *repo_arr = toml_get_array(root, "repo");
    if (repo_arr) {
        out_cfg->repos = (NlpmRepoConfig *)calloc(repo_arr->count, sizeof(NlpmRepoConfig));
        out_cfg->repo_count = repo_arr->count;

        for (size_t i = 0; i < repo_arr->count; ++i) {
            if (repo_arr->items[i].type == TOML_TABLE && repo_arr->items[i].table) {
                const TomlTable *rt = repo_arr->items[i].table;
                const char *name = toml_get_string(rt, "name", "");
                const char *url = toml_get_string(rt, "url", "");
                bool enabled = toml_get_bool(rt, "enabled", true);

                out_cfg->repos[i].name = strdup(name);
                out_cfg->repos[i].url = strdup(url);
                out_cfg->repos[i].enabled = enabled;
            }
        }
    }

    return true;
}

bool config_load(const char *path, NlpmConfig *out_cfg) {
    if (!path) path = DEFAULT_CONFIG_PATH;

    char errbuf[256];
    TomlTable *root = toml_parse_file(path, errbuf, sizeof(errbuf));
    if (!root) {
        set_defaults(out_cfg);
        return false;
    }

    bool ok = parse_config_table(root, out_cfg);
    toml_free(root);
    return ok;
}

bool config_load_string(const char *content, NlpmConfig *out_cfg) {
    if (!content) return false;

    char errbuf[256];
    TomlTable *root = toml_parse(content, errbuf, sizeof(errbuf));
    if (!root) {
        set_defaults(out_cfg);
        return false;
    }

    bool ok = parse_config_table(root, out_cfg);
    toml_free(root);
    return ok;
}

void config_free(NlpmConfig *cfg) {
    if (!cfg) return;

    free(cfg->rootdir);
    free(cfg->cachedir);
    free(cfg->dbdir);

    if (cfg->repos) {
        for (size_t i = 0; i < cfg->repo_count; ++i) {
            free(cfg->repos[i].name);
            free(cfg->repos[i].url);
        }
        free(cfg->repos);
    }

    memset(cfg, 0, sizeof(*cfg));
}
