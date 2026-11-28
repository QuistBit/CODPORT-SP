/* Experimental handoff only after the independent full audit. This does not
 * eliminate reference scans. A failed observer must not change the result. */
static int r54_verified_choice(GscRuntime *r,uint64_t stamp,int bucket,Task **chosen,int *next){
    R54DirtyScope *s=(R54DirtyScope*)r->r54_dirty_scope;
    R54QEntry e={0};Task *candidate=NULL;int has=0,deadline=INT32_MAX;
    if(!s||!R54D_Choice(&s->journal,stamp,bucket,&has,&e,&deadline))return 0;
    if(has){
        if(e.slot>=(size_t)r->task_count)return 0;
        candidate=task_at(r,(int)e.slot);
        if(!candidate||candidate!=*chosen||candidate->serial!=e.serial||candidate->r237_insertion!=e.insertion||candidate->wake_ms!=e.deadline)return 0;
    }else if(*chosen||deadline!=*next)return 0;
    *chosen=candidate;if(!has)*next=deadline;return 1;
}
