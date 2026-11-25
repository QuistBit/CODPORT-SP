#include "../state9_result.h"
/* Additive selected-frontdoor buffer guard; old public storage ABI stays
 * untouched. All owned separately allocated VM/IR banks are covered. */
int gsc_runtime_r243_controller_storage_separate(const GscRuntime*r,const void*p,size_t n){
 const GscProgram*g;int i;
 if(!r||r243_source_private(r,p,n)||!gsc_runtime_storage_separate(r,p,n))return 0;g=r->program;
 if(!g||Cod1ModelStringOverlap(p,n,g,sizeof(*g)))return 0;
#define C1CTRL_BANK(field,capacity) if(Cod1ModelStringOverlap(p,n,g->field,(size_t)g->capacity*sizeof(*g->field)))return 0
 C1CTRL_BANK(expr,expr_capacity);C1CTRL_BANK(code,code_capacity);C1CTRL_BANK(loops,loop_capacity);
 C1CTRL_BANK(whiles,while_capacity);C1CTRL_BANK(do_whiles,do_while_capacity);C1CTRL_BANK(switches,switch_capacity);C1CTRL_BANK(cases,case_capacity);
 C1CTRL_BANK(funcs,func_capacity);C1CTRL_BANK(modules,module_capacity);C1CTRL_BANK(dev_blocks,dev_capacity);C1CTRL_BANK(strings,string_capacity);
#undef C1CTRL_BANK
 for(i=0;i<g->func_count;i++)if(Cod1ModelStringOverlap(p,n,g->funcs[i].params,(size_t)g->funcs[i].params_capacity*sizeof(*g->funcs[i].params)))return 0;
 for(i=0;i<r->task_count;i++){const Task*t=task_at(r,i);
  if(Cod1ModelStringOverlap(p,n,t,sizeof(*t))||Cod1ModelStringOverlap(p,n,t->results,t->results?sizeof(*t->results):0))return 0;}
 return gsc_native_registry_storage_separate(r->native_registry,p,n);
}

/* Fixed selected controller prefix. Per-actor retained references block Task
 * REUSE only: they do not suppress real safe-point GC or root dead Values. */
int gsc_runtime_select_r243_scripted_controller(GscRuntime*r,const GscActorScriptedControllerHost*h){
 return gsc_runtime_select_r243_controller_with_cleanup(r,h,NULL);
}
int gsc_runtime_select_r243_controller_with_cleanup(GscRuntime*r,const GscActorScriptedControllerHost*input,GscControllerSelectWithCleanup select){
 GscActorScriptedControllerHost h;
 if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    r->r243_controller_enabled||r->r243s_profile.enabled||
    (r->program&&r->program->r243s_requested)||!r->r243_scripted_start_enabled||!r->r243_thread_entities_enabled||
    !gsc_runtime_r237_frame_buckets_selected(r)||!gsc_runtime_r243_controller_storage_separate(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.init_return||!h.select_main||!h.animation_step||!h.normal_update)return 0;
 r->r243_controller=h;r->r243_select_with_cleanup=select;r->r243_controller_enabled=1;r->r243_next_main_receipt=1;return 1;
}
typedef struct {
 GscRuntime*r;GscActorScriptedControllerHost host;GscControllerSelectWithCleanup select;GscActorControllerRequest q;
 GscMapExecutionCertificate map,animationMap;const GscActorControllerRequest*input;
 uint64_t instance,catalog,time_root;int now,bucket,checking,cancelled;
} R243ControllerLease;

/* Ownership of ONE entry-returned reference during this frontdoor. This is
 * private VM state, not an actor-ready flag or a general host capability. */
typedef struct {
 R243ControllerLease*lease;R243ControllerMainOwner*owner;
 GscActorControllerSelection selection;R243ControllerMainOwner owner_seal;
 Task*pending;Task*claimed_task;uint64_t claimed_serial,claimed_receipt;
 int armed,claimed,initial_stop,ordinary_source;
} R243MainCapture;

/* First member is the capture identity. Protect the complete enclosing call
 * through every configured callback, including before/after source offers. */
