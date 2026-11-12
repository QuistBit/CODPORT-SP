/* Dedicated two-catalog commands. Ordinals are copied ATR declarations, never
 * native tree IDs. Effects and continuation events belong to the real host. */
int gsc_program_select_r232_turret_animations(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||
       !g->anim_data_selected||!g->names_syntax||!g->scalar_switch_syntax||
       g->animation_namespace_selected||g->animation_refs_selected||
       g->animation_length_selected||g->animation_methods_selected||g->start_origin_selected)return 0;
    g->turret_animations_selected=1;g->animation_namespace_selected=1;return 1;
}
int gsc_program_select_training_rigs(GscProgram *g){
    if(!g||!g->turret_animations_selected||g->module_count||g->linked||g->invalid||g->training_rigs_selected)return 0;
    g->training_rigs_selected=1;return 1;
}
int gsc_runtime_select_training_trees(GscRuntime *r,const GscTurretAnimationSelection *input,const GscAnimationCatalog *gear,const GscAnimationCatalog *wire){
    GscTurretAnimationSelection copy;GscAnimationNamespaceSelection ns;
    if(!input||!animation_namespace_quiescent(r)||!r->program->turret_animations_selected||
       r->turret_animations_enabled||runtime_private_overlap(r,input,sizeof(*input)))return 0;
    copy=*input;if(!copy.host.validate||!copy.host.invoke)return 0;
    memset(&ns,0,sizeof(ns));ns.catalogs[0]=copy.catalogs[0];ns.catalogs[1]=copy.catalogs[1];ns.map_certificate=copy.map_certificate;
    if((gear!=NULL)!=r->program->training_rigs_selected||(wire!=NULL)!=(gear!=NULL))return 0;
    ns.catalogs[2]=gear;
    if(!animation_namespace_select(r,&ns,wire))return 0;
    if(memcmp(input,&copy,sizeof(copy)))return rfail(r,0,"turret animation selection changed during certificate callback");
    r->turret_animations=copy.host;r->turret_animations_enabled=1;return 1;
}
typedef struct {
    GscRuntime *runtime;Task *task;uint64_t serial,instance,catalog;int now,checking;
    GscTurretAnimationHost host;GscEntityRef entity;
} R232TurretAnimationLease;
static int turret_animation_scope(const R232TurretAnimationLease *l) {
    GscRuntime *r=l->runtime;
    return r&&!r->error[0]&&r->turret_animations_enabled&&r->entity_method_busy&&
        r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&
        animation_namespace_scope(r)&&r->now_ms==l->now&&r->executing_task==l->task&&
        l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&
        !memcmp(&r->turret_animations,&l->host,sizeof(l->host));
}
static int turret_animation_current(void *context) {
    R232TurretAnimationLease *l=context;GscRuntime *r=l->runtime;int ok;
    if(!turret_animation_scope(l))return 0;
    if(l->checking)return rfail(r,0,"turret animation current callback reentered");
    l->checking=1;
    ok=entity_task_current(r)&&turret_animation_scope(l)&&
        anim_data_current(r,l->task)&&turret_animation_scope(l)&&
        state_entity_live(r,&l->entity)==1&&turret_animation_scope(l)&&
        entity_task_current(r)&&turret_animation_scope(l);
    l->checking=0;return ok;
}
static int turret_animation_invoke(GscRuntime *r,const Value *receiver,int descriptor,
    const Value *args,int argc,int line) {
    GscTurretAnimationRequest q,original;R232TurretAnimationLease lease;
    GscTurretAnimationHost host;GscEntityRef borrowed;char error[384];int method,tree=-1,n,ok;
    method=turret_animation_kind(descriptor);
    if(!r->turret_animations_enabled||!method||!receiver||receiver->kind!=V_ENTITY||
       !entity_method_argc_valid(descriptor,argc)||(argc&&!args))return rfail(r,line,"turret animation receiver/capability/arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"turret animation callback reentered");
    memset(&q,0,sizeof(q));q.method=method;q.entity=receiver->entity;q.now_ms=r->now_ms;
    q.precision_bits=53;q.rounding=GSC_FLOAT_ROUND_NEAREST;
    q.catalog_identity=r->animation_namespace.identity;q.animation.runtime_identity=r->array_token;
    if(method!=GSC_ANIMATION_STOP_USE_TREE) {
        if(!animation_namespace_value_valid(r,&args[0])||
           args[0].kind!=(method==GSC_ANIMATION_USE_TREE?V_ANIMATION_TREE:V_NAMESPACED_ANIMATION))
            return rfail(r,line,"turret animation requires a current typed tree/animation value");
        if(method==GSC_ANIMATION_USE_TREE)tree=(int)args[0].object-1;
        else {
            const AnimationNamespaceEntry *entry=&r->animation_namespace.entries[args[0].object-1];
            tree=(int)entry->catalog;q.animation.ordinal=entry->declaration.ordinal;q.animation.flags=entry->declaration.flags;
            strcpy(q.animation.name,entry->declaration.name);
        }
        if(tree<0||tree>=animation_namespace_count(r->program))return rfail(r,line,"turret animation catalog unavailable");
        strcpy(q.tree,r->animation_namespace.catalogs[tree].tree);
        if(method!=GSC_ANIMATION_USE_TREE)strcpy(q.animation.tree,q.tree);
    }
    if(method==GSC_ANIMATION_SET_KNOB_RESTART) {
        uint32_t *fields[3]={&q.weight_bits,&q.blend_bits,&q.rate_bits};
        q.weight_bits=UINT32_C(0x3f800000);q.blend_bits=UINT32_C(0x3e4ccccd);q.rate_bits=UINT32_C(0x3f800000);
        for(n=1;n<argc;n++)if(!animation_method_float(r,&args[n],line,fields[n-1]))return 0;
    }
    original=q;host=r->turret_animations;memset(&lease,0,sizeof(lease));lease.runtime=r;
    lease.task=r->executing_task;lease.serial=lease.task?lease.task->serial:0;
    lease.instance=r->array_token;lease.catalog=r->animation_namespace.identity;
    lease.now=r->now_ms;lease.host=host;lease.entity=q.entity;borrowed=q.entity;
    r->entity_method_busy=1;
    if(!turret_animation_current(&lease))goto retired;
    error[0]=0;ok=host.validate(host.user,&borrowed,lease.instance,lease.now,error,sizeof(error),turret_animation_current,&lease);error[sizeof(error)-1]=0;
    if(!turret_animation_current(&lease))goto retired;
    if(ok!=1||memcmp(&borrowed,&q.entity,sizeof(borrowed)))goto refused;
    error[0]=0;ok=host.invoke(host.user,&q,error,sizeof(error),turret_animation_current,&lease);error[sizeof(error)-1]=0;
    if(!turret_animation_current(&lease))goto retired;
    if(ok!=1||memcmp(&q,&original,sizeof(q)))goto refused;
    r->entity_method_busy=0;return 1;
retired:
    r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"turret animation task/entity/resources retired");
refused:
    r->entity_method_busy=0;return rfail(r,line,"turret animation host refused or changed request: %s",error);
}

int gsc_runtime_select_r232_turret_animations(GscRuntime *r,const GscTurretAnimationSelection *input) {
 return gsc_runtime_select_training_trees(r,input,NULL,NULL);
}
