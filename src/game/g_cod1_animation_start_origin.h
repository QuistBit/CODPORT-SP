#ifndef G_COD1_ANIMATION_START_ORIGIN_H
#define G_COD1_ANIMATION_START_ORIGIN_H
#include <stddef.h>
#include <stdint.h>

/* Private R208 resource dependency. Caller supplies genuine name->VFS-winning
 * ATR/XAnim byte authority. Selection copies derived state and retains no
 * source pointers. No FS loader, retail tree IDs, VM/Session, actor movement,
 * item or mission capability is installed. One bank per live map; reset on
 * MapBegin/End. Failed selection may be retried; a live bank cannot be replaced.
 * Names and byte ranges remain readable for the entire synchronous call. */
#define COD1_START_ORIGIN_MAX 256u
typedef struct {
    const char *tree;
    const void *atr_bytes;
    size_t atr_size;
    const char *name;
    const void *xanim_bytes;
    size_t xanim_size;
} cod1StartOriginResourceInput_t;

typedef struct {
    uint64_t epoch, receipt;
    uint32_t row, ordinal, flags;
    char tree[64], name[64];
} cod1StartOriginRef_t;

int G_Cod1SelectStartOriginResources(const cod1StartOriginResourceInput_t *,
                                   size_t, char *, size_t);
int G_Cod1StartOriginResourceRef(const char *, const char *,
                               cod1StartOriginRef_t *, char *, size_t);
/* Exact selected PC53/nearest transform; finite angles bounded to3600 degrees.
 * current/context are borrowed for this call only and must certify its actual
 * caller scope. They do not become an implicit Session/task certificate.
 * Reentry cancels the outer operation. Refusal leaves out untouched; error
 * writes are permitted only after all diagnostic/output aliases are admitted.
 * Source buffers may be released after selection. No callback follows output
 * commit. This private function may be stripped when no gameplay caller exists. */
int G_Cod1GetStartOrigin(const cod1StartOriginRef_t *, const float [3],
                        const float [3], float [3], char *, size_t,
                        int (*)(void *), void *);
#endif
