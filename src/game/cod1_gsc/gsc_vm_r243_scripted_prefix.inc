#ifdef GAMEDLL
extern void __cdecl G_Printf(const char *,...);
#endif
/* Explicit selected fixed-source dispatch. Fixed original init dispatch after a genuine GAME native prefix.
 * No generic callback reentry exception and no exported animation Values. */
static int execute_entity_values_map_r243(GscRuntime*,const GscEntityScriptEntry*,
 const GscEntityExecutionCertificate*,const GscMapExecutionCertificate*,
 const Value*,int,GscEntityScriptRun*);
int gsc_program_select_r243_scripted_prefix(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||!g->function_admission_selected||
    !g->actor_animations_selected||g->r243_scripted_prefix_selected)return 0;
 g->r243_scripted_prefix_selected=1;return 1;
}
int gsc_runtime_select_r243_scripted_prefix(GscRuntime*r,const GscActorScriptedPrefixHost*input){
 GscActorScriptedPrefixHost host;int module,fn;
 if(!r||!input||!animation_namespace_quiescent(r)||!r->program->r243_scripted_prefix_selected||
    !r->r243_thread_start_enabled||!r->actor_animations_enabled||r->r243_scripted_prefix_enabled||
    runtime_private_overlap(r,input,sizeof(*input))||
    !gsc_runtime_r243_scripted_init_available(r))return 0;
 host=*input;if(!host.invoke||!animation_namespace_scope(r)||!anim_data_current(r,NULL)||
    memcmp(input,&host,sizeof(host)))return 0;
 module=find_module(r->program,"animscripts/scripted");fn=find_func(r->program,module,"init");
 r->r243_scripted_prefix=host;r->r243_scripted_init_fn=fn;r->r243_scripted_prefix_enabled=1;return 1;
}
typedef struct {
 GscRuntime*r;Task*caller;uint64_t serial,instance,catalog,time_root;const GscProgram*program;
 int now,bucket,*budget,budget_value,checking;
 GscActorScriptedPrefixHost host;GscActorScriptedPrefixRequest request;
 const GscActorScriptedPrefixRequest*input;
 GscEntityExecutionCertificate caller_certificate;
 GscMapExecutionCertificate caller_map,anim_map;
} R243ScriptedPrefixLease;
static int r243_scripted_prefix_scope(const R243ScriptedPrefixLease*l){
 const GscRuntime*r=l->r;
 return r&&r237_owner_current(r,l->time_root)&&!r->error[0]&&r->r243_scripted_prefix_enabled&&r->entity_method_busy&&
  r->advancing&&r->program==l->program&&r->executing_task==l->caller&&
  l->caller->serial==l->serial&&l->caller->status==TASK_READY&&
  r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&
  r->now_ms==l->now&&r237_bucket(r)==l->bucket&&
  r->execution_budget==l->budget&&l->budget&&*l->budget==l->budget_value&&
  !memcmp(&r->r243_scripted_prefix,&l->host,sizeof(l->host))&&
  !memcmp(l->input,&l->request,sizeof(l->request))&&
  !memcmp(&l->caller->certificate,&l->caller_certificate,sizeof(l->caller_certificate))&&
  !memcmp(&l->caller->map_certificate,&l->caller_map,sizeof(l->caller_map))&&
  !memcmp(&r->anim_data,&l->anim_map,sizeof(l->anim_map));
}
static int r243_scripted_prefix_current(void*opaque){
 R243ScriptedPrefixLease*l=opaque;GscRuntime*r=l->r;int ok;
 if(!r243_scripted_prefix_scope(l))return 0;
 if(l->checking)return rfail(r,0,"scripted prefix current callback reentered");
 l->checking=1;
 ok=entity_task_current(r)&&r243_scripted_prefix_scope(l)&&
    anim_data_current(r,l->caller)&&r243_scripted_prefix_scope(l)&&
    animation_namespace_scope(r)&&state_entity_live(r,&l->request.entity)==1&&
    r243_scripted_prefix_scope(l);
 l->checking=0;return ok;
}
static int r243_scripted_prefix_invoke(GscRuntime*r,const Value*receiver,
 const Value*args,int argc,int line){
 Value owned[6];GscActorScriptedPrefixRequest q;R243ScriptedPrefixLease l;
 GscEntityExecutionCertificate target;GscEntityScriptEntry entry;GscEntityScriptRun run;
 GscMapExecutionCertificate map;Task*caller;char why[384];const char*end;
 int i,ok=0,accepted,admitted=-1,module,capacity,isModel=0;
 if(!r)return 0;
 if(!r->r243_scripted_prefix_enabled||!receiver||receiver->kind!=V_ENTITY||
    argc<4||argc>6||!args||!r->advancing||!(caller=r->executing_task)||
    !r->execution_budget||caller->status!=TASK_READY)
  return rfail(r,line,"selected fresh AI animscripted requires four to six typed arguments");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
    caller->host_entry_pins==UINT_MAX||r->entity_entry_depth>=32||
    *r->execution_budget<=0||r->next_serial==UINT64_MAX)
  return rfail(r,line,"scripted prefix guarded entry/budget unavailable");
 capacity=r->task_count<GSC_LIFETIME_TASK_CAPACITY;
 for(i=0;!capacity&&i<r->task_count;i++){
  Task*t=r->lifetime_tasks[i];
  if(t&&t!=caller&&t->status==TASK_DEAD&&!t->host_entry_pins&&!t->retained_owner_refs)capacity=1;
 }
 if(!capacity)return rfail(r,line,"scripted init task capacity unavailable before native prefix");
 memset(owned,0,sizeof(owned));memcpy(owned,args,(size_t)argc*sizeof(Value));
 memset(&q,0,sizeof(q));q.entity=receiver->entity;q.now_ms=r->now_ms;
 q.runtime_identity=r->array_token;q.catalog_identity=r->animation_namespace.identity;
 q.argc=(uint32_t)argc;q.root_present=argc==6;
 /* Present undefined options are invalid native typed getters. Only omitted
  * arguments remain actual undefined Values for original init's defaults. */
 if(owned[0].kind!=V_STRING||!(end=memchr(owned[0].str,0,sizeof(owned[0].str)))||
    (size_t)(end-owned[0].str)>=sizeof(q.notify)||owned[1].kind!=V_VECTOR||
    owned[2].kind!=V_VECTOR)return rfail(r,line,"animscripted requires notify string and two vectors");
 memcpy(q.notify,owned[0].str,(size_t)(end-owned[0].str));
 for(i=0;i<3;i++){
  if(!isfinite(owned[1].vector[i])||!isfinite(owned[2].vector[i]))
   return rfail(r,line,"animscripted vectors outside selected finite domain");
  memcpy(q.origin_bits+i,owned[1].vector+i,4);memcpy(q.angles_bits+i,owned[2].vector+i,4);
 }
 if(argc>4&&(owned[4].kind!=V_STRING||!memchr(owned[4].str,0,sizeof(owned[4].str))||strcmp(owned[4].str,"normal")))
  return rfail(r,line,"animscripted selected normal-only optional mode requires typed normal string");
 ++caller->host_entry_pins;
 if(!entity_task_current(r)||caller->status==TASK_DEAD)goto done;
 if(!animation_binding_limited(r,owned+3,line,&q.animation,animation_namespace_count(r->program))||
    (q.root_present&&!actor_animation_binding(r,owned+5,line,&q.root)))goto done;
 isModel=r->program->training_rigs_selected&&argc==4&&q.animation.ordinal==1&&!q.animation.flags&&
    ((!strcmp(q.animation.tree,"animation_rig_gear_trainingjump")&&!strcmp(q.animation.name,"training_gear_practicejump"))||
     (!strcmp(q.animation.tree,"animation_rig_wire_trainingjump")&&!strcmp(q.animation.name,"training_wire_practicejump")));
 if((!isModel&&strcmp(q.animation.tree,"generic_human"))||
    (q.root_present&&strcmp(q.root.tree,q.animation.tree))){rfail(r,line,"animscripted selected AI tree mismatch");goto done;}
 module=find_module(r->program,"animscripts/scripted");
 if(module<0||find_func(r->program,module,"init")!=r->r243_scripted_init_fn||
    !gsc_runtime_r243_scripted_init_available(r)){rfail(r,line,"original scripted init unavailable before native prefix");goto done;}
 if(!anim_data_current(r,caller))goto done;
 map=caller->map_certificate.receipt?caller->map_certificate:r->anim_data;
 if(!map.receipt||!map.current){rfail(r,line,"animscripted has no genuine map authority");goto done;}
 memset(&l,0,sizeof(l));l.r=r;l.caller=caller;l.serial=caller->serial;
 l.time_root=r237_owner_stamp(r);l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.program=r->program;
 l.now=r->now_ms;l.bucket=r237_bucket(r);l.budget=r->execution_budget;l.budget_value=*l.budget;
 l.host=r->r243_scripted_prefix;l.request=q;l.input=&q;
 l.caller_certificate=caller->certificate;l.caller_map=caller->map_certificate;l.anim_map=r->anim_data;
 memset(&target,0,sizeof(target));why[0]=0;r->entity_method_busy=1;
 if(!r243_scripted_prefix_current(&l))goto callback_retired;
 accepted=l.host.invoke(l.host.user,&q,&target,&admitted,why,sizeof(why),r243_scripted_prefix_current,&l);
 why[sizeof(why)-1]=0;
 if(!r243_scripted_prefix_current(&l))goto callback_retired;
 r->entity_method_busy=0;
 if(accepted!=1){rfail(r,line,"native scripted prefix refused: %s",why);goto done;}
 if(isModel){
  if(admitted!=2||target.serial||target.current){rfail(r,line,"script_model did not start its selected record");goto done;}
  ok=1;goto done;
 }
 if((admitted!=0&&admitted!=1)||!target.serial||!target.current||
    !state_ref_equal(&target.owner,&q.entity)){rfail(r,line,"scripted prefix returned invalid target/queue receipt");goto done;}
 /* Host callback is CLOSED before entering the fixed original source. Queue
  * admission0 deliberately takes this same path; no fabricated completion. */
 memset(&entry,0,sizeof(entry));entry.entity=q.entity;
 strcpy(entry.module,"animscripts/scripted");strcpy(entry.function,"init");
 if(!execute_entity_values_map_r243(r,&entry,&target,&map,owned,6,&run))goto done;
 if(run.status!=GSC_ENTITY_ENTRY_RETURNED){rfail(r,line,"original scripted init did not return after native prefix");goto done;}
#ifdef GAMEDLL
 G_Printf("[Foley private] original scripted init returned slot=%u gen=%u ordinal=%u admitted=%d argc=%d\n",q.entity.slot,q.entity.generation,q.animation.ordinal,admitted,q.argc);
#endif
 ok=1;goto done;
callback_retired:
 r->entity_method_busy=0;
 if(!r->error[0])rfail(r,line,"scripted native prefix context retired (committed prefix is preserved)");
done:
 --caller->host_entry_pins;return ok;
}
