/* R199 copied map node identities. Node fields are genuine read-only host
 * metadata; they never enter entity fields/methods or a private field bag.
 * Equality compares complete current identities. Copies preserve identity;
 * isdefined reports a stale node as false, while dereference refuses it. */
typedef struct {GscRuntime *runtime;int now_ms;GscNodeQueryHost host;} NodeQueryLease;
static int node_ref_equal(const GscNodeRef *a,const GscNodeRef *b) {
    return a->epoch==b->epoch&&a->index==b->index&&a->generation==b->generation;
}
static int node_value_valid(const GscRuntime *r,const Value *value) {
    return r&&r->node_queries_enabled&&value&&value->kind==V_NODE&&
        value->node.epoch&&value->node.generation&&value->node.index<GSC_NODE_QUERY_CAPACITY;
}
static int node_query_current(void *context) {
    NodeQueryLease *lease=context;GscRuntime *r=lease->runtime;
    if(!r||r->error[0]||!r->node_queries_enabled||!r->node_query_busy||
       r->now_ms!=lease->now_ms||memcmp(&r->node_queries,&lease->host,sizeof(lease->host)))return 0;
    return entity_task_current(r);
}
static int node_query_begin(GscRuntime *r,int line,NodeQueryLease *lease) {
    if(!r->node_queries_enabled||!r->program->node_queries_selected||!r->arrays_enabled||
       !r->state_enabled||r->error[0])return rfail(r,line,"node query capability is unavailable");
    if(r->node_query_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"node query callback reentered");
    if(!entity_task_current(r))return 0;
    lease->runtime=r;lease->now_ms=r->now_ms;lease->host=r->node_queries;
    r->node_query_busy=1;r->entity_method_busy=1;return 1;
}
static int node_query_finish(GscRuntime *r,NodeQueryLease *lease,int line) {
    int current=node_query_current(lease);
    r->node_query_busy=0;r->entity_method_busy=0;
    if(r->error[0])return 0;
    if(!current) {
        if(!entity_task_current(r))return 0;
        return rfail(r,line,"node callback changed its clock or capability");
    }
    return 1;
}
static int node_ref_live(GscRuntime *r,const GscNodeRef *ref,int line,int *out) {
    NodeQueryLease lease;GscNodeRef borrowed=*ref,expected=*ref;char error[256];int ok,alive=-1;
    if(!ref->epoch||!ref->generation||ref->index>=GSC_NODE_QUERY_CAPACITY)
        return rfail(r,line,"malformed node identity");
    if(!node_query_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.validate(lease.host.user,&borrowed,lease.now_ms,&alive,
        error,sizeof(error),node_query_current,&lease);error[sizeof(error)-1]=0;
    if(!node_query_finish(r,&lease,line))return 0;
    if(!node_ref_equal(&borrowed,&expected))return rfail(r,line,"node validator mutated its borrowed identity");
    if(ok!=1)return rfail(r,line,"node validation refused: %s",error[0]?error:"host callback failed");
    if(alive!=0&&alive!=1)return rfail(r,line,"node validator returned an invalid alive flag");
    *out=alive;return 1;
}
static int node_value_live(GscRuntime *r,const Value *value,int line,int *alive) {
    if(!node_value_valid(r,value))return rfail(r,line,"invalid selected node value");
    return node_ref_live(r,&value->node,line,alive);
}
static int node_compare(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out) {
    int alive,equal;
    if(a->kind!=V_NODE||b->kind!=V_NODE||(op!=T_EQ&&op!=T_NE))
        return rfail(r,line,"node values support only identity equality");
    if(!node_value_live(r,a,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node comparison identity is stale");
    if(!node_value_live(r,b,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node comparison identity is stale");
    equal=node_ref_equal(&a->node,&b->node);scalar_integer(out,op==T_EQ?equal:!equal);return 1;
}
int gsc_runtime_select_node_queries(GscRuntime *r,const GscNodeQueryHost *host) {
    if(!r||!host||!host->query||!host->validate||!host->read||r->started||r->advancing||
       r->executing_task||r->task_count||r->error[0]||r->node_queries_enabled||
       !r->program->node_queries_selected||!r->state_enabled||!r->arrays_enabled||
       r->node_query_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->node_queries=*host;r->node_queries_enabled=1;return 1;
}
static int node_query_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscNodeQueryRequest request,expected;GscNodeRef refs[GSC_NODE_QUERY_CAPACITY];
    NodeQueryLease lease;Value staged,member;char error[256];size_t count=(size_t)-1,n;int ok,alive;
    if(kind==GSC_VEHICLE_QUERY_ONE||kind==GSC_VEHICLE_QUERY_ARRAY)return vehicle_node_query_call(r,kind,args,argc,line,out);
    if(!out||node_query_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid node query arity");
    memset(&request,0,sizeof(request));memset(&staged,0,sizeof(staged));
    request.kind=kind;request.now_ms=r->now_ms;
    if(argc) {
        if(args[0].kind!=V_STRING||args[1].kind!=V_STRING||
           !memchr(args[0].str,0,sizeof(args[0].str))||!memchr(args[1].str,0,sizeof(args[1].str))||!args[1].str[0])
            return rfail(r,line,"node query requires two ordinary strings and a nonempty field");
        memcpy(request.value,args[0].str,sizeof(request.value));memcpy(request.field,args[1].str,sizeof(request.field));
    }
    expected=request;memset(refs,0,sizeof(refs));
    if(!node_query_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.query(lease.host.user,&request,refs,GSC_NODE_QUERY_CAPACITY,&count,
        error,sizeof(error),node_query_current,&lease);error[sizeof(error)-1]=0;
    if(!node_query_finish(r,&lease,line))return 0;
    if(memcmp(&request,&expected,sizeof(request)))return rfail(r,line,"node query mutated its borrowed request");
    if(ok!=1)return rfail(r,line,"node query refused: %s",error[0]?error:"host callback failed");
    if(count>GSC_NODE_QUERY_CAPACITY||(kind==GSC_NODE_QUERY_ONE&&count>1))
        return rfail(r,line,"node query returned an invalid count or overflow");
    for(n=0;n<count;n++) {
        if(!node_ref_live(r,&refs[n],line,&alive))return 0;
        if(!alive)return rfail(r,line,"node query returned a stale identity");
        if(n&&(refs[n].epoch!=refs[0].epoch||refs[n].index<=refs[n-1].index))
            return rfail(r,line,"node query returned mixed map epochs or unordered duplicate identities");
    }
    for(n=0;n<count;n++) {
        if(!node_ref_live(r,&refs[n],line,&alive))return 0;
        if(!alive)return rfail(r,line,"node query identity retired before publication");
    }
    if(kind==GSC_NODE_QUERY_ONE) {
        if(count){staged.kind=V_NODE;staged.node=refs[0];}
    } else {
        if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||count>(size_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
            return rfail(r,line,"node query array arena limit");
        /* All callback work precedes allocation/publication. */
        if(!array_new(r,line,&staged))return 0;
        for(n=0;n<count;n++) {
            ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)staged.array;
            entry.key.kind=V_NUMBER;entry.key.number=(int)n;
            memset(&member,0,sizeof(member));member.kind=V_NODE;member.node=refs[n];entry.value=member;
            r->array_entries[r->array_entry_count++]=entry;
        }
        r->arrays[staged.array-1].count=r->arrays[staged.array-1].numeric_count=(int)count;
    }
    *out=staged;return 1;
}
static int node_field_read(GscRuntime *r,const Value *receiver,const char *name,int line,Value *out) {
    NodeQueryLease lease;GscNodeRef borrowed,expected;GscNodeFieldValue field;Value staged;
    char error[256],field_name[128],expected_name[128];int kind=node_field_descriptor(name),alive,ok,k,expected_kind;
    if(!name||!name[0]||strlen(name)>=sizeof(field_name))return rfail(r,line,"invalid node metadata field name");
    if(!node_value_live(r,receiver,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node field receiver is stale");
    borrowed=receiver->node;expected=borrowed;memset(&field,0,sizeof(field));memset(&staged,0,sizeof(staged));
    field.kind=-1;
    memset(field_name,0,sizeof(field_name));str_copy(field_name,sizeof(field_name),name);memcpy(expected_name,field_name,sizeof(field_name));
    if(!node_query_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.read(lease.host.user,&borrowed,field_name,lease.now_ms,&field,
        error,sizeof(error),node_query_current,&lease);error[sizeof(error)-1]=0;
    if(!node_query_finish(r,&lease,line))return 0;
    if(!node_ref_equal(&borrowed,&expected))return rfail(r,line,"node field host mutated its borrowed identity");
    if(memcmp(field_name,expected_name,sizeof(field_name)))return rfail(r,line,"node field host mutated its borrowed field name");
    if(ok!=1)return rfail(r,line,"node field read refused: %s",error[0]?error:"host callback failed");
    if(!node_value_live(r,receiver,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node field receiver retired before publication");
    expected_kind=kind==GSC_NODE_FIELD_ORIGIN||kind==GSC_NODE_FIELD_ANGLES?GSC_NODE_VALUE_VECTOR:
        kind==GSC_NODE_FIELD_SPAWNFLAGS?GSC_NODE_VALUE_INT32:GSC_NODE_VALUE_STRING;
    if(kind==GSC_NODE_FIELD_SCRIPT_DELAY||kind==GSC_NODE_FIELD_SCRIPT_FALLBACK)expected_kind=GSC_NODE_VALUE_FLOAT;
    /* R233 native built-in float+48 is always present, never optional custom metadata. */
    if(kind==GSC_NODE_FIELD_RADIUS){if(field.kind!=GSC_NODE_VALUE_FLOAT)return rfail(r,line,"node radius host returned the wrong value kind");expected_kind=GSC_NODE_VALUE_FLOAT;}
    if(field.kind==GSC_NODE_VALUE_UNDEFINED) {*out=staged;return 1;}
    if(!kind)return rfail(r,line,"node field host returned an unsupported present metadata field");
    if(field.kind!=expected_kind&&!(expected_kind==GSC_NODE_VALUE_FLOAT&&field.kind==GSC_NODE_VALUE_INT32))
        return rfail(r,line,"node field host returned the wrong value kind");
    if(field.kind==GSC_NODE_VALUE_INT32)scalar_integer(&staged,field.integer);
    else if(field.kind==GSC_NODE_VALUE_FLOAT) {
        if(!isfinite(field.number))return rfail(r,line,"node field host returned a nonfinite number");
        staged.kind=V_NUMBER;staged.number=field.number;
    } else if(field.kind==GSC_NODE_VALUE_STRING) {
        if(!memchr(field.text,0,sizeof(field.text)))return rfail(r,line,"node field host returned an unterminated string");
        staged.kind=V_STRING;memcpy(staged.str,field.text,sizeof(staged.str));
    } else {
        staged.kind=V_VECTOR;for(k=0;k<3;k++){
            if(!isfinite(field.vector[k]))return rfail(r,line,"node field host returned a nonfinite vector");
            staged.vector[k]=field.vector[k];
        }
    }
    *out=staged;return 1;
}
