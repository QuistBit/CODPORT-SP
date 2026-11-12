/* Explicit opt-in native tracking lane. No default method registry change.
 * The host commits previous before requesting arg0, current before arg1.
 * StringArguments supplies retained native conversion/error-prefix semantics. */
int gsc_program_select_r243_script_state(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->r243_script_state_selected||!g->names_syntax)return 0;
 g->r243_script_state_selected=1;return 1;
}
int gsc_runtime_select_r243_script_state(GscRuntime*r,const GscActorScriptStateHost*input){
 GscActorScriptStateHost host;
 if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    !r->program->r243_script_state_selected||r->r243_script_state_enabled||!r->entities_enabled||
    !r->state_enabled||!r->names_enabled||!r->lifetime_enabled||
    r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy||
    runtime_private_overlap(r,input,sizeof(*input)))return 0;
 host=*input;if(!host.invoke)return 0;
 r->r243_script_state=host;r->r243_script_state_enabled=1;return 1;
}
typedef struct {
 GscRuntime*r;Task*task;uint64_t serial,instance,time_root;const GscProgram*program;
 GscActorScriptStateHost host;StringArguments arguments;int line,next,now,bucket,*budget,budget_value,checking;
 GscEntityExecutionCertificate certificate;GscMapExecutionCertificate map;
} R243ScriptStateContext;
static int r243_script_state_scope(const R243ScriptStateContext*c){
 return c&&c->r&&r237_owner_current(c->r,c->time_root)&&!c->r->error[0]&&c->r->advancing&&c->r->entity_method_busy&&c->r->r243_script_state_enabled&&
  c->r->program==c->program&&c->r->executing_task==c->task&&c->task->serial==c->serial&&
  c->task->status==TASK_READY&&c->r->array_token==c->instance&&c->r->now_ms==c->now&&
  r237_bucket(c->r)==c->bucket&&c->r->execution_budget==c->budget&&c->budget&&*c->budget==c->budget_value&&
  !memcmp(&c->r->r243_script_state,&c->host,sizeof(c->host))&&
  !memcmp(&c->task->certificate,&c->certificate,sizeof(c->certificate))&&
  !memcmp(&c->task->map_certificate,&c->map,sizeof(c->map));
}
static int r243_script_state_current(void*context){
 R243ScriptStateContext*c=context;int ok;
 if(!r243_script_state_scope(c))return 0;
 if(c->checking)return rfail(c->r,c->line,"trackScriptState current callback reentered");
 c->checking=1;ok=entity_task_current(c->r)&&r243_script_state_scope(c);c->checking=0;return ok;
}
static int r243_script_state_read(void*context,unsigned index,char*out,size_t cap,char*error,size_t size){
 R243ScriptStateContext*c=context;const char*text;size_t n;(void)error;(void)size;
 if(!c||!out||index!=(unsigned)c->next||index>1||!r243_script_state_current(c))return 0;
 ++c->next;
 if(!string_arguments_get(c->r,&c->arguments,index,c->line,&text))return 0;
 if(!r243_script_state_current(c))return 0;
 n=strlen(text);if(n>=cap)return 0;memcpy(out,text,n+1);return 1;
}
static int r243_script_state_invoke(GscRuntime*r,const Value*receiver,const Value*args,int argc,int line){
 R243ScriptStateContext c;GscActorScriptStateCall call,expected;GscEntityRef owner,borrowed;
 char why[384];int accepted=0,finished,ok=0;
 if(!r||!r->r243_script_state_enabled||!receiver||receiver->kind!=V_ENTITY||
    argc<0||argc>MAX_PARAMS||(argc&&!args)||!r->executing_task||
    r->executing_task->status!=TASK_READY)
  return r?rfail(r,line,"selected trackScriptState receiver or capability unavailable"):0;
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
    r->executing_task->host_entry_pins==UINT_MAX)return rfail(r,line,"trackScriptState callback reentered");
 memset(&c,0,sizeof(c));c.r=r;c.task=r->executing_task;c.serial=c.task->serial;
 c.time_root=r237_owner_stamp(r);c.program=r->program;c.instance=r->array_token;c.now=r->now_ms;c.bucket=r237_bucket(r);
 c.budget=r->execution_budget;c.budget_value=c.budget?*c.budget:0;c.certificate=c.task->certificate;c.map=c.task->map_certificate;
 c.host=r->r243_script_state;c.line=line;
 if(!string_arguments_begin(r,args,argc,line,&c.arguments))return 0;
 owner=receiver->entity;borrowed=owner;call.context=&c;call.argc=argc;
 call.read=r243_script_state_read;call.current=r243_script_state_current;expected=call;
 ++c.task->host_entry_pins;r->entity_method_busy=1;
 finished=entity_method_finish(r,&owner,line);
 if(finished<0){ok=finished==-2;goto done;}
 why[0]=0;accepted=c.host.invoke(c.host.user,&borrowed,&call,why,sizeof(why));why[sizeof(why)-1]=0;
 if(r->error[0])goto done;
 if(!entity_task_current(r)){ok=!r->error[0];goto done;}
 if(accepted!=1){rfail(r,line,"trackScriptState refused: %s",why);goto done;}
 if(c.next!=2||memcmp(&call,&expected,sizeof(call))||memcmp(&borrowed,&owner,sizeof(owner))||
    !r243_script_state_current(&c)){rfail(r,line,"trackScriptState host did not consume the genuine ordered arguments");goto done;}
 finished=entity_method_finish(r,&owner,line);ok=finished==1||finished==-2;
done:
 r->entity_method_busy=0;if(c.task->host_entry_pins)--c.task->host_entry_pins;
 string_arguments_end(&c.arguments);return ok;
}
