#ifndef COD1_AUTHOR_FOG_H
#define COD1_AUTHOR_FOG_H

#include <stddef.h>

/* A source-derived starting profile, not execution of the mission's main. */
#define COD1_AUTHOR_FOG_MAX_SOURCE 1048576u
#define COD1_AUTHOR_FOG_MAX_CALLS 16u
#define COD1_AUTHOR_FOG_CVAR_CAP 256u

enum { COD1_AUTHOR_FOG_EXP = 1, COD1_AUTHOR_FOG_CULL = 2 };

typedef struct {
    int kind;
    float args[6];
    unsigned line, column;
} Cod1AuthorFogCall;

typedef struct {
    unsigned count;
    Cod1AuthorFogCall calls[COD1_AUTHOR_FOG_MAX_CALLS];
} Cod1AuthorFogPlan;

typedef int (*Cod1AuthorFogReadCvar)(void *user, const char *name,
                                   char *out, size_t out_cap);

/* Preparation has no fog effects. Failure leaves *out completely zero.
 * Each admitted cvar is snapshotted once. Unsupported profile expressions,
 * ambiguous control flow and malformed source are refused atomically. */
int Cod1AuthorFogParse(const char *source, size_t size,
                      Cod1AuthorFogReadCvar read_cvar, void *user,
                      Cod1AuthorFogPlan *out, char *error, size_t error_size);

#endif
