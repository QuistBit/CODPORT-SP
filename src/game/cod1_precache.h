/* R108 private server resource request; paired native game/EXE ABI49.
 * This owns a v14 model header and LOD0 bind skeleton, not a current pose,
 * collision object or a renderer handle. Existing network layouts are intact. */
#ifndef COD1_PRECACHE_H
#define COD1_PRECACHE_H
#include "q_shared.h"
#include <stdint.h>
#define COD1_PRECACHE_VERSION 1u
#define COD1_PRECACHE_MODELS 255
enum {
    COD1_PRECACHE_INVALID = -1,
    COD1_PRECACHE_UNAVAILABLE = -2,
    COD1_PRECACHE_OVERFLOW = -3,
    COD1_PRECACHE_STALE = -4
};
typedef struct {
    unsigned int version, bytes;
    char name[MAX_QPATH];
} cod1ModelPrecache_t;

/* R160 copied reference to an actually loaded map-owned server resource.
 * QUERY with slot0 finds an existing name without loading; positive slot
 * checks that exact bank entry. VALIDATE checks scene/server/slot/name.
 * This is a snapshot, not a pin; every later consumer must revalidate it.
 * No FS/zone/renderer or GAME callback occurs in this read-only syscall.
 * Existing precache request/IDs are unchanged; paired executable/GAME ABI57. */
#define COD1_MODEL_RESOURCE_REFERENCE_VERSION 1u
enum { COD1_MODEL_RESOURCE_QUERY = 1, COD1_MODEL_RESOURCE_VALIDATE = 2 };
typedef struct {
    uint32_t version, bytes, operation, slot;
    uint64_t scene;
    int32_t serverId;
    uint32_t reserved;
    char name[MAX_QPATH];
} cod1ModelResourceReference_t;
typedef char cod1ModelResourceReferenceMustBe96[
    sizeof(cod1ModelResourceReference_t) == 96 ? 1 : -1];

/* Validate a bounded relative name. Slash and backslash remain distinct
 * resource/table identities; only a copied path at the FS boundary normalizes
 * separators. This port refuses traversal and filesystem metacharacters. */
static int Cod1PrecacheNameBytes(const char *name, int model, size_t capacity) {
    size_t at, length, start = 0;
    unsigned char c;
    if (!name || !capacity) return 0;
    for(length=0;length<capacity && name[length];++length) {}
    if (!length || length==capacity) return 0;
    if (model && (Q_stricmpn(name,"xmodel",6) ||
        (name[6] != '/' && name[6] != '\\') || !name[7])) return 0;
    for (at=0;;++at) {
        c=(unsigned char)name[at];
        if (!c || c=='/' || c=='\\') {
            if (at==start || (at-start==1 && name[start]=='.') ||
                (at-start==2 && name[start]=='.' && name[start+1]=='.')) return 0;
            if (!c) return 1;
            start=at+1;
        } else if (c<33 || c>126 || c==':' || c=='*' || c=='?' ||
            c=='"' || c=='<' || c=='>' || c=='|') return 0;
    }
}
static int Cod1PrecacheName(const char *name, int model) {
    return Cod1PrecacheNameBytes(name,model,MAX_QPATH);
}
#endif