typedef struct {
 R243MainCapture capture;R243ControllerLease lease;GscActorControllerRequest copied;
} R243ControllerCall;

static int r243_controller_scope(const R243ControllerLease*l){
 GscRuntime*r=l?l->r:NULL;
 return r&&r237_owner_current(r,l->time_root)&&r->r243_controller_enabled&&r->r243_controller_busy&&!l->cancelled&&!r->error[0]&&
  r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&r->now_ms==l->now&&
  r->r237_frame_generation==l->q.frame_generation&&!r->r237_frame_open&&
  l->select==r->r243_select_with_cleanup&&!memcmp(&r->r243_controller,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->q,sizeof(l->q))&&
  !memcmp(&r->anim_data,&l->animationMap,sizeof(l->animationMap));
}
static int r243_controller_current(void*context){
 R243ControllerLease*l=context;GscRuntime*r=l->r;int ok;R243MainCapture*cap,saved;
 if(!r243_controller_scope(l))return 0;
 if(l->checking){l->cancelled=1;return rfail(r,0,"controller current reentered");}
 cap=r->r243_controller_capture;
 if(!cap||r->r243_controller_capture_bytes!=sizeof(R243ControllerCall)||cap->lease!=l)
  return rfail(r,0,"controller capture scope unavailable");
 saved=*cap;l->checking=1;
 ok=anim_data_current(r,NULL)&&r243_controller_scope(l)&&animation_namespace_scope(r)&&
    r243_controller_scope(l)&&state_entity_live(r,&l->q.entity)==1&&r243_controller_scope(l);
 if(ok){r->entity_certificate_busy=1;ok=l->map.current(l->map.receipt)==1;r->entity_certificate_busy=0;}
 l->checking=0;
 if(r->r243_controller_capture!=cap||r->r243_controller_capture_bytes!=sizeof(R243ControllerCall)||memcmp(cap,&saved,sizeof(saved))){
  l->cancelled=1;return rfail(r,0,"controller retained-reference scope changed during callback");
 }
 return ok&&r243_controller_scope(l);
}
typedef struct {
 R243ControllerLease*lease;int(*run_current)(void*);void*run_context;
} R243ControllerStep;
static int r243_controller_step_current(void*context){
 R243ControllerStep*s=context;
 return s&&r243_controller_current(s->lease)&&s->lease->r->r243_actor_script_scope&&
  s->lease->r->advancing&&!s->lease->r->executing_task&&s->lease->r->execution_budget&&!s->lease->r->r243_source_failed;
}

/* Readonly VM->Session bridge only. This does not permit writes, reentry,
 * GAME mutation, source dispatch or any other private input address. */
int gsc_runtime_r243_controller_request_input(const GscRuntime*r,const void*p,size_t n,
 int(*current)(void*),void*context){
 const R243MainCapture*c;const R243ControllerLease*l;
 if(!r||!p||n!=sizeof(GscActorControllerRequest)||
    !(c=r->r243_controller_capture)||r->r243_controller_capture_bytes!=sizeof(R243ControllerCall)||
    !(l=c->lease)||l!=(const R243ControllerLease*)((const unsigned char*)c+offsetof(R243ControllerCall,lease))||
    p!=&l->q||l->r!=r||l->checking||!r243_controller_scope(l))return 0;
 if(current==r243_controller_current&&context==l)return 1;
 if(current==r243_controller_step_current&&context==r->r243_source_step_owner&&
    r->r243_source_step_active&&r->r243_source_step_owner_bytes==sizeof(R243ControllerStep)){
  const R243ControllerStep*s=context;
  return s&&s->lease==l&&r->r243_actor_script_scope&&r->advancing&&!r->executing_task&&r->execution_budget&&!r->r243_source_failed;
 }
 return 0;
}

