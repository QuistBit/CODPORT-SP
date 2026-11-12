/* Explicit bounded method capability; original-source MAIN transport stays held. */
int R243ProgramSelectFlaggedAll(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||!g->actor_animations_selected||!g->native_animation_roots_selected||g->flagged_all_selected)return 0;
 g->flagged_all_selected=1;return 1;
}
int R243RuntimeSelectFlaggedAll(GscRuntime*r){
 if(!animation_namespace_quiescent(r)||!r->program->flagged_all_selected||!r->program->native_animation_roots_selected||!r->actor_animations_enabled||!animation_namespace_scope(r)||r->flagged_all_enabled||!anim_data_current(r,NULL))return 0;
 r->flagged_all_enabled=1;return 1;
}
/* Native gamex86 200379E0 -> 200377D0. Selection does not supply playback. */
int gsc_program_select_r233_flagged_all_restart(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||!g->actor_animations_selected||!g->native_animation_roots_selected||g->flagged_all_restart_selected)return 0;g->flagged_all_restart_selected=1;return 1;
}
int gsc_runtime_select_r233_flagged_all_restart(GscRuntime*r){
 if(!animation_namespace_quiescent(r)||!r->program->flagged_all_restart_selected||!r->program->native_animation_roots_selected||!r->actor_animations_enabled||!animation_namespace_scope(r)||r->flagged_all_restart_enabled||!anim_data_current(r,NULL))return 0;r->flagged_all_restart_enabled=1;return 1;
}
/* Explicit immediate-command capability. The VM supplies no prone physics,
 * tree state, notify completion or successful replacement implementation. */
