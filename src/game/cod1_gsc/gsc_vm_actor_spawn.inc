/* R101 typed actor-spawner dispatch. Construction is host-owned; this bridge
 * neither fabricates actor state nor re-enters the public notification API. */
static int actor_spawn_runtime_preflight(const GscProgram *g,
    const GscActorSpawnHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
           actor_spawn_native_descriptor(i->receiver_native_target)&&(!host||!host->prepare||!host->finish)) {
            if(error&&size)snprintf(error,size,"line %d: dospawn requires a complete actor-spawner capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind==E_METHOD_CALL&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
           actor_spawn_native_descriptor(e->receiver_native_target)&&(!host||!host->prepare||!host->finish)) {
            if(error&&size)snprintf(error,size,"line %d: dospawn requires a complete actor-spawner capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int actor_spawn_finish(GscRuntime *r,const GscActorSpawnTicket *ticket,
    int published,int line) {
    GscActorSpawnTicket copy=*ticket;char error[256];int ok;
    error[0]=0;
    if(r->actor_spawn_finishing)return rfail(r,line,"recursive actor-spawner finalization");
    r->actor_spawn_finishing=&copy;r->actor_spawn_finish_published=published;
    ok=r->actor_spawners.finish(r->actor_spawners.user,&copy,published,error,sizeof(error));
    r->actor_spawn_finishing=NULL;r->actor_spawn_finish_published=0;
    error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!ok)return rfail(r,line,"actor-spawner finalization: %s",error[0]?error:"host rejected ticket");
    return 1;
}
static int actor_spawn_context(GscRuntime *r) {
    if(r->error[0])return 0;
    return !r->executing_task||
        (refresh_state_context(r,r->executing_task)&&r->executing_task->status!=TASK_DEAD);
}
static int actor_spawn_expression(GscRuntime *r,int descriptor,const Value *receiver,const Value *args,
    int argc,int line,Value *out) {
    GscActorSpawnRequest request,borrowed;
    GscActorSpawnTicket produced,ticket;Value payload,result;
    char error[256];int created=-1,ok,alive,prior_used;
    Task *prior_preparing;const GscActorSpawnRequest *prior_request;uint64_t prior_serial;GscEntityRef prior_source;
    GscEntityExecutionCertificate prior_issued;int prior_init_used;
    if(!r->actor_spawners_enabled||!r->actor_spawners.prepare||!r->actor_spawners.finish||
       !actor_spawn_native_descriptor(descriptor)||!receiver||!out||argc<0||argc>MAX_PARAMS||(!args&&argc))
        return rfail(r,line,"invalid actor-spawner binding or capability");
    if(receiver->kind!=V_ENTITY)return rfail(r,line,"dospawn requires an entity receiver");
    memset(&request,0,sizeof(request));request.spawner=receiver->entity;request.arg_count=argc;
    request.wrapper=descriptor==ENTITY_METHOD_STALINGRADSPAWN?GSC_ACTOR_SPAWN_STALINGRAD:GSC_ACTOR_SPAWN_DOSPAWN;
    if(argc==1) {
        /* Value tags are translated explicitly, never original VM IDs. The
         * host gate precedes getter/type admission; a nonstring is not rejected
         * here because same-frame calls must return undefined without getter. */
        switch(args[0].kind) {
          case V_UNDEFINED:request.single_kind=GSC_ACTOR_SPAWN_UNDEFINED;break;
          case V_NUMBER:request.single_kind=GSC_ACTOR_SPAWN_NUMBER;break;
          case V_STRING:
            request.single_kind=GSC_ACTOR_SPAWN_STRING;
            memcpy(request.single_string,args[0].str,sizeof(request.single_string));break;
          case V_VECTOR:request.single_kind=GSC_ACTOR_SPAWN_VECTOR;break;
          case V_ENTITY:request.single_kind=GSC_ACTOR_SPAWN_ENTITY;break;
          case V_LEVEL:request.single_kind=GSC_ACTOR_SPAWN_LEVEL;break;
          case V_ARRAY:request.single_kind=GSC_ACTOR_SPAWN_ARRAY;break;
          case V_FUNCTION:
            if(!function_value_valid(r,&args[0]))return rfail(r,line,"invalid dospawn function value");
            request.single_kind=GSC_ACTOR_SPAWN_FUNCTION;break;
          default:return rfail(r,line,"invalid dospawn optional value tag");
        }
    }
    if(!actor_spawn_context(r))return 0;
    alive=state_entity_live(r,&request.spawner);
    if(!actor_spawn_context(r))return 0;
    if(alive!=1)return rfail(r,line,"dospawn receiver is stale or missing");
    borrowed=request;memset(&produced,0,sizeof(produced));error[0]=0;
    /* A factory may invoke one projected native-data/entry prefix for this
     * exact source and caller. Finish and arbitrary callbacks receive no such
     * authority. A nested script task cannot borrow the suspended caller. */
    prior_request=r->actor_spawn_preparing_request;r->actor_spawn_preparing_request=&request;
    prior_preparing=r->actor_spawn_preparing;prior_serial=r->actor_spawn_preparing_serial;
    prior_source=r->actor_spawn_preparing_source;prior_used=r->actor_spawn_preparing_used;
    prior_init_used=r->actor_spawn_init_used;r->actor_spawn_init_used=0;
    prior_issued=r->actor_spawn_issued;memset(&r->actor_spawn_issued,0,sizeof(r->actor_spawn_issued));
    r->actor_spawn_preparing=r->executing_task;
    r->actor_spawn_preparing_serial=r->executing_task?r->executing_task->serial:0;
    r->actor_spawn_preparing_source=request.spawner;r->actor_spawn_preparing_used=0;
    ok=r->actor_spawners.prepare(r->actor_spawners.user,&borrowed,&produced,&created,error,sizeof(error));
    r->actor_spawn_preparing_request=prior_request;
    r->actor_spawn_preparing=prior_preparing;r->actor_spawn_preparing_serial=prior_serial;
    r->actor_spawn_preparing_source=prior_source;r->actor_spawn_preparing_used=prior_used;
    r->actor_spawn_issued=prior_issued;r->actor_spawn_init_used=prior_init_used;
    /* Copy before ANY validation callback. A retained output pointer is not a
     * supported host contract and cannot retarget the copied result. */
    ticket=produced;error[sizeof(error)-1]=0;
    if(!ok||r->error[0]) {
        if(created==1||ticket.operation)(void)actor_spawn_finish(r,&ticket,0,line);
        if(!entity_task_current(r))return 0;
        return rfail(r,line,"actor-spawner prepare: %s",error[0]?error:"host rejected spawn");
    }
    if((created!=0&&created!=1)||(!created&&ticket.operation)) {
        rfail(r,line,"actor-spawner returned invalid created flag or no-spawn reservation");
        if(ticket.operation)(void)actor_spawn_finish(r,&ticket,0,line);
        return 0;
    }
    memset(&result,0,sizeof(result));
    if(!created) {
        if(!actor_spawn_context(r))return 0;
        *out=result;return 1;
    }
    if(!actor_spawn_context(r))goto cancel;
    if(!ticket.operation||!entity_ref_equal(&ticket.spawner,&request.spawner)||
       !ticket.actor.epoch||!ticket.actor.generation) {
        rfail(r,line,"actor-spawner returned malformed or retargeted ticket");goto cancel;
    }
    alive=state_entity_live(r,&ticket.spawner);
    if(!actor_spawn_context(r))goto cancel;
    if(alive!=1) {rfail(r,line,"actor-spawner retired its spawner");goto cancel;}
    alive=state_entity_live(r,&ticket.actor);
    if(!actor_spawn_context(r))goto cancel;
    if(alive!=1) {rfail(r,line,"actor-spawner returned a stale actor");goto cancel;}
    memset(&payload,0,sizeof(payload));payload.kind=V_ENTITY;payload.entity=ticket.actor;
    /* Current waiters only; binding is atomic and execution remains deferred.
     * Native order: full construction, spawned(actor), count--, time, result. */
    if(payload_deliver(r,&ticket.spawner,"spawned",&payload,1,line)<0)goto cancel;
    if(!actor_spawn_finish(r,&ticket,1,line))return 0;
    if(!actor_spawn_context(r))return 0;
    alive=state_entity_live(r,&ticket.spawner);
    if(!actor_spawn_context(r))return 0;
    if(alive!=1)return rfail(r,line,"actor-spawner finalization retired its spawner");
    alive=state_entity_live(r,&ticket.actor);
    if(!actor_spawn_context(r))return 0;
    if(alive!=1)return rfail(r,line,"actor-spawner finalization retired its actor");
    result=payload;*out=result;return 1;
  cancel:
    (void)actor_spawn_finish(r,&ticket,0,line);
    return 0;
}

/* Private VM callback scope. Independent of the expired prepare grant and of
 * executing-task liveness: endon(spawned) may retire that task before finish. */
uint64_t gsc_actor_spawn_finish_program(const GscRuntime*r,const GscActorSpawnTicket*t,int published){
 if(!r||!t||!r->actor_spawn_finishing||(published!=0&&published!=1)||
    r->actor_spawn_finish_published!=published||memcmp(t,r->actor_spawn_finishing,sizeof(*t)))return 0;
 return gsc_runtime_program_identity(r);
}

uint64_t gsc_actor_spawn_prepare_program(const GscRuntime*r,const GscActorSpawnRequest*q){
 if(!r||!q||!r->actor_spawn_preparing_request||r->error[0]||!r->actor_spawn_preparing||
    r->executing_task!=r->actor_spawn_preparing||
    r->actor_spawn_preparing->serial!=r->actor_spawn_preparing_serial||
    memcmp(q,r->actor_spawn_preparing_request,sizeof(*q)))return 0;
 return gsc_runtime_program_identity(r);
}

uint64_t gsc_actor_spawn_commit_program(const GscRuntime*r,const GscActorSpawnTicket*t){
 return r&&!r->error[0]?gsc_actor_spawn_finish_program(r,t,1):0;
}
