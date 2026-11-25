typedef struct {
 GscRuntime*r;const GscProgram*g;Task*t;Frame*f;
 uint64_t serial,program,runtime,catalog,timeRoot,call;
 unsigned profile_mask;int now,depth,fn,pc;GscMapExecutionCertificate map;R243ActorFloatHost host;
} R243ActorFloatIdentity;
typedef struct {
 R243ActorFloatIdentity id,seal;R243ActorFloatRequest q,expected;
 R243ActorFloatReply reply;char error[384];int checking,cancelled;
} R243ActorFloatCall;
static R243ActorFloatCall*r243ActorFloatActive;
static uint64_t r243ActorFloatCallNonce;
static int r243_actor_float_vm_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&r243ActorFloatActive,sizeof(r243ActorFloatActive))||
  Cod1ModelStringOverlap(p,n,&r243ActorFloatCallNonce,sizeof(r243ActorFloatCallNonce))||
  (r243ActorFloatActive&&Cod1ModelStringOverlap(p,n,r243ActorFloatActive,sizeof(*r243ActorFloatActive)));
}
int R243ProgramSelectActorFloat(GscProgram*g,unsigned mask){
 if(!mask||(mask&~7u)||!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
    !g->function_ref_syntax||!g->function_program_token||!g->anim_data_selected||!g->animation_namespace_selected||g->actor_float_selected)return 0;
 g->actor_float_selected=mask;return 1;
}
int R243RuntimeSelectActorFloat(GscRuntime*r,const R243ActorFloatHost*input){
 R243ActorFloatHost h;const GscProgram*g;uint64_t instance,catalog;unsigned mask;
 if(!input||r243ActorFloatActive||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    !r->results_enabled||!r->state_enabled||!r->entities_enabled||!r->program->actor_float_selected||r->actor_float_enabled||
    !gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.validate||!h.query)return 0;g=r->program;mask=g->actor_float_selected;instance=r->array_token;catalog=r->animation_namespace.identity;
 if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    r->program!=g||g->actor_float_selected!=mask||r->array_token!=instance||r->animation_namespace.identity!=catalog||r->actor_float_enabled)return 0;
 r->actor_float_host=h;r->actor_float_mask=mask;r->actor_float_enabled=1;return 1;
}
static int r243_actor_float_scope(R243ActorFloatCall*c){
 const R243ActorFloatIdentity*i;GscRuntime*r;Task*t;int found=0;
 if(!c||c!=r243ActorFloatActive||c->cancelled||memcmp(&c->id,&c->seal,sizeof(c->id))||memcmp(&c->q,&c->expected,sizeof(c->q)))return 0;
 i=&c->seal;r=i->r;t=i->t;
 if(!r||!r->started||!r->advancing||r->error[0]||!r->actor_float_enabled||!r->actor_float_busy||!r->entity_method_busy||
    r->program!=i->g||r->program->actor_float_selected!=i->profile_mask||r->actor_float_mask!=i->profile_mask||!(i->profile_mask&R243ActorFloatMask(c->expected.method))||r->program->function_program_token!=i->program||
    r->array_token!=i->runtime||r->now_ms!=i->now||r->executing_task!=t||!animation_namespace_scope(r)||
    r->animation_namespace.identity!=i->catalog||!r237_owner_current(r,i->timeRoot)||
    memcmp(&r->actor_float_host,&i->host,sizeof(i->host))||memcmp(&r->anim_data,&i->map,sizeof(i->map)))return 0;
 found=task_member(r,t);
 return found&&t&&t->serial==i->serial&&t->status==TASK_READY&&!t->context_retired&&t->depth==i->depth&&
  t->depth>0&&t->depth<=MAX_FRAMES&& &t->frames[t->depth-1]==i->f&&i->f->fn==i->fn&&i->f->pc==i->pc;
}
int R243ActorFloatRuntimeMaskCurrent(const GscRuntime*r,unsigned mask){
 return r&&mask&&!(mask&~7u)&&r->actor_float_enabled&&r->actor_float_mask==mask&&r->program&&r->program->actor_float_selected==mask&&
  (!r243ActorFloatActive||(r243ActorFloatActive->seal.r==r&&r243_actor_float_scope(r243ActorFloatActive)));
}
static int r243_actor_float_current(void*opaque){
 R243ActorFloatCall*c=r243ActorFloatActive;GscRuntime*r;int ok;
 if(!c||(uintptr_t)opaque!=c->seal.call||!r243_actor_float_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;r=c->seal.r;
 ok=entity_task_current(r)&&r243_actor_float_scope(c)&&anim_data_current(r,c->seal.t)&&r243_actor_float_scope(c)&&
  state_entity_live(r,&c->expected.entity)==1&&r243_actor_float_scope(c)&&
  c->seal.host.validate(c->seal.host.user,&c->q.entity)==1&&r243_actor_float_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243ActorFloatExactCallCurrent(const R243ActorFloatRequest*q,int(*current)(void*),void*context){
 R243ActorFloatCall*c=r243ActorFloatActive;
 if(!c||!q||current!=r243_actor_float_current||(uintptr_t)context!=c->seal.call||!r243_actor_float_scope(c)||memcmp(q,&c->expected,sizeof(*q)))return 0;
 return current(context)==1&&r243_actor_float_scope(c);
}
/* Named readonly native tuples, never a global private-buffer exception.
 * The copied request is checked by VALUE against the independent sealed call
 * as well as actual stack/Task membership. Public Actor mutators stay closed. */
int R243ActorFloatValidationRefCurrent(const GscEntityRef*entity){
 R243ActorFloatCall*c=r243ActorFloatActive;
 return c&&c->checking&&entity==&c->q.entity&&r243_actor_float_scope(c);
}
int R243ActorFloatNativeBuffersCurrent(const R243ActorFloatRequest*q,const R243ActorFloatReply*out,const char*error,size_t cap,int(*current)(void*),void*context){
 R243ActorFloatCall*c=r243ActorFloatActive;
 if(!c||!q||!R243ActorFloatExactCallCurrent(q,current,context))return 0;
 if(q==&c->q&&out==&c->reply&&error==c->error&&cap==sizeof(c->error))return r243_actor_float_scope(c);
 return R243ActorFloatSessionBuffersCurrent(q,out,error,cap,current,context)&&r243_actor_float_scope(c);
}
static int r243_actor_float_call(GscRuntime*r,const Value*receiver,int method,int argc,int line,Value*out){
 R243ActorFloatCall c;Value value;int accepted=0,ok=0;unsigned mask=R243ActorFloatMask(method);
 if(!r||!receiver||argc!=0||!mask||receiver->kind!=V_ENTITY||!(r->program->actor_float_selected&mask)||!(r->actor_float_mask&mask)||!r->actor_float_enabled)
  return rfail(r,line,"actor float query selected Actor float query unavailable");
 if(r243ActorFloatActive||r->actor_float_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)return rfail(r,line,"actor float query callback reentered");
 if(r243ActorFloatCallNonce>=UINTPTR_MAX||!r->executing_task||r->executing_task->depth<1||r->executing_task->depth>MAX_FRAMES)
  return rfail(r,line,"actor float query genuine executing call unavailable");
 memset(&c,0,sizeof(c));c.id.r=r;c.id.g=r->program;c.id.t=r->executing_task;c.id.serial=c.id.t->serial;
 c.id.f=&c.id.t->frames[c.id.t->depth-1];c.id.depth=c.id.t->depth;c.id.fn=c.id.f->fn;c.id.pc=c.id.f->pc;
 c.id.profile_mask=r->actor_float_mask;c.id.program=r->program->function_program_token;c.id.runtime=r->array_token;c.id.catalog=r->animation_namespace.identity;
 c.id.timeRoot=r237_owner_stamp(r);c.id.call=++r243ActorFloatCallNonce;c.id.now=r->now_ms;c.id.map=r->anim_data;c.id.host=r->actor_float_host;
 c.q.entity=receiver->entity;c.q.program=c.id.program;c.q.runtime=c.id.runtime;c.q.catalog=c.id.catalog;c.q.task=c.id.serial;
 c.q.call=c.id.call;c.q.now=c.id.now;c.q.method=(uint32_t)method;
 /* All receiver/argument data is owned before callbacks can retire Results. */
 c.seal=c.id;c.expected=c.q;c.reply.bits=0x7fc00000u;r243ActorFloatActive=&c;r->actor_float_busy=1;r->entity_method_busy=1;
 if(!r243_actor_float_current((void*)(uintptr_t)c.id.call))goto done;
 accepted=c.seal.host.query(c.seal.host.user,&c.q,&c.reply,c.error,sizeof(c.error),r243_actor_float_current,(void*)(uintptr_t)c.id.call);c.error[sizeof(c.error)-1]=0;
 if(!r243_actor_float_current((void*)(uintptr_t)c.id.call))goto done;
 if(accepted!=1){rfail(r,line,"actor float query native provider refused: %s",c.error[0]?c.error:"Actor/path owner unavailable");goto done;}
 if(!ActorFloatReplyValid(method,c.reply.bits)){rfail(r,line,"actor float query reply outside the selected method range");goto done;}
 memset(&value,0,sizeof(value));value.kind=V_NUMBER;memcpy(&value.number,&c.reply.bits,sizeof(c.reply.bits));ok=1;
done:
 r243ActorFloatActive=NULL;r->actor_float_busy=0;r->entity_method_busy=0;
 if(!ok&&!r->error[0]&&c.seal.t->status!=TASK_DEAD)rfail(r,line,"actor float query caller/Actor scope changed");
 if(!ok||r->error[0])return 0;if(out)*out=value;return 1;
}
