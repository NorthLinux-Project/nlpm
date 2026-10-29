#ifndef CMD_H
#define CMD_H

#include "config/config.h"

int cmd_sync(int argc, char **argv, const NlpmConfig *cfg);
int cmd_install(int argc, char **argv, const NlpmConfig *cfg);
int cmd_remove(int argc, char **argv, const NlpmConfig *cfg);
int cmd_upgrade(int argc, char **argv, const NlpmConfig *cfg);
int cmd_search(int argc, char **argv, const NlpmConfig *cfg);
int cmd_info(int argc, char **argv, const NlpmConfig *cfg);
int cmd_list(int argc, char **argv, const NlpmConfig *cfg);
int cmd_owns(int argc, char **argv, const NlpmConfig *cfg);
int cmd_build(int argc, char **argv, const NlpmConfig *cfg);
int cmd_index(int argc, char **argv, const NlpmConfig *cfg);

#endif
