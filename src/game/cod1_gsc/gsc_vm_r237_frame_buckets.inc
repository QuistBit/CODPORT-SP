/* Explicit R237 scheduler. Native evidence: original 45DFA0 inserts at the
 * bucket head, 468BB0 selects/unlinks that head and re-reads it, 4695B0 drains
 * current VMpubTime, and 46ACA0 drains [old,target), leaving target untouched.
 * A monotonic insertion identity models those links without changing legacy
 * birth serials. This is not the original serialized stack interpreter. */
int gsc_program_select_r237_frame_buckets(GscProgram *g){
 if(!g||g->invalid||g->linked||g->r237_frame_buckets_selected||g->frame_cadence_selected)return 0;
 g->r237_frame_buckets_selected=1;return 1;
}
static int r237_busy(const GscRuntime *r){
 return r->r243_time_root_teardown||r->advancing||r->executing_task||r->r243_actor_script_scope||r->hud_elements_busy||r->entity_prepare_busy||
 r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||
 r->entity_field_busy||r->entity_method_busy;
}
int gsc_runtime_select_r237_frame_buckets(GscRuntime *r){
 if(!r||r->started||r237_busy(r)||r->task_count||r->error[0]||r->r237_frame_buckets_enabled||
 !r->program->r237_frame_buckets_selected||r->frame_cadence_enabled)return 0;
 r->r237_frame_buckets_enabled=1;r->r237_next_insertion=1;return 1;
}
int gsc_runtime_r237_frame_buckets_selected(const GscRuntime *r){
 return r&&r->program&&r->program->r237_frame_buckets_selected&&r->r237_frame_buckets_enabled&&!r->error[0];
}
int gsc_runtime_r237_frame_view(const GscRuntime *r,GscR237FrameView *out){
 GscR237FrameView v;
 if(!out||!gsc_runtime_r237_frame_buckets_selected(r)||!r237_owner_current(r,r237_owner_stamp(r))||!gsc_runtime_storage_separate(r,out,sizeof(*out)))return 0;
 memset(&v,0,sizeof(v));v.host_now_ms=r->now_ms;v.vm_bucket_ms=r237_bucket(r);
 v.frame_open=r->r237_frame_open;v.frame_generation=r->r237_frame_generation;*out=v;return 1;
}
static int r237_queue_available(GscRuntime *r,int count,int line){
 if(!r237_owner_current(r,r237_owner_stamp(r)))return rfail(r,line,"R237 required time-root unavailable");
 if(count<0||!r237_insertion_next(r)||r237_bucket(r)<0||r237_bucket(r)>INT32_C(0xffffff)||
 (uint64_t)count>UINT64_MAX-r237_insertion_next(r))return rfail(r,line,"R237 bucket insertion identity/domain exhausted");
 return 1;
}
static int r237_queue_offer(GscRuntime *r,Task *t,int key,int line){
 uint64_t stamp=r237_owner_stamp(r),insertion;int i,member=!r243_time_root_required(r);
 if(!r237_owner_current(r,stamp))return rfail(r,line,"R237 queue time-root retired");
 if(!member)for(i=0;i<r->task_count;i++)if(task_at(r,i)==t){member=1;break;}
 if(!member)return rfail(r,line,"R237 queue Task belongs to another namespace");
 if(key<r237_bucket(r)||key>INT32_C(0xffffff))return rfail(r,line,"R237 queue key outside nonwrapping cursor domain");
 if(t->r237_enqueued){
  /* Native GetVariable(existing key) does not relink the child. */
  if(t->r243_time_root_incarnation!=stamp||t->wake_ms!=key||!t->r237_insertion)return rfail(r,line,"R237 task already owns another queue key");
  return 1;
 }
 if(!r237_queue_available(r,1,line))return 0;
 insertion=r237_insertion_claim(r,line);if(!insertion)return 0;
 t->wake_ms=key;t->r237_insertion=insertion;t->r243_time_root_incarnation=stamp;t->r237_enqueued=1;return 1;
}
/* Original wait_value4645F3: nonnegative float seconds must be strictly
 * below FLOAT16777 at55B8C8. FMUL1000 stores FLOAT32, then PC53 adds2^-30
 * before nearest-even FISTP. Animation cadence's <=1-second domain is not a
 * script-wait limit. Keep its helper/legacy lanes unchanged; widen only this
 * explicit scheduler. Zero delay and24-bit wrap remain held by the caller. */
