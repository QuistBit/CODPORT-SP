/* Selected typed effects only. No animation, AI state or fire event is
 * simulated by the interpreter: every admitted effect reaches this host. */
int gsc_program_select_r232_turrets(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->turrets_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->turrets_selected=1;return 1;
}
int gsc_runtime_select_r232_turrets(GscRuntime *r,const GscTurretHost *host) {
    if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
       !r->program->turrets_selected||r->turrets_enabled||!r->entities_enabled||
       !r->state_enabled||!r->results_enabled||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->turrets=*host;r->turrets_enabled=1;return 1;
}
typedef struct {
    GscRuntime *runtime;Task *task;uint64_t serial,instance;int now_ms,checking;
    GscTurretHost host;GscEntityRef entity,argument;int has_argument;
} R232TurretLease;
static int turret_scope(const R232TurretLease *l) {
    GscRuntime *r=l->runtime;
    return r&&!r->error[0]&&r->turrets_enabled&&r->entity_method_busy&&
        r->array_token==l->instance&&r->now_ms==l->now_ms&&
        r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&
        l->task->status!=TASK_DEAD&&!memcmp(&r->turrets,&l->host,sizeof(l->host));
}
static int turret_current(void *context) {
    R232TurretLease *l=context;GscRuntime *r=l->runtime;int ok;
    if(!turret_scope(l))return 0;
    if(l->checking)return rfail(r,0,"turret current callback reentered");
    l->checking=1;
    ok=entity_task_current(r)&&turret_scope(l)&&state_entity_live(r,&l->entity)==1&&
        turret_scope(l)&&(!l->has_argument||state_entity_live(r,&l->argument)==1)&&
        turret_scope(l)&&entity_task_current(r)&&turret_scope(l);
    l->checking=0;return ok;
}
static int turret_invoke(GscRuntime *r,const Value *receiver,int descriptor,
    const Value *args,int argc,int line,Value *out) {
    GscTurretRequest request,original;GscTurretReply reply;GscTurretHost host;
    R232TurretLease lease;Value staged;int kind=turret_method_kind(descriptor),ok;
    char error[384];GscEntityRef zero={0};
    if(!r->turrets_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||
       argc!=entity_method_arity(descriptor)||(argc&&!args))
        return rfail(r,line,"turret capability, receiver or arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"turret callback reentered");
    memset(&request,0,sizeof(request));memset(&reply,0,sizeof(reply));reply.kind=-1;
    memset(&staged,0,sizeof(staged));memset(&lease,0,sizeof(lease));
    request.kind=kind;request.now_ms=r->now_ms;request.entity=receiver->entity;
    if(kind==GSC_TURRET_SET_MODE) {
        const char *end;
        if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||
           end==args[0].str||(size_t)(end-args[0].str)>=sizeof(request.mode))
            return rfail(r,line,"setmode requires a bounded nonempty plain string");
        memcpy(request.mode,args[0].str,(size_t)(end-args[0].str));
    } else if(kind==GSC_TURRET_SET_TARGET_ENTITY||kind==GSC_TURRET_USE) {
        if(args[0].kind!=V_ENTITY||args[0].entity.epoch!=request.entity.epoch)
            return rfail(r,line,"turret argument requires a current same-map entity");
        request.argument=args[0].entity;
    }
    original=request;host=r->turrets;
    lease.runtime=r;lease.task=r->executing_task;lease.serial=lease.task?lease.task->serial:0;
    lease.instance=r->array_token;lease.now_ms=r->now_ms;lease.host=host;
    lease.entity=request.entity;lease.argument=request.argument;
    lease.has_argument=kind==GSC_TURRET_SET_TARGET_ENTITY||kind==GSC_TURRET_USE;
    r->entity_method_busy=1;
    if(!turret_current(&lease)){r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"turret entity/task retired before invocation");}
    error[0]=0;ok=host.invoke(host.user,&request,&reply,error,sizeof(error),turret_current,&lease);
    error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!turret_current(&lease)){r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"turret entity/task retired during callback");}
    if(ok!=1||memcmp(&request,&original,sizeof(request))) {
        r->entity_method_busy=0;return rfail(r,line,"turret refused: %s",error[0]?error:"host failed or changed its request");
    }
    if(kind==GSC_TURRET_USE||kind==GSC_TURRET_IS_FIRING) {
        if(reply.kind!=GSC_TURRET_RESULT_BOOL||(reply.boolean!=0&&reply.boolean!=1)||
           memcmp(&reply.entity,&zero,sizeof(zero)))goto invalid;
        scalar_integer(&staged,reply.boolean);
    } else if(kind==GSC_TURRET_GET) {
        if(reply.boolean)goto invalid;
        if(reply.kind==GSC_TURRET_RESULT_ENTITY) {
            if(!reply.entity.epoch||!reply.entity.generation||reply.entity.epoch!=original.entity.epoch||
               state_entity_live(r,&reply.entity)!=1||!turret_current(&lease))goto invalid;
            staged.kind=V_ENTITY;staged.entity=reply.entity;
        } else if(reply.kind!=GSC_TURRET_RESULT_UNDEFINED||
                  memcmp(&reply.entity,&zero,sizeof(zero)))goto invalid;
    } else if(reply.kind!=GSC_TURRET_RESULT_VOID||reply.boolean||
              memcmp(&reply.entity,&zero,sizeof(zero)))goto invalid;
    if(!turret_current(&lease))goto invalid;
    r->entity_method_busy=0;if(out)*out=staged;return 1;
invalid:
    r->entity_method_busy=0;return rfail(r,line,"turret host returned an invalid typed reply or retired entity");
}
