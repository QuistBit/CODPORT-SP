/* Storage for native model submissions queued for the render backend.
 * One arena belongs to each SMP frame. Reset it only after that frame's
 * backend work has finished, never between scenes within a frame.
 */
#ifndef COD1_NATIVE_SNAPSHOT_ARENA_H
#define COD1_NATIVE_SNAPSHOT_ARENA_H

#include <stddef.h>

typedef struct {
    unsigned char *data;
    size_t capacity;
    size_t used;
} cnsArena_t;

static void CNS_Reset(cnsArena_t *arena) {
    if(arena)arena->used=0;
}

/* The whole snapshot (header, pose and vertices) is reserved at once, so an
 * out-of-space result cannot leave a partially queued model behind. */
static void *CNS_Alloc(cnsArena_t *arena,size_t bytes,size_t alignment) {
    size_t base,at,pad;
    if(!arena || !arena->data || !bytes || !alignment ||
       (alignment & (alignment-1)) || arena->used>arena->capacity)return NULL;
    base=(size_t)arena->data;
    if(base>(size_t)-1-arena->used)return NULL;
    at=base+arena->used;
    pad=(alignment-(at & (alignment-1))) & (alignment-1);
    if(pad>arena->capacity-arena->used ||
       bytes>arena->capacity-arena->used-pad)return NULL;
    arena->used+=pad+bytes;
    return arena->data+(at+pad-base);
}

#endif
