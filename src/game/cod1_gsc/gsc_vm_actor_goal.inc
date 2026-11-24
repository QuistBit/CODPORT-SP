int gsc_program_select_r222_actor_goal(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->actor_goal_selected||!g->node_queries_selected)return 0;
    g->actor_goal_selected=1;return 1;
}
int gsc_program_select_map_actor_goal(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->actor_goal_selected!=2||g->map_actor_goal_selected||
    !g->map_actor_animations_selected||!g->map_animation_names.count)return 0;
 g->map_actor_goal_selected=1;return 1;
}
int gsc_program_select_entity_goal(GscProgram *g){
    if(!g||g->invalid||g->linked||g->module_count||g->actor_goal_selected!=1)return 0;
    g->actor_goal_selected=2;return 1;
}
int gsc_runtime_select_r222_actor_goal(GscRuntime *r,const GscActorGoalHost *h) {
    if(!r||!h||!h->invoke||r->started||r->advancing||r->error[0]||r->actor_goal_enabled||
       !r->program->actor_goal_selected||!r->node_queries_enabled||!r->entities_enabled||
       runtime_private_overlap(r,h,sizeof(*h)))return 0;
    r->actor_goal=*h;r->actor_goal_enabled=1;return 1;
}
static int actor_goal_current(void *p){GscRuntime *r=p;return r&&!r->error[0]&&entity_task_current(r);}
static int actor_goal_invoke(GscRuntime *r,int descriptor,const Value *receiver,const Value *args,int argc,int line) {
    GscActorGoalRequest q,before;int alive,ok,finished;char error[256];
    if(!r->actor_goal_enabled||!receiver||receiver->kind!=V_ENTITY||argc!=1||!args||args[0].kind!=(descriptor==503?V_NODE:descriptor==COD1_SET_GOAL_ENTITY_METHOD?V_ENTITY:V_VECTOR))
        return rfail(r,line,"actor goal requires its selected host and the original argument type");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"setgoalnode callback reentered");
    memset(&q,0,sizeof(q));q.entity=receiver->entity;q.now_ms=r->now_ms;
    if(descriptor==503){
        if(!node_value_live(r,args,line,&alive))return 0;
        if(!alive)return rfail(r,line,"setgoalnode requires a live authored node");
        q.node=args[0].node;
    }else if(descriptor==COD1_SET_GOAL_ENTITY_METHOD){
        q.positional=2;q.target=args[0].entity;
    }else{
        unsigned i;
        for(i=0;i<3;i++)if(!isfinite(args[0].vector[i]))return rfail(r,line,"setgoalpos requires a finite vector");
        q.positional=1;memcpy(q.position,args[0].vector,sizeof(q.position));
    }
    before=q;
    r->entity_method_busy=1;finished=entity_method_finish(r,&q.entity,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    error[0]=0;ok=r->actor_goal.invoke(r->actor_goal.user,&q,error,sizeof(error),actor_goal_current,r);error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1||memcmp(&q,&before,sizeof(q))){r->entity_method_busy=0;return rfail(r,line,"%s refused: %s",descriptor==503?"setgoalnode":descriptor==COD1_SET_GOAL_ENTITY_METHOD?"setgoalentity":"setgoalpos",error[0]?error:"request changed or host failed");}
    finished=entity_method_finish(r,&before.entity,line);r->entity_method_busy=0;
    return finished<0?finished==-2:1;
}

/* Sample the actual VM-owned fields only at the movement owner's three
 * selection points. No script callback, random draw, or field is created. */
int gsc_runtime_training_appearance(GscRuntime *r,const GscEntityRef *ref,int kind,GscTrainingAppearance *out){
 Value receiver;const Value *v;int n,h,end;
 GscTrainingAppearance result={0};
 if(!r || !ref || !out || !r->started || r->advancing || r->error[0] || !r->state_enabled ||
    !r->actor_goal_enabled || !r->program->actor_animations_selected || r->program->carbine_items_selected!=2 ||
    kind<1 || kind>3)return 0;
 memset(&receiver,0,sizeof(receiver));receiver.kind=V_ENTITY;receiver.entity=*ref;
 n=state_field_index(r,&receiver,kind==2?"anim_idleSet":"animplaybackrate");
 if(n<0)return 0;v=&r->state_fields[n].value;
 if(kind==2){
  if(v->kind!=V_STRING || !( !strcmp(v->str,"none") || !strcmp(v->str,"w") ||
     !strcmp(v->str,"a") || !strcmp(v->str,"b")))return 0;
  strcpy(result.idle,v->str);
 }else{
  if(v->kind!=V_NUMBER || !(v->number>=.8f && v->number<=1.2f))return 0;
  result.rate=v->number;
  if(kind==3){
   h=state_field_index(r,&receiver,"hasWeapon");end=state_field_index(r,&receiver,"anim_combatEndTime");
   result.rest=(h<0 || r->state_fields[h].value.kind==V_UNDEFINED ||
       (r->state_fields[h].value.kind==V_NUMBER && r->state_fields[h].value.number!=0)) &&
      (end<0 || r->state_fields[end].value.kind==V_UNDEFINED ||
       (r->state_fields[end].value.kind==V_NUMBER && r->state_fields[end].value.number<=r->now_ms));
  }
 }
 *out=result;return 1;
}
