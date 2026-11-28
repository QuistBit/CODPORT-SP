#ifndef GSC_R237_QUEUE_INDEX54_H
#define GSC_R237_QUEUE_INDEX54_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Standalone candidate, not wired to VM. The caller owns disjoint storage,
 * and supplies validated queue records, with unique insertion identities per
 * root. Slot indices are physical directory positions, not birth order.
 * No callback, allocation or retained Task pointer. Position 0 = absent. */
typedef struct {
    size_t slot;
    uint64_t serial, insertion;
    int deadline;
} R54QEntry;
typedef struct {
    R54QEntry *heap;
    size_t *position;
    size_t count, capacity;
    uint64_t stamp;
} R54Queue;
static inline int R54Q_Init(R54Queue *q,R54QEntry *heap,size_t *position,size_t capacity,uint64_t stamp){
    if(!q||!heap||!position||!capacity||capacity>SIZE_MAX/sizeof(*heap)||capacity>SIZE_MAX/sizeof(*position))return 0;
    q->heap=heap;q->position=position;q->count=0;q->capacity=capacity;q->stamp=stamp;
    memset(position,0,capacity*sizeof(*position));return 1;
}
static inline int R54Q_Before(R54QEntry a,R54QEntry b){
    if(a.deadline!=b.deadline)return a.deadline<b.deadline;
    if(a.insertion!=b.insertion)return a.insertion>b.insertion;
    return a.slot<b.slot; /* diagnostic tie only; unique insertion is required */
}
static inline int R54Q_Equal(R54QEntry a,R54QEntry b){
    return a.slot==b.slot&&a.serial==b.serial&&a.insertion==b.insertion&&a.deadline==b.deadline;
}
static inline void R54Q_Swap(R54Queue *q,size_t a,size_t b){
    R54QEntry tmp=q->heap[a];q->heap[a]=q->heap[b];q->heap[b]=tmp;
    q->position[q->heap[a].slot]=a+1;q->position[q->heap[b].slot]=b+1;
}
static inline void R54Q_Up(R54Queue *q,size_t i){
    while(i){size_t p=(i-1)/2;if(!R54Q_Before(q->heap[i],q->heap[p]))break;R54Q_Swap(q,i,p);i=p;}
}
static inline void R54Q_Down(R54Queue *q,size_t i){
    while(i<q->count/2){size_t a=i*2+1,b=a+1;if(b<q->count&&R54Q_Before(q->heap[b],q->heap[a]))a=b;
        if(!R54Q_Before(q->heap[a],q->heap[i]))break;
        R54Q_Swap(q,i,a);i=a;}
}
/* An existing key is idempotent, never relinked. Refuse a stale slot birth
 * or changed key: the caller must explicitly remove the old record first. */
static inline int R54Q_Put(R54Queue *q,uint64_t stamp,R54QEntry e){
    size_t pos;if(!q||q->stamp!=stamp||e.slot>=q->capacity||!e.serial||!e.insertion||e.deadline<0||e.deadline>0xffffff)return 0;
    pos=q->position[e.slot];if(pos)return R54Q_Equal(q->heap[pos-1],e);
    if(q->count>=q->capacity)return 0;
    pos=q->count++;q->heap[pos]=e;q->position[e.slot]=pos+1;R54Q_Up(q,pos);return 1;
}
/* Exact identity is mandatory: a stale notification cannot remove a reused
 * physical slot. Absence is a refusal, not an implicit successful cancel. */
static inline int R54Q_Remove(R54Queue *q,uint64_t stamp,R54QEntry e){
    size_t pos,last;if(!q||q->stamp!=stamp||e.slot>=q->capacity)return 0;
    pos=q->position[e.slot];if(!pos||!R54Q_Equal(q->heap[pos-1],e))return 0;
    --pos;last=--q->count;q->position[e.slot]=0;
    if(pos!=last){q->heap[pos]=q->heap[last];q->position[q->heap[pos].slot]=pos+1;
        if(pos&&R54Q_Before(q->heap[pos],q->heap[(pos-1)/2]))R54Q_Up(q,pos);else R54Q_Down(q,pos);}
    return 1;
}
/* 1 = record, 0 = empty or wrong root. The caller validates deadline against
 * its current cursor and never drains an exclusive target. */
static inline int R54Q_Peek(const R54Queue *q,uint64_t stamp,R54QEntry *out){
    if(!q||!out||q->stamp!=stamp||!q->count)return 0;
    *out=q->heap[0];return 1;
}
#endif
