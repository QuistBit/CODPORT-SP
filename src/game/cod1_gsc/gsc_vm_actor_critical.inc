/* R190 explicitly selected methods; neither the native registry nor default
 * Session selectors gain these names. The host owns Actor state and labels. */
int gsc_program_select_actor_critical_methods(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->actor_critical_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->actor_critical_selected=1;return 1;
}
int gsc_runtime_select_actor_critical_methods(GscRuntime *r,const GscActorCriticalHost *host) {
    if(!r||!host||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->actor_critical_enabled||!r->program->actor_critical_selected||!r->entities_enabled||
       !r->state_enabled||!r->names_enabled||!r->scalar_switch_enabled||!r->results_enabled||
       r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       !host->invoke||runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->actor_critical=*host;r->actor_critical_enabled=1;return 1;
}

/* Exact synchronous critical call; no caller receives a writable context. */
typedef struct {
 GscRuntime*r;Task*task;Frame*frame;const GscProgram*program;
 GscActorCriticalHost host;GscEntityRef owner,borrowed;GscActorCriticalCall call,expected;
 Value label;uint64_t serial,instance,programToken;uintptr_t nonce;
 uint32_t method;int depth,fn,pc,now,reads,checking,cancelled;
} R243CriticalVMCall;
static R243CriticalVMCall*r243CriticalVMActive;static uintptr_t r243CriticalVMNonce;
static int r243_critical_vm_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&r243CriticalVMActive,sizeof(r243CriticalVMActive))||Cod1ModelStringOverlap(p,n,&r243CriticalVMNonce,sizeof(r243CriticalVMNonce))||
 (r243CriticalVMActive&&Cod1ModelStringOverlap(p,n,r243CriticalVMActive,sizeof(*r243CriticalVMActive)));
}
static int r243_critical_vm_scope(R243CriticalVMCall*c){
 GscRuntime*r;Task*t;int found=0;
 if(!c||c!=r243CriticalVMActive||c->cancelled)return 0;r=c->r;t=c->task;
 if(!r||r->error[0]||!r->actor_critical_enabled||!r->entity_method_busy||r->program!=c->program||!r->program->actor_critical_selected||
 r->program->function_program_token!=c->programToken||r->array_token!=c->instance||r->now_ms!=c->now||r->executing_task!=t||
 memcmp(&r->actor_critical,&c->host,sizeof(c->host))||memcmp(&c->borrowed,&c->owner,sizeof(c->owner))||memcmp(&c->call,&c->expected,sizeof(c->call)))return 0;
 found=task_member(r,t);
 return found&&t->serial==c->serial&&t->status==TASK_READY&&!t->context_retired&&t->depth==c->depth&&t->depth>0&&t->depth<=MAX_FRAMES&&
 &t->frames[t->depth-1]==c->frame&&c->frame->fn==c->fn&&c->frame->pc==c->pc;
}
static int actor_critical_context_current(void*token){
 R243CriticalVMCall*c=r243CriticalVMActive;int ok;
 if(!c||(uintptr_t)token!=c->nonce)return 0;
 if(!r243_critical_vm_scope(c)){c->cancelled=1;return 0;}
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;
 ok=entity_task_current(c->r)&&r243_critical_vm_scope(c);c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243CriticalVMExact(const GscEntityRef*e,uint32_t method,const GscActorCriticalCall*q){
 R243CriticalVMCall*c=r243CriticalVMActive;
 return c&&e&&q&&method==c->method&&!memcmp(e,&c->owner,sizeof(*e))&&!memcmp(q,&c->expected,sizeof(*q))&&actor_critical_context_current(q->context);
}
static int actor_critical_read_label(void*token,char*out,size_t cap,char*error,size_t size){
 R243CriticalVMCall*c=r243CriticalVMActive;size_t n;
 if(!c||!actor_critical_context_current(token)||!c->expected.has_argument||++c->reads!=1||
 !gsc_runtime_r243_controller_storage_separate(c->r,out,cap)||!gsc_runtime_r243_controller_storage_separate(c->r,error,size)||
 Cod1ModelStringOverlap(out,cap,error,size))return 0;
 if(c->label.kind!=V_STRING||!memchr(c->label.str,0,sizeof(c->label.str))){if(error&&size)snprintf(error,size,"critical GetConstString0 outside selected plain-string domain");return 0;}
 n=strlen(c->label.str);if(!out||n>=cap){if(error&&size)snprintf(error,size,"critical label output too small");return 0;}
 memcpy(out,c->label.str,n+1);return actor_critical_context_current(token);
}
static int actor_critical_invoke(GscRuntime*r,const Value*receiver,int descriptor,const Value*args,int argc,int line,Value*out){
 R243CriticalVMCall c;char error[384];int accepted,finished;int32_t result=-1;
 if(!r->actor_critical_enabled||!receiver||receiver->kind!=V_ENTITY||descriptor<ENTITY_METHOD_ENTERCRITICAL||descriptor>ENTITY_METHOD_ISCRITICAL||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"Actor critical capability or receiver unavailable");
 if(r243CriticalVMActive||r243CriticalVMNonce==UINTPTR_MAX||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"Actor critical callback reentered");
 memset(&c,0,sizeof(c));c.r=r;c.task=r->executing_task;
 if(!c.task||c.task->depth<=0||c.task->depth>MAX_FRAMES)return rfail(r,line,"Actor critical executing task missing");
 c.owner=c.borrowed=receiver->entity;c.host=r->actor_critical;c.program=r->program;c.programToken=r->program->function_program_token;c.instance=r->array_token;c.now=r->now_ms;
 c.serial=c.task->serial;c.depth=c.task->depth;c.frame=&c.task->frames[c.depth-1];c.fn=c.frame->fn;c.pc=c.frame->pc;c.nonce=++r243CriticalVMNonce;
 c.method=descriptor==ENTITY_METHOD_ENTERCRITICAL?GSC_ACTOR_CRITICAL_ENTER:descriptor==ENTITY_METHOD_LEAVECRITICAL?GSC_ACTOR_CRITICAL_LEAVE:GSC_ACTOR_CRITICAL_QUERY;
 if(argc&&c.method==GSC_ACTOR_CRITICAL_ENTER)c.label=args[0];
 c.call.context=(void*)c.nonce;c.call.has_argument=argc>0&&c.method==GSC_ACTOR_CRITICAL_ENTER;c.call.read_label=actor_critical_read_label;c.call.current=actor_critical_context_current;c.expected=c.call;
 r->entity_method_busy=1;finished=entity_method_finish(r,&c.owner,line);
 if(finished<0){r->entity_method_busy=0;return finished==-2;}
 r243CriticalVMActive=&c;error[0]=0;
 accepted=actor_critical_context_current((void*)c.nonce)?c.host.invoke(c.host.user,&c.borrowed,c.method,&c.call,&result,error,sizeof(error)):0;
 error[sizeof(error)-1]=0;
 if(accepted==1&&!actor_critical_context_current((void*)c.nonce))accepted=0;
 r243CriticalVMActive=NULL;
 if(r->error[0]){r->entity_method_busy=0;return 0;}
 if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
 if(accepted!=1){r->entity_method_busy=0;return rfail(r,line,"Actor critical method refused: %s",error);}
 finished=entity_method_finish(r,&c.owner,line);r->entity_method_busy=0;if(finished<0)return finished==-2;
 if(c.method==GSC_ACTOR_CRITICAL_QUERY){if(result!=0&&result!=1)return rfail(r,line,"Actor critical query returned non-boolean result");if(out)scalar_integer(out,result);}
 return 1;
}
