/* R198 explicit finite filter lane. Event payload count must equal the number
 * of filters; prefix matching/implicit coercion/native object equality are not
 * inferred from original scripts, whose retained uses have one string filter.
 * Filters are captured once, before suspension, and never treated as lvalues. */
int gsc_program_select_waittillmatch(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->waittillmatch_selected||!g->event_payload_syntax||
       !g->names_syntax||!g->scalar_switch_syntax||!g->lifetime_syntax)return 0;
    g->waittillmatch_selected=1;return 1;
}
int gsc_runtime_select_waittillmatch(GscRuntime *r) {
    if(!r||!r->program->waittillmatch_selected||r->waittillmatch_enabled||r->started||
       r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->event_payloads_enabled||!r->state_enabled||!r->lifetime_enabled||
       !r->results_enabled||r->entity_prepare_busy||r->entity_certificate_busy)return 0;
    r->waittillmatch_enabled=1;return 1;
}
static int waittillmatch_value_valid(const Value *v) {
    int k;double d;
    switch(v->kind) {
      case V_UNDEFINED:return 1;
      case V_NUMBER:
        if(!isfinite(v->number))return 0;
        if(!v->integer||v->integer_exact)return 1;
        d=(double)v->number;return d>=INT32_MIN&&d<=INT32_MAX&&trunc(d)==d;
      case V_STRING:return memchr(v->str,0,sizeof(v->str))!=NULL;
      case V_VECTOR:for(k=0;k<3;k++)if(!isfinite(v->vector[k]))return 0;return 1;
      case V_ENTITY:return v->entity.epoch&&v->entity.generation;
      default:return 0;
    }
}
static double waittillmatch_number(const Value *v) {
    return v->integer&&v->integer_exact?(double)v->integer_value:(double)v->number;
}
static int waittillmatch_value_equal(const Value *a,const Value *b) {
    int k;if(a->kind!=b->kind||!waittillmatch_value_valid(a)||!waittillmatch_value_valid(b))return 0;
    switch(a->kind) {
      case V_UNDEFINED:return 1;
      case V_NUMBER:return waittillmatch_number(a)==waittillmatch_number(b);
      case V_STRING:return !strcmp(a->str,b->str);
      case V_VECTOR:for(k=0;k<3;k++)if(a->vector[k]!=b->vector[k])return 0;return 1;
      case V_ENTITY:return entity_ref_equal(&a->entity,&b->entity);
      default:return 0;
    }
}
static int waittillmatch_matches(const Task *task,const Value *values,int count) {
    int k;if(!task->event_match_active)return 1;
    if(task->event_filter_count!=count)return 0;
    for(k=0;k<count;k++)if(!waittillmatch_value_equal(&task->event_filters[k],&values[k]))return 0;
    return 1;
}
static int waittillmatch_register(GscRuntime *r,Task *task,Frame *frame,
    const GscEntityRef *owner,const char *event,const Value *filters,int count,int line,uint32_t object) {
    int k;
    if(!r->waittillmatch_enabled||!task||task!=r->executing_task||task->status!=TASK_READY||
       count<1||count>GSC_EVENT_PAYLOAD_CAPACITY)return rfail(r,line,"waittillmatch capability unavailable or invalid filter count");
    for(k=0;k<count;k++)if(!waittillmatch_value_valid(&filters[k]))return rfail(r,line,"unsupported waittillmatch filter value");
    if(r->blast_enabled&&r->next_blast_wait_serial==UINT64_MAX)return rfail(r,line,"blast wait registration serial exhausted");
    task->event_frame=(int)(frame-task->frames);task->event_count=0;
    memset(task->event_filters,0,sizeof(task->event_filters));
    memcpy(task->event_filters,filters,(size_t)count*sizeof(filters[0]));
    task->event_filter_count=count;task->event_match_active=1;
    task->event_scope=event_scope_for(owner,object);
    task->event_struct_id=object;
    if(owner)task->event_owner=*owner;else memset(&task->event_owner,0,sizeof(task->event_owner));
    if(r->blast_enabled)task->blast_wait_serial=r->next_blast_wait_serial++;
    str_copy(task->event,sizeof(task->event),event);task->status=TASK_EVENT;
    if(r->precache_string_enabled)r->precache_string_initial_window=0;
    return 1;
}
