/* Optional declared-namespace query; no tree association or playback. */
typedef struct {
    GscRuntime *runtime;Task *task;
    uint64_t task_serial,runtime_identity,namespace_identity;
    int now_ms;GscStartOriginHost host;
} NamespaceStartOriginLease;
static int namespace_start_origin_scope(const NamespaceStartOriginLease *lease){
    const GscRuntime *r=lease->runtime;
    return r&&r->started&&r->advancing&&!r->error[0]&&r->namespace_start_origin_enabled&&
        r->namespace_start_origin_busy&&r->entity_method_busy&&animation_namespace_scope(r)&&
        r->array_token==lease->runtime_identity&&r->animation_namespace.identity==lease->namespace_identity&&
        r->now_ms==lease->now_ms&&r->executing_task==lease->task&&lease->task&&
        lease->task->serial==lease->task_serial&&lease->task->status!=TASK_DEAD&&
        !memcmp(&r->namespace_start_origin,&lease->host,sizeof(lease->host));
}
static int namespace_start_origin_current(void *context){
    NamespaceStartOriginLease *lease=context;
    return namespace_start_origin_scope(lease)&&entity_task_current(lease->runtime)&&namespace_start_origin_scope(lease);
}
int gsc_program_select_namespace_start_origin_query(GscProgram *g){
    if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
       !g->anim_data_selected||!g->animation_namespace_selected||g->namespace_start_origin_selected||
       g->start_origin_selected||g->animation_refs_selected||g->animation_length_selected||g->animation_methods_selected)return 0;
    g->namespace_start_origin_selected=1;return 1;
}
static int namespace_start_origin_entry(const GscRuntime *r,uint32_t object){
    const AnimationNamespaceEntry *entry;
    if(!object||object>(uint32_t)r->animation_namespace.entry_count)return 0;
    entry=&r->animation_namespace.entries[object-1];
    return !entry->declaration.flags&&
        ((entry->catalog==0&&!strcmp(entry->declaration.name,"training_guy_practicejump"))||
         (entry->catalog==(r->program->training_rigs_selected?2u:1u)&&!strcmp(entry->declaration.name,"training_gear_practicejump")));
}
static int namespace_start_origin_literal_ready(const GscRuntime *r,int expression){
    int n;
    if(expression<0||expression>=r->program->expr_count)return 0;
    if(r->program->expr[expression].kind!=E_ANIM_REF)return 1;
    for(n=0;n<r->animation_namespace.atom_count;++n){
        const AnimationNamespaceAtom *atom=&r->animation_namespace.atoms[n];
        if(atom->expr==expression)return atom->kind==V_NAMESPACED_ANIMATION&&namespace_start_origin_entry(r,atom->object);
    }
    return 0;
}
int gsc_runtime_select_namespace_start_origin(GscRuntime *r,const GscStartOriginHost *input){
    GscStartOriginHost host;const GscProgram *program;uint64_t runtime_identity,identity;int n;
    if(!input||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
       !r->program->namespace_start_origin_selected||r->namespace_start_origin_enabled||r->start_origin_enabled||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    host=*input;if(!host.query)return 0;
    for(n=0;n<r->program->code_count;++n){
        const Instr *i=&r->program->code[n];
        if(i->kind==I_CALL&&i->target==-1-START_ORIGIN_STATEMENT_TARGET&&
           (i->argc!=3||!namespace_start_origin_literal_ready(r,i->args[2])))return 0;
    }
    for(n=0;n<r->program->expr_count;++n){
        const Expr *e=&r->program->expr[n];
        if(e->call_binding==GSC_EXPR_TARGET_START_ORIGIN_NATIVE&&
           (e->argc!=3||!namespace_start_origin_literal_ready(r,e->args[2])))return 0;
    }
    program=r->program;runtime_identity=r->array_token;identity=r->animation_namespace.identity;
    if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
       r->program!=program||r->array_token!=runtime_identity||r->animation_namespace.identity!=identity||
       !r->program->namespace_start_origin_selected||r->namespace_start_origin_enabled||r->start_origin_enabled)return 0;
    r->namespace_start_origin=host;r->namespace_start_origin_enabled=1;return 1;
}
static int namespace_start_origin_call(GscRuntime *r,const Value *args,int argc,int line,Value *out){
    GscStartOriginRequest request,expected;GscStartOriginReply reply;
    NamespaceStartOriginLease lease;Value staged;char error[384];int n,accepted,ok=0;
    const AnimationNamespaceEntry *entry;uint32_t poison=UINT32_C(0x7fc00001);
    if(!r||!out||!args||argc!=3||!r->program->namespace_start_origin_selected||!r->namespace_start_origin_enabled)
        return rfail(r,line,"namespace start-origin exact3argument capability unavailable");
    if(r->namespace_start_origin_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)
        return rfail(r,line,"namespace start-origin callback reentered");
    if(args[0].kind!=V_VECTOR||args[1].kind!=V_VECTOR||!start_origin_vector(args[0].vector,0)||
       !start_origin_vector(args[1].vector,1)||args[2].kind!=V_NAMESPACED_ANIMATION||
       !animation_namespace_value_valid(r,&args[2])||!namespace_start_origin_entry(r,args[2].object))
        return rfail(r,line,"namespace start-origin requires finite vectors and current Guy or gear leaf");
    entry=&r->animation_namespace.entries[args[2].object-1];memset(&request,0,sizeof(request));
    request.runtime_identity=r->array_token;request.catalog_identity=r->animation_namespace.identity;request.now_ms=r->now_ms;
    strcpy(request.tree,r->animation_namespace.catalogs[entry->catalog].tree);
    strcpy(request.name,entry->declaration.name);request.ordinal=entry->declaration.ordinal;request.flags=entry->declaration.flags;
    memcpy(request.origin,args[0].vector,sizeof(request.origin));memcpy(request.angles,args[1].vector,sizeof(request.angles));
    /* No borrowed argument or declaration access follows a certificate
     * callback: cancellation can retire the resumable Results arena. */
    expected=request;memset(&lease,0,sizeof(lease));lease.runtime=r;lease.task=r->executing_task;
    if(!lease.task)return rfail(r,line,"namespace start-origin requires executing task");
    lease.task_serial=lease.task->serial;lease.runtime_identity=r->array_token;
    lease.namespace_identity=r->animation_namespace.identity;lease.now_ms=r->now_ms;lease.host=r->namespace_start_origin;
    for(n=0;n<3;++n)memcpy(&reply.vector[n],&poison,4);error[0]=0;
    r->namespace_start_origin_busy=1;r->entity_method_busy=1;
    if(!namespace_start_origin_current(&lease))goto done;
    accepted=lease.host.query(lease.host.user,&request,&reply,error,sizeof(error),namespace_start_origin_current,&lease);
    error[sizeof(error)-1]=0;
    if(!namespace_start_origin_current(&lease))goto done;
    if(memcmp(&request,&expected,sizeof(request))){rfail(r,line,"namespace start-origin host mutated copied request");goto done;}
    if(accepted!=1){rfail(r,line,"namespace start-origin provider refused: %s",error[0]?error:"provider failure");goto done;}
    if(!start_origin_vector(reply.vector,0)){rfail(r,line,"namespace start-origin reply requires all3finite components");goto done;}
    memset(&staged,0,sizeof(staged));staged.kind=V_VECTOR;memcpy(staged.vector,reply.vector,sizeof(staged.vector));ok=1;
done:
    r->namespace_start_origin_busy=0;r->entity_method_busy=0;
    if(!ok&&!r->error[0]&&entity_task_current(r))rfail(r,line,"namespace start-origin caller scope changed");
    if(!ok||r->error[0])return 0;
    *out=staged;return 1;
}
static int start_origin_selected_call(GscRuntime *r,const Value *args,int argc,int line,Value *out){
    return r->program->namespace_start_origin_selected?
        namespace_start_origin_call(r,args,argc,line,out):start_origin_call(r,args,argc,line,out);
}