static int r237_wait_milliseconds(uint32_t seconds,uint32_t *out){
 cod1AnimationTimelinePolicy_t policy={53,0};cod1TimelineDyadic_t result,epsilon;
 uint32_t ms;uint64_t integer,remainder,half;int shift;
 if(!out||seconds>=UINT32_C(0x46831200))return 0;
 if(seconds<=UINT32_C(0x3f800000))return Cod1TimelineWaitMilliseconds(seconds,&policy,out);
 if(!Cod1TimelineMultiplyExtended(Cod1TimelineBits(seconds),Cod1TimelineBits(UINT32_C(0x447a0000)),&policy,&result)||
 !Cod1TimelineStore(result,0,&ms))return 0;
 epsilon.magnitude=1;epsilon.exponent=-30;epsilon.sign=0;
 if(!Cod1TimelineAddExtended(Cod1TimelineBits(ms),epsilon,&policy,&result)||result.sign)return 0;
 if(result.exponent>=0){
  if(result.exponent>=31||result.magnitude>(UINT32_MAX>>result.exponent))return 0;
  integer=result.magnitude<<result.exponent;
 }else{
  shift=-result.exponent;if(shift>=64)return 0;
  integer=result.magnitude>>shift;remainder=result.magnitude&((UINT64_C(1)<<shift)-1);
  half=UINT64_C(1)<<(shift-1);integer+=remainder>half||(remainder==half&&(integer&1));
 }
 if(integer>INT32_MAX)return 0;*out=(uint32_t)integer;return 1;
}
/* The port starts the stock tower thread from the adapted Training entry.
 * Retail starts that thread during map bootstrap, before the first regular
 * VM frame. Its initial 50 ms wait is issued at level time 300 and returns
 * at 400. Reused rigs issue their actor command exactly 50 ms after launch.
 * Account for the bootstrap frame where the wait is scheduled, preserving
 * the unmodified script and the normal actor tree/record on every takeoff. */
