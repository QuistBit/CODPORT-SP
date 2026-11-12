/* R148 bounded runtime-owned structure identities. Shared Value copies retain
 * field ownership; no engine entity, original heap/refcount/GC or save format.
 * The runtime token prevents a handle from another runtime being accepted.
 * Objects and their fixed field records live until the owning runtime ends. */
static int struct_value_valid(const GscRuntime *r,const Value *value) {
    return r&&r->structs_enabled&&value&&value->kind==V_STRUCT&&
        r->array_token&&value->object_token==r->array_token&&
        value->object&&value->object<=(uint32_t)r->struct_count;
}
static int struct_call(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    Value result;
    if(!r)return 0;
    if(!r->structs_enabled||!r->state_enabled||!r->arrays_enabled||
       !r->array_token||r->error[0]||!out||(!args&&argc)||
       struct_static_issues(GSC_STRUCT_SPAWN,argc))
        return rfail(r,line,"invalid spawnstruct invocation");
    /* Every supplied argument expression was evaluated by the ordinary VM
     * call path. The CreateStruct service consumes none of those values. */
    /* A structure handle is an identity, not an index into field storage.
     * The original map continually allocates empty synchronization objects.
     * Keep each issued handle unique for this runtime, including after every
     * waiter has returned. The existing field arena still bounds storage. */
    if(r->struct_count==UINT32_MAX ||
       (r->struct_count>=GSC_STRUCT_CAPACITY &&
        !(r->r237_frame_buckets_enabled&&r->program->carbine_items_selected)))
        return rfail(r,line,"structure object limit");
    memset(&result,0,sizeof(result));result.kind=V_STRUCT;
    result.object=(uint32_t)++r->struct_count;result.object_token=r->array_token;
    *out=result;return 1;
}
