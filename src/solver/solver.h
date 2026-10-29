#ifndef SOLVER_H
#define SOLVER_H

#include <stddef.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "incompat.h"

typedef struct {
    char *pkg;
    char *version;
    int decision_level;
    bool is_decision;
} SolverAssignment;

typedef struct {
    char *pkg;
    char *version;
} InstallStep;

typedef struct {
    InstallStep *steps;
    size_t count;
    bool success;
    char error_msg[512];
} SolvePlan;

typedef struct {
    sqlite3 *sync_db;
    Incompatibility *incompats;
    size_t incompat_count;
    SolverAssignment *trail;
    size_t trail_count;
    int current_level;
} Solver;

Solver solver_create(sqlite3 *sync_db);
void solver_destroy(Solver *s);
SolvePlan solver_solve(Solver *s, const char **root_pkgs, const char **root_constraints, size_t n_roots);
void solve_plan_free(SolvePlan *plan);

#endif
