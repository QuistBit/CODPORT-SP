/* Explicit immediate-command capability. The VM supplies no prone physics,
 * tree state, notify completion or successful replacement implementation. */
int gsc_program_select_r232_actor_look(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||
       g->actor_look_selected)return 0;g->actor_look_selected=1;return 1;
}
int gsc_runtime_select_r232_actor_look(GscRuntime *r,const GscActorLookHost *input) {
    GscActorLookHost copy;
    if(!input||!animation_namespace_quiescent(r)||!r->program->actor_look_selected||
       !r->turret_animations_enabled||!animation_namespace_scope(r)||r->actor_look_enabled||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    copy=*input;if(!copy.validate||!copy.invoke||!anim_data_current(r,NULL)||memcmp(input,&copy,sizeof(copy)))return 0;
    r->actor_look=copy;r->actor_look_enabled=1;return 1;
}
typedef struct {
    GscRuntime *runtime;Task *task;uint64_t serial,instance,catalog;int now,checking;
    GscActorLookHost host;GscEntityRef entity,argument;int has_argument;
} R232ActorLookLease;
static int actor_look_scope(const R232ActorLookLease *l) {
    GscRuntime *r=l->runtime;
    return r&&!r->error[0]&&r->actor_look_enabled&&r->entity_method_busy&&
        r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&animation_namespace_scope(r)&&
        r->now_ms==l->now&&r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&
        !memcmp(&r->actor_look,&l->host,sizeof(l->host));
}
static int actor_look_current(void *context) {
    R232ActorLookLease *l=context;GscRuntime *r=l->runtime;int ok;
    if(!actor_look_scope(l))return 0;
    if(l->checking)return rfail(r,0,"actor animation current callback reentered");l->checking=1;
    ok=entity_task_current(r)&&actor_look_scope(l)&&anim_data_current(r,l->task)&&actor_look_scope(l)&&
        state_entity_live(r,&l->entity)==1&&actor_look_scope(l)&&
        (!l->has_argument||state_entity_live(r,&l->argument)==1)&&actor_look_scope(l)&&entity_task_current(r)&&actor_look_scope(l);
    l->checking=0;return ok;
}
static int actor_look_invoke(GscRuntime *r,const Value *receiver,int descriptor,const Value *args,int argc,int line,Value *out){
    GscActorLookRequest q,original;GscActorLookReply reply;R232ActorLookLease l;GscActorLookHost host;GscEntityRef borrowed;
    Value value;char error[384];int kind=actor_look_kind(descriptor),i,ok;const char *end;
    if(!r->actor_look_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||!entity_method_argc_valid(descriptor,argc)||(argc&&!args))
        return rfail(r,line,"actor look receiver/capability/arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"actor look command reentered");
    memset(&q,0,sizeof(q));memset(&value,0,sizeof(value));memset(&reply,0xff,sizeof(reply));
    q.entity=receiver->entity;q.method=kind;q.now_ms=r->now_ms;q.runtime_identity=r->array_token;q.catalog_identity=r->animation_namespace.identity;q.precision_bits=53;
    if(kind==GSC_ACTOR_LOOK_NODES){for(i=0;i<3;i++)if(!actor_animation_binding(r,&args[i],line,&q.animation[i]))return 0;}
    else if(kind==GSC_ACTOR_LOOK_TARGET){
        if(args[0].kind!=V_VECTOR)return rfail(r,line,"setLookAt requires a current vector");
        memcpy(q.vector_bits,args[0].vector,12);for(i=0;i<3;i++)if((q.vector_bits[i]&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return rfail(r,line,"look target vector must be finite");
        if(argc>1&&!actor_animation_float(r,&args[1],line,1,&q.scalar_bits[0]))return 0;
    }else if(kind==GSC_ACTOR_LOOK_YAW_LIMITS){
        /* Original200674E1..20067501 obtains GetFloat(1), then (0), then (2).
         * Keep each converted value in its original argument slot. */
        if(!actor_animation_float(r,&args[1],line,1,&q.scalar_bits[1])||
           !actor_animation_float(r,&args[0],line,1,&q.scalar_bits[0])||
           !actor_animation_float(r,&args[2],line,1,&q.scalar_bits[2]))return 0;
    }
    else if(kind==GSC_ACTOR_LOOK_STOP){if(argc&&!actor_animation_float(r,&args[0],line,1,&q.scalar_bits[0]))return 0;}
    else if(actor_look_tag(kind)){
        if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||(size_t)(end-args[0].str)>=sizeof(q.tag))return rfail(r,line,"Tag query requires a bounded nonempty actual bone name");
        memcpy(q.tag,args[0].str,(size_t)(end-args[0].str));
    }else if(kind==GSC_ACTOR_LOOK_CAN_SEE){
        if(args[0].kind!=V_ENTITY||args[0].entity.epoch!=q.entity.epoch)return rfail(r,line,"CanSee requires a current same-map target entity");q.argument=args[0].entity;
    }
    original=q;host=r->actor_look;memset(&l,0,sizeof(l));l.runtime=r;l.task=r->executing_task;l.serial=l.task?l.task->serial:0;
    l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.now=r->now_ms;l.entity=q.entity;l.argument=q.argument;l.has_argument=kind==GSC_ACTOR_LOOK_CAN_SEE;l.host=host;
    borrowed=q.entity;r->entity_method_busy=1;
    if(!actor_look_current(&l))goto retired;
    if(kind==GSC_ACTOR_LOOK_CAN_SEE&&(state_entity_live(r,&q.argument)!=1||!actor_look_current(&l)))goto retired;
    error[0]=0;ok=host.validate(host.user,&borrowed,l.instance,l.now,error,sizeof(error),actor_look_current,&l);error[sizeof(error)-1]=0;
    if(!actor_look_current(&l))goto retired;if(ok!=1||memcmp(&borrowed,&q.entity,sizeof(borrowed)))goto refused;
    error[0]=0;ok=host.invoke(host.user,&q,&reply,error,sizeof(error),actor_look_current,&l);error[sizeof(error)-1]=0;
    if(!actor_look_current(&l))goto retired;if(ok!=1||memcmp(&q,&original,sizeof(q)))goto refused;
    if(kind==GSC_ACTOR_LOOK_CAN_SEE){
        if(reply.kind!=GSC_ACTOR_LOOK_RESULT_BOOL||(reply.boolean!=0&&reply.boolean!=1)||reply.vector_bits[0]||reply.vector_bits[1]||reply.vector_bits[2]||state_entity_live(r,&q.argument)!=1||!actor_look_current(&l))goto invalid;
        scalar_integer(&value,reply.boolean);
    }else if(actor_look_tag(kind)){
        if(reply.kind!=GSC_ACTOR_LOOK_RESULT_VECTOR||reply.boolean)goto invalid;
        for(i=0;i<3;i++)if((reply.vector_bits[i]&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))goto invalid;
        value.kind=V_VECTOR;memcpy(value.vector,reply.vector_bits,12);
    }else if(reply.kind!=GSC_ACTOR_LOOK_RESULT_VOID||reply.boolean||reply.vector_bits[0]||reply.vector_bits[1]||reply.vector_bits[2])goto invalid;
    if(!actor_look_current(&l))goto retired;r->entity_method_busy=0;if(out)*out=value;return 1;
invalid:r->entity_method_busy=0;return rfail(r,line,"actor look returned invalid typed reply or stale target");
retired:r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"actor look task/entity/resources retired");
refused:r->entity_method_busy=0;return rfail(r,line,"actor look host refused or changed request: %s",error);
}
