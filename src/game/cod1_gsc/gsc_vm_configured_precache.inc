/* Optional configured-root invocation lifetime. Selection is not permission.
 * Only the actually spawned root's first dispatch arms publication; synchronous
 * frames and entered inline children share that interval. It is never stored
 * in a Task, queue, Results archive, host certificate or detached continuation.
 * Resource publication still requires the independent startup/publish hosts. */
enum { CONFIGURED_PRECACHE_UNBORN, CONFIGURED_PRECACHE_ARMED,
       CONFIGURED_PRECACHE_ACTIVE, CONFIGURED_PRECACHE_CLOSED };
static void configured_precache_close(GscRuntime *r){
 if(!r||!r->configured_precache_interval_enabled)return;
 r->configured_precache_phase=CONFIGURED_PRECACHE_CLOSED;
 r->configured_precache_depth=0;r->precache_string_initial_window=0;
 memset(r->configured_precache_chain,0,sizeof(r->configured_precache_chain));
}
int gsc_program_select_configured_precache_interval(GscProgram *g){
 if(!g||g->invalid||g->linked||g->configured_precache_interval_selected||
    !g->r243_thread_start_selected||!g->precache_string_syntax)return 0;
 g->configured_precache_interval_selected=1;return 1;
}
int gsc_runtime_select_configured_precache_interval(GscRuntime *r){
 if(!r||!r->program||!r->program->configured_precache_interval_selected||
    r->configured_precache_interval_enabled||!r->r243_thread_start_enabled||
    !r->r237_frame_buckets_enabled||!r->lifetime_enabled||!r->state_enabled||
    !r->results_enabled||!r->precache_string_enabled||!r->program->precache_string_mode||
    r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||
    r->entity_field_busy||r->entity_method_busy||r->entity_entry_depth||r->state_arena_busy)return 0;
 r->configured_precache_interval_enabled=1;return 1;
}
static int configured_precache_member(const GscRuntime *r,const Task *task){
 int n;for(n=0;n<r->task_count;n++)if(task_at(r,n)==task)return 1;return 0;
}
static void configured_precache_node(ConfiguredPrecacheNode *node,Task *task){
 memset(node,0,sizeof(*node));node->task=task;node->serial=task->serial;
 node->map=task->map_certificate;node->entity=task->certificate;
}
static int configured_precache_node_current(const GscRuntime *r,
 const ConfiguredPrecacheNode *node,int ready){
 const Task *task=node->task;
 if(!task||!configured_precache_member(r,task))return 0;
 /* The first genuine birth is serial0. Membership plus the captured birth
  * and monotonic next_serial authenticate it; zero is not a missing owner. */
 return node->serial<r->next_serial&&task->serial==node->serial&&!task->context_retired&&
   (!ready||(task->status==TASK_READY&&task->depth>0))&&
   !memcmp(&node->map,&task->map_certificate,sizeof(node->map))&&
   !memcmp(&node->entity,&task->certificate,sizeof(node->entity));
}
/* Pure predicate, including when called from Session/GAME host adapters. */
static int configured_precache_active(const GscRuntime *r){
 unsigned n;
 if(!r||!r->configured_precache_interval_enabled||
    r->configured_precache_phase!=CONFIGURED_PRECACHE_ACTIVE||
    !r->configured_precache_depth||r->configured_precache_depth>33||
    r->configured_precache_checking||r->error[0]||!r->advancing||
    r->program!=r->configured_precache_program||!r->program->configured_precache_interval_selected||
    !r->program->linked||!r->program->r243_thread_start_selected||!r->program->precache_string_mode||
    r->program->entry_fn!=r->configured_precache_entry_fn||
    !r->r243_thread_start_enabled||!r->r237_frame_buckets_enabled||
    !r->configured_precache_budget||r->execution_budget!=r->configured_precache_budget||
    r->array_token!=r->configured_precache_runtime_identity||
    r->now_ms!=r->configured_precache_now||r237_bucket(r)!=r->configured_precache_bucket||
    !r237_owner_current(r,r->configured_precache_time_root)||
    r237_owner_stamp(r)!=r->configured_precache_time_root||
    r->anim_data_enabled!=r->configured_precache_anim_enabled||
    memcmp(&r->anim_data,&r->configured_precache_anim,sizeof(r->anim_data))||
    r->executing_task!=r->configured_precache_chain[r->configured_precache_depth-1].task||
    r->entity_prepare_busy||r->entity_certificate_busy)return 0;
 for(n=0;n<r->configured_precache_depth;n++)
  if(!configured_precache_node_current(r,r->configured_precache_chain+n,1))return 0;
 return 1;
}
static int configured_precache_current(GscRuntime *r){
 unsigned n,count;int *budget,budget_value;
 if(!r->configured_precache_interval_enabled)return 1;
 if(!configured_precache_active(r)){configured_precache_close(r);return 0;}
 count=r->configured_precache_depth;budget=r->execution_budget;budget_value=*budget;r->configured_precache_checking=1;
 for(n=0;n<count;n++){
  if(!refresh_state_context(r,r->configured_precache_chain[n].task)||
     !configured_precache_node_current(r,r->configured_precache_chain+n,1)){
   r->configured_precache_checking=0;configured_precache_close(r);return 0;
  }
  /* Certificate callbacks may revoke or poison the interval. Never reacquire
   * from a saved phase, Task address, returned bytes or previous child. */
  if(r->configured_precache_phase!=CONFIGURED_PRECACHE_ACTIVE||r->error[0]||
     r->configured_precache_depth!=count||r->execution_budget!=budget||*budget!=budget_value){
   r->configured_precache_checking=0;configured_precache_close(r);return 0;
  }
 }
 r->configured_precache_checking=0;
 if(!configured_precache_active(r)){configured_precache_close(r);return 0;}
 return 1;
}
static int configured_precache_arm(GscRuntime *r,Task *root){
 if(!r->configured_precache_interval_enabled)return 1;
 if(r->configured_precache_phase!=CONFIGURED_PRECACHE_UNBORN||
    !root||!configured_precache_member(r,root)||root->status!=TASK_READY||
    root->serial!=r->resources_entry_serial||root->depth!=1||
    root->frames[0].fn!=r->program->entry_fn||!r->array_token||
    (!root->map_certificate.receipt&&!r->anim_data_enabled)){
  configured_precache_close(r);return rfail(r,0,"configured precache requires actual owned main Task and map");
 }
 r->configured_precache_program=r->program;r->configured_precache_entry_fn=r->program->entry_fn;
 r->configured_precache_runtime_identity=r->array_token;
 r->configured_precache_now=r->now_ms;r->configured_precache_bucket=r237_bucket(r);
 r->configured_precache_time_root=r237_owner_stamp(r);
 if(!r237_owner_current(r,r->configured_precache_time_root)){
  configured_precache_close(r);return rfail(r,0,"configured precache time-root unavailable");
 }
 r->configured_precache_anim_enabled=r->anim_data_enabled;r->configured_precache_anim=r->anim_data;
 configured_precache_node(r->configured_precache_chain,root);
 r->configured_precache_depth=1;r->configured_precache_phase=CONFIGURED_PRECACHE_ARMED;return 1;
}
static void configured_precache_dispatch_begin(GscRuntime *r,Task *task){
 if(!r->configured_precache_interval_enabled||r->configured_precache_phase!=CONFIGURED_PRECACHE_ARMED)return;
 if(r->configured_precache_chain[0].task!=task||
    !configured_precache_node_current(r,r->configured_precache_chain,1)){
  configured_precache_close(r);return;
 }
 r->configured_precache_budget=r->execution_budget;
 r->configured_precache_phase=CONFIGURED_PRECACHE_ACTIVE;
}
static void configured_precache_dispatch_end(GscRuntime *r,Task *task){
 if(r->configured_precache_interval_enabled&&r->configured_precache_depth&&
    r->configured_precache_chain[0].task==task)configured_precache_close(r);
}
static unsigned configured_precache_child_begin(GscRuntime *r,Task *parent,Task *child){
 unsigned depth;
 if(!r->configured_precache_interval_enabled||r->configured_precache_phase!=CONFIGURED_PRECACHE_ACTIVE)return 0;
 depth=r->configured_precache_depth;
 if(!depth||depth>=33||r->configured_precache_chain[depth-1].task!=parent||
    !configured_precache_node_current(r,r->configured_precache_chain+depth-1,1)){
  configured_precache_close(r);return 0;
 }
 configured_precache_node(r->configured_precache_chain+depth,child);
 r->configured_precache_depth=depth+1;return depth+1;
}
static void configured_precache_child_end(GscRuntime *r,Task *child,unsigned joined){
 if(!joined||r->configured_precache_phase!=CONFIGURED_PRECACHE_ACTIVE)return;
 if(r->configured_precache_depth!=joined||
    r->configured_precache_chain[joined-1].task!=child||
    !configured_precache_node_current(r,r->configured_precache_chain+joined-1,0)){
  configured_precache_close(r);return;
 }
 memset(r->configured_precache_chain+joined-1,0,sizeof(*r->configured_precache_chain));
 --r->configured_precache_depth;
}
static void configured_precache_unwind(GscRuntime *r){
 if(r->configured_precache_interval_enabled&&r->configured_precache_phase==CONFIGURED_PRECACHE_ACTIVE&&
    !configured_precache_active(r))configured_precache_close(r);
}
static void precache_string_wait_boundary(GscRuntime *r,Task *task){
 if(!r->configured_precache_interval_enabled){r->precache_string_initial_window=0;return;}
 if(r->configured_precache_phase==CONFIGURED_PRECACHE_ACTIVE&&r->configured_precache_depth&&
    r->configured_precache_chain[0].task==task)configured_precache_close(r);
}