int gsc_program_select_r232_actor_animations(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||
       g->actor_animations_selected)return 0;g->actor_animations_selected=1;return 1;
}
int gsc_runtime_select_r232_actor_animations(GscRuntime *r,const GscActorAnimationHost *input) {
    GscActorAnimationHost copy;
    if(!input||!animation_namespace_quiescent(r)||!r->program->actor_animations_selected||
       !r->turret_animations_enabled||!animation_namespace_scope(r)||r->actor_animations_enabled||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    copy=*input;if(!copy.validate||!copy.invoke||!anim_data_current(r,NULL)||memcmp(input,&copy,sizeof(copy)))return 0;
    r->actor_animations=copy;r->actor_animations_enabled=1;return 1;
}
typedef struct {
    GscRuntime *runtime;Task *task;uint64_t serial,instance,catalog;int now,checking,flagged_all;
    GscActorAnimationHost host;GscEntityRef entity;
} R232ActorAnimationLease;
static int actor_animation_scope(const R232ActorAnimationLease *l) {
    GscRuntime *r=l->runtime;
    return r&&!r->error[0]&&r->actor_animations_enabled&&(!l->flagged_all||(l->flagged_all==2?r->flagged_all_enabled:r->flagged_all_restart_enabled))&&r->entity_method_busy&&
        r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&animation_namespace_scope(r)&&
        r->now_ms==l->now&&r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&
        !memcmp(&r->actor_animations,&l->host,sizeof(l->host));
}
static int r243_source_runtime_current(GscRuntime*);
static int actor_animation_current(void *context) {
    R232ActorAnimationLease *l=context;GscRuntime *r=l->runtime;int ok;
    if(!actor_animation_scope(l)||!r243_source_runtime_current(r))return 0;
    if(l->checking)return rfail(r,0,"actor animation current callback reentered");l->checking=1;
    ok=entity_task_current(r)&&actor_animation_scope(l)&&anim_data_current(r,l->task)&&actor_animation_scope(l)&&
        state_entity_live(r,&l->entity)==1&&actor_animation_scope(l)&&entity_task_current(r)&&actor_animation_scope(l);
    l->checking=0;if(!r243_source_runtime_current(r))return 0;return ok;
}

/* Method12 owns an independent sealed call. Borrowed code receives only a
 * nonreused nonce; neither a copied lease nor its mutable flag is authority. */
typedef struct {
 R232ActorAnimationLease lease;
 GscActorAnimationRequest expected;const GscActorAnimationRequest *request;
 const GscProgram *program;Frame *frame;uint64_t programToken,timeRoot,nonce;
 int depth,fn,pc,checking,cancelled;
} R243CommandCall;
static R243CommandCall *r243CommandActive;
static uintptr_t r243CommandNonce;
static int r243_command_vm_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&r243CommandActive,sizeof(r243CommandActive))||
 Cod1ModelStringOverlap(p,n,&r243CommandNonce,sizeof(r243CommandNonce))||
 (r243CommandActive&&Cod1ModelStringOverlap(p,n,r243CommandActive,sizeof(*r243CommandActive)));
}
static int r243_command_scope(R243CommandCall*c){
 GscRuntime*r;Task*t;int i,found=0;
 if(!c||c!=r243CommandActive||c->cancelled||c->lease.flagged_all!=2||
 c->expected.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL||!c->request||memcmp(c->request,&c->expected,sizeof(c->expected)))return 0;
 r=c->lease.runtime;t=c->lease.task;
 if(!actor_animation_scope(&c->lease)||!r->program->flagged_all_selected||r->program!=c->program||
 r->program->function_program_token!=c->programToken||!r237_owner_current(r,c->timeRoot))return 0;
 for(i=0;i<r->task_count;i++)if(task_at(r,i)==t){found=1;break;}
 return found&&t->status==TASK_READY&&!t->context_retired&&t->depth==c->depth&&t->depth>0&&t->depth<=MAX_FRAMES&&
 &t->frames[t->depth-1]==c->frame&&c->frame->fn==c->fn&&c->frame->pc==c->pc;
}
static int r243_command_current(void*context){
 R243CommandCall*c=r243CommandActive;int ok;
 if(!c||(uintptr_t)context!=c->nonce||!r243_command_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;
 ok=actor_animation_current(&c->lease)&&r243_command_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243CommandValidationCurrent(const GscEntityRef*entity,uint64_t runtime,int now,int(*current)(void*),void*context){
 R243CommandCall*c=r243CommandActive;
 return c&&entity&&current==r243_command_current&&(uintptr_t)context==c->nonce&&
 runtime==c->expected.runtime_identity&&now==c->expected.now_ms&&
 !memcmp(entity,&c->expected.entity,sizeof(*entity))&&current(context)==1;
}
int R243CommandExactCurrent(const GscActorAnimationRequest*q,int(*current)(void*),void*context){
 R243CommandCall*c=r243CommandActive;
 return c&&q&&current==r243_command_current&&(uintptr_t)context==c->nonce&&
 !memcmp(q,&c->expected,sizeof(*q))&&current(context)==1;
}
#include "gsc_vm_r243_source_dispatch.inc"
static int animation_binding_limited(GscRuntime *r,const Value *v,int line,GscAnimationLengthRequest *out,unsigned count) {
    const AnimationNamespaceEntry *entry;
    if(!animation_namespace_value_valid(r,v)||v->kind!=V_NAMESPACED_ANIMATION)
        return rfail(r,line,"actor animation command requires a current typed animation");
    entry=&r->animation_namespace.entries[v->object-1];if(entry->catalog>=count)return rfail(r,line,"actor animation catalog outside selected namespace");
    memset(out,0,sizeof(*out));out->runtime_identity=r->array_token;out->ordinal=entry->declaration.ordinal;out->flags=entry->declaration.flags;
    strcpy(out->tree,r->animation_namespace.catalogs[entry->catalog].tree);strcpy(out->name,entry->declaration.name);return 1;
}
static int actor_animation_binding(GscRuntime *r,const Value *v,int line,GscAnimationLengthRequest *out){return animation_binding_limited(r,v,line,out,2);}
static int actor_animation_float(GscRuntime *r,const Value *v,int line,int signed_value,uint32_t *bits) {
    uint32_t magnitude;
    if(!float_arithmetic_number_bits(r,v,line,bits))return 0;magnitude=*bits&UINT32_C(0x7fffffff);
    if((!signed_value&&(*bits>>31))||((magnitude>>23)==255)||(magnitude&&!(magnitude>>23)))
        return rfail(r,line,"actor animation GetFloat requires finite normal values or zero in selected sign domain");
    return 1;
}
static int actor_animation_invoke(GscRuntime *r,const Value *receiver,int descriptor,const Value *args,int argc,int line) {
    GscActorAnimationRequest q,original;R232ActorAnimationLease l;GscActorAnimationHost host;GscEntityRef borrowed;R243CommandCall command;int guarded=0;int(*call_current)(void*)=actor_animation_current;void*call_context=&l;
    char error[384];int kind=actor_animation_kind(descriptor),i,offset,ok,source=0,orientError=0,orientHold=0;Value orientAngle;const char *end;R243SourceAnimationDispatch dispatch;
    if(!r->actor_animations_enabled||((kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART&&!r->flagged_all_restart_enabled)||(kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL&&!r->flagged_all_enabled))||!kind||!receiver||receiver->kind!=V_ENTITY||
       !entity_method_argc_valid(descriptor,argc)||(argc&&!args))return rfail(r,line,"actor animation command receiver/capability/arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"actor animation command reentered");
    memset(&q,0,sizeof(q));q.entity=receiver->entity;q.method=kind;q.now_ms=r->now_ms;
    q.runtime_identity=r->array_token;q.catalog_identity=r->animation_namespace.identity;q.precision_bits=53;q.rounding=0;
    if(kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL){
        q.scalar_bits[0]=0x3f800000;q.scalar_bits[1]=0x3e4ccccd;q.scalar_bits[2]=0x3f800000;
        /* Original getters: optional rate/blend/weight, root, target, notify. */
        for(i=argc-1;i>=3;i--)if(!actor_animation_float(r,args+i,line,0,q.scalar_bits+i-3))return 0;
        if(!(q.scalar_bits[0]&0x7fffffff))return rfail(r,line,"flagged all restart requires positive weight");
        if(!actor_animation_binding(r,args+2,line,q.animation+1)||!actor_animation_binding(r,args+1,line,q.animation))return 0;
        if(strcmp(q.animation[0].tree,q.animation[1].tree)||!(q.animation[1].flags&GSC_ANIMATION_NODE_BRANCH)||q.animation[0].ordinal==q.animation[1].ordinal)return rfail(r,line,"flagged all restart requires distinct target and real same-catalog branch root");
        if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||(size_t)(end-args[0].str)>=sizeof(q.notify))return rfail(r,line,"flagged all restart requires bounded nonempty notify string");
        memcpy(q.notify,args[0].str,(size_t)(end-args[0].str));
    }else if(kind==GSC_ACTOR_ANIMATION_UPDATE_PRONE){
        for(i=0;i<2;i++)if(!actor_animation_binding(r,args+i,line,q.animation+i))return 0;
        if(strcmp(q.animation[0].tree,q.animation[1].tree))return rfail(r,line,"UpdateProne requires two current same-catalog animations");
        for(i=0;i<3;i++)if(!actor_animation_float(r,args+i+2,line,0,q.scalar_bits+i))return 0;
    }else if(kind==GSC_ACTOR_ANIMATION_ORIENT_MODE){
        /* Classify/copy only, without errors or foreign callbacks. Values may
         * be released by Actor validation; nothing below reads args again. */
        if(r->r243_source_step_active)return rfail(r,line,"original-source OrientMode transport HOLD");
        memset(&orientAngle,0,sizeof(orientAngle));
        if(!argc)orientError=1;
        else if(args[0].kind!=V_STRING){
            if(args[0].kind==V_UNDEFINED||args[0].kind==V_LOCALIZED_STRING)orientError=1;
            else orientHold=1;
        }else if(!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||
                 (size_t)(end-args[0].str)>=sizeof(q.notify))orientHold=1;
        else{
            memcpy(q.notify,args[0].str,(size_t)(end-args[0].str));
            if(!strcmp(q.notify,"face angle")){
                if(argc<2)orientError=2;
                else if(args[1].kind==V_NUMBER)orientAngle=args[1];
                else if(args[1].kind==V_UNDEFINED||args[1].kind==V_VECTOR||args[1].kind==V_LOCALIZED_STRING)orientError=2;
                else orientHold=2;
                if(orientError==2)q.method=GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX;
            }else if(strcmp(q.notify,"face default"))orientHold=1;
        }
    }else if(kind==GSC_ACTOR_ANIMATION_ANIM_MODE){
        if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||
           (size_t)(end-args[0].str)>=sizeof(q.notify))return rfail(r,line,"animation mode requires a bounded nonempty mode string");
        memcpy(q.notify,args[0].str,(size_t)(end-args[0].str));
    }else if(kind==GSC_ACTOR_ANIMATION_KNOB_ALL){
        if(!actor_animation_binding(r,&args[0],line,&q.animation[0])||!actor_animation_binding(r,&args[1],line,&q.animation[1]))return 0;
        if(strcmp(q.animation[0].tree,q.animation[1].tree)||!(q.animation[1].flags&GSC_ANIMATION_NODE_BRANCH)||q.animation[0].ordinal==q.animation[1].ordinal)return rfail(r,line,"SetAnimKnobAll requires distinct target and real same-catalog branch root");
        q.scalar_bits[0]=0x3f800000;q.scalar_bits[1]=0x3e4ccccd;q.scalar_bits[2]=0x3f800000;
        for(i=2;i<argc;i++)if(!actor_animation_float(r,&args[i],line,0,&q.scalar_bits[i-2]))return 0;
    }else if(kind==GSC_ACTOR_ANIMATION_PRONE_NODES){
        for(i=0;i<2;i++)if(!actor_animation_float(r,&args[i],line,1,&q.scalar_bits[i]))return 0;
        for(i=0;i<3;i++)if(!actor_animation_binding(r,&args[i+2],line,&q.animation[i]))return 0;
    }else if(kind==GSC_ACTOR_ANIMATION_ENTER_PRONE||kind==GSC_ACTOR_ANIMATION_EXIT_PRONE){
        if(!actor_animation_float(r,&args[0],line,0,&q.scalar_bits[0]))return 0;
    }else {
        offset=kind==GSC_ACTOR_ANIMATION_FLAGGED_RESTART?1:0;
        if(offset){
            if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||
               (size_t)(end-args[0].str)>=sizeof(q.notify))return rfail(r,line,"flagged animation requires a bounded nonempty notify string");
            memcpy(q.notify,args[0].str,(size_t)(end-args[0].str));
        }
        if(!actor_animation_binding(r,&args[offset],line,&q.animation[0]))return 0;
        if(kind==GSC_ACTOR_ANIMATION_CLEAR){if(!actor_animation_float(r,&args[1],line,0,&q.scalar_bits[0]))return 0;}
        else {
            q.scalar_bits[0]=UINT32_C(0x3f800000);q.scalar_bits[1]=UINT32_C(0x3e4ccccd);q.scalar_bits[2]=UINT32_C(0x3f800000);
            for(i=offset+1;i<argc;i++)if(!actor_animation_float(r,&args[i],line,0,&q.scalar_bits[i-offset-1]))return 0;
        }
    }
    /* All arguments/resource metadata are copied before any borrowed callback.
     * A cancelled task may release its argument arena during that callback. */
    original=q;host=r->actor_animations;memset(&l,0,sizeof(l));l.runtime=r;l.task=r->executing_task;
    l.serial=l.task?l.task->serial:0;l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.now=r->now_ms;l.entity=q.entity;l.host=host;l.flagged_all=kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL?2:kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART;
    if(kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL){
      if(r243CommandActive||r243CommandNonce==UINTPTR_MAX||!l.task||l.task->depth<=0||l.task->depth>MAX_FRAMES)return rfail(r,line,"method12 sealed call unavailable");
      memset(&command,0,sizeof(command));command.lease=l;command.expected=q;command.request=&q;
      command.program=r->program;command.programToken=r->program->function_program_token;
      command.timeRoot=r237_owner_stamp(r);command.depth=l.task->depth;command.frame=&l.task->frames[l.task->depth-1];
      command.fn=command.frame->fn;command.pc=command.frame->pc;command.nonce=++r243CommandNonce;
      r243CommandActive=&command;guarded=1;call_current=r243_command_current;call_context=(void*)(uintptr_t)command.nonce;
    }
    borrowed=q.entity;r->entity_method_busy=1;
    if(!call_current(call_context))goto retired;
    error[0]=0;ok=host.validate(host.user,&borrowed,l.instance,l.now,error,sizeof(error),call_current,call_context);error[sizeof(error)-1]=0;
    if(!call_current(call_context))goto retired;
    if(ok!=1||memcmp(&borrowed,&q.entity,sizeof(borrowed)))goto refused;
    if(kind==GSC_ACTOR_ANIMATION_ORIENT_MODE){
        if(orientError==1){r->entity_method_busy=0;return rfail(r,line,"OrientMode GetConstString(0) failed");}
        if(orientHold){r->entity_method_busy=0;return rfail(r,line,orientHold==2?
            "OrientMode STRING GetFloat conversion HOLD (not a native getter failure)":
            "OrientMode GetConstString coercion or mode outside selected domain HOLD");}
        if(!orientError&&!strcmp(q.notify,"face angle")){
            if(!actor_animation_float(r,&orientAngle,line,1,q.scalar_bits)||
               (q.scalar_bits[0]&0x7fffffffu)>0x49742400u){r->entity_method_busy=0;return rfail(r,line,"face angle requires one finite bounded angle (port domain HOLD)");}
            original=q;
        }
    }
    source=r->r243_source_step_active;
    if(source&&kind==GSC_ACTOR_ANIMATION_FLAGGED_ALL){if(guarded)r243CommandActive=NULL;r->entity_method_busy=0;return rfail(r,line,"nonrestart flagged all original-source transport HOLD");}
    if(source&&!r243_source_begin(r,&l,&q,&dispatch))goto retired;
    error[0]=0;ok=host.invoke(host.user,&q,error,sizeof(error),call_current,call_context);error[sizeof(error)-1]=0;
    if(!call_current(call_context))goto retired;
    if(ok!=1||memcmp(&q,&original,sizeof(q)))goto refused;
    if(orientError==2){
        /* Host1 acknowledges only the two committed stores. The original
         * script call still fails; no angle setter or success continuation. */
        r->entity_method_busy=0;return rfail(r,line,"OrientMode GetFloat(1) failed after committed face-angle prefix");
    }
    if(source){int accepted=r243_source_end(r,&dispatch,1);source=0;if(!accepted)goto refused;}
    if(guarded)r243CommandActive=NULL;r->entity_method_busy=0;return 1;
retired:if(source&&r->r243_source_dispatch==&dispatch)r243_source_end(r,&dispatch,0);if(guarded)r243CommandActive=NULL;r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"actor animation task/entity/resources retired");
refused:if(source&&r->r243_source_dispatch==&dispatch)r243_source_end(r,&dispatch,0);if(guarded)r243CommandActive=NULL;r->entity_method_busy=0;return rfail(r,line,"actor animation host refused or changed command: slot=%u generation=%u method=%d ordinal=%u notify=%.63s precision=%d rounding=%d %s",original.entity.slot,original.entity.generation,(int)original.method,original.animation[0].ordinal,original.notify,original.precision_bits,original.rounding,error);
}
