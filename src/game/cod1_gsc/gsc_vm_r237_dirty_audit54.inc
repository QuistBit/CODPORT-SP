/* Full audit is deliberately independent of the mutation marks. The reference
 * selector is authoritative, even if a notification was missed. */
static void r54_dirty_audit(GscRuntime *r,uint64_t stamp,int bucket,const Task *chosen,int next,GscR237Profile53 *p){
    R54DirtyScope *s=(R54DirtyScope*)r->r54_dirty_scope;
    R54Dirty *d=s?&s->journal:NULL;
    R54QEntry e={0};size_t slot;int n,has=0,deadline=INT32_MAX;
    if(!d)return;
    ++p->dirty_checks;
    if(r->task_count<0||!R54D_Begin(d,stamp,bucket,(size_t)r->task_count))goto skipped;
    if(d->full)++p->dirty_reseeds;
    while(R54D_Next(d,&slot)){
        const Task *t=task_at(r,(int)slot);
        ++p->dirty_feeds;
        if(!R54D_Row(d,slot,t->status==TASK_READY||t->status==TASK_SLEEP,t->r237_enqueued,
          t->r243_time_root_incarnation,t->serial,t->r237_insertion,t->wake_ms))goto skipped;
    }
    if(!R54D_End(d))goto skipped;
    for(n=0;n<r->task_count;n++){
        const Task *t=task_at(r,n);
        ++p->dirty_audit_rows;
        if(!R54D_VerifyRow(d,(size_t)n,t->status==TASK_READY||t->status==TASK_SLEEP,t->r237_enqueued,
          t->r243_time_root_incarnation,t->serial,t->r237_insertion,t->wake_ms))goto mismatch;
    }
    if(!R54D_Choice(d,stamp,bucket,&has,&e,&deadline))goto skipped;
    if(has!=(chosen!=NULL)||(has&&(e.slot>=(size_t)r->task_count||task_at(r,(int)e.slot)!=chosen||
       e.serial!=chosen->serial||e.insertion!=chosen->r237_insertion||e.deadline!=chosen->wake_ms))||
       (!has&&deadline!=next))goto mismatch;
    ++p->dirty_matches;return;
mismatch:
    ++p->dirty_mismatches;R54D_Invalidate(d);return;
skipped:
    ++p->dirty_skipped;R54D_Invalidate(d);
}
