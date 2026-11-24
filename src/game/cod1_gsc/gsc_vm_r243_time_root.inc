/* WORK-ONLY PRIVATE DRAFT; NEVER BUILT/RUN BY ANALYTICAL AGENT.
 * Include after rfail definition, before early state/animation includes.
 * Prerequisites: complete GscRuntime/Task/R243ControllerMainOwner, exact
 * fields in r243_time_root_types_draft.h, stdint/string/stdlib, rfail,
 * Cod1ModelStringOverlap prototype, and task_at FORWARD prototype.
 *
 * Calls require a VALID, STILL-ALLOCATED Runtime pointer or NULL. Registry
 * lookup compares Runtime POINTER VALUES before reading Runtime.root; this
 * is NOT an arbitrary freed-Runtime pointer validator. Private registry list
 * integrity is a VM ownership invariant. No concurrent VM threads/reentrant
 * allocator callbacks. Actual GSC_RUNTIME_CALLOC/FREE hooks are used.
 *
 * This allocated descriptor owns the SELECTED queue namespace and canonical
 * cursor/counter. ROOT must migrate EVERY selected consumer and actual Task
 * queue stamp; this file alone cannot certify that integration.
 * No scene readiness or Task-liveness projection is stored or inferred.
 */
struct R243TimeRoot {
 R243TimeRoot *registry_next;
 GscRuntime *runtime;
 const GscProgram *program;
 uint64_t incarnation,next_insertion;
 int bucket,closing;
};
static R243TimeRoot *r243_time_root_registry;
/* Reserve UINT64_MAX, never wrap/reuse allocation incarnation. */
static uint64_t r243_time_root_next_incarnation=1;

/* Actual allocation membership only; no read through candidate r->root and
 * no dereference of the supplied Runtime itself before a member is found. */
static R243TimeRoot *r243_time_root_find(const GscRuntime *r){
 R243TimeRoot *a;
 if(!r)return NULL;
 for(a=r243_time_root_registry;a;a=a->registry_next)
  if((const void*)a->runtime==(const void*)r)return a;
 return NULL;
}
/* A registered object or nonempty root/teardown state NEVER downgrades to
 * legacy inline storage merely because controller_enabled was changed. */
static int r243_time_root_required(const GscRuntime *r){
 if(!r)return 0;
 if(r243_time_root_find(r))return 1;
 return r243_controller_lane_selected(r)||r->r243_time_root!=NULL||r->r243_time_root_teardown;
}
/* Pure physical Runtime/program/allocation binding. No callbacks, errors,
 * controller selection, map/actor certificate, Task status/count or array
 * token. Closing object still physically exists. Runtime must be valid. */
static R243TimeRoot *r243_time_root_physical(const GscRuntime *r){
 R243TimeRoot *a=r243_time_root_find(r);
 if(!a)return NULL;
 /* Membership found before reading even the candidate pointer value. */
 if(r->r243_time_root!=a||a->runtime!=r||!a->program||a->program!=r->program||
    !a->incarnation||a->incarnation==UINT64_MAX)return NULL;
 return a;
}
static uint64_t r237_owner_stamp(const GscRuntime *r){
 R243TimeRoot *a;
 if(!r||!r243_time_root_required(r))return 0; /* actual legacy stamp0 */
 a=r243_time_root_physical(r);return a?a->incarnation:0;
}
/* Active storage binding only, NOT complete gameplay authority. In
 * particular no error[] gate: genuine private phase6 abort can restoreH into
 * the SAME owned root while error is sticky. Public queue/entry/drain must
 * retain their own current/error/one-use-dispatch preconditions. */
static int r237_owner_current(const GscRuntime *r,uint64_t seal){
 R243TimeRoot *a;
 if(!r)return 0;
 if(!r243_time_root_required(r))return seal==0;
 a=r243_time_root_physical(r);
 return a&&r243_controller_lane_selected(r)&&!r->r243_time_root_teardown&&!a->closing&&
  seal!=0&&a->incarnation==seal;
}
static int r237_bucket(const GscRuntime *r){
 uint64_t seal;R243TimeRoot *a;
 if(!r)return -1;
 if(!r243_time_root_required(r))return r->r237_vm_bucket_ms;
 a=r243_time_root_physical(r);seal=a?a->incarnation:0;
 return r237_owner_current(r,seal)?a->bucket:-1;
}
static uint64_t r237_insertion_next(const GscRuntime *r){
 uint64_t seal;R243TimeRoot *a;
 if(!r)return 0;
 if(!r243_time_root_required(r))return r->r237_next_insertion;
 a=r243_time_root_physical(r);seal=a?a->incarnation:0;
 return r237_owner_current(r,seal)?a->next_insertion:0;
}
/* Private trusted cursor transition only; no public clock-rewind API. Caller
 * supplies its captured stamp, and independently validates exact native
 * P/H transition and request/callback stage. No native busy grant is added. */
