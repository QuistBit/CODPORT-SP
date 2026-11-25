/* Selected native GAME20032B10/20032B90 receiver bridge. The first getter of
 * play is GetString(0); stop consumes no native argument getters. Extra script
 * arguments are evaluated by the VM and then ignored as in these handlers. */
int gsc_program_select_loop_sound(GscProgram *g){
 if(!g||g->invalid||g->linked||g->loop_sound_selected)return 0;
 g->loop_sound_selected=1;return 1;
}
int gsc_runtime_select_loop_sound(GscRuntime *r,const GscLoopSoundHost *h){
 if(!r||!h||!h->invoke||!h->finish||r->started||r->advancing||r->error[0]||
    !r->program->loop_sound_selected||r->loop_sound_enabled||
    !r->entities_enabled||!r->state_enabled||
    runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->loop_sound=*h;r->loop_sound_enabled=1;return 1;
}
typedef struct {
 GscRuntime *r;GscLoopSoundRequest request,sealed;GscLoopSoundHost host;
 Task *task;Frame *frame;const GscProgram *program;uint64_t serial,instance,programToken;
 uintptr_t token;int depth,fn,pc,now,checking,cancelled,active;
} GscLoopSoundCall;
static GscLoopSoundCall *loopSoundActive;static uintptr_t loopSoundToken;
static int loop_sound_scope(GscLoopSoundCall *c){
 int found=0;Task *t;
 if(!(c&&c==loopSoundActive&&c->active&&!c->cancelled&&c->r&&!c->r->error[0]&&
  c->r->entity_method_busy&&c->r->loop_sound_enabled&&
  c->r->program==c->program&&c->r->array_token==c->instance&&c->r->now_ms==c->now&&
  c->program->function_program_token==c->programToken&&c->r->executing_task==c->task&&
  !memcmp(&c->request,&c->sealed,sizeof(c->sealed))&&
  c->r->loop_sound.user==c->host.user&&c->r->loop_sound.invoke==c->host.invoke&&c->r->loop_sound.finish==c->host.finish))return 0;
 t=c->task;found=task_member(c->r,t);
 return found&&t->serial==c->serial&&t->status==TASK_READY&&!t->context_retired&&
  t->depth==c->depth&&t->depth>0&&t->depth<=MAX_FRAMES&&
  &t->frames[t->depth-1]==c->frame&&c->frame->fn==c->fn&&c->frame->pc==c->pc;
}
static int loop_sound_current(void *p){
 GscLoopSoundCall *c=loopSoundActive;int ok;
 if(!c||(uintptr_t)p!=c->token)return 0;
 if(!loop_sound_scope(c)||c->checking){if(c)c->cancelled=1;return 0;}
 c->checking=1;
 ok=entity_task_current(c->r)&&loop_sound_scope(c)&&
  state_entity_live(c->r,&c->sealed.owner)==1&&loop_sound_scope(c)&&entity_task_current(c->r);
 c->checking=0;if(!ok)c->cancelled=1;return ok&&loop_sound_scope(c);
}
static int loop_sound_invoke(GscRuntime *r,const Value *receiver,int descriptor,
 const Value *args,int argc,int line){
 GscLoopSoundCall c;int finished,ok,current;char error[384]={0};
 if(!r->loop_sound_enabled||!receiver||receiver->kind!=V_ENTITY||argc<0||argc>MAX_PARAMS)
  return rfail(r,line,"loop sound capability or receiver unavailable");
 if(loopSoundActive||loopSoundToken==UINTPTR_MAX||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
  return rfail(r,line,"loop sound callback reentered");
 memset(&c,0,sizeof(c));c.r=r;c.host=r->loop_sound;c.request.owner=receiver->entity;
 c.request.stop=descriptor==ENTITY_METHOD_STOPLOOPSOUND;
 r->entity_method_busy=1;finished=entity_method_finish(r,&c.request.owner,line);
 if(finished<0){r->entity_method_busy=0;return finished==-2;}
 if(!c.request.stop){
  if(argc<1||!args||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str))){
   r->entity_method_busy=0;return rfail(r,line,"playLoopSound GetString0 requires plain string");
  }
  memcpy(c.request.alias,args[0].str,sizeof(c.request.alias));
 }
 c.request.runtime=r->array_token;c.request.operation=loopSoundToken+1;
 c.sealed=c.request;c.task=r->executing_task;
 if(!c.task||c.task->depth<1||c.task->depth>MAX_FRAMES){r->entity_method_busy=0;return rfail(r,line,"loop sound task unavailable");}
 c.depth=c.task->depth;c.frame=&c.task->frames[c.depth-1];c.fn=c.frame->fn;c.pc=c.frame->pc;c.serial=c.task->serial;
 c.program=r->program;c.programToken=r->program->function_program_token;c.instance=r->array_token;c.now=r->now_ms;
 c.token=++loopSoundToken;c.active=1;loopSoundActive=&c;
 ok=loop_sound_current((void*)c.token)&&c.host.invoke(c.host.user,&c.request,error,sizeof(error),loop_sound_current,(void*)c.token)==1;
 error[sizeof(error)-1]=0;current=loop_sound_current((void*)c.token);
 finished=ok&&current&&!r->error[0]?entity_method_finish(r,&c.sealed.owner,line):0;
 ok=ok&&current&&finished==1&&loop_sound_current((void*)c.token);
 if(ok)ok=c.host.finish(c.host.user,&c.sealed,1,error,sizeof(error),loop_sound_current,(void*)c.token)==1;
 current=loop_sound_current((void*)c.token);
 if(!ok||!current||r->error[0]){
  /* Cancellation is addressed by immutable operation identity, also after
   * Session abort. Do not pass the potentially mutated request back. */
  if(c.host.finish(c.host.user,&c.sealed,0,error,sizeof(error),loop_sound_current,(void*)c.token)!=1)
   rfail(r,line,"loop sound exact cancellation failed");
  ok=0;
 }
 c.active=0;loopSoundActive=NULL;r->entity_method_busy=0;
 error[sizeof(error)-1]=0;
 if(!ok)return rfail(r,line,"loop sound transaction refused or owner changed: %s",error);
 return 1;
}
