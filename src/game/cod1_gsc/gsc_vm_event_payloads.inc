/* R96 bounded payload delivery. Waiter slots are reserved together before
 * registration; notification performs no allocation and no partial binding. */
static int payload_value_valid(const GscRuntime *r,const Value *v) {
    int k;
    switch(v->kind) {
      case V_UNDEFINED:return 1;
      case V_NUMBER:return isfinite(v->number);
      case V_STRING:return memchr(v->str,0,sizeof(v->str))!=NULL;
      case V_VECTOR:
        for(k=0;k<3;k++)if(!isfinite(v->vector[k]))return 0;
        return 1;
      case V_ENTITY:return v->entity.epoch&&v->entity.generation;
      case V_ARRAY:return array_value_valid(r,v->array)&&animation_value_exportable(r,v);
      case V_FUNCTION:return function_value_valid(r,v);
      default:return 0;
    }
}
#include "gsc_vm_waittillmatch.inc"
static int payload_register(GscRuntime *r,Task *task,Frame *frame,
    const GscEntityRef *owner,const char *event,const int *args,int argc,int line,uint32_t object) {
    int slots[GSC_EVENT_PAYLOAD_CAPACITY],n,k,unique=0,base=frame->local_count;
    char names[GSC_EVENT_PAYLOAD_CAPACITY][64];
    if(argc<1||argc>MAX_PARAMS)return rfail(r,line,"invalid payload wait arity");
    for(n=1;n<argc;n++) {
        const char *name=r->program->expr[args[n]].name;
        if(r->anim_data_enabled&&!strcmp(name,"anim"))return rfail(r,line,"selected anim cannot be a waittill output");
        if(game_array_selected(r,name))return rfail(r,line,"selected game cannot be a waittill output");
        for(k=0;k<base;k++)if(!strcmp(frame->locals[k].name,name))break;
        if(k==base) {
            for(k=0;k<unique;k++)if(!strcmp(names[k],name))break;
            if(k==unique) {
                if(base+unique>=MAX_LOCALS)return rfail(r,line,"waittill output local limit");
                str_copy(names[unique++],64,name);
            }
            k+=base;
        }
        slots[n-1]=k;
    }
    if(r->blast_enabled&&r->next_blast_wait_serial==UINT64_MAX)
        return rfail(r,line,"blast wait registration serial exhausted");
    /* Capacity for ALL unique bindings has passed; commit cannot fail. A
     * preexisting local retains its value until the notification is delivered. */
    for(k=0;k<unique;k++) {
        Local *local=&frame->locals[base+k];memset(local,0,sizeof(*local));
        str_copy(local->name,sizeof(local->name),names[k]);
    }
    frame->local_count=base+unique;
    task->event_frame=(int)(frame-task->frames);task->event_count=argc-1;
    task->event_match_active=task->event_filter_count=0;
    memset(task->event_filters,0,sizeof(task->event_filters));
    for(k=0;k<argc-1;k++)task->event_slots[k]=slots[k];
    task->event_scope=event_scope_for(owner,object);
    task->event_struct_id=object;
    if(owner)task->event_owner=*owner;
    else memset(&task->event_owner,0,sizeof(task->event_owner));
    if(r->blast_enabled)task->blast_wait_serial=r->next_blast_wait_serial++;
    str_copy(task->event,sizeof(task->event),event);task->status=TASK_EVENT;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);
    return 1;
}
static int payload_matches_owner(const Task *task,const GscEntityRef *owner,const char *event,uint32_t object) {
    return task->status==TASK_EVENT && task->event_scope==event_scope_for(owner,object) &&
        (!object || task->event_struct_id==object) && (!owner || entity_ref_equal(&task->event_owner,owner)) &&
        !strcmp(task->event,event);
}
static int payload_matches(const Task *task,const GscEntityRef *owner,const char *event) {
    return payload_matches_owner(task,owner,event,0);
}
static int blast_waiter_selected(const Task *task,const GscBlastWaiterSet *selected) {
    size_t n;if(!selected)return 1;
    for(n=0;n<selected->count;n++)if(selected->registrations[n].task_serial==task->serial&&
        selected->registrations[n].registration_serial==task->blast_wait_serial)return 1;
    return 0;
}
static int payload_deliver_owner(GscRuntime *r,const GscEntityRef *owner,const char *event,
    const Value *values,int count,int line,const GscBlastWaiterSet *selected,uint32_t object) {
    int n,k,awakened=0,queued=0;Value undefined;memset(&undefined,0,sizeof(undefined));
    if(count<0||count>GSC_EVENT_PAYLOAD_CAPACITY||(!values&&count)) {
        rfail(r,line,"invalid event payload count");return -1;
    }
    for(k=0;k<count;k++)if(!payload_value_valid(r,&values[k])) {
        rfail(r,line,"unsupported event payload value");return -1;
    }
    /* Validate every matched binding before writing any waiter. Registration
     * already reserved storage, so this is an allocation-free atomic commit. */
    for(n=0;n<r->task_count;n++) {
        Task *task=task_at(r,n);
        if(!payload_matches_owner(task,owner,event,object)||!blast_waiter_selected(task,selected))continue;
        if(endon_matches_owner(r,task,owner,event,object))continue;
        if(!waittillmatch_matches(task,values,count))continue;
        if(r->owned_wait_delivery_enabled&&(!task->map_certificate.receipt||!task->map_certificate.current)) {
            rfail(r,line,"native delivered waiter lacks its own map certificate");return -1;
        }
        if(!entity_certificate_current(r,task)) {
            if(r->error[0])return -1;
            continue;
        }
        if(task->event_frame<0||task->event_frame>=task->depth||
           task->event_count<0||task->event_count>GSC_EVENT_PAYLOAD_CAPACITY) {
            rfail(r,line,"invalid event waiter frame");return -1;
        }
        for(k=0;k<task->event_count;k++)if(task->event_slots[k]<0||
            task->event_slots[k]>=task->frames[task->event_frame].local_count) {
            rfail(r,line,"invalid event waiter binding");return -1;
        }
        if(r->r237_frame_buckets_enabled){if(task->r237_enqueued){rfail(r,line,"R237 waiter already queued");return -1;}queued++;}
    }
    if(r->r237_frame_buckets_enabled&&!r237_queue_available(r,queued,line))return -1;
    endon_cancel_owner(r,owner,event,object);
    for(n=0;n<r->task_count;n++) {
        Task *task=task_at(r,n);Frame *frame;
        if(!payload_matches_owner(task,owner,event,object)||!blast_waiter_selected(task,selected))continue;
        if(!waittillmatch_matches(task,values,count))continue;
        if(r->r237_frame_buckets_enabled&&!r237_queue_offer(r,task,r237_bucket(r),line))return -1;
        frame=&task->frames[task->event_frame];
        for(k=0;k<task->event_count;k++)
            frame->locals[task->event_slots[k]].value=k<count?values[k]:undefined;
        if(r->frame_cadence_enabled){task->wake_ms=r->cadence_drain_ms;task->cadence_event_resume=1;}
        task->event_match_active=task->event_filter_count=0;
        memset(task->event_filters,0,sizeof(task->event_filters));
        /* Payload locals and queue membership are committed. Only this wait
         * is consumed; the task retains its original independent owners. */
        if(r->owned_wait_delivery_enabled)clear_wait_owner(task);
        task->status=TASK_READY;awakened++;
    }
    return awakened;
}
static int payload_deliver_selected(GscRuntime *r,const GscEntityRef *owner,const char *event,
    const Value *values,int count,int line,const GscBlastWaiterSet *selected) {
    return payload_deliver_owner(r,owner,event,values,count,line,selected,0);
}
static int payload_deliver(GscRuntime *r,const GscEntityRef *owner,const char *event,
    const Value *values,int count,int line) {
    return payload_deliver_selected(r,owner,event,values,count,line,NULL);
}
static int run_payload_event(GscRuntime *r,Task *task,Frame *frame,int descriptor,
    const Value *receiver,const Value *values,const int *args,int argc,int line) {
    char event[128];const GscEntityRef *owner=NULL;uint32_t object=0;int alive;
    if(!r->event_payloads_enabled||argc<1||argc>MAX_PARAMS||
       !event_string(r,values[0],event,sizeof(event),line))return 0;
    if(receiver) {
        if(receiver->kind==V_ENTITY) {
            owner=&receiver->entity;alive=event_owner_live(r,owner);
            if(!entity_task_current(r))return 0;
            if(alive<0)return 0;
            if(!alive)return rfail(r,line,"entity event owner is stale or missing");
        } else if(receiver->kind==V_STRUCT) {
            if(!struct_value_valid(r,receiver))return rfail(r,line,"invalid structure event owner");
            object=receiver->object;
        } else if(receiver->kind!=V_LEVEL)return rfail(r,line,"event requires entity or level receiver");
    }
    if(descriptor==ENTITY_EVENT_ENDON) {
        if(argc!=1)return rfail(r,line,"endon requires exactly one event name");
        return endon_register(r,task,owner,event,line,object);
    }
    if(descriptor==ENTITY_EVENT_WAITTILLMATCH)
        return waittillmatch_register(r,task,frame,owner,event,values+1,argc-1,line,object);
    if(descriptor==ENTITY_EVENT_WAITTILL)
        return payload_register(r,task,frame,owner,event,args,argc,line,object);
    if(descriptor!=ENTITY_EVENT_NOTIFY)return rfail(r,line,"invalid payload event descriptor");
    return payload_deliver_owner(r,owner,event,values+1,argc-1,line,NULL,object)>=0;
}
/* Copy every public byte range before entering a validation callback. No
 * external handles or runtime pointers are accepted, and stale entity values
 * remain opaque until dereference. Malformed requests leave the VM unchanged. */
