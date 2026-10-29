#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include "util/version.h"

typedef struct {
    char *pkg;
    VersionConstraint constraint;
    bool positive;
} Term;

Term term_create(const char *pkg, const char *constraint_str, bool positive);
Term term_clone(const Term *src);
void term_free(Term *t);
bool term_satisfies(const Term *term, const char *pkg, const char *ver_str);

#endif