static int training_tower_bootstrap_wait(const GscRuntime *r,const Task *t,uint32_t bits){
    const Func *fn;
    if(r->training_tower_bootstrap_waited||!r->program->carbine_items_selected||
       bits!=UINT32_C(0x3d4ccccd)||t->depth!=1)return 0;
    fn=&r->program->funcs[t->frames[0].fn];
    return !strcmp(fn->name,"jumpers")&&!strcmp(r->program->modules[fn->module],"maps/training");
}
static int r237_timed_wait(GscRuntime *r,Task *t,const Value *value,int line){
 uint32_t bits,delay;int bootstrap;
 if(!float_arithmetic_number_bits(r,value,line,&bits))return 0;
 if(r->r243_same_bucket_wait_enabled&&bits==UINT32_C(0x80000000))bits=0;
 if(!r237_wait_milliseconds(bits,&delay)||(!delay&&!r->r243_same_bucket_wait_enabled)||delay>(uint32_t)(INT32_C(0xffffff)-r237_bucket(r)))
  return rfail(r,line,"R237 wait requires positive PC53 nearest nonwrapping deadline; zero-delay held");
 bootstrap=training_tower_bootstrap_wait(r,t,bits);
 if(bootstrap){
  if(delay!=50||delay+50u>(uint32_t)(INT32_C(0xffffff)-r237_bucket(r)))
   return rfail(r,line,"Training bootstrap wait outside the live bucket range");
  delay+=50;
 }
 if(!r237_queue_offer(r,t,r237_bucket(r)+(int)delay,line))return 0;
 if(bootstrap)r->training_tower_bootstrap_waited=1;
 return 1;
}
static int r237_drain_bucket(GscRuntime *r,int *budget){
 Task *chosen;int n,ok;uint64_t stamp=r237_owner_stamp(r);
 while(!r->error[0]){
  if(!r237_owner_current(r,stamp))return rfail(r,0,"R237 drain time-root retired");
  chosen=NULL;
  for(n=0;n<r->task_count;n++){
   Task *t=task_at(r,n);
   if(t->status!=TASK_READY&&t->status!=TASK_SLEEP)continue;
   if(t->r243_time_root_incarnation!=stamp||!t->r237_enqueued||!t->r237_insertion||t->wake_ms<r237_bucket(r)||t->wake_ms>INT32_C(0xffffff))
    return rfail(r,0,"R237 pending task has invalid queue ownership");
   if(t->wake_ms==r237_bucket(r)){
    if(chosen&&t->r237_insertion==chosen->r237_insertion)return rfail(r,0,"R237 duplicate queue insertion identity");
    if(!chosen||t->r237_insertion>chosen->r237_insertion)chosen=t;
   }
  }
  if(!chosen)return 1;
  chosen->r237_enqueued=0;chosen->r237_insertion=0;chosen->r243_time_root_incarnation=0;
  if(!refresh_state_context(r,chosen)||!refresh_wait_owner(r,chosen))return 0;
  if(!r237_owner_current(r,stamp))return rfail(r,0,"R237 time-root changed during queued certificate callbacks");
  if(chosen->status!=TASK_DEAD){
   chosen->status=TASK_READY;
   if(chosen->event_scope==EVENT_ENTITY)clear_wait_owner(chosen);
   r->executing_task=chosen;
   configured_precache_dispatch_begin(r,chosen);
   ok=r->results_enabled?run_task_results(r,chosen,budget):run_task(r,chosen,budget);
   configured_precache_dispatch_end(r,chosen);
   r->executing_task=NULL;
   if(!r237_owner_current(r,stamp))return rfail(r,0,"R237 time-root changed during source continuation");
   if(!ok&&!(r->state_enabled&&chosen->status==TASK_DEAD&&!r->error[0]))return 0;
   if(chosen->status==TASK_READY)return rfail(r,0,"R237 continuation returned without a suspension");
  }
  if(r->resources_enabled&&chosen->serial==r->resources_entry_serial)r->resources_closed=1;
  if(chosen->status==TASK_DEAD&&chosen->results){GSC_RUNTIME_FREE(chosen->results);chosen->results=NULL;}
 }
 return 0;
}
static int r237_finish_operation(GscRuntime *r,int ok){
 /* A genuine bootstrap operation may finish before the configured root is
  * born. UNBORN grants nothing and remains armable only by the actual root. */
 if(r->configured_precache_phase!=CONFIGURED_PRECACHE_UNBORN)configured_precache_close(r);
 r->execution_budget=NULL;r->executing_task=NULL;r->advancing=0;
 if(!ok||r->error[0])return 0;
 endon_collect_retired(r);return array_collect_safe_point(r);
}
static int r237_bootstrap_drain(GscRuntime *r){
 int budget=MAX_STEPS,ok;
 if(!r->r237_start_draining||r->r237_frame_open||r->now_ms||r237_bucket(r))return 0;
 r->advancing=1;r->execution_budget=&budget;ok=r237_drain_bucket(r,&budget);
 return r237_finish_operation(r,ok);
}
int gsc_r237_frame_begin(GscRuntime *r,int host_now,uint64_t *out_generation){
 uint64_t stamp;
 if(r&&r237_busy(r))return rfail(r,0,"reentrant R237 frame scheduler call");
 if(!out_generation||!gsc_runtime_r237_frame_buckets_selected(r)||!r->started||
 !gsc_runtime_storage_separate(r,out_generation,sizeof(*out_generation))||r->r237_frame_open||
 host_now<=r->now_ms||host_now<r237_bucket(r)||host_now>INT32_C(0xffffff)||r->r237_frame_generation==UINT64_MAX)return 0;
 stamp=r237_owner_stamp(r);if(!r237_owner_current(r,stamp))return 0;
 r->r243_previous_host_ms=r->now_ms;r->r237_frame_time_root=stamp;
 r->now_ms=host_now;r->r237_frame_target_ms=host_now;r->r237_frame_open=1;
 r->r237_frame_budget=MAX_STEPS;r->r237_frame_generation++;*out_generation=r->r237_frame_generation;return 1;
}
static int r237_frame_valid(GscRuntime *r,uint64_t generation){
 return gsc_runtime_r237_frame_buckets_selected(r)&&r237_owner_current(r,r->r237_frame_time_root)&&r->started&&r->r237_frame_open&&generation&&
 generation==r->r237_frame_generation&&r237_bucket(r)>=0&&r237_bucket(r)<=r->r237_frame_target_ms&&
 r->r237_frame_target_ms==r->now_ms&&r->r237_frame_target_ms<=INT32_C(0xffffff);
}
int gsc_r237_run_current_bucket(GscRuntime *r,uint64_t generation){
 int ok;
 if(r&&r237_busy(r))return rfail(r,0,"reentrant R237 frame scheduler call");
 if(!r237_frame_valid(r,generation))return 0;
 r->advancing=1;r->execution_budget=&r->r237_frame_budget;
 ok=r237_drain_bucket(r,&r->r237_frame_budget);return r237_finish_operation(r,ok);
}
int gsc_r237_advance_exclusive(GscRuntime *r,uint64_t generation){
 int ok=1;
 if(r&&r237_busy(r))return rfail(r,0,"reentrant R237 frame scheduler call");
 if(!r237_frame_valid(r,generation))return 0;
 r->advancing=1;r->execution_budget=&r->r237_frame_budget;
 while(r237_bucket(r)<r->r237_frame_target_ms){
  if(!r237_drain_bucket(r,&r->r237_frame_budget)){ok=0;break;}
  if(!r237_bucket_set(r,r237_bucket(r)+1,r->r237_frame_time_root)){ok=0;break;}
 }
 if(ok)r->r237_frame_open=0;
 return r237_finish_operation(r,ok);
}
