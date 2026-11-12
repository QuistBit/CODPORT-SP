/* Optional R212 typed facade. The caller owns Runtime/namespace/task identity;
 * its composed current fence must prove that scope at the real math boundaries.
 * This host authenticates the exact two owned GAME rows, never native tree IDs. */
static int StartOriginNamespaceTypedQuery(void *user,const GscStartOriginRequest *input,
    GscStartOriginReply *out,char *error,size_t size,
    int (*current)(void *),void *context) {
    Cod1StartOriginTypedLease lease;
    cod1StartOriginRef_t ref;
    GscStartOriginReply staged;
    int allowed;
    (void)user;
    if(!input || !out || !current || (size&&!error) ||
       StartOriginPrivate(input,sizeof(*input)) || StartOriginPrivate(out,sizeof(*out)) ||
       StartOriginPrivate(error,size) || ActorOverlap(input,sizeof(*input),out,sizeof(*out)) ||
       ActorOverlap(input,sizeof(*input),error,size) || ActorOverlap(out,sizeof(*out),error,size))return 0;
    memset(&lease,0,sizeof(lease));lease.input=input;lease.copied=*input;
    lease.out=out;lease.error=error;lease.size=size;lease.current=current;lease.context=context;
    if(!memchr(lease.copied.tree,0,sizeof(lease.copied.tree)) ||
       !memchr(lease.copied.name,0,sizeof(lease.copied.name)))goto refuse;
    allowed=(!strcmp(lease.copied.tree,"generic_human") &&
             !strcmp(lease.copied.name,"training_guy_practicejump")) ||
            (!strcmp(lease.copied.tree,"animation_rig_gear_trainingjump") &&
             !strcmp(lease.copied.name,"training_gear_practicejump"));
    if(!allowed || !lease.copied.runtime_identity || !lease.copied.catalog_identity ||
       lease.copied.now_ms<0 || !lease.copied.ordinal || lease.copied.flags)goto refuse;
    if(!G_Cod1StartOriginResourceRef(lease.copied.tree,lease.copied.name,&ref,error,size))return 0;
    lease.receipt=ref.receipt;
    if(ref.ordinal!=lease.copied.ordinal || ref.flags!=lease.copied.flags ||
       !StartOriginTypedPure(&lease))goto refuse;
    /* Preserve the actual R208 transform's exact two forwarded caller fences. */
    if(!G_Cod1GetStartOrigin(&ref,lease.copied.origin,lease.copied.angles,
        staged.vector,error,size,StartOriginTypedCurrent,&lease) ||
       !StartOriginTypedPure(&lease))goto refuse;
    *out=staged;return 1;
refuse:
    if(!StartOriginPrivate(error,size))EntityError(error,size,"namespaced typed start-origin query refused");
    return 0;
}
const GscStartOriginHost *G_Cod1NamespaceStartOriginQueryHost(void) {
    return &cod1NamespaceStartOriginTypedHost;
}
