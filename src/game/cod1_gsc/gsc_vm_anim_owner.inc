/* Fixed global STRUCT data owner. Native object/GC/animation resources are
 * separate; portable data uses existing bounded inline fields/array arenas. */
int gsc_program_select_anim_data(GscProgram *g){
    if(!g||g->invalid||g->linked||g->anim_data_selected||!g->names_syntax||!g->scalar_switch_syntax||!g->struct_syntax)return 0;
    g->anim_data_selected=1;return 1;
}
/* Whole-function admission views retain original declarations. A declaration
 * belonging only to another deferred function must not poison this complete
 * function's preflight. Active functions still validate every expression,
 * including dead branches, and every reserved formal/wait-result binding. */
static int anim_data_function_selected(const GscProgram*g,int f,const unsigned char*mask){
 return !g->function_admission_selected||(g->funcs[f].admission&&(!mask||mask[f]));
}
static int anim_data_tree_literal_needed(const GscProgram*g,int index,const unsigned char*mask){
 const StringLiteral*s=g->strings+index;int f,n,k,latest;
 if(!g->function_admission_selected)return 1;
 for(f=0;f<g->func_count;f++)if(anim_data_function_selected(g,f,mask))
  for(n=g->funcs[f].expr_start;n<g->funcs[f].expr_end;n++){
   const Expr*e=g->expr+n;
   if((e->kind!=E_ANIM_REF&&e->kind!=E_ANIMTREE_VALUE)||e->owner_module!=s->module||s->source_offset>=e->audit_offset)continue;
   latest=index;
   for(k=0;k<g->string_count;k++)if(g->strings[k].owner_kind==GSC_STRING_OWNER_ANIMTREE&&g->strings[k].module==s->module&&g->strings[k].source_offset<e->audit_offset&&g->strings[k].source_offset>g->strings[latest].source_offset)latest=k;
   if(latest==index)return 1;
  }
 return 0;
}
static int anim_data_program_ready_checked(const GscProgram *g,const unsigned char*mask,char*error,size_t cap){
 int n,k;
 if(!g||!g->anim_data_selected||(g->requires_anim_ref_semantics&&!g->animation_refs_selected&&!g->animation_namespace_selected)||(g->requires_animtree_value_semantics&&!g->animation_namespace_selected))return 0;
 for(n=0;n<g->string_count;++n){
  const StringLiteral *literal=&g->strings[n];
  int decoded;
  if(literal->owner_kind!=GSC_STRING_OWNER_ANIMTREE)continue;
  decoded=string_literal_decoded(g,literal);
  if(decoded && (g->animation_namespace_selected
      ? animation_namespace_tree_index_for_program(g,literal->decoded)>=0
      : !strcmp(literal->decoded,"generic_human")))continue;
  /* Only an unsupported declaration needs the per-function admission search. */
  if(anim_data_tree_literal_needed(g,n,mask)){
   if(error&&cap)snprintf(error,cap,"animtree declaration unsupported: %s in %s (decoded=%d)",literal->decoded,g->modules[literal->module],decoded);
   return 0;
  }
 }
 for(n=0;n<g->func_count;++n)if(anim_data_function_selected(g,n,mask))for(k=0;k<g->funcs[n].argc;++k)if(!strcmp(g->funcs[n].params[k],"anim")){if(error&&cap)snprintf(error,cap,"reserved anim formal in %s::%s",g->modules[g->funcs[n].module],g->funcs[n].name);return 0;}
 {int f,first,last;
  for(f=0;f<(g->function_admission_selected?g->func_count:1);f++){
   if(g->function_admission_selected&&!anim_data_function_selected(g,f,mask))continue;
   first=g->function_admission_selected?g->funcs[f].start:0;last=g->function_admission_selected?g->funcs[f].end:g->code_count;
   for(n=first;n<last;n++){const Instr*i=g->code+n;
    if(i->kind!=I_WAITER&&!(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT&&i->receiver_event_target==ENTITY_EVENT_WAITTILL))continue;
    for(k=1;k<i->argc;++k)if(i->args[k]>=0&&i->args[k]<g->expr_count&&g->expr[i->args[k]].kind==E_VAR&&!strcmp(g->expr[i->args[k]].name,"anim")){if(error&&cap)snprintf(error,cap,"reserved anim wait result at line%d",i->line);return 0;}
   }
  }
 }
 return 1;
}
static int anim_data_program_ready_mask(const GscProgram*g,const unsigned char*mask){return anim_data_program_ready_checked(g,mask,NULL,0);}
int gsc_runtime_select_anim_data(GscRuntime *r,const GscMapExecutionCertificate *input){
    GscMapExecutionCertificate copy;int current;
    if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->anim_data_enabled||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
       !r->state_enabled||!r->arrays_enabled||!r->results_enabled||!r->structs_enabled||!r->array_token||
       !anim_data_program_ready_mask(r->program,r->program->function_admission_selected?r->function_admission:NULL)||runtime_private_overlap(r,input,sizeof(*input)))return 0;
    copy=*input;if(!copy.receipt||!copy.current)return 0;
    r->entity_certificate_busy=1;current=copy.current(copy.receipt);r->entity_certificate_busy=0;
    if(current!=1||r->error[0]||r->started||r->anim_data_enabled)return 0;
    r->anim_data=copy;r->anim_data_enabled=1;return 1;
}
static int anim_data_value_valid(const GscRuntime *r,const Value *value){
    return r&&r->anim_data_enabled&&value&&value->kind==V_ANIM&&value->object==1&&value->object_token==r->array_token;
}
static int anim_data_current(GscRuntime *r,Task *task){
    GscMapExecutionCertificate copy;int current;uint64_t namespace_identity=0,runtime_identity=0,time_root=0;
    if(!r->anim_data_enabled)return 1;
    if(task){time_root=r237_owner_stamp(r);if(!r237_owner_current(r,time_root))return rfail(r,0,"anim owner Task has no current time-root");}
    if(r->animation_namespace_enabled&&!animation_namespace_scope(r))return rfail(r,0,"animation namespace authority changed");
    if(r->animation_namespace_enabled){namespace_identity=r->animation_namespace.identity;runtime_identity=r->array_token;}
    if(task&&!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during animation namespace validation");
    if(r->animation_refs_enabled&&!animation_references_current(r,task))return 0;
    if(task&&!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during animation references validation");
    if(task&&task->status==TASK_DEAD)return 0;
    if(r->error[0])return 0;
    if(r->entity_certificate_busy||r->entity_prepare_busy)return rfail(r,0,"anim owner callback reentered");
    copy=r->anim_data;r->entity_certificate_busy=1;current=copy.current(copy.receipt);r->entity_certificate_busy=0;
    if(task&&!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during anim owner certificate");
    if(r->error[0])return 0;
    if(r->animation_namespace_enabled&&!animation_namespace_scope(r))return rfail(r,0,"animation namespace authority changed during callback");
    if(task&&!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during animation namespace revalidation");
    if(namespace_identity&&(!r->animation_namespace_enabled||r->animation_namespace.identity!=namespace_identity||r->array_token!=runtime_identity))return rfail(r,0,"animation namespace identity changed during callback");
    if(current!=0&&current!=1)return rfail(r,0,"invalid anim owner certificate result");
    if(!current&&task){r54_dirty_mark_task(r,task);entity_task_retire(task);}
    return current;
}
static int anim_data_read(GscRuntime *r,int line,Value *out){
    Value value;(void)line;if(!anim_data_current(r,r->executing_task))return 0;
    memset(&value,0,sizeof(value));value.kind=V_ANIM;value.object=1;value.object_token=r->array_token;*out=value;return 1;
}
