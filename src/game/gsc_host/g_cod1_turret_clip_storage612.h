/* Registration owns these decoded clips until map reset. A sealed copy
 * lets repeat validators check one table of live pointers while retaining
 * exact payload bytes. Allocation or validation failure keeps the old bank. */
#ifndef G_COD1_TURRET_CLIP_STORAGE612_H
#define G_COD1_TURRET_CLIP_STORAGE612_H
#include "../../qcommon/cod1_actor_bone14.h"
#if defined(_WIN32) && !defined(Q3_VM) && !defined(_WINDOWS_)
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualFree(void *,size_t,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void *,size_t,unsigned long,unsigned long *);
#endif
typedef struct {
    c1ar14Root_t *roots[2][21],*gunRoots[2];
    c1ab14Clip_t *bodies[2][21],*gunBodies[2];
} Cod1TurretClipTable;
typedef struct {
    Cod1TurretClipTable clips;
    size_t bytes;
    uint64_t epoch;
} Cod1SealedTurretClips;

static int TurretClipSealedCurrent(const Cod1SealedTurretClips *bank,
                                  const Cod1TurretClipTable *clips,uint64_t epoch) {
    return bank && epoch && bank->epoch==epoch && !memcmp(&bank->clips,clips,sizeof(*clips));
}
static void TurretClipStorageFree(Cod1SealedTurretClips *bank) {
#if defined(_WIN32) && !defined(Q3_VM)
    if(bank)VirtualFree(bank,0,0x8000);
#else
    (void)bank;
#endif
}
#if defined(_WIN32) && !defined(Q3_VM)
static int TurretClipAddSize(size_t *total,size_t bytes) {
    if(bytes>SIZE_MAX-7)return 0;
    bytes=(bytes+7)&~(size_t)7;
    if(bytes>SIZE_MAX-*total)return 0;
    *total+=bytes;return 1;
}
static void *TurretClipTrackAddress(const c1ab14Clip_t *source,c1ab14Clip_t *copy,
                                   const void *track,unsigned count,size_t width,int *ok) {
    size_t offset;
    if(!count) {
        if(track)*ok=0;
        return NULL;
    }
    if(!track||count>65535u||(uintptr_t)track<(uintptr_t)source){*ok=0;return NULL;}
    offset=(uintptr_t)track-(uintptr_t)source;
    if(offset<sizeof(*source)||offset>source->packedBytes||count>(source->packedBytes-offset)/width) {
        *ok=0;return NULL;
    }
    return (unsigned char*)copy+offset;
}
static int TurretClipCopyBody(const c1ab14Clip_t *source,c1ab14Clip_t *copy) {
    unsigned i;int ok=1;
    memcpy(copy,source,source->packedBytes);
    for(i=0;i<C1AB14_BONES;i++){
        const c1ab14Bone_t *a=source->bone+i;c1ab14Bone_t *b=copy->bone+i;
        b->q=TurretClipTrackAddress(source,copy,a->q,a->nq,sizeof(*a->q),&ok);
        b->qi=TurretClipTrackAddress(source,copy,a->qi,a->nq,sizeof(*a->qi),&ok);
        b->t=TurretClipTrackAddress(source,copy,a->t,a->nt,sizeof(*a->t),&ok);
        b->ti=TurretClipTrackAddress(source,copy,a->ti,a->nt,sizeof(*a->ti),&ok);
    }
    return ok;
}
#endif
static Cod1SealedTurretClips *TurretClipStorageCreate(const Cod1TurretClipTable *source,uint64_t epoch) {
#if defined(_WIN32) && !defined(Q3_VM)
    Cod1SealedTurretClips *bank;
    int group,idx;size_t at=0,bytes;unsigned long previous;
    if(!source||!epoch||!TurretClipAddSize(&at,sizeof(*bank)))return NULL;
    bytes=at;
    for(group=0;group<2;group++)for(idx=0;idx<=21;idx++){
        const c1ar14Root_t *root=idx==21?source->gunRoots[group]:source->roots[group][idx];
        const c1ab14Clip_t *body=idx==21?source->gunBodies[group]:source->bodies[group][idx];
        if(!root||root->magic!=C1AR14_MAGIC||root->noteCount||!body||body->magic!=C1AB14_MAGIC||
           body->bones>C1AB14_BONES||body->packedBytes<sizeof(*body)||
           !TurretClipAddSize(&bytes,sizeof(*root))||!TurretClipAddSize(&bytes,body->packedBytes))return NULL;
    }
    bank=VirtualAlloc(NULL,bytes,0x3000,0x04);if(!bank)return NULL;
    bank->epoch=epoch;bank->bytes=bytes;
    for(group=0;group<2;group++)for(idx=0;idx<=21;idx++){
        const c1ar14Root_t *root=idx==21?source->gunRoots[group]:source->roots[group][idx];
        const c1ab14Clip_t *body=idx==21?source->gunBodies[group]:source->bodies[group][idx];
        c1ar14Root_t *r=(c1ar14Root_t*)((unsigned char*)bank+at);
        c1ab14Clip_t *b;
        memcpy(r,root,sizeof(*r));TurretClipAddSize(&at,sizeof(*r));
        b=(c1ab14Clip_t*)((unsigned char*)bank+at);
        if(!TurretClipCopyBody(body,b)){TurretClipStorageFree(bank);return NULL;}
        TurretClipAddSize(&at,body->packedBytes);
        if(idx==21){bank->clips.gunRoots[group]=r;bank->clips.gunBodies[group]=b;}
        else{bank->clips.roots[group][idx]=r;bank->clips.bodies[group][idx]=b;}
    }
    if(at!=bytes||!VirtualProtect(bank,bytes,0x02,&previous)){TurretClipStorageFree(bank);return NULL;}
    return bank;
#else
    (void)source;(void)epoch;return NULL;
#endif
}
#endif
