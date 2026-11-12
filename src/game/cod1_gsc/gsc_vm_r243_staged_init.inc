/* Private genuine-normal-return INIT owner component. No Session/native
 * provider is selected by production code yet. A future native entrance MUST
 * verify the actual Session-origin lease before claim/native callbacks and
 * exclude the old FIXED entry using genuine prebirth profile association.
 * Bare VM broker possession is not authority to enter GAME. */
enum {R243SI_PREPOP=0,R243SI_OFFERED=1,R243SI_TAKEN=2,R243SI_CLAIMED=3};
struct R243SInitCall {
 GscRuntime*r;Task*t;Frame*returning_frame;Value returning_self;
 uint64_t task_serial,instance,catalog,time_root;
 GscStagedScriptedControllerHostV2 host;
 GscEntityExecutionCertificate certificate,certificate_seal;GscMapExecutionCertificate map,anim_map;
 int init_fn,depth,now,bucket,*budget,budget_value,checking,cancelled,phase;
 unsigned pins;int task_count;uint64_t next_task_serial;
 GscStagedControllerRequest q,q_seal;
 GscStagedControllerCall call,call_seal;
 GscStagedControllerBroker broker,broker_seal;
 GscStagedControllerReceipt receipt;
 GscStagedControllerResult result,result_initial;
 char error[384],error_initial[384];
};
/* Callback context is &owner.call. Its observer copies are never the
 * authority for pin cleanup, canonical broker phase or immutable seals. */
struct R243SInitOwner {
 Task*task;uint64_t serial;unsigned pins;
 int phase,checking,cancelled;
 R243SInitCall seal,call;
};
static int r243s_init_current(void*);
static int r243s_init_take(void*,const GscStagedControllerCall*,int(*)(void*),void*,GscStagedControllerReceipt*);
static int r243s_init_claim(void*,const GscStagedControllerCall*,int(*)(void*),void*,GscStagedControllerReceipt*);
static int r243s_init_receipt_current(void*,const GscStagedControllerReceipt*,const GscStagedControllerCall*,int(*)(void*),void*);

/* Actual root registry supplies physical Runtime membership before arbitrary
 * borrowed broker.context dereference. Valid registered Runtime lifetime and
 * private registry integrity are existing owner preconditions. */
static GscRuntime*r243s_broker_runtime(void*opaque){
 R243TimeRoot*a=r243_time_root_find((const GscRuntime*)opaque);
 return a?a->runtime:NULL;
}
/* VM current context itself is borrowed. Compare pointer value against active
 * VM-owned call first, without dereferencing the arbitrary candidate. */