static int r243_controller_step_run(void*context){
 R243ControllerStep*s=context;
 return r243_controller_step_current(s)&&s->run_current(s->run_context)==1&&r243_controller_step_current(s);
}
static int r243_controller_step_notify(void*context,const GscEntityRef*owner,const char*event,const char*payload){
 R243ControllerStep*s=context;Value v;char name[128];size_t n;int count;GscRuntime*r=s->lease->r;
 if(!owner||!event||!payload||!r243_controller_step_current(s)||!state_ref_equal(owner,&s->lease->q.entity))return 0;
 for(n=0;n<sizeof(name)&&event[n];n++){}if(!n||n==sizeof(name))return 0;memcpy(name,event,n+1);
 memset(&v,0,sizeof(v));v.kind=V_STRING;
 for(n=0;n<sizeof(v.str)&&payload[n];n++){}if(n==sizeof(v.str))return 0;memcpy(v.str,payload,n+1);
 if(event_owner_live(r,owner)!=1||!r243_controller_step_current(s))return 0;
 /* This is the same actual payload scheduler primitive used by native notify,
  * within the owned phase6 scope; it queues, and does not execute, waiters. */
 count=payload_deliver_selected(r,owner,name,&v,1,0,NULL);
 return count>=0&&r243_controller_step_current(s);
}
static int r243_step_source_take(void*context,const GscActorAnimationRequest*q,int(*f)(void*),void*x,GscActorSourceAnimationReceipt*out){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;
 if(!r243_controller_scope(s->lease))return r243_source_fail(r);return r243_source_take(r,s,q,f,x,out);
}
static int r243_step_source_claim(void*context,const GscActorAnimationRequest*q,int(*f)(void*),void*x,GscActorSourceAnimationReceipt*out){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;R243SourceAnimationDispatch*d=r->r243_source_dispatch;
 if(!r243_controller_scope(s->lease)||!d||!out||!gsc_runtime_r243_controller_storage_separate(r,out,sizeof(*out))||
    !r243_source_receipt_current(r,s,&d->receipt,q,f,x,1))return r243_source_fail(r);
 *out=d->receipt;return 1;
}
static int r243_step_source_current(void*context,const GscActorSourceAnimationReceipt*receipt,const GscActorAnimationRequest*q,int(*f)(void*),void*x){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;
 return r243_controller_scope(s->lease)&&r243_source_receipt_current(r,s,receipt,q,f,x,0);
}
static int r402_step_source_take(void*context,const GscActorLookRequest*q,int(*f)(void*),void*x,GscActorSourceLookReceipt*out){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;
 if(!r243_controller_scope(s->lease))return r402_source_fail(r);return r402_source_take(r,s,q,f,x,out);
}
static int r402_step_source_claim(void*context,const GscActorLookRequest*q,int(*f)(void*),void*x,GscActorSourceLookReceipt*out){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;R402SourceLookDispatch*d=r->r402_source_dispatch;
 if(!r243_controller_scope(s->lease)||!d||!out||!gsc_runtime_r243_controller_storage_separate(r,out,sizeof(*out))||
    !r402_source_receipt_current(r,s,&d->receipt,q,f,x,1))return r402_source_fail(r);
 *out=d->receipt;return 1;
}
static int r402_step_source_current(void*context,const GscActorSourceLookReceipt*receipt,const GscActorLookRequest*q,int(*f)(void*),void*x){
 R243ControllerStep*s=context;GscRuntime*r=s->lease->r;
 return r243_controller_scope(s->lease)&&r402_source_receipt_current(r,s,receipt,q,f,x,0);
}
static int r243_controller_phase6_step(void*context,int(*run_current)(void*),void*run_context){
 R243ControllerLease*l=context;R243ControllerStep step;GscActorControllerAnimationServices api;char error[384];int ok;
 step.lease=l;step.run_current=run_current;step.run_context=run_context;
 memset(&api,0,sizeof(api));api.source.context=&step;api.source.take=r243_step_source_take;api.source.claim=r243_step_source_claim;api.source.current=r243_step_source_current;
 api.sourceLook.context=&step;api.sourceLook.take=r402_step_source_take;api.sourceLook.claim=r402_step_source_claim;api.sourceLook.current=r402_step_source_current;
 api.context=&step;api.current=r243_controller_step_current;api.run_current=r243_controller_step_run;api.notify=r243_controller_step_notify;
 if(!r243_controller_step_current(&step))return 0;error[0]=0;
 l->r->r243_source_step_lease=l;l->r->r243_source_step_lease_bytes=sizeof(*l);
 l->r->r243_source_step_api=&api;l->r->r243_source_step_api_bytes=sizeof(api);
 l->r->r243_source_failed=0;l->r->r243_source_step_active=1;l->r->r243_source_step_owner=&step;l->r->r243_source_step_owner_bytes=sizeof(step);
 ok=l->host.animation_step(l->host.user,&l->q,&api,error,sizeof(error),r243_controller_step_current,&step);
 l->r->r243_source_step_active=0;l->r->r243_source_step_owner=NULL;l->r->r243_source_step_owner_bytes=0;
 l->r->r243_source_step_lease=l->r->r243_source_step_api=NULL;
 l->r->r243_source_step_lease_bytes=l->r->r243_source_step_api_bytes=0;
 if(!r243_controller_step_current(&step))return 0;
 if(ok!=1)return rfail(l->r,0,"actual phase6 animation service refused: %s",error);
 return 1;
}
static int r243_main_owner_current(R243ControllerLease*l,const R243ControllerMainOwner*o){
 GscRuntime*r=l->r;Task*t=o->task;int ok;
 if(!o->used||o->runtime!=r->array_token||o->actor_serial!=l->q.actor_serial||
    !state_ref_equal(&o->entity,&l->q.entity)||!t||!t->retained_owner_refs||t->serial!=o->task_serial||
    memcmp(&o->map,&l->map,sizeof(o->map))||!r243_controller_current(l))return 0;
 r->entity_certificate_busy=1;ok=o->certificate.current(&o->certificate.owner,o->certificate.serial)==1;r->entity_certificate_busy=0;
 return ok&&r243_controller_current(l);
}

