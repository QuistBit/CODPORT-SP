/* Scoped, non-authoritative observer. Address index borrows no Task fields;
 * directory slots are stable until Runtime destruction. No pointer survives
 * the enclosing drain. The original scheduler remains authoritative. */
#include "gsc_r237_queue_dirty54.h"
#define R54_DIRTY_CAPACITY 1024u
#define R54_DIRTY_MAP_CAPACITY 2048u
typedef struct {
    R54Dirty journal;
    R54QEntry heap[R54_DIRTY_CAPACITY];
    size_t positions[R54_DIRTY_CAPACITY],dirty[R54_DIRTY_CAPACITY];
    unsigned char marked[R54_DIRTY_CAPACITY];
    uint16_t address_map[R54_DIRTY_MAP_CAPACITY]; /* slot+1; zero is empty */
    size_t mapped;
    uint64_t lookups,probes,map_rows,misses;
} R54DirtyScope;
static size_t r54_dirty_hash(const Task *task){
    uintptr_t v=(uintptr_t)task;
    return (size_t)((v>>4)^(v>>13)^(v>>23))&(R54_DIRTY_MAP_CAPACITY-1u);
}
static int r54_dirty_map_grow(R54DirtyScope *s,const GscRuntime *r){
    size_t slot,i,h;const Task *task;
    if(r->task_count<0||(unsigned)r->task_count>R54_DIRTY_CAPACITY)return 0;
    if(s->mapped>(size_t)r->task_count){
        memset(s->address_map,0,sizeof(s->address_map));s->mapped=0;
        R54D_Invalidate(&s->journal);return 0;
    }
    for(slot=s->mapped;slot<(size_t)r->task_count;slot++){
        task=task_at(r,(int)slot);if(!task)return 0;
        h=r54_dirty_hash(task);
        for(i=0;i<R54_DIRTY_MAP_CAPACITY;i++,h=(h+1)&(R54_DIRTY_MAP_CAPACITY-1u)){
            unsigned value=s->address_map[h];
            if(!value){s->address_map[h]=(uint16_t)(slot+1);break;}
            if(value>s->mapped||task_at(r,(int)value-1)==task)return 0;
        }
        if(i==R54_DIRTY_MAP_CAPACITY)return 0;
        ++s->mapped;++s->map_rows;
    }
    return 1;
}
static void r54_dirty_mark_task(GscRuntime *r,const Task *task){
    R54DirtyScope *s=(R54DirtyScope*)r->r54_dirty_scope;size_t h,i;
    if(!s||s->journal.state!=R54D_READY)return;
    ++s->lookups;
    if(!task||!r54_dirty_map_grow(s,r))goto missing;
    h=r54_dirty_hash(task);
    for(i=0;i<R54_DIRTY_MAP_CAPACITY;i++,h=(h+1)&(R54_DIRTY_MAP_CAPACITY-1u)){
        unsigned value=s->address_map[h];++s->probes;
        if(!value||value>s->mapped)break;
        if(task_at(r,(int)value-1)==task){(void)R54D_Mark(&s->journal,(size_t)value-1);return;}
    }
missing:
    ++s->misses;memset(s->address_map,0,sizeof(s->address_map));s->mapped=0;
    R54D_Invalidate(&s->journal);
}
