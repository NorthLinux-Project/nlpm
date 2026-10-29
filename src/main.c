#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config/config.h"
#include "cmd/cmd.h"

#define NLPM_VERSION "0.1.0"

static void print_usage(const char *prog) {
    printf("nlpm - North Linux Package Manager %s\n\n", NLPM_VERSION);
    printf("Usage: %s [options] <command> [arguments...]\n\n", prog);
    printf("Commands:\n");
    printf("  sync             Synchronize repository databases\n");
    printf("  install <pkg..>  Install one or more packages\n");
    printf("  remove  <pkg..>  Remove one or more packages\n");
    printf("  upgrade [pkg..]  Upgrade installed packages\n");
    printf("  search  <query>  Search for packages matching query\n");
    printf("  info    <pkg>    Show detailed information about package\n");
    printf("  list             List installed packages\n");
    printf("  owns    <path>   Find package that owns a file\n");
    printf("  build   <dir>    Build a .nlpkg package from directory\n");
    printf("  index   <dir>    Generate .nldb database from directory\n\n");
    printf("Options:\n");
    printf("  -c <config_path> Use alternate configuration file\n");
    printf("  -r <root_dir>    Set an alternate installation root\n");
    printf("  -h, --help       Show this help message\n");
    printf("  -v, --version    Show version information\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *config_path = NULL;
    const char *root_override = NULL;
    int cmd_arg_index = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("nlpm %s (North Linux)\n", NLPM_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            root_override = argv[++i];
            continue;
        }
        if (argv[i][0] != '-') {
            cmd_arg_index = i;
            break;
        }
    }

    if (cmd_arg_index == -1) {
        print_usage(argv[0]);
        return 1;
    }

    NlpmConfig cfg;
    config_load(config_path, &cfg);

    if (root_override) {
        free(cfg.rootdir);
        cfg.rootdir = strdup(root_override);
    }

    const char *command = argv[cmd_arg_index];
    int sub_argc = argc - cmd_arg_index;
    char **sub_argv = &argv[cmd_arg_index];

    bool requires_root = (strcmp(command, "sync") == 0 ||
                          strcmp(command, "install") == 0 ||
                          strcmp(command, "remove") == 0 ||
                          strcmp(command, "upgrade") == 0);

    if (requires_root && strcmp(cfg.rootdir, "/") == 0 && geteuid() != 0) {
        fprintf(stderr, "Error: '%s' requires root privileges when modifying system root '/'.\n", command);
        fprintf(stderr, "Hint: Run with sudo or specify an alternate root using -r <dir>.\n");
        config_free(&cfg);
        return 1;
    }

    int rc = 1;
    if (strcmp(command, "sync") == 0) {
        rc = cmd_sync(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "install") == 0) {
        rc = cmd_install(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "remove") == 0) {
        rc = cmd_remove(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "upgrade") == 0) {
        rc = cmd_upgrade(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "search") == 0) {
        rc = cmd_search(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "info") == 0) {
        rc = cmd_info(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "list") == 0) {
        rc = cmd_list(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "owns") == 0) {
        rc = cmd_owns(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "build") == 0) {
        rc = cmd_build(sub_argc, sub_argv, &cfg);
    } else if (strcmp(command, "index") == 0) {
        rc = cmd_index(sub_argc, sub_argv, &cfg);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'. Run '%s --help' for usage.\n", command, argv[0]);
        rc = 1;
    }

    config_free(&cfg);
    return rc;
}
