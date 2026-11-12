typedef struct {
 GscRuntime*r;const GscProgram*g;Task*t;Frame*f;
 uint64_t serial,program,runtime,catalog,timeRoot,call;
 int now,depth,fn,pc;GscMapExecutionCertificate map;R243StanceHost host;
} R243StanceIdentity;
typedef struct {
 R243StanceIdentity id,seal;R243StanceRequest q,expected;
 R243StanceReply reply;char error[384];int checking,cancelled;
} R243StanceCall;
static R243StanceCall*r243StanceActive;
static uint64_t r243StanceCallNonce;
static int r243_stance_vm_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&r243StanceActive,sizeof(r243StanceActive))||
  Cod1ModelStringOverlap(p,n,&r243StanceCallNonce,sizeof(r243StanceCallNonce))||
  (r243StanceActive&&Cod1ModelStringOverlap(p,n,r243StanceActive,sizeof(*r243StanceActive)));
}
int R243ProgramSelectStance(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
    !g->function_ref_syntax||!g->function_program_token||!g->anim_data_selected||!g->animation_namespace_selected||g->stance_selected)return 0;
 g->stance_selected=1;return 1;
}
int Cod1ProgramSelectStanceWrite(GscProgram *g){
 if(!g || g->stance_selected!=1 || g->linked || g->module_count)return 0;
 g->stance_selected=2;
 return 1;
}
int R243RuntimeSelectStance(GscRuntime*r,const R243StanceHost*input){
 R243StanceHost h;const GscProgram*g;uint64_t instance,catalog;
 if(!input||r243StanceActive||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    !r->results_enabled||!r->state_enabled||!r->entities_enabled||!r->program->stance_selected||r->stance_enabled||
    !gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
 h=*input;if(!h.validate||!h.query)return 0;g=r->program;instance=r->array_token;catalog=r->animation_namespace.identity;
 if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    r->program!=g||r->array_token!=instance||r->animation_namespace.identity!=catalog||r->stance_enabled)return 0;
 r->stance_host=h;r->stance_enabled=1;return 1;
}
static int r243_stance_scope(R243StanceCall*c){
 const R243StanceIdentity*i;GscRuntime*r;Task*t;int n,found=0;
 if(!c||c!=r243StanceActive||c->cancelled||memcmp(&c->id,&c->seal,sizeof(c->id))||memcmp(&c->q,&c->expected,sizeof(c->q)))return 0;
 i=&c->seal;r=i->r;t=i->t;
 if(!r||!r->started||!r->advancing||r->error[0]||!r->stance_enabled||!r->stance_busy||!r->entity_method_busy||
    r->program!=i->g||!r->program->stance_selected||r->program->function_program_token!=i->program||
    r->array_token!=i->runtime||r->now_ms!=i->now||r->executing_task!=t||!animation_namespace_scope(r)||
    r->animation_namespace.identity!=i->catalog||!r237_owner_current(r,i->timeRoot)||
    memcmp(&r->stance_host,&i->host,sizeof(i->host))||memcmp(&r->anim_data,&i->map,sizeof(i->map)))return 0;
 for(n=0;n<r->task_count;n++)if(task_at(r,n)==t){found=1;break;}
 return found&&t&&t->serial==i->serial&&t->status==TASK_READY&&!t->context_retired&&t->depth==i->depth&&
  t->depth>0&&t->depth<=MAX_FRAMES&& &t->frames[t->depth-1]==i->f&&i->f->fn==i->fn&&i->f->pc==i->pc;
}
static int r243_stance_current(void*opaque){
 R243StanceCall*c=r243StanceActive;GscRuntime*r;int ok;
 if(!c||(uintptr_t)opaque!=c->seal.call||!r243_stance_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;r=c->seal.r;
 ok=entity_task_current(r)&&r243_stance_scope(c)&&anim_data_current(r,c->seal.t)&&r243_stance_scope(c)&&
  state_entity_live(r,&c->expected.entity)==1&&r243_stance_scope(c)&&
  c->seal.host.validate(c->seal.host.user,&c->q.entity)==1&&r243_stance_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243StanceExactCallCurrent(const R243StanceRequest*q,int(*current)(void*),void*context){
 R243StanceCall*c=r243StanceActive;
 if(!c||!q||current!=r243_stance_current||(uintptr_t)context!=c->seal.call||!r243_stance_scope(c)||memcmp(q,&c->expected,sizeof(*q)))return 0;
 return current(context)==1&&r243_stance_scope(c);
}
/* Named readonly native tuples, never a global private-buffer exception.
 * The copied request is checked by VALUE against the independent sealed call
 * as well as actual stack/Task membership. Public Actor mutators stay closed. */
int R243StanceValidationRefCurrent(const GscEntityRef*entity){
 R243StanceCall*c=r243StanceActive;
 return c&&c->checking&&entity==&c->q.entity&&r243_stance_scope(c);
}
int R243StanceNativeBuffersCurrent(const R243StanceRequest*q,const R243StanceReply*out,const char*error,size_t cap,int(*current)(void*),void*context){
 R243StanceCall*c=r243StanceActive;
 if(!c||!q||!R243StanceExactCallCurrent(q,current,context))return 0;
 if(q==&c->q&&out==&c->reply&&error==c->error&&cap==sizeof(c->error))return r243_stance_scope(c);
 return R243StanceSessionBuffersCurrent(q,out,error,cap,current,context)&&r243_stance_scope(c);
}
static int cod1_stance_invoke(GscRuntime*r,const Value*receiver,const Value*args,int argc,int line,Value*out,int write){
 uint32_t mask=0;
 int i;
 R243StanceCall c;const char*end;Value value;int accepted=0,ok=0;uint32_t bit;
 if(!r||!receiver||!args||argc<1||argc>MAX_PARAMS||(!write&&argc!=1)||receiver->kind!=V_ENTITY||!r->program->stance_selected||!r->stance_enabled)
  return rfail(r,line,"isStanceAllowed selected Actor predicate unavailable");
 if(write){
  if(r->program->stance_selected!=2||!r->stance_host.allowWrite)return rfail(r,line,"allowedStances setter unavailable");
  for(i=0;i<argc;i++){
   uint32_t flag;
   if(args[i].kind!=V_STRING||!memchr(args[i].str,0,sizeof(args[i].str))||
      !(flag=R243StanceBit(args[i].str)))return rfail(r,line,"allowedStances requires stand, crouch or prone");
   mask|=flag;
  }
 }
 if(r243StanceActive||r->stance_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)return rfail(r,line,"isStanceAllowed callback reentered");
 if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||
    end-args[0].str>=8||(bit=R243StanceBit(args[0].str))==0)return rfail(r,line,"isStanceAllowed requires stand, crouch or prone");
 if(r243StanceCallNonce>=UINTPTR_MAX||!r->executing_task||r->executing_task->depth<1||r->executing_task->depth>MAX_FRAMES)
  return rfail(r,line,"isStanceAllowed genuine executing call unavailable");
 memset(&c,0,sizeof(c));c.id.r=r;c.id.g=r->program;c.id.t=r->executing_task;c.id.serial=c.id.t->serial;
 c.id.f=&c.id.t->frames[c.id.t->depth-1];c.id.depth=c.id.t->depth;c.id.fn=c.id.f->fn;c.id.pc=c.id.f->pc;
 c.id.program=r->program->function_program_token;c.id.runtime=r->array_token;c.id.catalog=r->animation_namespace.identity;
 c.id.timeRoot=r237_owner_stamp(r);c.id.call=++r243StanceCallNonce;c.id.now=r->now_ms;c.id.map=r->anim_data;c.id.host=r->stance_host;
 c.q.entity=receiver->entity;c.q.program=c.id.program;c.q.runtime=c.id.runtime;c.q.catalog=c.id.catalog;c.q.task=c.id.serial;
 c.q.call=c.id.call;c.q.now=c.id.now;c.q.bit=bit;c.q.writeMask=mask;memcpy(c.q.stance,args[0].str,(size_t)(end-args[0].str));
 /* All receiver/argument data is owned before callbacks can retire Results. */
 c.seal=c.id;c.expected=c.q;c.reply.allowed=2;r243StanceActive=&c;r->stance_busy=1;r->entity_method_busy=1;
 if(!r243_stance_current((void*)(uintptr_t)c.id.call))goto done;
 accepted=c.seal.host.query(c.seal.host.user,&c.q,&c.reply,c.error,sizeof(c.error),r243_stance_current,(void*)(uintptr_t)c.id.call);c.error[sizeof(c.error)-1]=0;
 if(!r243_stance_current((void*)(uintptr_t)c.id.call))goto done;
 if(accepted!=1){rfail(r,line,"isStanceAllowed native provider refused: %s",c.error[0]?c.error:"Actor/path owner unavailable");goto done;}
 if(c.reply.allowed!=0&&c.reply.allowed!=1){rfail(r,line,"isStanceAllowed requires a typed integer boolean reply");goto done;}
 scalar_integer(&value,c.reply.allowed);ok=1;
done:
 r243StanceActive=NULL;r->stance_busy=0;r->entity_method_busy=0;
 if(!ok&&!r->error[0]&&c.seal.t->status!=TASK_DEAD)rfail(r,line,"isStanceAllowed caller/Actor scope changed");
 if(!ok||r->error[0])return 0;if(out)*out=value;return 1;
}
static int r243_stance_call(GscRuntime*r,const Value*v,const Value*args,int argc,int line,Value*out){
 return cod1_stance_invoke(r,v,args,argc,line,out,0);
}
