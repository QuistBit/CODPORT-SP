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
/* Map Actor commands bind the existing native command implementation to
 * a genuine map namespace. This grants neither turret nor controller state. */
int gsc_program_select_map_actor_look(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||!g->animation_namespace_selected||
    !g->map_animation_names.count||strcmp(g->map_animation_names.names[0],"generic_human")||
    !g->map_actor_animations_selected||!g->native_animation_roots_selected||g->turret_animations_selected||g->actor_look_selected)return 0;
 g->actor_look_selected=g->map_actor_look_selected=1;return 1;
}
int gsc_runtime_select_map_actor_look(GscRuntime*r,const GscActorLookHost*input){
 GscActorLookHost h;
 if(!input||!animation_namespace_quiescent(r)||!r->program->map_actor_look_selected||
    !r->program->actor_look_selected||!r->program->map_animation_names.count||r->turret_animations_enabled||
    !animation_namespace_scope(r)||r->actor_look_enabled||runtime_private_overlap(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.validate||!h.invoke||!anim_data_current(r,NULL)||memcmp(input,&h,sizeof(h)))return 0;
 r->actor_look=h;r->actor_look_enabled=r->map_actor_look_enabled=1;return 1;
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
    if(!actor_look_scope(l)||!r243_source_runtime_current(r))return 0;
    if(l->checking)return rfail(r,0,"actor animation current callback reentered");l->checking=1;
    ok=entity_task_current(r)&&actor_look_scope(l);
    if(ok&&r->readonly_context.frame_active&&readonly_context_selected(r,l->task)){
        int alive=actor_readonly_entity_live(r,&l->entity)&&actor_look_scope(l)&&
            (!l->has_argument||actor_readonly_entity_live(r,&l->argument));
        ok=entity_task_current(r)&&actor_look_scope(l)&&alive;
    }else if(ok)ok=anim_data_current(r,l->task)&&actor_look_scope(l)&&
        state_entity_live(r,&l->entity)==1&&actor_look_scope(l)&&
        (!l->has_argument||state_entity_live(r,&l->argument)==1)&&actor_look_scope(l)&&entity_task_current(r)&&actor_look_scope(l);
    l->checking=0;return ok&&r243_source_runtime_current(r);
}
#include "gsc_vm_r402_source_look.inc"
static int actor_look_invoke(GscRuntime *r,const Value *receiver,int descriptor,const Value *args,int argc,int line,Value *out){
    GscActorLookRequest q,original;GscActorLookReply reply;R232ActorLookLease l;GscActorLookHost host;GscEntityRef borrowed;
    R402SourceLookDispatch dispatch;int source=0;
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
    if(kind==GSC_ACTOR_TRAVERSE_MODE){
        if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||
           (size_t)(end-args[0].str)>=sizeof(q.tag))return rfail(r,line,"traverseMode requires a bounded string");
        memcpy(q.tag,args[0].str,(size_t)(end-args[0].str));
    }
    if(kind==GSC_ACTOR_LOOK_AIM_AT_POS){
        if(args[0].kind!=V_VECTOR)return rfail(r,line,"aimAtPos requires a target vector");
        memcpy(q.vector_bits,args[0].vector,12);
        for(i=0;i<3;i++)if((q.vector_bits[i]&0x7f800000u)==0x7f800000u)return rfail(r,line,"aimAtPos requires a finite vector");
    }
    if(kind==GSC_ACTOR_LOOK_CHECK_PRONE){
        int32_t already;
        if(args[0].kind!=V_VECTOR)return rfail(r,line,"checkProne requires origin vector");
        memcpy(q.vector_bits,args[0].vector,12);
        for(i=0;i<3;i++)if((q.vector_bits[i]&0x7f800000u)==0x7f800000u)return rfail(r,line,"checkProne origin must be finite");
        if(!actor_animation_float(r,&args[1],line,1,&q.scalar_bits[0])||!scalar_get_integer(r,&args[2],line,&already))return 0;
        memcpy(q.scalar_bits+1,&already,4);
    }
    if(kind==GSC_ACTOR_LOOK_MAY_MOVE){
        if(args[0].kind!=V_VECTOR)return rfail(r,line,"mayMoveToPoint requires a target vector");
        memcpy(q.vector_bits,args[0].vector,12);
        for(i=0;i<3;i++)if((q.vector_bits[i]&0x7f800000u)==0x7f800000u)return rfail(r,line,"mayMoveToPoint requires a finite vector");
    }
    if(kind==GSC_ACTOR_LOOK_CAN_SHOOT){
        if(args[0].kind!=V_VECTOR||(argc>1&&args[1].kind!=V_VECTOR))return rfail(r,line,"canShoot requires target and optional offset vectors");
        memcpy(q.vector_bits,args[0].vector,12);
        if(argc>1)memcpy(q.scalar_bits,args[1].vector,12);
        for(i=0;i<3;i++)if((q.vector_bits[i]&0x7f800000u)==0x7f800000u||(q.scalar_bits[i]&0x7f800000u)==0x7f800000u)return rfail(r,line,"canShoot requires finite vectors");
    }
    if(kind==GSC_ACTOR_LOOK_SHOOT){
        q.scalar_bits[0]=0x3f800000u;
        if(argc==1&&!actor_animation_float(r,&args[0],line,1,&q.scalar_bits[0]))return 0;
        if(argc>=2){
            if(args[1].kind!=V_VECTOR)return rfail(r,line,"shoot argument 1 must be a vector");
            q.tag[0]=1;memcpy(q.vector_bits,args[1].vector,12);
            for(i=0;i<3;i++)if((q.vector_bits[i]&0x7f800000u)==0x7f800000u)return rfail(r,line,"shoot requires a finite target vector");
        }
    }
    original=q;host=r->actor_look;memset(&l,0,sizeof(l));l.runtime=r;l.task=r->executing_task;l.serial=l.task?l.task->serial:0;
    l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.now=r->now_ms;l.entity=q.entity;l.argument=q.argument;l.has_argument=kind==GSC_ACTOR_LOOK_CAN_SEE;l.host=host;
    borrowed=q.entity;r->entity_method_busy=1;
    if(!actor_look_current(&l))goto retired;
    if(kind==GSC_ACTOR_LOOK_CAN_SEE&&(state_entity_live(r,&q.argument)!=1||!actor_look_current(&l)))goto retired;
    error[0]=0;ok=host.validate(host.user,&borrowed,l.instance,l.now,error,sizeof(error),actor_look_current,&l);error[sizeof(error)-1]=0;
    if(!actor_look_current(&l))goto retired;if(ok!=1||memcmp(&borrowed,&q.entity,sizeof(borrowed)))goto refused;
    source=r->r243_source_step_active&&kind==GSC_ACTOR_LOOK_AIM_AT_POS;
    if(source&&!r402_source_begin(r,&l,&q,&dispatch))goto retired;
    error[0]=0;ok=host.invoke(host.user,&q,&reply,error,sizeof(error),actor_look_current,&l);error[sizeof(error)-1]=0;
    if(!actor_look_current(&l))goto retired;if(ok!=1||memcmp(&q,&original,sizeof(q)))goto refused;
    if(kind==GSC_ACTOR_LOOK_CAN_SEE||kind==GSC_ACTOR_LOOK_CAN_SHOOT||kind==GSC_ACTOR_LOOK_MAY_MOVE||kind==GSC_ACTOR_LOOK_CHECK_PRONE){
        if(reply.kind!=GSC_ACTOR_LOOK_RESULT_BOOL||(reply.boolean!=0&&reply.boolean!=1)||reply.vector_bits[0]||reply.vector_bits[1]||reply.vector_bits[2]||(kind==GSC_ACTOR_LOOK_CAN_SEE&&state_entity_live(r,&q.argument)!=1)||!actor_look_current(&l))goto invalid;
        scalar_integer(&value,reply.boolean);
    }else if(kind==GSC_ACTOR_LOOK_AIM_AT_POS){
        if(reply.kind!=GSC_ACTOR_LOOK_RESULT_FLOAT||reply.boolean||reply.vector_bits[0]!=0x3dcccccdu||reply.vector_bits[1]||reply.vector_bits[2])goto invalid;
        value.kind=V_NUMBER;memcpy(&value.number,reply.vector_bits,4);
    }else if(actor_look_tag(kind)||kind==GSC_ACTOR_NEGOTIATION_END){
        if(reply.kind!=GSC_ACTOR_LOOK_RESULT_VECTOR||reply.boolean)goto invalid;
        for(i=0;i<3;i++)if((reply.vector_bits[i]&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))goto invalid;
        value.kind=V_VECTOR;memcpy(value.vector,reply.vector_bits,12);
    }else if(reply.kind!=GSC_ACTOR_LOOK_RESULT_VOID||reply.boolean||reply.vector_bits[0]||reply.vector_bits[1]||reply.vector_bits[2])goto invalid;
    if(!actor_look_current(&l))goto retired;
    if(source){int accepted=r402_source_end(r,&dispatch,1);source=0;if(!accepted)goto refused;}
    r->entity_method_busy=0;if(out)*out=value;return 1;
invalid:if(source&&r->r402_source_dispatch==&dispatch)r402_source_end(r,&dispatch,0);r->entity_method_busy=0;return rfail(r,line,"actor look returned invalid typed reply or stale target");
retired:if(source&&r->r402_source_dispatch==&dispatch)r402_source_end(r,&dispatch,0);r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"actor look task/entity/resources retired");
refused:if(source&&r->r402_source_dispatch==&dispatch)r402_source_end(r,&dispatch,0);r->entity_method_busy=0;return rfail(r,line,"actor look host refused or changed request: %s",error);
}
