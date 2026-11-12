/* Pure private VM offer. Only actor_animation_invoke constructs this lease,
 * after its argument, certificate and host.validate callbacks have returned. */
typedef struct {
 GscRuntime*r;R232ActorAnimationLease*actor;Task*task;void*budget,*step;
 const GscActorAnimationRequest*input;GscActorAnimationRequest q;
 GscActorSourceAnimationReceipt receipt;
 int(*current)(void*);void*context;int phase,cancelled;
} R243SourceAnimationDispatch;
static int r243_source_method(int method){
 return method==GSC_ACTOR_ANIMATION_SET||method==GSC_ACTOR_ANIMATION_CLEAR||
  method==GSC_ACTOR_ANIMATION_KNOB_ALL||method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART||
  method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART;
}
static int r243_source_scope(const R243SourceAnimationDispatch*d){
 GscRuntime*r=d?d->r:NULL;R232ActorAnimationLease*a=d?d->actor:NULL;
 return r&&a&&!d->cancelled&&!r->r243_source_failed&&r->r243_source_dispatch==d&&
  r->r243_source_step_active&&r->r243_source_step_owner==d->step&&r->r243_controller_busy&&
  r->r243_actor_script_scope&&r->entity_method_busy&&r->executing_task==d->task&&
  d->task&&d->task->serial==d->receipt.task_serial&&d->task->status!=TASK_DEAD&&
  r->execution_budget==d->budget&&r->array_token==d->receipt.runtime_identity&&
  r->animation_namespace.identity==d->receipt.catalog_identity&&r->now_ms==d->q.now_ms&&
  r->r237_frame_generation==d->receipt.frame_generation&&actor_animation_scope(a)&&
  d->current==actor_animation_current&&d->context==a&&d->input&&
  !memcmp(d->input,&d->q,sizeof(d->q));
}
static int r243_source_fail(GscRuntime*r){
 if(r){R243SourceAnimationDispatch*d=r->r243_source_dispatch;r->r243_source_failed=1;if(d)d->cancelled=1;}return 0;
}
static int r243_source_begin(GscRuntime*r,R232ActorAnimationLease*a,
 const GscActorAnimationRequest*q,R243SourceAnimationDispatch*d){
 if(!r->r243_source_step_active)return 1; /* caller uses this only in selected step */
 if(r->r243_source_dispatch||r->r243_source_failed||!r->r243_source_step_owner||
    r->r243_next_source_dispatch==UINT64_MAX||!r243_source_method(q->method))
  return rfail(r,0,"source animation dispatch requires a fresh supported typed command");
 memset(d,0,sizeof(*d));d->r=r;d->actor=a;d->task=r->executing_task;
 d->budget=r->execution_budget;d->step=r->r243_source_step_owner;d->input=q;d->q=*q;
 d->current=actor_animation_current;d->context=a;d->phase=1;
 d->receipt.dispatch=++r->r243_next_source_dispatch;d->receipt.task_serial=d->task?d->task->serial:0;
 d->receipt.runtime_identity=r->array_token;d->receipt.catalog_identity=r->animation_namespace.identity;
 d->receipt.frame_generation=r->r237_frame_generation;d->receipt.entity=q->entity;
 r->r243_source_dispatch=d;
 if(!r243_source_scope(d))return r243_source_fail(r);return 1;
}
/* Pure queries: no resource/certificate/current callback is invoked here. */
static int r243_source_take(GscRuntime*r,void*step,const GscActorAnimationRequest*q,
 int(*current)(void*),void*context,GscActorSourceAnimationReceipt*out){
 R243SourceAnimationDispatch*d=r?r->r243_source_dispatch:NULL;
 if(!d||!out||!r243_source_scope(d)||d->phase!=1||d->step!=step||d->actor->checking||
    d->input!=q||current!=d->current||context!=d->context||
    !gsc_runtime_r243_controller_storage_separate(r,out,sizeof(*out)))return r243_source_fail(r);
 d->phase=2;*out=d->receipt;return 1;
}
static int r243_source_receipt_current(GscRuntime*r,void*step,const GscActorSourceAnimationReceipt*receipt,
 const GscActorAnimationRequest*q,int(*current)(void*),void*context,int claim){
 R243SourceAnimationDispatch*d=r?r->r243_source_dispatch:NULL;
 if(!d||!receipt||!r243_source_scope(d)||d->step!=step||d->actor->checking||
    d->input!=q||current!=d->current||context!=d->context||
    memcmp(receipt,&d->receipt,sizeof(*receipt))||d->phase!=(claim?2:3))return r243_source_fail(r);
 if(claim)d->phase=3;return 1;
}
static int r243_source_end(GscRuntime*r,R243SourceAnimationDispatch*d,int accepted){
 int ok=accepted&&r243_source_scope(d)&&d->phase==3;
 if(r->r243_source_dispatch==d)r->r243_source_dispatch=NULL;
 if(!ok)r->r243_source_failed=1;return ok;
}
static int r243_source_private(const GscRuntime*r,const void*p,size_t n){
 const R243SourceAnimationDispatch*d=r?r->r243_source_dispatch:NULL;
 return r&&((r->execution_budget&&Cod1ModelStringOverlap(p,n,r->execution_budget,sizeof(*r->execution_budget)))||
  (r->r243_source_step_owner&&Cod1ModelStringOverlap(p,n,r->r243_source_step_owner,r->r243_source_step_owner_bytes))||
  /* Enclosing owner/API are legal inputs during initial adoption. Protect
   * their full storage once an actual source offer exists, before any query
   * may write a receipt into a provider-owned output. */
  (d&&((r->r243_source_step_lease&&Cod1ModelStringOverlap(p,n,r->r243_source_step_lease,r->r243_source_step_lease_bytes))||
       (r->r243_source_step_api&&Cod1ModelStringOverlap(p,n,r->r243_source_step_api,r->r243_source_step_api_bytes))||
       Cod1ModelStringOverlap(p,n,d,sizeof(*d))||Cod1ModelStringOverlap(p,n,d->input,sizeof(d->q))||
       Cod1ModelStringOverlap(p,n,d->actor,sizeof(*d->actor)))));
}

static int r243_source_runtime_current(GscRuntime*r){return !r->r243_source_dispatch||r243_source_scope(r->r243_source_dispatch);}