static int r243_main_task_owned(const GscRuntime*r,const Task*t,uint64_t serial){
 if(!r||!t||!serial)return 0;
 /* Physical membership BEFORE dereferencing any supplied Task pointer. */
 return task_member(r,t)&&t->serial==serial&&t->retained_owner_refs!=0;
}
/* No certificate/resource/current or allocator callback in this transfer. */
static int r243_controller_entry_retained(void*context,Task*t,const GscEntityScriptRun*run){
 R243MainCapture*c=context;R243ControllerLease*l=c?c->lease:NULL;GscRuntime*r=l?l->r:NULL;
 R243ControllerMainOwner*o;unsigned i;int found=0;
 if(!r||r->r243_controller_capture!=c||r->r243_controller_capture_bytes!=sizeof(R243ControllerCall)||
    !r243_controller_scope(l)||!c->armed||c->claimed||!c->owner||!run||c->pending!=t||
    !run->task_serial||!state_ref_equal(&run->entity,&l->q.entity))return 0;
 for(i=0;i<32;i++)if(c->owner==r->r243_controller_main+i){found=1;break;}
 if(!found||c->owner->used||!r243_main_task_owned(r,t,run->task_serial)||
    t->retained_owner_refs!=1||t->host_entry_pins||
    memcmp(&t->certificate,&c->selection.certificate,sizeof(t->certificate))||
    memcmp(&t->map_certificate,&l->map,sizeof(t->map_certificate))||
    t->initial_self.kind!=V_ENTITY||!state_ref_equal(&t->initial_self.entity,&l->q.entity)||
    ((!c->selection.selection_serial)!=!!c->initial_stop)||r->r243_next_main_receipt==UINT64_MAX)return 0;
 o=c->owner;memset(o,0,sizeof(*o));o->used=1;o->initial_stop=c->initial_stop;o->ordinary_source=c->ordinary_source;o->entity=l->q.entity;o->actor_serial=l->q.actor_serial;
 o->runtime=l->instance;o->selection=c->selection.selection_serial;o->receipt=r->r243_next_main_receipt++;
 o->task=t;o->task_serial=run->task_serial;o->certificate=c->selection.certificate;o->map=l->map;
 c->owner_seal=*o;c->claimed_task=t;c->claimed_serial=t->serial;c->claimed_receipt=o->receipt;
 c->claimed=1;c->pending=NULL;return 1;
}
/* Storage teardown of only THIS new transfer. A failed unchanged call owns
 * no new reference and cannot revoke a historical MAIN. No gameplay current
 * callback is required after an Actor/map retirement. A release is not cancel. */
