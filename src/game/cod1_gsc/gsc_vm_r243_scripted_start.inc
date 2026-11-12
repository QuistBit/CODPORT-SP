/* Explicit six-value normal start used by whole original scripted::main.
 * Native optional/deathplant/non-AI paths are not silently admitted. */
int gsc_program_select_r243_scripted_start(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||!g->r243_scripted_prefix_selected||
    !g->r243_script_state_selected||g->r243_scripted_start_selected)return 0;
 g->r243_scripted_start_selected=1;return 1;
}
int gsc_runtime_select_r243_scripted_start(GscRuntime*r,const GscActorScriptedStartHost*input){
 GscActorScriptedStartHost host;
 if(!r||!input||!animation_namespace_quiescent(r)||!r->program->r243_scripted_start_selected||
    !r->r243_scripted_prefix_enabled||!r->r243_script_state_enabled||r->r243_scripted_start_enabled||
    runtime_private_overlap(r,input,sizeof(*input)))return 0;
 host=*input;if(!host.invoke||!animation_namespace_scope(r)||!anim_data_current(r,NULL))return 0;
 r->r243_scripted_start=host;r->r243_scripted_start_enabled=1;return 1;
}
typedef struct {
 GscRuntime*r;Task*task;const GscProgram*program;uint64_t serial,instance,catalog,time_root;
 GscActorScriptedStartHost host;GscActorScriptedPrefixRequest q;
 const GscActorScriptedPrefixRequest*input;
 GscEntityExecutionCertificate certificate;GscMapExecutionCertificate map,animationMap;
 int now,bucket,*budget,budget_value,checking;
} R243ScriptedStartLease;
static int r243_scripted_start_scope(const R243ScriptedStartLease*l){
 GscRuntime*r=l?l->r:NULL;
 return r&&r237_owner_current(r,l->time_root)&&!r->error[0]&&r->advancing&&r->entity_method_busy&&r->r243_scripted_start_enabled&&
  r->program==l->program&&r->executing_task==l->task&&l->task->serial==l->serial&&l->task->status==TASK_READY&&
  r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&r->now_ms==l->now&&
  r237_bucket(r)==l->bucket&&r->execution_budget==l->budget&&l->budget&&*l->budget==l->budget_value&&
  !memcmp(&r->r243_scripted_start,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->q,sizeof(l->q))&&
  !memcmp(&l->task->certificate,&l->certificate,sizeof(l->certificate))&&
  !memcmp(&l->task->map_certificate,&l->map,sizeof(l->map))&&!memcmp(&r->anim_data,&l->animationMap,sizeof(l->animationMap));
}
static int r243_scripted_start_current(void*context){
 R243ScriptedStartLease*l=context;int ok;if(!r243_scripted_start_scope(l))return 0;
 if(l->checking)return rfail(l->r,0,"scripted start current reentered");
 l->checking=1;ok=entity_task_current(l->r)&&r243_scripted_start_scope(l)&&
  anim_data_current(l->r,l->task)&&r243_scripted_start_scope(l)&&animation_namespace_scope(l->r)&&
  state_entity_live(l->r,&l->q.entity)==1&&r243_scripted_start_scope(l);l->checking=0;return ok;
}
static int r243_scripted_start_invoke(GscRuntime*r,const Value*receiver,const Value*args,int argc,int line){
 Value owned[6];GscActorScriptedPrefixRequest q;R243ScriptedStartLease l;Task*task;
 const char*end;char error[384];int i,accepted,ok=0;
 if(!r||!r->r243_scripted_start_enabled||!receiver||receiver->kind!=V_ENTITY||argc!=6||!args||
    !(task=r->executing_task)||task->status!=TASK_READY||!r->execution_budget)
  return r?rfail(r,line,"selected normal startscriptedanim requires six real typed arguments"):0;
 if(r->entity_method_busy||r->entity_field_busy||r->entity_prepare_busy||r->entity_certificate_busy||
    r->entity_inheritance_busy||r->entity_sequence_busy||task->host_entry_pins==UINT_MAX)
  return rfail(r,line,"normal scripted start guarded entry unavailable");
 memcpy(owned,args,sizeof(owned));memset(&q,0,sizeof(q));q.entity=receiver->entity;
 q.runtime_identity=r->array_token;q.catalog_identity=r->animation_namespace.identity;q.now_ms=r->now_ms;q.argc=6;q.root_present=1;
 if(owned[0].kind!=V_STRING||!(end=memchr(owned[0].str,0,sizeof(owned[0].str)))||
    end==owned[0].str||(size_t)(end-owned[0].str)>=sizeof(q.notify)||
    owned[1].kind!=V_VECTOR||owned[2].kind!=V_VECTOR||owned[4].kind!=V_STRING||
    !memchr(owned[4].str,0,sizeof(owned[4].str))||strcmp(owned[4].str,"normal"))
  return rfail(r,line,"normal scripted start typed notify/mode/vectors unavailable");
 memcpy(q.notify,owned[0].str,(size_t)(end-owned[0].str));
 for(i=0;i<3;i++){
  if(!isfinite(owned[1].vector[i])||!isfinite(owned[2].vector[i]))return rfail(r,line,"scripted start finite vector required");
  memcpy(q.origin_bits+i,owned[1].vector+i,4);memcpy(q.angles_bits+i,owned[2].vector+i,4);
 }
 ++task->host_entry_pins;
 if(!entity_task_current(r)||!actor_animation_binding(r,owned+3,line,&q.animation)||
    !actor_animation_binding(r,owned+5,line,&q.root))goto done;
 if(strcmp(q.animation.tree,"generic_human")||strcmp(q.root.tree,q.animation.tree)){
  rfail(r,line,"scripted start selected tree mismatch");goto done;}
 memset(&l,0,sizeof(l));l.r=r;l.task=task;l.program=r->program;l.serial=task->serial;
 l.time_root=r237_owner_stamp(r);l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.host=r->r243_scripted_start;
 l.q=q;l.input=&q;l.certificate=task->certificate;l.map=task->map_certificate;l.animationMap=r->anim_data;
 l.now=r->now_ms;l.bucket=r237_bucket(r);l.budget=r->execution_budget;l.budget_value=*l.budget;
 error[0]=0;r->entity_method_busy=1;
 if(!r243_scripted_start_current(&l))goto retired;
 accepted=l.host.invoke(l.host.user,&q,error,sizeof(error),r243_scripted_start_current,&l);error[sizeof(error)-1]=0;
 if(!r243_scripted_start_current(&l))goto retired;
 r->entity_method_busy=0;
 if(accepted!=1){rfail(r,line,"native normal scripted start refused: %s",error);goto done;}
 ok=1;goto done;
retired:
 r->entity_method_busy=0;if(!r->error[0])rfail(r,line,"scripted start owner/context retired; actual effects retained");
done:
 --task->host_entry_pins;return ok;
}
