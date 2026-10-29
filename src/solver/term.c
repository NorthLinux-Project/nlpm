#define _POSIX_C_SOURCE 200809L
#include "term.h"
#include <stdlib.h>
#include <string.h>

Term term_create(const char *pkg, const char *constraint_str, bool positive) {
    Term t;
    t.pkg = strdup(pkg ? pkg : "");
    t.positive = positive;
    version_parse_constraint(constraint_str ? constraint_str : "*", &t.constraint);
    return t;
}

Term term_clone(const Term *src) {
    Term t;
    t.pkg = strdup(src->pkg);
    t.positive = src->positive;
    t.constraint.op = src->constraint.op;
    memset(&t.constraint.version, 0, sizeof(t.constraint.version));
    t.constraint.version.epoch = src->constraint.version.epoch;
    t.constraint.version.major = src->constraint.version.major;
    t.constraint.version.minor = src->constraint.version.minor;
    t.constraint.version.patch = src->constraint.version.patch;
    t.constraint.version.prerelease = src->constraint.version.prerelease ? strdup(src->constraint.version.prerelease) : NULL;
    return t;
}

void term_free(Term *t) {
    if (!t) return;
    free(t->pkg);
    version_constraint_free(&t->constraint);
    t->pkg = NULL;
}

bool term_satisfies(const Term *term, const char *pkg, const char *ver_str) {
    if (!term || !pkg || !ver_str) return false;
    if (strcmp(term->pkg, pkg) != 0) return false;

    bool sat = version_satisfies_str(ver_str, NULL);
    Version ver = {0};
    if (version_parse(ver_str, &ver)) {
        sat = version_satisfies(&ver, &term->constraint);
        version_free(&ver);
    } else {
        sat = false;
    }

    return term->positive ? sat : !sat;
}
