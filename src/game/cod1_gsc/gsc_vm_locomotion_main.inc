/* Reuse the same successful linked-program closure proof already used by
 * push_frame. Runtime token, mask and function identity must still match;
 * controller, map, entity and callback checks below remain per-frame. */
static int locomotion_admission_current(GscRuntime*r,int fn,char*why,size_t size){
 int reusable;
 if(!r||!r->program)return 0;
 reusable=r->started&&r->program->linked&&r->function_admission_token&&
  r->function_admission_token==r->program->function_admission_token&&
  fn>=0&&fn<r->program->func_count&&r->function_admission[fn];
 if(!reusable||!r->verified_function_closure[fn]){
  if(!function_admission_runtime_current(r,fn,why,size))return 0;
  if(reusable)r->verified_function_closure[fn]=1;
 }
 return 1;
}
/* Owned ordinary animation MAIN. Keys name program-bound original sources;
 * they are not native AI categories or movement-ready flags. Existing frame,
 * map, entity, endon and retained-reference services remain authoritative. */
int gsc_runtime_locomotion_main(GscRuntime*r,const GscEntityExecutionCertificate*input,
 const GscMapExecutionCertificate*map,int source,GscEntityScriptRun*out){
 R243ControllerCall call;R243ControllerLease*l=&call.lease;R243MainCapture*c=&call.capture;
 R243ControllerMainOwner*owner=NULL;R243MainReplacement replacement={0};
 GscEntityExecutionCertificate cert;GscEntityScriptEntry entry={0};GscEntityScriptRun run={0};
 GscActorControllerSelection selected={0};GscMainReplacementService service={0};
 int i,module,fn,ok=0;char why[384];
 if(!r||!input||!map||!out||(source<0||source>2)||!r->r243_controller_enabled||
    r->r243_controller_busy||r->r243_controller_capture||r237_busy(r)||!r->started||r->error[0]||
    r->advancing||r->executing_task||r->entity_entry_depth||r->entity_sequence_busy||
    !r237_owner_current(r,r->r237_frame_time_root)||r->r237_frame_open||r237_bucket(r)!=r->now_ms||
    !gsc_runtime_r243_controller_storage_separate(r,input,sizeof(*input))||
    !gsc_runtime_r243_controller_storage_separate(r,map,sizeof(*map))||
    !gsc_runtime_r243_controller_storage_separate(r,out,sizeof(*out))||
    Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
    Cod1ModelStringOverlap(map,sizeof(*map),out,sizeof(*out)))return 0;
 cert=*input;if(!cert.serial||!cert.current||!cert.owner.epoch||!cert.owner.generation||
    !map->receipt||!map->current||!r->r237_frame_generation||r->r243_next_main_receipt==UINT64_MAX)return 0;
 if(source){
 entry.entity=cert.owner;strcpy(entry.module,source==1?"animscripts/stop":"animscripts/run");strcpy(entry.function,"main");
 module=find_module(r->program,entry.module);if(module<0||(fn=find_func(r->program,module,"main"))<0||
    !locomotion_admission_current(r,fn,why,sizeof(why)))return 0;
 }
 memset(&call,0,sizeof(call));call.copied.entity=cert.owner;call.copied.actor_serial=cert.serial;
 call.copied.runtime_identity=r->array_token;call.copied.catalog_identity=r->animation_namespace.identity;
 call.copied.frame_generation=r->r237_frame_generation;call.copied.now_ms=r->now_ms;call.copied.locomotion_source=source?source:-1;
 l->r=r;l->host=r->r243_controller;l->select=r->r243_select_with_cleanup;l->q=call.copied;l->input=&call.copied;
 l->map=*map;l->animationMap=r->anim_data;l->instance=r->array_token;l->catalog=r->animation_namespace.identity;
 l->time_root=r->r237_frame_time_root;l->now=r->now_ms;l->bucket=r237_bucket(r);c->lease=l;
 r->r243_controller_capture=c;r->r243_controller_capture_bytes=sizeof(call);r->r243_controller_busy=1;r->advancing=1;
 if(!r243_controller_current(l))goto done;
 for(i=0;i<32;i++)if(r->r243_controller_main[i].used&&state_ref_equal(&r->r243_controller_main[i].entity,&cert.owner)){
  owner=r->r243_controller_main+i;break;
 }
 if(owner){
  if((owner->ordinary_source!=1&&owner->ordinary_source!=2)||owner->initial_stop||owner->selection==UINT64_MAX||
     owner->last_frame>=l->q.frame_generation||!r243_main_task_owned(r,owner->task,owner->task_serial)||
     !r243_main_owner_current(l,owner)||memcmp(&owner->certificate,&cert,sizeof(cert)))goto done;
  replacement.lease=l;replacement.owner=owner;replacement.seal=*owner;
  service.context=&replacement;service.run=r243_replace_main;
 }else{
  if(!source)goto done;
  for(i=0;i<32;i++)if(!r->r243_controller_main[i].used){owner=r->r243_controller_main+i;break;}
  if(!owner)goto done;
 }
 why[0]=0;
 if(!l->select||l->select(l->host.user,&l->q,&selected,why,sizeof(why),r243_controller_current,l,service.run?&service:NULL)!=1){
  rfail(r,0,"ordinary MAIN native selection refused: %s",why);goto done;
 }
 if(!r243_controller_current(l)||!selected.selection_serial||memcmp(&selected.certificate,&cert,sizeof(cert))||
    replacement.failed||replacement.busy||(service.run&&(selected.source_changed?replacement.next!=2:replacement.next!=0)))goto done;
 if(!selected.source_changed){
  Task*t=owner->task;
  if(!owner->used||owner->selection!=selected.selection_serial||!r243_main_task_owned(r,t,owner->task_serial)||
     (source&&owner->ordinary_source!=source)||(t->status!=TASK_EVENT&&t->status!=TASK_SLEEP))goto done;
  run.entity=owner->entity;run.task_serial=t->serial;
  run.status=t->status==TASK_EVENT?GSC_ENTITY_ENTRY_WAITING:GSC_ENTITY_ENTRY_SLEEPING;
 }else{
  if(!source||owner->used)goto done;
  c->owner=owner;c->selection=selected;c->ordinary_source=source;c->armed=1;
  r->advancing=0;
  i=r243_actor_script_phase6_owned_hook(r,l->q.frame_generation,&entry,&cert,&l->map,
      r243_controller_phase6_step,l,&run,&c->pending,r243_controller_entry_retained,c);
  r->advancing=1;
  if(!i||!c->claimed||c->pending||!r243_controller_current(l)||
     (run.status!=GSC_ENTITY_ENTRY_WAITING&&run.status!=GSC_ENTITY_ENTRY_SLEEPING))goto done;
 }
 if(memcmp(input,&cert,sizeof(cert))||memcmp(map,&l->map,sizeof(*map))||!r243_main_owner_current(l,owner))goto done;
 owner->last_frame=l->q.frame_generation;
 if(selected.source_changed)
 G_Printf("[464 owned MAIN] tick=%d owner=%u source=%s task=%llu selection=%llu receipt=%llu\n",
  l->now,cert.owner.slot,entry.module,(unsigned long long)run.task_serial,
  (unsigned long long)selected.selection_serial,(unsigned long long)owner->receipt);
 *out=run;ok=1;
done:
 if(!ok)r243_main_capture_abort(c);
 r->r243_controller_capture=NULL;r->r243_controller_capture_bytes=0;r->r243_controller_busy=0;r->advancing=0;
 return ok;
}
