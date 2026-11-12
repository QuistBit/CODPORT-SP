typedef struct {
 GscRuntime*r;const GscProgram*g;Task*t;Frame*f;
 uint64_t serial,program,runtime,catalog,timeRoot,call;
 int now,depth,fn,pc;GscMapExecutionCertificate map;R243AnimTimeHost host;
} R243AnimTimeIdentity;
typedef struct {
 R243AnimTimeIdentity id,seal;R243AnimTimeRequest q,expected;
 R243AnimTimeReply reply;char error[384];int checking,cancelled;
} R243AnimTimeCall;
static R243AnimTimeCall*r243AnimTimeActive;
static uint64_t r243AnimTimeCallNonce;
static int r243_animtime_vm_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&r243AnimTimeActive,sizeof(r243AnimTimeActive))||
  Cod1ModelStringOverlap(p,n,&r243AnimTimeCallNonce,sizeof(r243AnimTimeCallNonce))||
  (r243AnimTimeActive&&Cod1ModelStringOverlap(p,n,r243AnimTimeActive,sizeof(*r243AnimTimeActive)));
}
int R243ProgramSelectAnimTime(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
    !g->function_ref_syntax||!g->function_program_token||!g->anim_data_selected||!g->animation_namespace_selected||g->animtime_selected)return 0;
 g->animtime_selected=1;return 1;
}
int R243RuntimeSelectAnimTime(GscRuntime*r,const R243AnimTimeHost*input){
 R243AnimTimeHost h;const GscProgram*g;uint64_t instance,catalog;
 if(!input||r243AnimTimeActive||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    !r->results_enabled||!r->state_enabled||!r->entities_enabled||!r->program->animtime_selected||r->animtime_enabled||
    !gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.validate||!h.query)return 0;g=r->program;instance=r->array_token;catalog=r->animation_namespace.identity;
 if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    r->program!=g||r->array_token!=instance||r->animation_namespace.identity!=catalog||r->animtime_enabled)return 0;
 r->animtime_host=h;r->animtime_enabled=1;return 1;
}
static int r243_animtime_scope(R243AnimTimeCall*c){
 const R243AnimTimeIdentity*i;GscRuntime*r;Task*t;int n,found=0;
 if(!c||c!=r243AnimTimeActive||c->cancelled||memcmp(&c->id,&c->seal,sizeof(c->id))||memcmp(&c->q,&c->expected,sizeof(c->q)))return 0;
 i=&c->seal;r=i->r;t=i->t;
 if(!r||!r->started||!r->advancing||r->error[0]||!r->animtime_enabled||!r->animtime_busy||!r->entity_method_busy||
    r->program!=i->g||!r->program->animtime_selected||r->program->function_program_token!=i->program||
    r->array_token!=i->runtime||r->now_ms!=i->now||r->executing_task!=t||!animation_namespace_scope(r)||
    r->animation_namespace.identity!=i->catalog||!r237_owner_current(r,i->timeRoot)||
    memcmp(&r->animtime_host,&i->host,sizeof(i->host))||memcmp(&r->anim_data,&i->map,sizeof(i->map)))return 0;
 for(n=0;n<r->task_count;n++)if(task_at(r,n)==t){found=1;break;}
 return found&&t&&t->serial==i->serial&&t->status==TASK_READY&&!t->context_retired&&t->depth==i->depth&&
  t->depth>0&&t->depth<=MAX_FRAMES&& &t->frames[t->depth-1]==i->f&&i->f->fn==i->fn&&i->f->pc==i->pc;
}
static int r243_animtime_current(void*opaque){
 R243AnimTimeCall*c=r243AnimTimeActive;GscRuntime*r;int ok;
 if(!c||(uintptr_t)opaque!=c->seal.call||!r243_animtime_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;r=c->seal.r;
 ok=entity_task_current(r)&&r243_animtime_scope(c)&&anim_data_current(r,c->seal.t)&&r243_animtime_scope(c)&&
  state_entity_live(r,&c->expected.entity)==1&&r243_animtime_scope(c)&&
  c->seal.host.validate(c->seal.host.user,&c->q.entity)==1&&r243_animtime_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243AnimTimeExactCallCurrent(const R243AnimTimeRequest*q,int(*current)(void*),void*context){
 R243AnimTimeCall*c=r243AnimTimeActive;
 if(!c||!q||current!=r243_animtime_current||(uintptr_t)context!=c->seal.call||!r243_animtime_scope(c)||memcmp(q,&c->expected,sizeof(*q)))return 0;
 return current(context)==1&&r243_animtime_scope(c);
}
/* Named readonly native tuples, never a global private-buffer exception.
 * The copied request is checked by VALUE against the independent sealed call
 * as well as actual stack/Task membership. Public Actor mutators stay closed. */
int R243AnimTimeValidationRefCurrent(const GscEntityRef*entity){
 R243AnimTimeCall*c=r243AnimTimeActive;
 return c&&c->checking&&entity==&c->q.entity&&r243_animtime_scope(c);
}
int R243AnimTimeNativeBuffersCurrent(const R243AnimTimeRequest*q,const R243AnimTimeReply*out,const char*error,size_t cap,int(*current)(void*),void*context){
 R243AnimTimeCall*c=r243AnimTimeActive;
 if(!c||!q||!R243AnimTimeExactCallCurrent(q,current,context))return 0;
 if(q==&c->q&&out==&c->reply&&error==c->error&&cap==sizeof(c->error))return r243_animtime_scope(c);
 return R243AnimTimeSessionBuffersCurrent(q,out,error,cap,current,context)&&r243_animtime_scope(c);
}
static int r243_animtime_call(GscRuntime*r,const Value*receiver,const Value*args,int argc,int line,Value*out){
 R243AnimTimeCall c;Value value;int accepted=0,ok=0;GscAnimationLengthRequest animation;
 if(!r||!receiver||!args||argc!=1||receiver->kind!=V_ENTITY||!r->program->animtime_selected||!r->animtime_enabled)
  return rfail(r,line,"getAnimTime selected Actor predicate unavailable");
 if(r243AnimTimeActive||r->animtime_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)return rfail(r,line,"getAnimTime callback reentered");
 if(!actor_animation_binding(r,args,line,&animation))return 0;
 if(r243AnimTimeCallNonce>=UINTPTR_MAX||!r->executing_task||r->executing_task->depth<1||r->executing_task->depth>MAX_FRAMES)
  return rfail(r,line,"getAnimTime genuine executing call unavailable");
 memset(&c,0,sizeof(c));c.id.r=r;c.id.g=r->program;c.id.t=r->executing_task;c.id.serial=c.id.t->serial;
 c.id.f=&c.id.t->frames[c.id.t->depth-1];c.id.depth=c.id.t->depth;c.id.fn=c.id.f->fn;c.id.pc=c.id.f->pc;
 c.id.program=r->program->function_program_token;c.id.runtime=r->array_token;c.id.catalog=r->animation_namespace.identity;
 c.id.timeRoot=r237_owner_stamp(r);c.id.call=++r243AnimTimeCallNonce;c.id.now=r->now_ms;c.id.map=r->anim_data;c.id.host=r->animtime_host;
 c.q.entity=receiver->entity;c.q.program=c.id.program;c.q.runtime=c.id.runtime;c.q.catalog=c.id.catalog;c.q.task=c.id.serial;
 c.q.call=c.id.call;c.q.now=c.id.now;c.q.animation=animation;
 /* All receiver/argument data is owned before callbacks can retire Results. */
 c.seal=c.id;c.expected=c.q;c.reply.bits=0x7fc00000u;r243AnimTimeActive=&c;r->animtime_busy=1;r->entity_method_busy=1;
 if(!r243_animtime_current((void*)(uintptr_t)c.id.call))goto done;
 accepted=c.seal.host.query(c.seal.host.user,&c.q,&c.reply,c.error,sizeof(c.error),r243_animtime_current,(void*)(uintptr_t)c.id.call);c.error[sizeof(c.error)-1]=0;
 if(!r243_animtime_current((void*)(uintptr_t)c.id.call))goto done;
 if(accepted!=1){rfail(r,line,"getAnimTime native provider refused: %s",c.error[0]?c.error:"Actor/path owner unavailable");goto done;}
 if(c.reply.bits>0x3f800000u){rfail(r,line,"getAnimTime requires finite normalized float32 phase");goto done;}
 memset(&value,0,sizeof(value));value.kind=V_NUMBER;memcpy(&value.number,&c.reply.bits,4);ok=1;
done:
 r243AnimTimeActive=NULL;r->animtime_busy=0;r->entity_method_busy=0;
 if(!ok&&!r->error[0]&&c.seal.t->status!=TASK_DEAD)rfail(r,line,"getAnimTime caller/Actor scope changed");
 if(!ok||r->error[0])return 0;if(out)*out=value;return 1;
}
