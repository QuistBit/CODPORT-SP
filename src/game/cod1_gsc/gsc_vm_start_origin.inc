/* R209 transports one fixed resource query; it performs no native math. */
typedef struct {
    GscRuntime *runtime;Task *task;
    uint64_t task_serial,runtime_identity,catalog_identity;
    int now_ms;GscStartOriginHost host;
} StartOriginLease;

static int start_origin_scope(const StartOriginLease *lease) {
    const GscRuntime *r=lease->runtime;
    return r&&r->started&&r->advancing&&!r->error[0]&&r->start_origin_enabled&&r->start_origin_busy&&
        r->entity_method_busy&&r->animation_refs_enabled&&r->anim_data_enabled&&
        r->array_token==lease->runtime_identity&&r->animation_catalog_token==lease->catalog_identity&&
        r->now_ms==lease->now_ms&&r->executing_task==lease->task&&lease->task&&
        lease->task->serial==lease->task_serial&&lease->task->status!=TASK_DEAD&&
        !memcmp(&r->start_origin,&lease->host,sizeof(lease->host));
}
static int start_origin_current(void *context) {
    StartOriginLease *lease=context;
    if(!start_origin_scope(lease)||!entity_task_current(lease->runtime))return 0;
    return start_origin_scope(lease);
}
int gsc_program_select_start_origin_query(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->start_origin_selected||
       !g->anim_data_selected||!g->animation_refs_selected)return 0;
    g->start_origin_selected=1;return 1;
}
static int start_origin_literal_ready(const GscRuntime *r,int expression) {
    int n;
    if(expression<0||expression>=r->program->expr_count)return 0;
    if(r->program->expr[expression].kind!=E_ANIM_REF)return 1;
    for(n=0;n<r->animation_ref_count;++n) {
        const AnimationBinding *b=&r->animation_bindings[n];
        if(b->expr==expression)return !b->flags&&!strcmp(b->tree,"generic_human")&&start_origin_guy(b->name);
    }
    return 0;
}
int gsc_runtime_select_start_origin(GscRuntime *r,const GscStartOriginHost *input) {
    GscStartOriginHost host;int n;
    if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->program->start_origin_selected||r->start_origin_enabled||!r->state_enabled||
       !r->arrays_enabled||!r->results_enabled||!r->anim_data_enabled||!r->animation_refs_enabled||
       r->start_origin_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_sequence_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    host=*input;if(!host.query||!anim_data_current(r,NULL))return 0;
    /* The known literal resource is checked after actual catalog binding,
     * including retained dead functions. Computed Values are checked at call. */
    for(n=0;n<r->program->code_count;++n) {
        const Instr *i=&r->program->code[n];
        if(i->kind==I_CALL&&i->target==-1-START_ORIGIN_STATEMENT_TARGET&&
           (i->argc!=3||!start_origin_literal_ready(r,i->args[2])))return 0;
    }
    for(n=0;n<r->program->expr_count;++n) {
        const Expr *e=&r->program->expr[n];
        if(e->call_binding==GSC_EXPR_TARGET_START_ORIGIN_NATIVE&&
           (e->argc!=3||!start_origin_literal_ready(r,e->args[2])))return 0;
    }
    r->start_origin=host;r->start_origin_enabled=1;return 1;
}
static int start_origin_vector(const float *vector,int angles) {
    int i;uint32_t bits;
    for(i=0;i<3;++i) {
        memcpy(&bits,&vector[i],sizeof(bits));
        if((bits&UINT32_C(0x7f800000))==UINT32_C(0x7f800000)||
           (angles&&(bits&UINT32_C(0x7fffffff))>UINT32_C(0x45610000)))return 0;
    }
    return 1;
}
static int start_origin_call(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    GscStartOriginRequest request,expected;GscStartOriginReply reply;
    StartOriginLease lease;Value staged;char error[384];int n,found=0,accepted,ok=0;
    uint32_t poison=UINT32_C(0x7fc00001);
    if(!r||!out||!args||argc!=3||!r->program->start_origin_selected||!r->start_origin_enabled)
        return rfail(r,line,"getstartorigin capability or exact3argument domain unavailable");
    if(r->start_origin_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)
        return rfail(r,line,"start-origin callback reentered");
    if(args[0].kind!=V_VECTOR||args[1].kind!=V_VECTOR||
       !start_origin_vector(args[0].vector,0)||!start_origin_vector(args[1].vector,1)||
       !animation_value_valid(r,&args[2]))
        return rfail(r,line,"getstartorigin requires finite vectors and current typed Guy animation");
    memset(&request,0,sizeof(request));
    request.runtime_identity=r->array_token;request.catalog_identity=r->animation_catalog_token;
    request.now_ms=r->now_ms;
    memcpy(request.origin,args[0].vector,sizeof(request.origin));
    memcpy(request.angles,args[1].vector,sizeof(request.angles));
    for(n=0;n<r->animation_ref_count;++n)if(r->animation_bindings[n].entry==args[2].object) {
        const AnimationBinding *b=&r->animation_bindings[n];
        if(strcmp(b->tree,"generic_human")||!start_origin_guy(b->name)||b->flags)
            return rfail(r,line,"getstartorigin is restricted to generic_human/Guy leaf");
        strcpy(request.tree,b->tree);strcpy(request.name,b->name);
        request.ordinal=b->entry;request.flags=b->flags;found=1;break;
    }
    if(!found)return rfail(r,line,"getstartorigin animation binding unavailable");
    /* No argument-arena access follows a certificate callback: cancellation
     * can retire/clear the executing task and its resumable result nodes. */
    expected=request;memset(&lease,0,sizeof(lease));lease.runtime=r;lease.task=r->executing_task;
    if(!lease.task)return rfail(r,line,"getstartorigin requires an executing task");
    lease.task_serial=lease.task->serial;lease.runtime_identity=r->array_token;
    lease.catalog_identity=r->animation_catalog_token;lease.now_ms=r->now_ms;lease.host=r->start_origin;
    for(n=0;n<3;++n)memcpy(&reply.vector[n],&poison,sizeof(poison));error[0]=0;
    r->start_origin_busy=1;r->entity_method_busy=1;
    if(!start_origin_current(&lease))goto done;
    accepted=lease.host.query(lease.host.user,&request,&reply,error,sizeof(error),start_origin_current,&lease);
    error[sizeof(error)-1]=0;
    if(!start_origin_current(&lease))goto done;
    if(memcmp(&request,&expected,sizeof(request))) {rfail(r,line,"start-origin host mutated copied request");goto done;}
    if(accepted!=1) {rfail(r,line,"start-origin host refused: %s",error[0]?error:"provider failure");goto done;}
    if(!start_origin_vector(reply.vector,0)) {rfail(r,line,"start-origin reply requires all3finite components");goto done;}
    memset(&staged,0,sizeof(staged));staged.kind=V_VECTOR;
    memcpy(staged.vector,reply.vector,sizeof(staged.vector));ok=1;
 done:
    r->start_origin_busy=0;r->entity_method_busy=0;
    if(!ok&&!r->error[0]&&entity_task_current(r))rfail(r,line,"start-origin callback changed caller scope");
    if(!ok||r->error[0])return 0;
    *out=staged;return 1;
}
