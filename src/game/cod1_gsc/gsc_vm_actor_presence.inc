/* R191 explicitly selected global isAI(value). Old R98 predicates are unchanged.
 * Native reads argument0 only. 1..MAX_PARAMS are admitted; expressions for all
 * supplied operands still execute normally. Missing-argument getter is held. */
int gsc_program_select_actor_presence(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->actor_presence_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->actor_presence_selected=1;return 1;
}
int gsc_runtime_select_actor_presence(GscRuntime *r,const GscActorPresenceHost *host) {
    if(!r||!host||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->actor_presence_enabled||!r->program->actor_presence_selected||!r->entities_enabled||
       !r->state_enabled||!r->names_enabled||!r->scalar_switch_enabled||!r->results_enabled||
       r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->actor_presence_busy||
       !host->read||!host->current||runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->actor_presence=*host;r->actor_presence_enabled=1;return 1;
}
static int actor_presence_expression(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    GscEntityRef owner,borrowed;char error[256];uint64_t serial=0;
    int alive,accepted,current=0;
    if(!r->actor_presence_enabled||!args||!out||argc<1||argc>MAX_PARAMS)
        return rfail(r,line,"invalid isAI binding or arity");
    if(r->actor_presence_busy||r->entity_method_busy||r->entity_field_busy||
       r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"isAI callback reentered");
    /* Original type gates reject non-entity values before the entity getter. */
    if(args[0].kind!=V_ENTITY){scalar_integer(out,0);return 1;}
    owner=args[0].entity;r->actor_presence_busy=1;
    alive=state_entity_live(r,&owner);
    if(alive<0||r->error[0])goto failed;
    if(!entity_task_current(r))goto cancelled;
    if(!alive){scalar_integer(out,0);r->actor_presence_busy=0;return 1;}
    borrowed=owner;error[0]=0;
    accepted=r->actor_presence.read(r->actor_presence.user,&borrowed,&serial,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(r->error[0])goto failed;
    if(!entity_task_current(r))goto cancelled;
    if(accepted!=1){r->actor_presence_busy=0;return rfail(r,line,"isAI host refused: %s",error);}
    alive=state_entity_live(r,&owner);
    if(alive<0||r->error[0])goto failed;
    if(!entity_task_current(r))goto cancelled;
    if(alive){
        borrowed=owner;current=r->actor_presence.current(r->actor_presence.user,&borrowed,serial);
        if(r->error[0])goto failed;
        if(!entity_task_current(r))goto cancelled;
        if(current!=0&&current!=1){r->actor_presence_busy=0;return rfail(r,line,"isAI invalid observation certificate");}
        alive=state_entity_live(r,&owner);
        if(alive<0||r->error[0])goto failed;
        if(!entity_task_current(r))goto cancelled;
    }
    scalar_integer(out,alive&&current&&serial!=0);r->actor_presence_busy=0;return 1;
cancelled:
    r->actor_presence_busy=0;return !r->error[0];
failed:
    r->actor_presence_busy=0;return 0;
}