static R243SInitCall*r243s_init_context(void*opaque){
 R243TimeRoot*a;for(a=r243_time_root_registry;a;a=a->registry_next){
  GscRuntime*r=a->runtime;
  if(r&&r->r243s_profile.active_init==(R243SInitCall*)opaque)return r->r243s_profile.active_init;
 }return NULL;
}
static GscRuntime*r243s_init_owner(const R243SInitCall*c){
 R243TimeRoot*a;for(a=r243_time_root_registry;a;a=a->registry_next){
  GscRuntime*r=a->runtime;if(r&&r->r243s_profile.active_init==c)return r;
 }return NULL;
}
static R243SInitOwner*r243s_init_hidden_owner(const R243SInitCall*c){
 GscRuntime*r=c?r243s_init_owner(c):NULL;
 R243SInitOwner*o=r?r->r243s_profile.active_owner:NULL;
 /* Compare addresses before dereferencing a borrowed candidate. */
 return o&&(const unsigned char*)o+offsetof(R243SInitOwner,call)==(const unsigned char*)c?o:NULL;
}
static int r243s_init_immutable(const R243SInitCall*c,int prepop){
 R243SInitOwner*o=r243s_init_hidden_owner(c);R243SInitCall observed;
 if(!o||c->t!=o->task||c->task_serial!=o->serial||c->pins!=o->pins||
    c->phase!=o->phase||c->checking!=o->checking||c->cancelled!=o->cancelled)return 0;
 observed=*c;
 /* These are the only legitimately changing call members. Canonical flags
  * are checked against the hidden owner above; result/error are validated by
  * their separate exact return bridge and the real returned result checks. */
 observed.phase=o->seal.phase;observed.checking=o->seal.checking;
 observed.cancelled=o->seal.cancelled;observed.result=o->seal.result;
 memcpy(observed.error,o->seal.error,sizeof(observed.error));
 /* Pre-pop certificate is the actual target service's one output. */
 if(prepop)observed.certificate=o->seal.certificate;
 return !memcmp(&observed,&o->seal,sizeof(observed));
}
static int r243s_init_member(const GscRuntime*r,const Task*t){
 int n,limit;if(!r||!t)return 0;limit=r->lifetime_enabled?GSC_LIFETIME_TASK_CAPACITY:MAX_THREADS;
 if(r->task_count<0||r->task_count>limit)return 0;
 for(n=0;n<r->task_count;n++)if(task_at(r,n)==t)return 1;return 0;
}
static int r243s_init_fail(GscRuntime*r){
 if(r){R243SInitCall*c=r->r243s_profile.active_init;R243SInitOwner*o=r243s_init_hidden_owner(c);
  r->r243s_profile.failed=1;if(o){o->cancelled=1;c->cancelled=1;}
  rfail(r,0,"staged INIT dispatch ownership refused");}return 0;
}
static int r243s_init_prepop_scope(const R243SInitCall*c){
 GscRuntime*r=c?r243s_init_owner(c):NULL;
 return r&&r243s_init_immutable(c,1)&&c->r==r&&r243s_profile_current(r)&&!r->error[0]&&!c->cancelled&&c->phase==R243SI_PREPOP&&
  r->r243s_profile.active_init==c&&r237_owner_current(r,c->time_root)&&
  r->advancing&&r->executing_task==c->t&&r243s_init_member(r,c->t)&&
  c->t->serial==c->task_serial&&c->t->status==TASK_READY&&c->t->depth==c->depth&&
  c->t->host_entry_pins==c->pins&&c->returning_frame==c->t->frames+c->depth-1&&
  c->returning_frame->fn==c->init_fn&&c->returning_frame->self.kind==V_ENTITY&&
  state_ref_equal(&c->returning_frame->self.entity,&c->returning_self.entity)&&
  r->task_count==c->task_count&&r->next_serial==c->next_task_serial&&
  r->array_token==c->instance&&r->animation_namespace.identity==c->catalog&&
  r->now_ms==c->now&&r237_bucket(r)==c->bucket&&
  r->execution_budget==c->budget&&c->budget&&*c->budget==c->budget_value&&
  !r->r243_controller_callback_busy&&!memcmp(&r->r243s_profile.host,&c->host,sizeof(c->host))&&
  !memcmp(&c->t->map_certificate,&c->map,sizeof(c->map))&&!memcmp(&r->anim_data,&c->anim_map,sizeof(c->anim_map));
}
static int r243s_init_scope(const R243SInitCall*c){
 GscRuntime*r=c?r243s_init_owner(c):NULL;const R243SProfile*p;
 if(!r||!r243s_init_immutable(c,0)||c->r!=r||!r243s_profile_current(r)||r->error[0])return 0;p=&r->r243s_profile;
 if(p->active_init!=c||c->cancelled||c->phase<R243SI_OFFERED||c->phase>R243SI_CLAIMED||
    !r237_owner_current(r,c->time_root)||!r->advancing||r->executing_task!=c->t||
    !r243s_init_member(r,c->t))return 0;
 return c->t->serial==c->task_serial&&c->t->status==TASK_READY&&c->t->depth==c->depth&&
  c->t->host_entry_pins==c->pins&&r->task_count==c->task_count&&r->next_serial==c->next_task_serial&&
  r->r243_controller_callback_busy&&r->array_token==c->instance&&
  r->animation_namespace.identity==c->catalog&&r->now_ms==c->now&&r237_bucket(r)==c->bucket&&
  r->execution_budget==c->budget&&c->budget&&*c->budget==c->budget_value&&
  c->init_fn==p->descriptor->init_fn&&!memcmp(&p->host,&c->host,sizeof(c->host))&&
  !memcmp(&c->certificate,&c->certificate_seal,sizeof(c->certificate))&&
  !memcmp(&c->t->map_certificate,&c->map,sizeof(c->map))&&!memcmp(&r->anim_data,&c->anim_map,sizeof(c->anim_map))&&
  !memcmp(&c->q,&c->q_seal,sizeof(c->q))&&!memcmp(&c->call,&c->call_seal,sizeof(c->call))&&
  !memcmp(&c->broker,&c->broker_seal,sizeof(c->broker));
}
static int r243s_init_current(void*opaque){
 R243SInitCall*c=r243s_init_context(opaque),saved;GscRuntime*r;R243SInitOwner*o;int ok;
 r=c?r243s_init_owner(c):NULL;o=r243s_init_hidden_owner(c);
 if(!c||!r||!o||!r243s_init_scope(c))return r?r243s_init_fail(r):0;
 /* No ordinary current before Session FIRST take. */
 if(o->phase<R243SI_TAKEN||o->checking)return r243s_init_fail(r);
 o->checking=1;c->checking=1;saved=*c;
 ok=entity_task_current(r)&&r243s_init_scope(c)&&anim_data_current(r,c->t)&&
  r243s_init_scope(c)&&animation_namespace_scope(r)&&r243s_init_scope(c);
 if(ok){r->entity_certificate_busy=1;ok=c->certificate.current(&c->certificate.owner,c->certificate.serial)==1;r->entity_certificate_busy=0;}
 if(memcmp(c,&saved,sizeof(saved)))ok=0;o->checking=0;c->checking=0;
 return ok&&r243s_init_scope(c)?1:r243s_init_fail(r);
}
static int r243s_init_exact(const R243SInitCall*c,const GscStagedControllerCall*call,
 const GscStagedControllerBroker*api,int(*current)(void*),void*context){
 return c&&call==&c->call&&api==&c->broker&&current==r243s_init_current&&context==c&&
  r243s_init_scope(c)&&!c->checking;
}
/* Pure INPUT admission only, exact original whole-call tuple. This does not
 * broaden generic storage_separate or grant another service an output alias. */
