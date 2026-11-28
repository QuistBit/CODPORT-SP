#ifndef GSC_R237_QUEUE_DIRTY54_H
#define GSC_R237_QUEUE_DIRTY54_H
#include "gsc_r237_queue_index54.h"
/* Incremental candidate. Caller owns disjoint backing arrays and reports
 * every slot mutation before the next committed selection boundary. No
 * allocation or VM pointers. Missing mutation notifications are NOT proven
 * impossible here: compare all rows against the reference before authority. */
enum { R54D_INVALID, R54D_READY, R54D_BUILDING };
typedef struct {
    R54Queue queue;
    size_t *dirty;
    unsigned char *marked;
    size_t slots, target, count, fed, changes;
    int cursor, state, full;
} R54Dirty;
static inline void R54D_Invalidate(R54Dirty *d){if(d)d->state=R54D_INVALID;}
static inline int R54D_Init(R54Dirty *d,R54QEntry *heap,size_t *position,size_t *dirty,unsigned char *marked,size_t capacity){
    if(!d)return 0;
    memset(d,0,sizeof(*d));
    if(!dirty||!marked||capacity>SIZE_MAX/sizeof(*dirty)||!R54Q_Init(&d->queue,heap,position,capacity,0))return 0;
    d->dirty=dirty;d->marked=marked;memset(marked,0,capacity);return 1;
}
static inline int R54D_Append(R54Dirty *d,size_t slot){
    if(slot>=d->queue.capacity)return 0;
    if(!d->marked[slot]){
        if(d->count>=d->queue.capacity)return 0;
        d->marked[slot]=1;d->dirty[d->count++]=slot;
    }
    return 1;
}
/* Mark at mutation, even when the row is temporarily inconsistent. Never
 * read task contents here. Repeated notifications coalesce by physical slot. */
static inline int R54D_Mark(R54Dirty *d,size_t slot){
    if(!d||d->state!=R54D_READY||!R54D_Append(d,slot)){R54D_Invalidate(d);return 0;}
    return 1;
}
/* Root change, shrink or explicit invalidation forces a complete reseed.
 * Growth adds only newly allocated physical slots. A dirty slot beyond the
 * committed directory is an incomplete transaction, not an absent task. */
static inline int R54D_Begin(R54Dirty *d,uint64_t stamp,int cursor,size_t slots){
    size_t i;int reset;
    if(!d)return 0;
    reset=d->state!=R54D_READY||d->queue.stamp!=stamp||slots<d->slots||cursor<d->cursor;
    R54D_Invalidate(d);
    if(!d->dirty||!d->marked||!d->queue.heap||!d->queue.position||!d->queue.capacity||slots>d->queue.capacity||cursor<0||cursor>0xffffff)return 0;
    if(reset){
        if(!R54Q_Init(&d->queue,d->queue.heap,d->queue.position,d->queue.capacity,stamp))return 0;
        memset(d->marked,0,d->queue.capacity);d->count=0;
        for(i=0;i<slots;i++)if(!R54D_Append(d,i))return 0;
    }else{
        for(i=0;i<d->count;i++)if(d->dirty[i]>=slots)return 0;
        for(i=d->slots;i<slots;i++)if(!R54D_Append(d,i))return 0;
    }
    d->target=slots;d->cursor=cursor;d->fed=0;d->changes=0;d->full=reset;d->state=R54D_BUILDING;return 1;
}
static inline int R54D_Next(const R54Dirty *d,size_t *slot){
    if(!d||!slot||d->state!=R54D_BUILDING||d->fed>=d->count)return 0;
    *slot=d->dirty[d->fed];return 1;
}
static inline int R54D_Row(R54Dirty *d,size_t slot,int pending,int enqueued,uint64_t stamp,uint64_t serial,uint64_t insertion,int deadline){
    R54QEntry row,old;size_t pos;
    if(!d||d->state!=R54D_BUILDING||d->fed>=d->count||slot!=d->dirty[d->fed]||slot>=d->target)goto fail;
    if(pending&&(!enqueued||stamp!=d->queue.stamp||!serial||!insertion||deadline<d->cursor||deadline>0xffffff))goto fail;
    row.slot=slot;row.serial=serial;row.insertion=insertion;row.deadline=deadline;
    pos=d->queue.position[slot];
    if(pos){
        if(pos>d->queue.count)goto fail;
        old=d->queue.heap[pos-1];if(old.slot!=slot)goto fail;
        if(pending&&R54Q_Equal(old,row)){++d->fed;return 1;}
        if(!R54Q_Remove(&d->queue,d->queue.stamp,old))goto fail;
        ++d->changes;
    }
    if(pending){if(!R54Q_Put(&d->queue,d->queue.stamp,row))goto fail;++d->changes;}
    ++d->fed;return 1;
fail:
    R54D_Invalidate(d);return 0;
}
static inline int R54D_End(R54Dirty *d){
    R54QEntry head;size_t i;
    if(!d||d->state!=R54D_BUILDING||d->fed!=d->count||(R54Q_Peek(&d->queue,d->queue.stamp,&head)&&head.deadline<d->cursor)){R54D_Invalidate(d);return 0;}
    for(i=0;i<d->count;i++)d->marked[d->dirty[i]]=0;
    d->slots=d->target;d->count=0;d->state=R54D_READY;return 1;
}
static inline int R54D_Choice(const R54Dirty *d,uint64_t stamp,int cursor,int *has,R54QEntry *current,int *next){
    R54QEntry head;
    if(!d||d->state!=R54D_READY||d->count||d->queue.stamp!=stamp||d->cursor!=cursor||!has||!current||!next)return 0;
    if(!R54Q_Peek(&d->queue,stamp,&head)){*has=0;*next=INT32_MAX;return 1;}
    if(head.deadline<cursor)return 0;
    *has=head.deadline==cursor;if(*has)*current=head;else *next=head.deadline;return 1;
}
/* Full shadow audit, including non-head records. An omitted notification
 * might not affect the current selection but must prevent future authority. */
static inline int R54D_VerifyRow(const R54Dirty *d,size_t slot,int pending,int enqueued,uint64_t stamp,uint64_t serial,uint64_t insertion,int deadline){
    R54QEntry e;size_t pos;
    if(!d||d->state!=R54D_READY||d->count||slot>=d->slots)return 0;
    pos=d->queue.position[slot];if(!pending)return !pos;
    if(!pos||pos>d->queue.count||!enqueued||stamp!=d->queue.stamp||!serial||!insertion||deadline<d->cursor||deadline>0xffffff)return 0;
    e.slot=slot;e.serial=serial;e.insertion=insertion;e.deadline=deadline;
    return R54Q_Equal(d->queue.heap[pos-1],e);
}
#endif
