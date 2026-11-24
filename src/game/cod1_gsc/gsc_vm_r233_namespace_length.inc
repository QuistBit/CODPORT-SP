/* Explicit namespace transport; legacy R186 references retain their path. */
int gsc_program_select_r233_namespace_animation_length(GscProgram *g){
 if(!g||g->invalid||g->linked||g->module_count||!g->native_animation_roots_selected||
    !g->animation_namespace_selected||!g->r232_source_float_selected||g->animation_refs_selected||g->animation_length_selected)return 0;
 g->namespace_length_selected=g->animation_length_selected=1;return 1;
}
int gsc_runtime_select_r233_namespace_animation_length(GscRuntime *r,const GscAnimationLengthHost *input){GscAnimationLengthHost h;
 if(!input||!animation_namespace_quiescent(r)||!r->program->namespace_length_selected||
    !r->turret_animations_enabled||!animation_namespace_scope(r)||r->animation_length_enabled||
    runtime_private_overlap(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.query||!anim_data_current(r,NULL)||memcmp(input,&h,sizeof(h))||!animation_namespace_scope(r))return 0;
 r->animation_length=h;r->animation_length_enabled=1;return 1;
}
/* Explicit independent map selection; actor/turret selectors remain strict. */
int gsc_program_select_map_animation_length(GscProgram*g){
 if(!g||!g->map_animation_names.count||!g->map_source_float_selected||g->map_animation_length_selected||!gsc_program_select_r233_namespace_animation_length(g))return 0;
 g->map_animation_length_selected=1;return 1;
}
int gsc_runtime_select_map_animation_length(GscRuntime*r,const GscAnimationLengthHost*input){
 GscAnimationLengthHost host;const GscProgram*program;uint64_t instance,catalog;
 if(!input||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||!r->program->map_animation_length_selected||!r->map_source_float_enabled||r->turret_animations_enabled||r->animation_length_enabled||runtime_private_overlap(r,input,sizeof(*input)))return 0;
 host=*input;if(!host.query)return 0;program=r->program;instance=r->array_token;catalog=r->animation_namespace.identity;
 if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||r->program!=program||r->array_token!=instance||r->animation_namespace.identity!=catalog||r->animation_length_enabled||memcmp(input,&host,sizeof(host)))return 0;
 r->animation_length=host;r->animation_length_enabled=1;return 1;
}
typedef struct {GscRuntime*r;Task*task;uint64_t serial,instance,catalog;int now,checking,cancelled;
 GscAnimationLengthHost host;GscAnimationLengthRequest *input,copy;} NamespaceLengthLease;
static int namespace_length_scope(const NamespaceLengthLease*l){GscRuntime*r=l->r;
 return !l->cancelled&&r&&!r->error[0]&&r->animation_length_busy&&r->animation_length_enabled&&
  r->program->namespace_length_selected&&r->array_token==l->instance&&r->now_ms==l->now&&
  r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&
  animation_namespace_scope(r)&&r->animation_namespace.identity==l->catalog&&
  !memcmp(&r->animation_length,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));
}
static int namespace_length_current(void*opaque){NamespaceLengthLease*l=opaque;int ok;
 if(!namespace_length_scope(l))return 0;if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=entity_task_current(l->r)&&namespace_length_scope(l)&&anim_data_current(l->r,l->task)&&
  namespace_length_scope(l)&&entity_task_current(l->r)&&namespace_length_scope(l);l->checking=0;return ok;
}
static int namespace_length_call(GscRuntime*r,const Value*args,int argc,int selected,int line,Value*out){
 const AnimationNamespaceEntry*entry;GscAnimationLengthRequest q;GscAnimationLengthReply reply;
 NamespaceLengthLease l;Value value;char error[384];uint32_t bits;unsigned rounding=GSC_FLOAT_ROUND_NEAREST;int exact,ok;
 if(!r->animation_length_enabled||!out||!args||argc<1||argc>MAX_PARAMS)return rfail(r,line,"namespace animation length capability/arity unavailable");
 if(r->animation_length_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"namespace animation length callback reentered");
 if(args[0].kind!=V_NAMESPACED_ANIMATION||!animation_namespace_value_valid(r,args))return rfail(r,line,"getanimlength requires a current namespace animation");
 entry=&r->animation_namespace.entries[args[0].object-1];
 if((r->program->map_animation_length_selected?entry->catalog>=r->program->map_animation_names.count:entry->catalog>=2)||!entry->declaration.ordinal||(entry->declaration.flags&GSC_ANIMATION_NODE_BRANCH))return rfail(r,line,"getanimlength requires a declared leaf animation");
 memset(&q,0,sizeof(q));q.runtime_identity=r->array_token;q.ordinal=entry->declaration.ordinal;q.flags=entry->declaration.flags;
 strcpy(q.tree,r->animation_namespace.catalogs[entry->catalog].tree);strcpy(q.name,entry->declaration.name);
 /* The selected native query materializes binary32 independently of the
  * caller's scalar-expression opcode selection. This does not enable scalar
  * arithmetic in additional modules. The profile is explicit at selection. */
 (void)selected;if(!r->float_arithmetic_enabled||!float_arithmetic_profile_valid(&r->program->float_arithmetic_profile))return rfail(r,line,"namespace length arithmetic profile unavailable");rounding=r->program->float_arithmetic_profile.rounding;
 memset(&l,0,sizeof(l));l.r=r;l.task=r->executing_task;l.serial=l.task?l.task->serial:0;l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.now=r->now_ms;l.host=r->animation_length;l.input=&q;l.copy=q;
 r->animation_length_busy=1;if(!namespace_length_current(&l))goto retired;
 memset(&reply,0xff,sizeof(reply));error[0]=0;ok=l.host.query(l.host.user,&q,&reply,error,sizeof(error),namespace_length_current,&l);error[sizeof(error)-1]=0;
 if(!namespace_length_current(&l))goto retired;r->animation_length_busy=0;
 if(ok!=1)return rfail(r,line,"namespace animation length host refused: %s",error[0]?error:"unloaded or unavailable clip");
 if(!animation_length_ratio(reply.frames,reply.fps,rounding,&bits,&exact))return rfail(r,line,"namespace length metadata outside validated domain");
 memset(&value,0,sizeof(value));value.kind=V_NUMBER;memcpy(&value.number,&bits,4);*out=value;return 1;
retired:r->animation_length_busy=0;return r->error[0]?0:rfail(r,line,"namespace length task/resources retired or intent changed");
}

int gsc_runtime_map_animation_metadata_request(const GscRuntime*r,unsigned index,GscAnimationLengthRequest*out){
 GscAnimationLengthRequest q;unsigned i,n=0;
 if(!out||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||!r->program->map_animation_length_selected||!r->animation_length_enabled||runtime_private_overlap(r,out,sizeof(*out)))return -1;
 for(i=0;i<(unsigned)r->animation_namespace.entry_count;i++){
  const AnimationNamespaceEntry*e=r->animation_namespace.entries+i;
  if(e->declaration.flags||!e->declaration.ordinal)continue;
  if(n++!=index)continue;
  if(e->catalog>=r->program->map_animation_names.count)return -1;
  memset(&q,0,sizeof(q));q.runtime_identity=r->array_token;q.ordinal=e->declaration.ordinal;q.flags=e->declaration.flags;
  strcpy(q.tree,r->animation_namespace.catalogs[e->catalog].tree);strcpy(q.name,e->declaration.name);*out=q;return 1;
 }
 return 0;
}