static void r243_main_capture_abort(R243MainCapture*c){
 GscRuntime*r;Task*t;uint64_t serial;int i,owner_found=0;
 if(!c||!c->lease||(r=c->lease->r)==NULL)return;
 t=c->claimed?c->claimed_task:c->pending;
 serial=c->claimed?c->claimed_serial:0;
 if(t){
  /* Pending capture has no saved serial: authenticate physical membership
   * once, then use its current birth identity without another directory scan. */
  if(!c->claimed ? (task_member(r,t)&&t->serial&&t->retained_owner_refs) : r243_main_task_owned(r,t,serial))
   --t->retained_owner_refs;
  else if(!r->error[0])rfail(r,0,"retained MAIN storage release identity changed");
 }
 if(c->claimed){
  for(i=0;i<32;i++)if(c->owner==r->r243_controller_main+i){owner_found=1;break;}
  if(owner_found&&c->owner->used&&c->owner->receipt==c->claimed_receipt&&
     c->owner->task==c->claimed_task&&c->owner->task_serial==c->claimed_serial)
   memset(c->owner,0,sizeof(*c->owner));
  else if(!r->error[0])rfail(r,0,"new MAIN receipt changed before storage release");
 }
 c->pending=NULL;c->claimed_task=NULL;c->claimed=0;c->armed=0;
}

/* One select call owns the old reference and fixes its map, entity and H.
 * Only the accepted GAME selection can ask this VM service to discharge it. */
typedef struct {
 R243ControllerLease *lease;
 R243ControllerMainOwner *owner,seal;
 int next,busy,failed;
} R243MainReplacement;

static int r243_replace_main(void *context,int phase,uint64_t selection,int (*fence)(void*),void *game){
 R243MainReplacement *x=context;
 R243ControllerLease *l;
 R243ControllerMainOwner *o;
 GscRuntime *r;
 Task *t;
 int matched,ok=0,kill_endon=0,j,queued,wake;uint64_t insertion,stamp;
 if(!x)return 0;
 if(x->busy||x->failed||phase!=x->next||phase<0||phase>1||!fence){x->failed=1;return 0;}
 x->busy=1;l=x->lease;r=l->r;o=x->owner;
 if(!o||!o->used||selection!=o->selection||!o->receipt||memcmp(o,&x->seal,sizeof(*o))||
    !r->event_payloads_enabled||!r->endon_enabled||!r->r237_frame_buckets_enabled||
    r->executing_task||r->r237_frame_open||r237_bucket(r)!=l->now||
    !r243_main_task_owned(r,o->task,o->task_serial)||!r243_main_owner_current(l,o)||!fence(game)||
    x->failed||memcmp(o,&x->seal,sizeof(*o))||!r243_main_task_owned(r,o->task,o->task_serial))goto done;
 t=o->task;
 /* Initial stop owns an endon, whereas scripted MAIN owns an ordinary
  * waiter. Both are real native57080 recipients; the runtime gate is not a
  * task-status test. Keep unsupported receiver domains closed. */
 if(o->initial_stop||o->ordinary_source){
  if((o->initial_stop&&o->selection)||t->status==TASK_DEAD||t->context_retired)goto done;
  for(j=0;j<t->endon_count;j++)if(t->endon[j].scope==EVENT_ENTITY&&
     state_ref_equal(&t->endon[j].owner,&l->q.entity)&&!strcmp(t->endon[j].event,"killanimscript"))kill_endon++;
  if(kill_endon!=1)goto done;
 }else if(t->status!=TASK_EVENT||t->event_scope!=EVENT_ENTITY||strcmp(t->event,"killanimscript")||
    !state_ref_equal(&t->event_owner,&l->q.entity))goto done;
 if(t->retained_owner_refs!=1||t->host_entry_pins||r237_bucket(r)!=l->now||!r237_owner_current(r,l->time_root))goto done;
 if(phase==1){
  /* Native 57080: deliver zero arguments on this entity at H, before unref. */
  queued=t->r237_enqueued;wake=t->wake_ms;insertion=t->r237_insertion;stamp=t->r243_time_root_incarnation;
  matched=entity_notify_internal(r,&l->q.entity,"killanimscript");
  if(matched<0||r->error[0]||x->failed||!r243_controller_scope(l)||!fence(game)||
     x->failed||memcmp(o,&x->seal,sizeof(*o))||!r243_main_task_owned(r,t,x->seal.task_serial)||
     t->retained_owner_refs!=1||r237_bucket(r)!=l->now)goto done;
  if(kill_endon){
   /* Actual endon retirement leaves inert queue metadata on DEAD tasks.
    * The scheduler visits READY/SLEEP only. Preserve that metadata exactly;
    * do not dequeue or force a continuation while releasing the reference. */
   if(t->status!=TASK_DEAD||!t->context_retired||t->depth||t->endon_count||
      t->r237_enqueued!=queued||t->wake_ms!=wake||t->r237_insertion!=insertion||t->r243_time_root_incarnation!=stamp)goto done;
  }else if(matched<1||t->status!=TASK_READY||!t->r237_enqueued||t->wake_ms!=l->now||
           t->r243_time_root_incarnation!=l->time_root)goto done;
  --t->retained_owner_refs;
  if(kill_endon)G_Printf("[395 MAIN cleanup] tick=%d owner=%u task=%llu endonDead=1 refs=1->0 ordinaryWoken=%d\n",
      l->now,l->q.entity.slot,(unsigned long long)t->serial,matched);
  memset(o,0,sizeof(*o));x->seal.task=NULL;

 }
 ++x->next;ok=1;
done:
 x->busy=0;if(!ok)x->failed=1;return ok;
}

