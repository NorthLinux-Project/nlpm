#define _POSIX_C_SOURCE 200809L
#include "solver.h"
#include "db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Solver solver_create(sqlite3 *sync_db) {
    Solver s;
    memset(&s, 0, sizeof(s));
    s.sync_db = sync_db;
    return s;
}

void solver_destroy(Solver *s) {
    if (!s) return;
    for (size_t i = 0; i < s->incompat_count; ++i) {
        incompat_free(&s->incompats[i]);
    }
    free(s->incompats);

    for (size_t i = 0; i < s->trail_count; ++i) {
        free(s->trail[i].pkg);
        free(s->trail[i].version);
    }
    free(s->trail);
    memset(s, 0, sizeof(*s));
}

static void solver_add_incompat(Solver *s, Incompatibility inc) {
    s->incompats = (Incompatibility *)realloc(s->incompats, (s->incompat_count + 1) * sizeof(Incompatibility));
    s->incompats[s->incompat_count++] = inc;
}

static const SolverAssignment *find_assignment(const Solver *s, const char *pkg) {
    for (size_t i = 0; i < s->trail_count; ++i) {
        if (strcmp(s->trail[i].pkg, pkg) == 0) {
            return &s->trail[i];
        }
    }
    return NULL;
}

static void solver_assign(Solver *s, const char *pkg, const char *version, bool is_decision) {
    s->trail = (SolverAssignment *)realloc(s->trail, (s->trail_count + 1) * sizeof(SolverAssignment));
    s->trail[s->trail_count].pkg = strdup(pkg);
    s->trail[s->trail_count].version = strdup(version);
    s->trail[s->trail_count].decision_level = s->current_level;
    s->trail[s->trail_count].is_decision = is_decision;
    s->trail_count++;
}

static void solver_backtrack_to(Solver *s, int target_level) {
    while (s->trail_count > 0 && s->trail[s->trail_count - 1].decision_level > target_level) {
        s->trail_count--;
        free(s->trail[s->trail_count].pkg);
        free(s->trail[s->trail_count].version);
    }
    s->current_level = target_level;
}

static bool check_incompatibility(const Solver *s, const Incompatibility *inc, bool *out_conflict, const Term **out_unassigned) {
    *out_conflict = false;
    *out_unassigned = NULL;
    size_t satisfied_count = 0;
    size_t unassigned_count = 0;
    const Term *last_unassigned = NULL;

    for (size_t i = 0; i < inc->count; ++i) {
        const Term *t = &inc->terms[i];
        const SolverAssignment *as = find_assignment(s, t->pkg);
        if (!as) {
            unassigned_count++;
            last_unassigned = t;
        } else {
            if (term_satisfies(t, as->pkg, as->version)) {
                satisfied_count++;
            } else {
                return false;
            }
        }
    }

    if (satisfied_count == inc->count) {
        *out_conflict = true;
        return true;
    }

    if (satisfied_count == inc->count - 1 && unassigned_count == 1) {
        *out_unassigned = last_unassigned;
        return true;
    }

    return false;
}

static bool fetch_package_candidates(sqlite3 *db, const char *pkg, char ***out_versions, int64_t **out_pkg_ids, size_t *out_count) {
    *out_versions = NULL;
    *out_pkg_ids = NULL;
    *out_count = 0;

    const char *sql = "SELECT id, version FROM packages WHERE name = ? ORDER BY id DESC;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, pkg, -1, SQLITE_TRANSIENT);

    size_t cap = 4;
    size_t count = 0;
    char **versions = (char **)malloc(cap * sizeof(char *));
    int64_t *ids = (int64_t *)malloc(cap * sizeof(int64_t));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= cap) {
            cap *= 2;
            versions = (char **)realloc(versions, cap * sizeof(char *));
            ids = (int64_t *)realloc(ids, cap * sizeof(int64_t));
        }
        ids[count] = sqlite3_column_int64(stmt, 0);
        versions[count] = strdup((const char *)sqlite3_column_text(stmt, 1));
        count++;
    }

    sqlite3_finalize(stmt);
    *out_versions = versions;
    *out_pkg_ids = ids;
    *out_count = count;
    return (count > 0);
}

static void add_package_dependencies(Solver *s, const char *pkg_name, const char *version, int64_t pkg_id) {
    DbDepRecord *deps = NULL;
    size_t dep_count = 0;
    if (!db_sync_get_deps(s->sync_db, pkg_id, &deps, &dep_count)) {
        return;
    }

    for (size_t i = 0; i < dep_count; ++i) {
        if (strcmp(deps[i].kind, "depend") == 0) {
            char cause[256];
            snprintf(cause, sizeof(cause), "%s %s depends on %s %s",
                     pkg_name, version, deps[i].name, deps[i].constraint ? deps[i].constraint : "*");
            Incompatibility inc = incompat_create(INCOMPAT_DEPENDENCY, cause);
            incompat_add_term(&inc, term_create(pkg_name, version, true));
            incompat_add_term(&inc, term_create(deps[i].name, deps[i].constraint, false));
            solver_add_incompat(s, inc);
        } else if (strcmp(deps[i].kind, "conflict") == 0) {
            char cause[256];
            snprintf(cause, sizeof(cause), "%s %s conflicts with %s", pkg_name, version, deps[i].name);
            Incompatibility inc = incompat_create(INCOMPAT_CONFLICT, cause);
            incompat_add_term(&inc, term_create(pkg_name, version, true));
            incompat_add_term(&inc, term_create(deps[i].name, "*", true));
            solver_add_incompat(s, inc);
        }
    }

    db_sync_deps_free(deps, dep_count);
}

