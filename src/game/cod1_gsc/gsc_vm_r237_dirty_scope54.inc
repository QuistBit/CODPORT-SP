/* Scoped, non-authoritative observer. Never retain a Task or scratch pointer
 * after the enclosing drain. Marking uses pointer membership, not Task fields. */
#include "gsc_r237_queue_dirty54.h"
static void r54_dirty_mark_task(GscRuntime *r,const Task *task){
    R54Dirty *d=(R54Dirty*)r->r54_dirty_scope;int n;
    if(!d)return;
    if(d->state!=R54D_READY)return; /* next successful scan will reseed */
    for(n=0;n<r->task_count;n++)if(task_at(r,n)==task){(void)R54D_Mark(d,(size_t)n);return;}
    R54D_Invalidate(d);
}
