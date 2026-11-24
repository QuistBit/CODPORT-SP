/* R218 selected training safe-point collection. Handles are private to the
 * runtime: public host/event schemas never export raw array Values. Compact
 * only after the ready-task drain, with no evaluator or borrowed host entry.
 * The existing live-container/entry limits and fail-closed allocation remain.
 * Other profiles keep their previous bounded-arena lifetime. */
typedef struct {
    unsigned char marked[GSC_ARRAY_CONTAINER_CAPACITY];
    uint32_t queue[GSC_ARRAY_CONTAINER_CAPACITY];
    uint32_t map[GSC_ARRAY_CONTAINER_CAPACITY+1];
    int count,failed,remap;
} ArrayCollection;
/* Native 4603C0 copies a referenced container before indexed mutation.
 * Until that path is reconstructed, the new sparse lane must not silently use
 * the legacy shared-DAG mutation. Conservatively include all persistent and
 * suspended roots, excluding only the current lvalue's borrowed captures. */
static int array_sparse_unshared(GscRuntime *r,uint32_t owner,const ArrayLvalue *path) {
    int count=0,n,f,k,j;Task *task;ResultsTask *s;
#define R233_ARRAY_REF(v) do{if((v)->kind==V_ARRAY&&(v)->array==owner&&++count>1)return 0;}while(0)
    if(r->game_array_enabled)R233_ARRAY_REF(&r->game_array_root);
    for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used)R233_ARRAY_REF(&r->state_fields[n].value);
    for(n=0;n<r->array_entry_count;n++)R233_ARRAY_REF(&r->array_entries[n].value);
    for(n=0;n<r->task_count;n++) {
        task=task_at(r,n);if(task->status==TASK_DEAD)continue;
        R233_ARRAY_REF(&task->initial_self);
        for(k=0;k<task->event_filter_count;k++)R233_ARRAY_REF(&task->event_filters[k]);
        for(f=0;f<task->depth;f++) {
            R233_ARRAY_REF(&task->frames[f].self);
            for(k=0;k<task->frames[f].local_count;k++)R233_ARRAY_REF(&task->frames[f].locals[k].value);
        }
        s=task->results;if(!s)continue;
        for(k=0;k<s->node_count;k++) {
            ResultsNode *node=&s->nodes[k];R233_ARRAY_REF(&node->a);R233_ARRAY_REF(&node->b);R233_ARRAY_REF(&node->receiver);
            for(j=0;j<MAX_PARAMS;j++)R233_ARRAY_REF(&node->args[j]);
        }
        for(f=0;f<task->depth;f++)if(s->pending[f].active) {
            ResultsPending *p=&s->pending[f];
            R233_ARRAY_REF(&p->owner);R233_ARRAY_REF(&p->receiver);R233_ARRAY_REF(&p->value);R233_ARRAY_REF(&p->path.owner);
            if(&p->path!=path) {
                R233_ARRAY_REF(&p->current);R233_ARRAY_REF(&p->path.base);
                for(j=0;j<48;j++)R233_ARRAY_REF(&p->ancestors[j]);
            }
            for(j=0;j<MAX_PARAMS;j++)R233_ARRAY_REF(&p->args[j]);
        }
    }
#undef R233_ARRAY_REF
    return 1;
}
static int r238_array_reference_valid(const GscRuntime *r,uint32_t id,uint64_t token) {
    return array_value_valid(r,id) &&
        (r->game_array_enabled ? r->array_token && token==r->array_token : !token);
}
/* Both representations provide the mutable handle slot. Token validation
 * and marking precede remapping in precisely the same order for every root. */