int gsc_runtime_r243_controller_prefix(GscRuntime*r,const GscEntityRef*input,uint64_t actor_serial,uint64_t generation,
 const GscMapExecutionCertificate*map,GscActorControllerPrefixRun*out){
 R243ControllerCall call;R243ControllerLease*l=&call.lease;GscActorControllerSelection selection;GscEntityScriptEntry entry;
 GscEntityScriptRun run;R243ControllerMainOwner*owner=NULL;R243MainCapture*capture=&call.capture;
 GscActorControllerPrefixRun result;char error[384];int i,ok=0,present=-1,frontier;
 GscActorControllerRequest*copied=&call.copied;
 R243MainReplacement replacement={0};GscMainReplacementService service={0};
 if(!r||!input||!map||!out||!r->r243_controller_enabled||r->r243_controller_busy||r237_busy(r)||
    !r->started||r->error[0]||r->r243_controller_capture||!r237_owner_current(r,r->r237_frame_time_root)||r->r237_frame_open||r237_bucket(r)!=r->now_ms||
    !gsc_runtime_r243_controller_storage_separate(r,input,sizeof(*input))||!gsc_runtime_r243_controller_storage_separate(r,out,sizeof(*out))||
    Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||Cod1ModelStringOverlap(map,sizeof(*map),out,sizeof(*out)))return 0;
 memset(&call,0,sizeof(call));copied->entity=*input;copied->actor_serial=actor_serial;copied->frame_generation=generation;
 copied->runtime_identity=r->array_token;copied->catalog_identity=r->animation_namespace.identity;copied->now_ms=r->now_ms;
 memset(l,0,sizeof(*l));l->r=r;l->host=r->r243_controller;l->select=r->r243_select_with_cleanup;l->q=*copied;l->input=copied;
 l->map=*map;l->animationMap=r->anim_data;l->instance=r->array_token;l->catalog=r->animation_namespace.identity;
 l->time_root=r->r237_frame_time_root;l->now=r->now_ms;l->bucket=r237_bucket(r);
 if(!l->map.receipt||!l->map.current||!l->q.actor_serial||!l->q.frame_generation||l->q.frame_generation!=r->r237_frame_generation||
    l->q.runtime_identity!=l->instance||l->q.catalog_identity!=l->catalog||l->q.now_ms!=l->now)return 0;
 memset(capture,0,sizeof(*capture));capture->lease=l;
 r->r243_controller_capture=capture;r->r243_controller_capture_bytes=sizeof(call);
 r->r243_controller_busy=1;r->advancing=1;
 if(!r243_controller_current(l))goto done;
 for(i=0;i<32;i++)if(r->r243_controller_main[i].used&&state_ref_equal(&r->r243_controller_main[i].entity,&l->q.entity)){
  owner=r->r243_controller_main+i;break;}
 if(owner&&(owner->last_frame>=l->q.frame_generation||!r243_main_owner_current(l,owner)))goto done;
 if(!owner){for(i=0;i<32;i++)if(!r->r243_controller_main[i].used){owner=r->r243_controller_main+i;break;}}
 if(!owner||r->r243_next_main_receipt==UINT64_MAX)goto done;
 memset(&selection,0,sizeof(selection));error[0]=0;
 if(l->select&&owner->used){
  replacement.lease=l;replacement.owner=owner;replacement.seal=*owner;
  service.context=&replacement;service.run=r243_replace_main;
 }
 if((l->select?l->select(l->host.user,&l->q,&selection,error,sizeof(error),r243_controller_current,l,service.run?&service:NULL):
    l->host.select_main(l->host.user,&l->q,&selection,error,sizeof(error),r243_controller_current,l))!=1){
  rfail(r,0,"controller select prefix refused: %s",error);goto done;}
 if(!r243_controller_current(l)||!selection.selection_serial||selection.certificate.serial!=l->q.actor_serial||
    !selection.certificate.current||!state_ref_equal(&selection.certificate.owner,&l->q.entity))goto done;
 if(replacement.failed||replacement.busy||
    (service.run&&(selection.source_changed?replacement.next!=2:replacement.next!=0)))goto done;
 if(selection.source_changed){
  if(owner->used){rfail(r,0,"replacement MAIN cleanup remains outside selected fresh prefix");goto done;}
  memset(&entry,0,sizeof(entry));entry.entity=l->q.entity;strcpy(entry.module,"animscripts/scripted");strcpy(entry.function,"main");
  /* Store actual returned MAIN reference at571E1, before animation/advance.
   * Publication of a completed public transition remains at the final gate. */
  capture->owner=owner;capture->selection=selection;capture->armed=1;
  r->advancing=0;
  i=r243_actor_script_phase6_owned_hook(r,l->q.frame_generation,&entry,&selection.certificate,&l->map,
      r243_controller_phase6_step,l,&run,&capture->pending,r243_controller_entry_retained,capture);
  r->advancing=1;if(!i)goto done;
  if(!capture->claimed||capture->pending||owner->task_serial!=run.task_serial||
     !r243_controller_current(l))goto done;
 }else if(!owner->used||owner->selection!=selection.selection_serial){rfail(r,0,"unchanged native source has no owned MAIN task");goto done;}
 if(!r243_main_owner_current(l,owner))goto done;
 /* Native native45F7E0's active type covers READY/SLEEP/EVENT. Dead MAIN's
  * independent pop branch is deliberately held, never treated as active. */
 if(owner->task->status==TASK_DEAD){rfail(r,0,"terminal MAIN pop branch requires separate native owner");goto done;}
 error[0]=0;frontier=-1;
 if(l->host.normal_update(l->host.user,&l->q,&frontier,error,sizeof(error),r243_controller_current,l)!=1||
    (frontier!=GSC_ACTOR_CONTROLLER_STATE9_TAIL_HELD&&frontier!=GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD&&frontier!=C1_STATE9_EMPTY_CONTACT_RETURNED&&frontier!=C1_STATE9_TOWER_CONTACT_RETURNED)||
    !r243_main_owner_current(l,owner)){
  if(!r->error[0])rfail(r,0,"normal controller prefix refused: %s",error);goto done;}
 present=frontier==GSC_ACTOR_CONTROLLER_STATE9_TAIL_HELD||frontier==C1_STATE9_EMPTY_CONTACT_RETURNED||frontier==C1_STATE9_TOWER_CONTACT_RETURNED;
 owner->last_frame=l->q.frame_generation;memset(&result,0,sizeof(result));result.entity=l->q.entity;
 result.actor_serial=l->q.actor_serial;result.selection_serial=owner->selection;result.main_receipt=owner->receipt;
 result.task_serial=owner->task_serial;result.source_changed=selection.source_changed;result.main_status=1;
 result.frontier=frontier;result.record_present=present;ok=1;
done:
 if(!ok)r243_main_capture_abort(capture);
 r->r243_controller_capture=NULL;r->r243_controller_capture_bytes=0;
 r->advancing=0;r->r243_controller_busy=0;
 if(ok)*out=result;else if(!r->error[0])rfail(r,0,"controller prefix ownership retired; committed native effects retained");
 return ok;
}

