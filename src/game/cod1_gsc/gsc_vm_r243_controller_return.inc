/* One genuine normal-return pop shared by both engines. No cancelled/dead
 * task enters here. The exact unchanged init frame is copied BEFORE pop;
 * configured postinit work runs AFTER pop, before caller's next instruction. */
typedef struct {
 GscRuntime*r;Task*t;uint64_t serial,instance,catalog,time_root;
 GscActorScriptedControllerHost host;GscActorInitReturnRequest q;
 GscEntityExecutionCertificate certificate;GscMapExecutionCertificate map,animMap;
 int depth,now,bucket,*budget,budgetValue,checking;
} R243InitReturnLease;
static int r243_init_return_scope(const R243InitReturnLease*l){
 GscRuntime*r=l->r;
 return r&&r237_owner_current(r,l->time_root)&&!r->error[0]&&r->r243_controller_enabled&&r->r243_controller_callback_busy&&
  r->advancing&&r->executing_task==l->t&&l->t->status==TASK_READY&&l->t->serial==l->serial&&
  l->t->depth==l->depth&&r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&
  r->now_ms==l->now&&r237_bucket(r)==l->bucket&&r->execution_budget==l->budget&&l->budget&&
  *l->budget==l->budgetValue&&!memcmp(&r->r243_controller,&l->host,sizeof(l->host))&&
  !memcmp(&l->t->map_certificate,&l->map,sizeof(l->map))&&!memcmp(&r->anim_data,&l->animMap,sizeof(l->animMap));
}
static int r243_init_return_current(void*context){
 R243InitReturnLease*l=context;GscRuntime*r=l->r;int ok;
 if(!r243_init_return_scope(l))return 0;
 if(l->checking)return rfail(r,0,"postinit return current reentered");
 l->checking=1;ok=entity_task_current(r)&&r243_init_return_scope(l)&&anim_data_current(r,l->t)&&
  r243_init_return_scope(l)&&animation_namespace_scope(r)&&r243_init_return_scope(l);
 if(ok){r->entity_certificate_busy=1;ok=l->certificate.current(&l->certificate.owner,l->certificate.serial)==1;r->entity_certificate_busy=0;}
 l->checking=0;return ok&&r243_init_return_scope(l);
}
static int r243_pop_normal_frame(GscRuntime*r,Task*t,int line){
 Frame*f;const Func*fn;R243InitReturnLease l;Value self;char error[384];int ok;
 if(!r||!t||t->status!=TASK_READY||t->depth<=0)return rfail(r,line,"normal return has no live frame");
 f=t->frames+t->depth-1;fn=r->program->funcs+f->fn;
 if(r->r243s_profile.enabled){
  if(!r243s_profile_current(r))return rfail(r,line,"staged profile identity changed");
  if(f->fn==r->r243s_profile.descriptor->init_fn)return r243s_pop_original_init_frame(r,t,line);
  f->loop_count=0;--t->depth;return 1;
 }

 /* The dynamic spawn factory executes its original native postinit exactly
  * once after this actual root returns, using the returned task and allocation. */
 if(r->factory_init_entry && t->serial==r->factory_init_serial && t->depth==1 &&
    f->self.kind==V_ENTITY && state_ref_equal(&f->self.entity,&r->actor_spawn_issued.owner) &&
    !strcmp(r->program->modules[fn->module],"animscripts/init")&&!strcmp(fn->name,"main")){
  f->loop_count=0;--t->depth;return 1;
 }
 if(placed_return_matches(r,t,f->fn))return placed_return_pop(r,t,line);
 if(!r->r243_controller_enabled||strcmp(r->program->modules[fn->module],"animscripts/init")||strcmp(fn->name,"main")){
  f->loop_count=0;--t->depth;return 1;
 }
 if(r->r243_controller_callback_busy||r->entity_method_busy||r->entity_field_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||!r->execution_budget||t->host_entry_pins==UINT_MAX)
  return rfail(r,line,"postinit return callback scope unavailable");
 self=f->self;if(self.kind!=V_ENTITY)return rfail(r,line,"selected postinit requires actual init entity receiver");
 memset(&l,0,sizeof(l));l.r=r;l.t=t;l.serial=t->serial;l.instance=r->array_token;
 l.time_root=r237_owner_stamp(r);l.catalog=r->animation_namespace.identity;l.host=r->r243_controller;l.now=r->now_ms;l.bucket=r237_bucket(r);
 l.map=t->map_certificate;l.animMap=r->anim_data;l.budget=r->execution_budget;l.budgetValue=*l.budget;
 ++t->host_entry_pins;
 if(t->certificate.serial&&state_ref_equal(&t->certificate.owner,&self.entity))l.certificate=t->certificate;
 else if(!r243_thread_target(r,t,&self.entity,&l.certificate,line))goto failed;
 l.q.entity=self.entity;l.q.runtime_identity=l.instance;l.q.catalog_identity=l.catalog;
 l.q.task_serial=l.serial;l.q.now_ms=l.now;
 /* The frame is really gone before any postinit callback. A later refusal
  * retains that committed return and any genuine native prefix. */
 f->loop_count=0;--t->depth;l.depth=t->depth;r->r243_controller_callback_busy=1;
 error[0]=0;
 if(!r243_init_return_current(&l))goto retired;
 ok=l.host.init_return(l.host.user,&l.q,error,sizeof(error),r243_init_return_current,&l);error[sizeof(error)-1]=0;
 if(!r243_init_return_current(&l))goto retired;
 r->r243_controller_callback_busy=0;--t->host_entry_pins;
 if(ok!=1)return rfail(r,line,"actual postinit fragment refused: %s",error);
 return 1;
retired:
 r->r243_controller_callback_busy=0;if(!r->error[0])rfail(r,line,"postinit return context retired after actual pop");
failed:--t->host_entry_pins;return 0;
}
