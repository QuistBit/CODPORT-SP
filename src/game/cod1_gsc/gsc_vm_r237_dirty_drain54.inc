/* Scratch is scoped to one uninterrupted drain, never retained across phase
 * return or pause. Only opt-in shadow runs enter this stack-heavy wrapper. */
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static int r54_dirty_drain(GscRuntime *r,int *budget,int (*expired)(void),int *next_pending,GscR237Profile53 *p){
    R54QEntry heap[R54_SHADOW_CAPACITY];
    size_t positions[R54_SHADOW_CAPACITY],dirty[R54_SHADOW_CAPACITY];
    unsigned char marked[R54_SHADOW_CAPACITY];R54Dirty d;int ok;
    if(r->r54_dirty_scope||!R54D_Init(&d,heap,positions,dirty,marked,R54_SHADOW_CAPACITY))
        return r237_drain_bucket_core54(r,budget,expired,next_pending,p);
    r->r54_dirty_scope=&d;
    ok=r237_drain_bucket_core54(r,budget,expired,next_pending,p);
    r->r54_dirty_scope=NULL;
    return ok;
}
static int r237_drain_bucket_slice53(GscRuntime *r,int *budget,int (*expired)(void),int *next_pending,GscR237Profile53 *p){
    if(p&&p->queue_dirty)return r54_dirty_drain(r,budget,expired,next_pending,p);
    return r237_drain_bucket_core54(r,budget,expired,next_pending,p);
}