static int r237_bucket_set(GscRuntime *r,int value,uint64_t expected){
 R243TimeRoot *a;
 if(!r||value<0||value>INT32_C(0xffffff)||!r237_owner_current(r,expected))return 0;
 if(!r243_time_root_required(r)){r->r237_vm_bucket_ms=value;return 1;}
 a=r243_time_root_physical(r);if(!a)return 0;
 a->bucket=value;return 1;
}
/* Queue caller has already established full gameplay/native-stage authority
 * and availability. Claim exactly ONE insertion, no task/queue fabrication. */
static uint64_t r237_insertion_claim(GscRuntime *r,int line){
 R243TimeRoot *a;uint64_t next,seal;
 if(!r)return 0;
 seal=r237_owner_stamp(r);
 if(!r237_owner_current(r,seal)){rfail(r,line,"R243 time-root allocation is missing or retired");return 0;}
 next=r237_insertion_next(r);
 if(!next||next==UINT64_MAX){rfail(r,line,"R237 bucket insertion identity exhausted");return 0;}
 if(r243_time_root_required(r)){
  a=r243_time_root_physical(r);if(!a)return 0;
  a->next_insertion=next+1;
 }else r->r237_next_insertion=next+1;
 return next;
}

/* Producer called ONLY by actual gsc_start_entity_hooks AFTER ALL genuine
 * map/entity preflight callbacks, within advancing/entity_sequence_busy1,
 * BEFORE started/bootstrap flags/Task/source. Supported initial time0 only.
 * Root integration separately seals selected lane BEFORE those callbacks and
 * compares unchanged afterward, so a callback cannot choose another lane. */
static int r243_time_root_construct(GscRuntime *r){
 R243TimeRoot *a;uint64_t incarnation;
 if(!r)return 0;
 if(!r243_controller_lane_selected(r))return !r243_time_root_required(r); /* legacy no allocation */
 if(r->r243_time_root_teardown||r->r243_time_root||r243_time_root_find(r)||
    !r->program||!r->program->linked||!r->r237_frame_buckets_enabled||
    r->error[0]||!r->advancing||!r->entity_sequence_busy||r->started||r->bootstrap_started||
    r->task_count||r->executing_task||r->execution_budget||r->entity_entry_depth||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||
    r->entity_field_busy||r->entity_method_busy||r->hud_elements_busy||
    r->r237_frame_open||r->r237_frame_generation||r->now_ms!=0){
  rfail(r,0,"R243 time-root requires genuine first zero-time bootstrap ownership");return 0;
 }
 incarnation=r243_time_root_next_incarnation;
 if(!incarnation||incarnation==UINT64_MAX){rfail(r,0,"R243 time-root allocation incarnation exhausted");return 0;}
 a=GSC_RUNTIME_CALLOC(1,sizeof(*a));
 if(!a){rfail(r,0,"R243 time-root allocation failed");return 0;}
 a->runtime=r;a->program=r->program;a->incarnation=incarnation;
 a->bucket=0;a->next_insertion=1; /* actual canonical initial queue namespace */
 /* Callback-free publish of ACTUAL newly allocated object, not a ready flag. */
 a->registry_next=r243_time_root_registry;r243_time_root_registry=a;
 r->r243_time_root=a;r243_time_root_next_incarnation=incarnation+1;
 return 1;
}

/* Invoke after existing Runtime-free busy refusals, BEFORE external HUD retire.
 * A second begin refuses even when owner was unborn or already unlinked.
 * Does not toggle advancing or permit any foreign/native mutation. */