static int payload_external_copy(const GscEventValue *values,size_t count,Value *out) {
    size_t n;int k;GscEventValue copy[GSC_EVENT_PAYLOAD_CAPACITY];
    if(count>GSC_EVENT_PAYLOAD_CAPACITY||(!values&&count))return 0;
    if(count)memcpy(copy,values,count*sizeof(copy[0]));
    for(n=0;n<count;n++) {
        Value *v=&out[n];memset(v,0,sizeof(*v));
        switch(copy[n].kind) {
          case GSC_EVENT_UNDEFINED:break;
          case GSC_EVENT_NUMBER:
            if(!isfinite(copy[n].number))return 0;
            v->kind=V_NUMBER;v->number=copy[n].number;break;
          case GSC_EVENT_STRING:
            if(!memchr(copy[n].string,0,sizeof(copy[n].string)))return 0;
            v->kind=V_STRING;memcpy(v->str,copy[n].string,sizeof(v->str));break;
          case GSC_EVENT_VECTOR:
            for(k=0;k<3;k++)if(!isfinite(copy[n].vector[k]))return 0;
            v->kind=V_VECTOR;memcpy(v->vector,copy[n].vector,sizeof(v->vector));break;
          case GSC_EVENT_ENTITY:
            if(!copy[n].entity.epoch||!copy[n].entity.generation)return 0;
            v->kind=V_ENTITY;v->entity=copy[n].entity;break;
          default:return 0;
        }
    }
    return 1;
}
