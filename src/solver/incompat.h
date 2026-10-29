#ifndef INCOMPAT_H
#define INCOMPAT_H

#include "term.h"
#include <stddef.h>

typedef enum {
    INCOMPAT_ROOT,
    INCOMPAT_DEPENDENCY,
    INCOMPAT_CONFLICT,
    INCOMPAT_LEARNED
} IncompatKind;

typedef struct {
    Term *terms;
    size_t count;
    IncompatKind kind;
    char *cause;
} Incompatibility;

Incompatibility incompat_create(IncompatKind kind, const char *cause);
void incompat_add_term(Incompatibility *inc, Term t);
Incompatibility incompat_clone(const Incompatibility *src);
void incompat_free(Incompatibility *inc);

#endif