static void array_collect_reference(GscRuntime *r,ArrayCollection *c,uint32_t *handle,const uint64_t *token) {
    uint32_t id=*handle;
    if(r->r238_array_value_cow_enabled&&!r238_array_reference_valid(r,id,*token)){c->failed=1;return;}
    if(!id||id>(uint32_t)r->array_count){c->failed=1;return;}
    if(c->remap){if(!c->map[id])c->failed=1;else *handle=c->map[id];return;}
    if(!c->marked[id-1]){c->marked[id-1]=1;c->queue[c->count++]=id;}
}
static void array_collect_value(GscRuntime *r,ArrayCollection *c,Value *v) {
    if(v->kind==V_ARRAY)array_collect_reference(r,c,&v->array,&v->object_token);
}
static void array_collect_roots(GscRuntime *r,ArrayCollection *c) {
    int n,f,k,j;Task *task;ResultsTask *s;
    if(r->game_array_enabled)array_collect_value(r,c,&r->game_array_root);
    /* Conservative metadata roots also retain fields of allocated structures.
     * Structure lifetime is independent; do not infer its retirement here. */
    for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used)
        array_collect_value(r,c,&r->state_fields[n].value);
    for(n=0;n<r->task_count;n++) {
        task=task_at(r,n);if(task->status==TASK_DEAD)continue;
        array_collect_value(r,c,&task->initial_self);
        for(k=0;k<task->event_filter_count;k++)array_collect_value(r,c,&task->event_filters[k]);
        for(f=0;f<task->depth;f++) {
            array_collect_value(r,c,&task->frames[f].self);
            for(k=0;k<task->frames[f].local_count;k++)
                array_collect_value(r,c,&task->frames[f].locals[k].value);
        }
        s=task->results;if(!s)continue;
        for(k=0;k<s->node_count;k++) {
            ResultsNode *node=&s->nodes[k];
            array_collect_value(r,c,&node->a);array_collect_value(r,c,&node->b);
            array_collect_value(r,c,&node->receiver);
            for(j=0;j<MAX_PARAMS;j++)array_collect_value(r,c,&node->args[j]);
        }
        for(f=0;f<task->depth;f++)if(s->pending[f].active) {
            ResultsPending *p=&s->pending[f];
            array_collect_value(r,c,&p->owner);array_collect_value(r,c,&p->receiver);
            array_collect_value(r,c,&p->current);array_collect_value(r,c,&p->value);
            array_collect_value(r,c,&p->path.owner);array_collect_value(r,c,&p->path.base);
            for(j=0;j<MAX_PARAMS;j++)array_collect_value(r,c,&p->args[j]);
            for(j=0;j<48;j++) {
                ResultsAncestor *ref=&p->ancestors[j];
                if(ref->kind==V_ARRAY)array_collect_reference(r,c,&ref->array,&ref->object_token);
            }
        }
    }
}
/* Build a temporary owner-to-entry index only at the existing safe point.
 * The original graph walk rescans all entries for every reachable container.
 * Reverse insertion preserves each owner's ascending source-entry order and
 * the same breadth-first discovery order. No callbacks or heap allocation.
 * All entry owners are still checked, including unreachable entries. With
 * no roots, leave that validation to the original commit gate below so its
 * distinct error result is retained. Keep scratch off the fast-return path. */
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static void array_collect_mark_graph(GscRuntime *r,ArrayCollection *c){
    int head[GSC_ARRAY_CONTAINER_CAPACITY],next[GSC_ARRAY_ENTRY_CAPACITY];
    int n,k,owner;
    if(c->failed||!c->count)return;
    for(n=0;n<r->array_count;n++)head[n]=-1;
    for(n=r->array_entry_count-1;n>=0;n--){
        owner=r->array_entries[n].owner;
        if(owner<1||owner>r->array_count){c->failed=1;return;}
        next[n]=head[owner-1];head[owner-1]=n;
    }
    for(k=0;k<c->count&&!c->failed;k++)
        for(n=head[c->queue[k]-1];n>=0;n=next[n])
            array_collect_value(r,c,&r->array_entries[n].value);
}
static int array_collect_at_boundary(GscRuntime *r,Task *root) {
    ArrayCollection c;int n,live=0,entries=0,old_arrays,old_entries;
    if(!(r->program->grenade_items_selected||r->program->carbine_items_selected||r->r238_array_value_cow_enabled)||!r->arrays_enabled)return 1;
    if(r->array_count<GSC_ARRAY_CONTAINER_CAPACITY/2&&r->array_entry_count<GSC_ARRAY_ENTRY_CAPACITY/2)return 1;
    if(root){
        if(root!=r->array_dispatch_root || root!=r->executing_task || !r->advancing ||
           !r->execution_budget || r->r243_thread_start_depth || root->initial_thread_result ||
           r->entity_inheritance_busy)return 1;
    }else if(r->advancing || r->executing_task || r->execution_budget)return 1;
    if(r->entity_entry_depth||
       r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_field_busy||r->entity_method_busy||r->hud_elements_busy)return 1;
    for(n=0;n<r->task_count;n++)if(task_at(r,n)->host_entry_pins)return 1;
    if(r->array_count<0||r->array_count>GSC_ARRAY_CONTAINER_CAPACITY||
       r->array_entry_count<0||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY)
        return rfail(r,0,"selected array collector arena invalid");
    memset(&c,0,sizeof(c));array_collect_roots(r,&c);
    array_collect_mark_graph(r,&c);
    if(c.failed)return rfail(r,0,"selected array collector root invalid");
    for(n=1;n<=r->array_count;n++)if(c.marked[n-1])c.map[n]=(uint32_t)++live;
    /* Validate all owners before the first mutation, including unreachable
     * entries. No callback, allocation or host effect occurs during commit. */
    for(n=0;n<r->array_entry_count;n++)if(r->array_entries[n].owner<1||r->array_entries[n].owner>r->array_count)
        return rfail(r,0,"selected array collector entry owner invalid");
    old_arrays=r->array_count;old_entries=r->array_entry_count;c.remap=1;
    array_collect_roots(r,&c);
    for(n=0;n<old_entries;n++) {
        ArrayEntry entry=r->array_entries[n];
        if(!c.map[entry.owner])continue;
        entry.owner=(int)c.map[entry.owner];array_collect_value(r,&c,&entry.value);
        r->array_entries[entries++]=entry;
    }
    for(n=1;n<=old_arrays;n++)if(c.map[n])r->arrays[c.map[n]-1]=r->arrays[n-1];
    memset(r->arrays+live,0,(size_t)(old_arrays-live)*sizeof(r->arrays[0]));
    memset(r->array_entries+entries,0,(size_t)(old_entries-entries)*sizeof(r->array_entries[0]));
    r->array_count=live;r->array_entry_count=entries;
    return 1;
}

static int array_collect_safe_point(GscRuntime *r) {
    return array_collect_at_boundary(r,NULL);
}