int gsc_runtime_r243_staged_call_input(const GscRuntime*r,const GscStagedControllerCall*call,
 const GscStagedControllerBroker*api,int(*current)(void*),void*context){
 const R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 return r243s_init_exact(c,call,api,current,context)&&c->r==r&&
  call->request==&c->q&&call->result==&c->result&&call->error==c->error&&call->error_capacity==sizeof(c->error);
}
/* Trusted Session ORIGINAL-return bridge. Pure admission of ONE exact tuple,
 * not a service-output exemption or general public write capability. Only
 * Session's named wrapper uses this callback-free after claimed native return,
 * all post-callback fences and validation, while originals remain sentinel.
 */
int gsc_runtime_r243_staged_call_return(const GscRuntime*r,const GscStagedControllerCall*call,
 const GscStagedControllerBroker*api,int(*current)(void*),void*context){
 const R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 return gsc_runtime_r243_staged_call_input(r,call,api,current,context)&&
  c->phase==R243SI_CLAIMED&&!memcmp(&c->result,&c->result_initial,sizeof(c->result))&&
  !memcmp(c->error,c->error_initial,sizeof(c->error));
}
static int r243s_init_receipt_out(GscRuntime*r,const R243SInitCall*c,void*out,size_t n){
 /* Root must extend existing storage helpers with r243s_init_private below.
  * Session local take receipt is permitted; GAME forwarded claim must ALSO
  * reject all Session private/full-lease aliases in its own broker wrapper. */
 return out&&n==sizeof(GscStagedControllerReceipt)&&
  !Cod1ModelStringOverlap(out,n,c,sizeof(*c))&&gsc_runtime_r243_controller_storage_separate(r,out,n);
}
static int r243s_init_take(void*opaque,const GscStagedControllerCall*call,
 int(*current)(void*),void*context,GscStagedControllerReceipt*out){
 GscRuntime*r=r243s_broker_runtime(opaque);R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 if(!c||!r243s_init_exact(c,call,&c->broker,current,context)||c->phase!=R243SI_OFFERED||
    !r243s_init_receipt_out(r,c,out,sizeof(*out))||
    memcmp(&c->result,&c->result_initial,sizeof(c->result))||memcmp(c->error,c->error_initial,sizeof(c->error)))return r243s_init_fail(r);
 r243s_init_hidden_owner(c)->phase=R243SI_TAKEN;c->phase=R243SI_TAKEN;*out=c->receipt;return 1;
}
static int r243s_init_claim(void*opaque,const GscStagedControllerCall*call,
 int(*current)(void*),void*context,GscStagedControllerReceipt*out){
 GscRuntime*r=r243s_broker_runtime(opaque);R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 if(!c||!r243s_init_exact(c,call,&c->broker,current,context)||c->phase!=R243SI_TAKEN||
    !r243s_init_receipt_out(r,c,out,sizeof(*out))||
    memcmp(&c->result,&c->result_initial,sizeof(c->result))||memcmp(c->error,c->error_initial,sizeof(c->error)))return r243s_init_fail(r);
 r243s_init_hidden_owner(c)->phase=R243SI_CLAIMED;c->phase=R243SI_CLAIMED;*out=c->receipt;return 1;
}
static int r243s_init_receipt_current(void*opaque,const GscStagedControllerReceipt*receipt,
 const GscStagedControllerCall*call,int(*current)(void*),void*context){
 GscRuntime*r=r243s_broker_runtime(opaque);R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 if(!c||!receipt||!r243s_init_exact(c,call,&c->broker,current,context)||c->phase!=R243SI_CLAIMED||
    memcmp(receipt,&c->receipt,sizeof(*receipt)))return r243s_init_fail(r);return 1;
}
static int r243s_init_private(const GscRuntime*r,const void*p,size_t n){
 const R243SInitCall*c=r?r->r243s_profile.active_init:NULL;
 const R243SInitOwner*o=r?r->r243s_profile.active_owner:NULL;
 /* Entirely inert on legacy/default, including NULL/nonempty inputs. */
 if(!r||(!r->r243s_profile.enabled&&!c&&!o&&
    (!r->program||!r->program->r243s_requested)))return 0;
 if(!n)return 0;if(!p)return 1;
 return Cod1ModelStringOverlap(p,n,r243s_scripted_registration,sizeof(r243s_scripted_registration))||
  (r&&(Cod1ModelStringOverlap(p,n,&r->r243s_profile,sizeof(r->r243s_profile))||
   (r->program&&Cod1ModelStringOverlap(p,n,&r->program->r243s_descriptor,sizeof(r->program->r243s_descriptor)))||
   (o&&Cod1ModelStringOverlap(p,n,o,sizeof(*o)))||
   (c&&(Cod1ModelStringOverlap(p,n,c,sizeof(*c))||
    (c->budget&&Cod1ModelStringOverlap(p,n,c->budget,sizeof(*c->budget)))))));
}
/* Integration replaces ONLY the separately selected staged actual INIT pop.
 * Fixed helper/executors keep their old behavior. Actual Task pin and normal
 * frame pop are real; no call to this function can manufacture their proof.
 */
