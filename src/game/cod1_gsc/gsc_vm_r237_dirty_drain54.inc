/* Scratch is scoped to one uninterrupted drain, never retained across phase
 * return or pause. Only opt-in shadow runs enter this stack-heavy wrapper. */
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static int r54_dirty_drain(GscRuntime *r,int *budget,int (*expired)(void),int *next_pending,GscR237Profile53 *p){
    R54DirtyScope s;int ok;
    if(r->r54_dirty_scope)return r237_drain_bucket_core54(r,budget,expired,next_pending,p);
    memset(s.address_map,0,sizeof(s.address_map));
    s.mapped=0;s.lookups=s.probes=s.map_rows=s.misses=0;
    if(!R54D_Init(&s.journal,s.heap,s.positions,s.dirty,s.marked,R54_DIRTY_CAPACITY))
        return r237_drain_bucket_core54(r,budget,expired,next_pending,p);
    r->r54_dirty_scope_bytes=sizeof(s);r->r54_dirty_scope=&s;
    ok=r237_drain_bucket_core54(r,budget,expired,next_pending,p);
    r->r54_dirty_scope=NULL;r->r54_dirty_scope_bytes=0;
    p->dirty_mark_lookups+=s.lookups;p->dirty_mark_probes+=s.probes;
    p->dirty_map_rows+=s.map_rows;p->dirty_mark_misses+=s.misses;
    return ok;
}
static int r237_drain_bucket_slice53(GscRuntime *r,int *budget,int (*expired)(void),int *next_pending,GscR237Profile53 *p){
    if(p&&p->queue_dirty)return r54_dirty_drain(r,budget,expired,next_pending,p);
    return r237_drain_bucket_core54(r,budget,expired,next_pending,p);
}