/* Same reference owner as selected scripted MAIN. The fixed stop entry is
 * admitted by the existing stationary GAME caller; its actual Task is handed
 * off by the interpreter, never discovered by scanning tasks. Selection zero
 * explicitly means pre-selection ownership, not a fabricated native category.
 * No source request, queue, notification or frame time is changed here. */
int gsc_runtime_initial_stop_main(GscRuntime*r,const GscEntityExecutionCertificate*input,
 const GscMapExecutionCertificate*map,GscEntityScriptRun*out){
 R243ControllerCall call;R243ControllerLease*l=&call.lease;R243MainCapture*c=&call.capture;
 GscEntityExecutionCertificate cert;GscEntityScriptEntry entry={0};GscEntityScriptRun run={0};
 R243ControllerMainOwner*owner=NULL;int i,ok=0;
 if(!r||!input||!map||!out||!r->r243_controller_enabled||!r->r243_select_with_cleanup||
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
 memset(&call,0,sizeof(call));call.copied.entity=cert.owner;call.copied.actor_serial=cert.serial;
 call.copied.runtime_identity=r->array_token;call.copied.catalog_identity=r->animation_namespace.identity;
 call.copied.frame_generation=r->r237_frame_generation;call.copied.now_ms=r->now_ms;
 l->r=r;l->host=r->r243_controller;l->select=r->r243_select_with_cleanup;l->q=call.copied;l->input=&call.copied;
 l->map=*map;l->animationMap=r->anim_data;l->instance=r->array_token;l->catalog=r->animation_namespace.identity;
 l->time_root=r->r237_frame_time_root;l->now=r->now_ms;l->bucket=r237_bucket(r);c->lease=l;
 r->r243_controller_capture=c;r->r243_controller_capture_bytes=sizeof(call);r->r243_controller_busy=1;
 if(!r243_controller_current(l))goto done;
 for(i=0;i<32;i++){
  if(r->r243_controller_main[i].used&&state_ref_equal(&r->r243_controller_main[i].entity,&cert.owner))goto done;
  if(!r->r243_controller_main[i].used&&!owner)owner=r->r243_controller_main+i;
 }
 if(!owner)goto done;
 c->owner=owner;c->selection.certificate=cert;c->initial_stop=1;c->armed=1;
 entry.entity=cert.owner;strcpy(entry.module,"animscripts/stop");strcpy(entry.function,"main");
 if(!execute_entity_values_owned_map_r243(r,&entry,&cert,&l->map,NULL,0,&run,&c->pending,NULL)||
    !r243_controller_entry_retained(c,c->pending,&run)||!r243_controller_current(l)||
    memcmp(input,&cert,sizeof(cert))||memcmp(map,&l->map,sizeof(*map))||
    !r243_main_owner_current(l,owner)||
    (run.status!=GSC_ENTITY_ENTRY_WAITING&&run.status!=GSC_ENTITY_ENTRY_SLEEPING))goto done;
 owner->last_frame=l->q.frame_generation;
 G_Printf("[395 initial MAIN] tick=%d owner=%u task=%llu receipt=%llu retained=1 status=%d\n",
  l->now,cert.owner.slot,(unsigned long long)run.task_serial,(unsigned long long)owner->receipt,run.status);
 *out=run;ok=1;
done:
 if(!ok)r243_main_capture_abort(c);
 r->r243_controller_capture=NULL;r->r243_controller_capture_bytes=0;r->r243_controller_busy=0;
 return ok;
}