static int r243s_pop_original_init_frame(GscRuntime*r,Task*t,int line){
 R243SInitOwner owner;R243SInitCall*c=&owner.call;R243SProfile*p;const R243SProgramDescriptor*d;Frame*f;int ok=0,pinned=0,popped=0;
 if(!r||!r243s_profile_current(r)||!t||!r243s_init_member(r,t)||t->status!=TASK_READY||
    t->depth<=0||!r->advancing||r->executing_task!=t||r->error[0])return 0;
 p=&r->r243s_profile;d=p->descriptor;f=t->frames+t->depth-1;
 if(f->fn!=d->init_fn)return 0; /* caller normally pops non-init elsewhere */
 if(p->active_init||p->active_owner||r->r243_controller_callback_busy||r->entity_method_busy||r->entity_field_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||!r->execution_budget||t->host_entry_pins==UINT_MAX)
  return rfail(r,line,"staged normal-init return unavailable");
 if(f->self.kind!=V_ENTITY)return rfail(r,line,"staged init return lacks actual entity self");
 memset(&owner,0,sizeof(owner));c->r=r;c->t=t;c->task_serial=t->serial;c->instance=r->array_token;
 c->catalog=r->animation_namespace.identity;c->time_root=r237_owner_stamp(r);c->host=p->host;
 c->map=t->map_certificate;c->anim_map=r->anim_data;c->now=r->now_ms;c->bucket=r237_bucket(r);
 c->budget=r->execution_budget;c->budget_value=*c->budget;c->init_fn=f->fn;
 c->returning_frame=f;c->returning_self=f->self;c->depth=t->depth;
 c->task_count=r->task_count;c->next_task_serial=r->next_serial;
 /* Full call protected BEFORE the possible real target-certificate callback;
  * no offer exists during this PREPOP interval. */
 owner.task=t;owner.serial=t->serial;
 p->active_owner=&owner;p->active_init=c;++t->host_entry_pins;
 pinned=1;owner.pins=t->host_entry_pins;c->pins=owner.pins;owner.seal=*c;
 if(t->certificate.serial&&state_ref_equal(&t->certificate.owner,&c->returning_self.entity))c->certificate=t->certificate;
 else if(!r243_thread_target(r,t,&c->returning_self.entity,&c->certificate,line))goto done;
 if(!r243s_init_prepop_scope(c)||!c->certificate.current||!c->certificate.serial||
    !state_ref_equal(&c->certificate.owner,&c->returning_self.entity))goto done;
 c->certificate_seal=c->certificate;
 if(p->next_dispatch==0||p->next_dispatch==UINT64_MAX)goto done;
 c->q.role=GSC_STAGED_CONTROLLER_INIT;c->q.dispatch_nonce=p->next_dispatch++;
 c->q.transition_nonce=c->q.dispatch_nonce;r243s_binding(r,&c->q.binding);
 c->q.input.init.actor.entity=c->returning_self.entity;c->q.input.init.actor.task_serial=c->task_serial;
 c->q.input.init.actor.runtime_identity=c->instance;c->q.input.init.actor.catalog_identity=c->catalog;
 c->q.input.init.actor.now_ms=c->now;c->q_seal=c->q;
 memset(&c->result,0xa5,sizeof(c->result));c->result_initial=c->result;
 c->call.request=&c->q;c->call.result=&c->result;c->call.error=c->error;c->call.error_capacity=sizeof(c->error);c->call_seal=c->call;
 c->broker.context=r;c->broker.take=r243s_init_take;c->broker.claim=r243s_init_claim;c->broker.current=r243s_init_receipt_current;c->broker_seal=c->broker;
 c->receipt.role=GSC_STAGED_CONTROLLER_INIT;c->receipt.transition_nonce=c->q.transition_nonce;c->receipt.dispatch_nonce=c->q.dispatch_nonce;
 c->receipt.binding=c->q.binding;c->receipt.runtime_identity=c->instance;c->receipt.catalog_identity=c->catalog;c->receipt.time_root_incarnation=c->time_root;
 c->receipt.actor_serial=c->certificate.serial;c->receipt.entity=c->returning_self.entity;
 /* Genuine normal return, no callback from here until Session entrance. */
 f->loop_count=0;--t->depth;popped=1;c->depth=t->depth;r->r243_controller_callback_busy=1;owner.phase=R243SI_OFFERED;c->phase=R243SI_OFFERED;owner.seal=*c;
 if(!r243s_init_scope(c))goto done;
 ok=c->host.init_return(c->host.user,&c->call,&c->broker,r243s_init_current,c);
 if(ok!=1||c->phase!=R243SI_CLAIMED||!r243s_init_current(c)||
    c->result.role!=GSC_STAGED_CONTROLLER_INIT||c->result.transition_nonce!=c->q.transition_nonce||
    c->result.dispatch_nonce!=c->q.dispatch_nonce||c->result.selection_serial||
    memcmp(&c->result.output.init.bound,&c->q.binding,sizeof(c->q.binding))||
    !state_ref_equal(&c->result.certificate.owner,&c->returning_self.entity)||
    c->result.certificate.serial!=c->certificate.serial||
    c->result.certificate.current!=c->certificate.current){ok=0;goto done;}
 ok=1;
done:
 /* No source instruction runs between claimed return and close. Real pin
  * release is independent owned-storage cleanup, membership/serial first. */
 if(p->active_init==c)p->active_init=NULL;
 if(p->active_owner==&owner)p->active_owner=NULL;else if(pinned)ok=0;
 r->r243_controller_callback_busy=0;
 if(pinned&&r243s_init_member(r,owner.task)&&owner.task->serial==owner.serial&&
    owner.task->host_entry_pins==owner.pins)--owner.task->host_entry_pins;
 else if(pinned)ok=0;
 if(!ok){p->failed=1;if(!r->error[0])rfail(r,line,popped?
  "staged INIT refused; genuine normal pop/effects preserved":"staged INIT refused before normal pop");}
 return ok;
}