SolvePlan solver_solve(Solver *s, const char **root_pkgs, const char **root_constraints, size_t n_roots) {
    SolvePlan plan;
    memset(&plan, 0, sizeof(plan));

    s->current_level = 0;
    solver_assign(s, "$root", "1.0.0", false);

    for (size_t i = 0; i < n_roots; ++i) {
        char cause[256];
        snprintf(cause, sizeof(cause), "Root depends on %s %s", root_pkgs[i], root_constraints[i] ? root_constraints[i] : "*");
        Incompatibility inc = incompat_create(INCOMPAT_ROOT, cause);
        incompat_add_term(&inc, term_create("$root", "1.0.0", true));
        incompat_add_term(&inc, term_create(root_pkgs[i], root_constraints[i], false));
        solver_add_incompat(s, inc);
    }

    for (;;) {
        bool changed = true;
        bool conflict_found = false;
        const Incompatibility *conflict_inc = NULL;

        while (changed) {
            changed = false;
            for (size_t i = 0; i < s->incompat_count; ++i) {
                bool conflict = false;
                const Term *unassigned = NULL;
                if (check_incompatibility(s, &s->incompats[i], &conflict, &unassigned)) {
                    if (conflict) {
                        conflict_found = true;
                        conflict_inc = &s->incompats[i];
                        break;
                    }
                    if (unassigned) {
                    }
                }
            }
            if (conflict_found) break;
        }

        if (conflict_found) {
            if (s->current_level == 0) {
                plan.success = false;
                snprintf(plan.error_msg, sizeof(plan.error_msg),
                         "Version conflict detected: %s", conflict_inc->cause ? conflict_inc->cause : "unsolvable constraints");
                return plan;
            }

            solver_backtrack_to(s, s->current_level - 1);
            continue;
        }

        const char *next_pkg = NULL;
        const char *needed_constraint = "*";

        for (size_t i = 0; i < s->incompat_count; ++i) {
            for (size_t j = 0; j < s->incompats[i].count; ++j) {
                const Term *t = &s->incompats[i].terms[j];
                if (!t->positive && strcmp(t->pkg, "$root") != 0) {
                    if (!find_assignment(s, t->pkg)) {
                        next_pkg = t->pkg;
                        break;
                    }
                }
            }
            if (next_pkg) break;
        }

        if (!next_pkg) {
            break;
        }

        char **versions = NULL;
        int64_t *pkg_ids = NULL;
        size_t v_count = 0;
        if (!fetch_package_candidates(s->sync_db, next_pkg, &versions, &pkg_ids, &v_count)) {
            plan.success = false;
            snprintf(plan.error_msg, sizeof(plan.error_msg), "Package '%s' not found in repositories", next_pkg);
            return plan;
        }

        const char *chosen_version = NULL;
        int64_t chosen_id = 0;
        for (size_t i = 0; i < v_count; ++i) {
            if (version_satisfies_str(versions[i], needed_constraint)) {
                chosen_version = versions[i];
                chosen_id = pkg_ids[i];
                break;
            }
        }

        if (!chosen_version) {
            for (size_t i = 0; i < v_count; ++i) free(versions[i]);
            free(versions);
            free(pkg_ids);
            if (s->current_level == 0) {
                plan.success = false;
                snprintf(plan.error_msg, sizeof(plan.error_msg), "No compatible version found for '%s'", next_pkg);
                return plan;
            }
            solver_backtrack_to(s, s->current_level - 1);
            continue;
        }

        s->current_level++;
        solver_assign(s, next_pkg, chosen_version, true);
        add_package_dependencies(s, next_pkg, chosen_version, chosen_id);

        for (size_t i = 0; i < v_count; ++i) free(versions[i]);
        free(versions);
        free(pkg_ids);
    }

    size_t count = 0;
    for (size_t i = 0; i < s->trail_count; ++i) {
        if (strcmp(s->trail[i].pkg, "$root") != 0) {
            count++;
        }
    }

    plan.steps = (InstallStep *)malloc(count * sizeof(InstallStep));
    plan.count = count;
    size_t idx = 0;
    for (size_t i = s->trail_count; i > 0; --i) {
        size_t t_idx = i - 1;
        if (strcmp(s->trail[t_idx].pkg, "$root") != 0) {
            plan.steps[idx].pkg = strdup(s->trail[t_idx].pkg);
            plan.steps[idx].version = strdup(s->trail[t_idx].version);
            idx++;
        }
    }

    plan.success = true;
    return plan;
}

void solve_plan_free(SolvePlan *plan) {
    if (!plan) return;
    for (size_t i = 0; i < plan->count; ++i) {
        free(plan->steps[i].pkg);
        free(plan->steps[i].version);
    }
    free(plan->steps);
    plan->steps = NULL;
    plan->count = 0;
}
