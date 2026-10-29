#define _POSIX_C_SOURCE 200809L
#include "incompat.h"
#include <stdlib.h>
#include <string.h>

Incompatibility incompat_create(IncompatKind kind, const char *cause) {
    Incompatibility inc;
    inc.terms = NULL;
    inc.count = 0;
    inc.kind = kind;
    inc.cause = cause ? strdup(cause) : NULL;
    return inc;
}

void incompat_add_term(Incompatibility *inc, Term t) {
    inc->terms = (Term *)realloc(inc->terms, (inc->count + 1) * sizeof(Term));
    inc->terms[inc->count++] = t;
}

Incompatibility incompat_clone(const Incompatibility *src) {
    Incompatibility inc;
    inc.kind = src->kind;
    inc.cause = src->cause ? strdup(src->cause) : NULL;
    inc.count = src->count;
    inc.terms = (Term *)malloc(src->count * sizeof(Term));
    for (size_t i = 0; i < src->count; ++i) {
        inc.terms[i] = term_clone(&src->terms[i]);
    }
    return inc;
}

void incompat_free(Incompatibility *inc) {
    if (!inc) return;
    for (size_t i = 0; i < inc->count; ++i) {
        term_free(&inc->terms[i]);
    }
    free(inc->terms);
    free(inc->cause);
    inc->terms = NULL;
    inc->count = 0;
    inc->cause = NULL;
}
