#ifndef GSC_R237_QUEUE_SYNC54_H
#define GSC_R237_QUEUE_SYNC54_H
#include "gsc_r237_queue_index54.h"
/* Shadow-only reconciliation. Feed every physical slot exactly once, in
 * directory order, at the existing callback-free scheduler selection boundary.
 * No Task pointers, VM writes or allocations. Never query a partial snapshot.
 * All backing arrays must be disjoint, valid, and owned by the caller. */
typedef struct {
    R54Queue queue;
    size_t slots, fed, changes;
    int cursor, building, ready;
} R54Sync;
static inline void R54S_Invalidate(R54Sync *s){if(s){s->ready=0;s->building=0;}}
static inline int R54S_Init(R54Sync *s,R54QEntry *heap,size_t *position,size_t capacity){
    if(!s)return 0;
    memset(s,0,sizeof(*s));return R54Q_Init(&s->queue,heap,position,capacity,0);
}
static inline int R54S_Begin(R54Sync *s,uint64_t stamp,int cursor,size_t slots){
    int reset;
    if(!s)return 0;
    reset=!s->ready||s->queue.stamp!=stamp||slots<s->slots;
    R54S_Invalidate(s);
    if(!s->queue.heap||!s->queue.position||!s->queue.capacity||slots>s->queue.capacity||cursor<0||cursor>0xffffff)return 0;
    if(reset&&!R54Q_Init(&s->queue,s->queue.heap,s->queue.position,s->queue.capacity,stamp))return 0;
    s->slots=slots;s->fed=0;s->changes=0;s->cursor=cursor;s->building=1;return 1;
}
/* pending is exactly READY or SLEEP at this boundary, not the enqueued bit.
 * DEAD/EVENT rows may retain stale queue fields; the original scan ignores them.
 * Changed serial/key replaces the old shadow record, not the live Task. */
static inline int R54S_Row(R54Sync *s,size_t slot,int pending,int enqueued,uint64_t stamp,uint64_t serial,uint64_t insertion,int deadline){
    R54QEntry row,old;size_t pos;
    if(!s||!s->building||slot!=s->fed||slot>=s->slots)goto fail;
    if(pending&&(!enqueued||stamp!=s->queue.stamp||!serial||!insertion||deadline<s->cursor||deadline>0xffffff))goto fail;
    row.slot=slot;row.serial=serial;row.insertion=insertion;row.deadline=deadline;
    pos=s->queue.position[slot];
    if(pos){
        if(pos>s->queue.count)goto fail;
        old=s->queue.heap[pos-1];if(old.slot!=slot)goto fail;
        if(pending&&R54Q_Equal(old,row)){++s->fed;return 1;}
        if(!R54Q_Remove(&s->queue,s->queue.stamp,old))goto fail;
        ++s->changes;
    }
    if(pending){if(!R54Q_Put(&s->queue,s->queue.stamp,row))goto fail;++s->changes;}
    ++s->fed;return 1;
fail:
    R54S_Invalidate(s);return 0;
}
static inline int R54S_End(R54Sync *s){
    if(!s||!s->building||s->fed!=s->slots){R54S_Invalidate(s);return 0;}
    s->building=0;s->ready=1;return 1;
}
/* Only compare against the original selector. When a current task exists,
 * the original next_pending result is not published/used. When none exists,
 * next is the earliest future deadline, or INT32_MAX. Never drain target here.
 * Unique insertion IDs are a caller invariant, not proven by this shadow. */
static inline int R54S_Choice(const R54Sync *s,uint64_t stamp,int cursor,int *has_current,R54QEntry *current,int *next){
    R54QEntry head;
    if(!s||!s->ready||s->queue.stamp!=stamp||s->cursor!=cursor||!has_current||!current||!next)return 0;
    if(!R54Q_Peek(&s->queue,stamp,&head)){*has_current=0;*next=INT32_MAX;return 1;}
    if(head.deadline<cursor)return 0;
    *has_current=head.deadline==cursor;
    if(*has_current)*current=head;else *next=head.deadline;
    return 1;
}
#endif
