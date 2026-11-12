/* Original opcode42/44 recursively calls463850 before returning to the parent.
 * This explicit lane runs only the newborn statement thread to its first real
 * suspension/return; it never drains another ready task or changes VM time.
 * Native self ownership and portable host execution certificates are separate:
 * same-entity self inherits its prepared certificate, level/owned struct is
 * map-owned; an unrelated entity needs a future genuine target authority.
 * Depth32 is a bounded port policy, not an original VM limit. */
static int run_task(GscRuntime *,Task *,int *);
static int run_task_results(GscRuntime *,Task *,int *);
int gsc_program_select_r243_thread_start(GscProgram*g){
 if(!g||g->invalid||g->linked||g->r243_thread_start_selected||!g->r237_frame_buckets_selected||!g->lifetime_syntax)return 0;
 g->r243_thread_start_selected=1;return 1;
}
int gsc_runtime_select_r243_thread_start(GscRuntime*r){
 if(!r||!r->program||!r->program->r243_thread_start_selected||r->r243_thread_start_enabled||
    r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    !r->r237_frame_buckets_enabled||!r->lifetime_enabled||!r->results_enabled||!r->state_enabled||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||
    r->entity_field_busy||r->entity_method_busy||r->entity_entry_depth)return 0;
 r->r243_thread_start_enabled=1;return 1;
}
int gsc_runtime_r243_thread_start_selected(const GscRuntime*r){
 return r&&r->program&&r->program->r243_thread_start_selected&&r->r243_thread_start_enabled&&!r->error[0];
}
int gsc_runtime_select_r243_thread_entities(GscRuntime*r,const GscThreadEntityAuthorityHost*input){
 GscThreadEntityAuthorityHost copy;
 if(!r||!input||!r->r243_thread_start_enabled||r->r243_thread_entities_enabled||
    r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_field_busy||r->entity_method_busy||
    runtime_private_overlap(r,input,sizeof(*input)))return 0;
 copy=*input;if(!copy.resolve)return 0;r->r243_thread_entities=copy;r->r243_thread_entities_enabled=1;return 1;
}
static int r243_thread_target(GscRuntime*r,Task*parent,const GscEntityRef*target,
 GscEntityExecutionCertificate*out,int line){
 GscThreadEntityAuthorityHost host;GscEntityRef ref,borrowed;
 GscEntityExecutionCertificate result,parent_entity;GscMapExecutionCertificate parent_map,anim_map;
 const GscProgram*program;uint64_t serial,time_root;char why[256];int*budget;
 int physical,bucket,budget_value,ok;
 if(!r->r243_thread_entities_enabled)return rfail(r,line,"R243 thread target has no prepared entity authority");
 host=r->r243_thread_entities;ref=*target;borrowed=ref;program=r->program;
 time_root=r237_owner_stamp(r);serial=parent->serial;physical=r->now_ms;bucket=r237_bucket(r);
 parent_entity=parent->certificate;parent_map=parent->map_certificate;anim_map=r->anim_data;
 budget=r->execution_budget;budget_value=budget?*budget:0;
 if(!host.resolve||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"R243 thread entity authority callback reentered");
 memset(&result,0,sizeof(result));memset(why,0,sizeof(why));
 r->entity_prepare_busy=1;ok=host.resolve(host.user,&borrowed,&result,why,sizeof(why));r->entity_prepare_busy=0;
 why[sizeof(why)-1]=0;
 if(r->error[0])return 0;
 if(!r237_owner_current(r,time_root)||r->program!=program||r->executing_task!=parent||parent->serial!=serial||
    r->now_ms!=physical||r237_bucket(r)!=bucket||!r->r243_thread_entities_enabled||
    memcmp(&r->r243_thread_entities,&host,sizeof(host))||!state_ref_equal(&borrowed,&ref)||
    memcmp(&parent->certificate,&parent_entity,sizeof(parent_entity))||
    memcmp(&parent->map_certificate,&parent_map,sizeof(parent_map))||memcmp(&r->anim_data,&anim_map,sizeof(anim_map))||
    r->execution_budget!=budget||(budget&&*budget!=budget_value))
  return rfail(r,line,"R243 thread entity authority changed its guarded context");
 if(!refresh_state_context(r,parent)||parent->status==TASK_DEAD)return 0;
 if(ok!=1){
    if(parent->depth>0 && parent->depth<=MAX_FRAMES){
        int idx=parent->frames[parent->depth-1].fn;
        if(idx>=0 && idx<program->func_count){
            const Func*caller=&program->funcs[idx];
            if(caller->module>=0 && caller->module<program->module_count)
                return rfail(r,line,"R243 thread target authority refused: %s; caller=%s::%s slot=%u generation=%u",why[0]?why:"no owned allocation",program->modules[caller->module],caller->name,ref.slot,ref.generation);
        }
    }
    return rfail(r,line,"R243 thread target authority refused: %s",why[0]?why:"no owned allocation");
}
 if(!result.serial||!result.current||!state_ref_equal(&result.owner,&ref))
  return rfail(r,line,"R243 thread target returned invalid allocation authority");
 *out=result;return 1;
}
static int r243_spawn_initial(GscRuntime*r,int fn,const Value*args,int argc,const Value*self,int line){
 Task*parent=r->executing_task,*child=NULL;Value owned_args[MAX_PARAMS],owned_self;
 GscMapExecutionCertificate map;GscEntityExecutionCertificate entity;
 unsigned joined=0;int ok=0,ran=0;uint64_t time_root=r237_owner_stamp(r);
 if(!parent||parent->status!=TASK_READY||!r->advancing||!r->execution_budget||
    argc<0||argc>MAX_PARAMS||(!args&&argc)||!self)return rfail(r,line,"R243 thread start requires active owned arguments");
 if(r->r243_thread_start_depth>=32||parent->host_entry_pins==UINT_MAX)
  return rfail(r,line,"R243 thread start depth/pin limit");
 if(!r237_owner_current(r,time_root))return rfail(r,line,"R243 child entry has no current time-root");
 owned_self=*self;if(argc)memcpy(owned_args,args,(size_t)argc*sizeof(*args));
 map=parent->map_certificate;memset(&entity,0,sizeof(entity));
 ++parent->host_entry_pins;++r->r243_thread_start_depth;
 if(!refresh_state_context(r,parent)||parent->status==TASK_DEAD)goto done;
 if(!map.receipt){
  if(!r->anim_data_enabled||!r->anim_data.receipt||!r->anim_data.current){rfail(r,line,"R243 thread start requires genuine map authority");goto done;}
  if(!anim_data_current(r,parent)||parent->status==TASK_DEAD)goto done;
  map=r->anim_data;
 }
 if(!map.current){rfail(r,line,"R243 thread start invalid map authority");goto done;}
 if(owned_self.kind==V_ENTITY){
  if(parent->certificate.serial&&parent->certificate.current&&
     state_ref_equal(&parent->certificate.owner,&owned_self.entity))entity=parent->certificate;
  else if(!r243_thread_target(r,parent,&owned_self.entity,&entity,line))goto done;
 }else if(owned_self.kind!=V_LEVEL&&owned_self.kind!=V_UNDEFINED&&
          !(owned_self.kind==V_STRUCT&&struct_value_valid(r,&owned_self))){
  rfail(r,line,"R243 thread self is not an owned script receiver");goto done;
 }
 if(!r237_owner_current(r,time_root)){rfail(r,line,"R243 time-root changed before child acquisition");goto done;}
 child=task_acquire(r,line);if(!child)goto done;
 ++child->host_entry_pins;child->status=TASK_READY;child->serial=r->next_serial++;
 child->certificate=entity;child->map_certificate=map;child->initial_self=owned_self;
 /* Formal binding may validate opaque anim data: the newborn is already the
  * executing target, while both stable task/continuation allocations are pinned. */
 r->executing_task=child;
 if(!entity_certificate_current(r,child))goto child_done;
 if(!push_frame(r,child,fn,owned_args,argc,&owned_self,line))goto child_done;
 if(!r237_owner_current(r,time_root)){rfail(r,line,"R243 time-root changed during child formal binding");goto child_done;}
 joined=configured_precache_child_begin(r,parent,child);
 ran=1;ok=run_task_results(r,child,r->execution_budget);
 if(!r237_owner_current(r,time_root))rfail(r,line,"R243 time-root changed during child source");
child_done:
 if(child->context_retired&&r->configured_precache_phase==CONFIGURED_PRECACHE_ACTIVE)configured_precache_close(r);
 configured_precache_child_end(r,child,joined);
 if(r->error[0])ok=0;
 else if(child->context_retired&&child->status==TASK_DEAD)ok=1;
 else if(ok&&child->status!=TASK_DEAD&&child->status!=TASK_SLEEP&&child->status!=TASK_EVENT)
  ok=rfail(r,line,"R243 child did not reach a continuation boundary");
 if(!ok&&!child->context_retired){child->status=TASK_DEAD;child->depth=0;}
 if(child->status==TASK_DEAD||!ran)task_results_clear(child);
 --child->host_entry_pins;
done:
 r->executing_task=parent;--parent->host_entry_pins;--r->r243_thread_start_depth;
 configured_precache_unwind(r);
 /* A dead parent may still own the outer ResultsPending stack pointer. Its
  * normal interpreter/scheduler unwind, not this nested helper, releases it. */
 if(!r237_owner_current(r,time_root))return rfail(r,line,"R243 time-root changed during child entry");
 return ok||(!r->error[0]&&parent->status==TASK_DEAD);
}
