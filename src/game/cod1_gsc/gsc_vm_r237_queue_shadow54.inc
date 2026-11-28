/* Optional diagnostic only. At a successful reference scan boundary, rebuild
 * a bounded shadow from committed rows. No VM writes, allocation or retained
 * Task pointers. Keep the extra stack off the ordinary path. */
#define R54_SHADOW_CAPACITY 1024u
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static void r237_shadow_compare54(const GscRuntime *r,uint64_t stamp,int bucket,
                                 const Task *chosen,int next,GscR237Profile53 *p){
    R54QEntry heap[R54_SHADOW_CAPACITY],entry={0};
    size_t positions[R54_SHADOW_CAPACITY];R54Sync shadow;
    int n,has=0,shadow_next=INT32_MAX;
    ++p->shadow_checks;
    if(r->task_count<0||(unsigned)r->task_count>R54_SHADOW_CAPACITY){++p->shadow_skipped;return;}
    if(!R54S_Init(&shadow,heap,positions,R54_SHADOW_CAPACITY)||
       !R54S_Begin(&shadow,stamp,bucket,(size_t)r->task_count))goto skipped;
    for(n=0;n<r->task_count;n++){
        const Task *t=task_at(r,n);
        ++p->shadow_rows;
        if(!R54S_Row(&shadow,(size_t)n,t->status==TASK_READY||t->status==TASK_SLEEP,
            t->r237_enqueued,t->r243_time_root_incarnation,t->serial,t->r237_insertion,t->wake_ms))goto skipped;
    }
    if(!R54S_End(&shadow)||!R54S_Choice(&shadow,stamp,bucket,&has,&entry,&shadow_next))goto skipped;
    if(has!=(chosen!=NULL)||(has&&(entry.slot>=(size_t)r->task_count||task_at(r,(int)entry.slot)!=chosen||
       entry.serial!=chosen->serial||entry.insertion!=chosen->r237_insertion||entry.deadline!=chosen->wake_ms))||
       (!has&&shadow_next!=next))++p->shadow_mismatches;
    else ++p->shadow_matches;
    return;
skipped:
    ++p->shadow_skipped;
}