static int r243_time_root_free_begin(GscRuntime *r){
 R243TimeRoot *a;
 if(!r)return 0;
 if(r->r243_time_root_teardown){rfail(r,0,"R243 time-root destruction already begun");return 0;}
 if(!r243_time_root_required(r))return 1;
 if(r->r243_controller_busy||r->r243_controller_capture||r->r243_actor_script_scope||
    r->r243_source_dispatch||r->r243_source_step_active){
  rfail(r,0,"R243 time-root destruction during owned controller execution");return 0;
 }
 r->r243_time_root_teardown=1;
 a=r243_time_root_find(r);if(a)a->closing=1;
 return 1;
}
/* Pure owned-storage sweep WHILE actual Task bank still exists. No gameplay
 * certificate/callback, source event, Task status/depth/result/queue write.
 * Same Task may have multiple true owners: release exactly one ref per row.
 * Quiescent free must already have excluded any transient MainCapture. */
static void r243_time_root_release_mains(GscRuntime *r){
 unsigned i;int n,limit,found;R243ControllerMainOwner *o;Task *t;
 if(!r||!r->r243_time_root_teardown)return;
 if(r->r243_controller_capture){rfail(r,0,"R243 MAIN transfer still owns transient storage at teardown");return;}
 limit=task_directory_limit(r);
 if(r->task_count<0||r->task_count>limit){rfail(r,0,"R243 MAIN teardown task-bank count invalid");return;}
 for(i=0;i<sizeof(r->r243_controller_main)/sizeof(r->r243_controller_main[0]);i++){
  o=r->r243_controller_main+i;if(!o->used)continue;
  t=o->task;found=0;
  /* Pointer-value membership is established BEFORE supplied Task dereference. */
  if(t)for(n=0;n<r->task_count;n++)if(task_at(r,n)==t){found=1;break;}
  if(found&&o->task_serial&&t->serial==o->task_serial&&t->retained_owner_refs)
   --t->retained_owner_refs;
  else rfail(r,0,"R243 MAIN teardown retained-reference identity changed");
  /* Teardown never leaves a replayable receipt, even on integrity failure. */
  memset(o,0,sizeof(*o));
 }
}
/* Called AFTER Task/Results bank frees, BEFORE Runtime free. It MUST NOT use
 * task_at or inspect any Task pointer/field; only registry/owner headers.
 * Owned allocation lookup permits freeing a real registered root even when
 * a borrowed Runtime.root pointer was corrupted. Never free borrowed pointer. */
static void r243_time_root_free_end(GscRuntime *r){
 R243TimeRoot **link,*a;
 if(!r)return;
 if(!r243_time_root_required(r))return; /* actual legacy lane */
 if(!r->r243_time_root_teardown){rfail(r,0,"R243 time-root final free without destruction begin");return;}
 for(link=&r243_time_root_registry;(a=*link)!=NULL;link=&a->registry_next)
  if((const void*)a->runtime==(const void*)r){
   if(!a->closing){rfail(r,0,"R243 time-root final free without closed allocation");return;}
   *link=a->registry_next;r->r243_time_root=NULL;
   memset(a,0,sizeof(*a));GSC_RUNTIME_FREE(a);return;
  }
 /* Genuine unborn owner still closes safely. Never dereference/free a stale
  * or borrowed candidate stored only in Runtime.root. Runtime remains closed. */
 r->r243_time_root=NULL;
}
/* No callback or mutation. Root/registry aliases are private across ALL
 * runtimes, preventing output from corrupting a foreign owner's membership.
 * Existing storage guards separately protect each complete Runtime/Task bank.
 * Call this from source/controller private guards without recursive calls
 * back into those guards. NULL nonempty span is unsafe output, zero is empty. */
static int r243_time_root_private(const GscRuntime *r,const void *p,size_t n){
 const R243TimeRoot *a;(void)r;
 if(!n)return 0;if(!p)return 1;
 if(Cod1ModelStringOverlap(p,n,&r243_time_root_registry,sizeof(r243_time_root_registry))||
    Cod1ModelStringOverlap(p,n,&r243_time_root_next_incarnation,sizeof(r243_time_root_next_incarnation)))return 1;
 for(a=r243_time_root_registry;a;a=a->registry_next)
  if(Cod1ModelStringOverlap(p,n,a,sizeof(*a)))return 1;
 return 0;
}
