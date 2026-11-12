/* R88 private VM state. No engine pointers, raw-slot identity or native tags.
 * Fixed records own inline values; stale records remain until runtime free. */
static int state_ref_equal(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static void entity_task_retire(Task *task) {
    task->context_retired=1;task->depth=0;task->status=TASK_DEAD;
    task->event_scope=EVENT_LEVEL;
    memset(&task->event_owner,0,sizeof(task->event_owner));task->event[0]=0;
}
static int entity_certificate_current(GscRuntime *r,Task *task) {
    GscEntityExecutionCertificate copy;int current;uint64_t time_root;
    if(!task)return 1;
    time_root=r237_owner_stamp(r);
    if(!r237_owner_current(r,time_root))return rfail(r,0,"Task has no current time-root allocation");
    if(!developer_runtime_current(r))return 0;
    if(!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during developer validation");
    if(r->anim_data_enabled&&!anim_data_current(r,task))return 0;
    if(!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during animation validation");
    if(task->map_certificate.receipt) {
        GscMapExecutionCertificate map=task->map_certificate;
        if(task->status==TASK_DEAD)return 0;
        if(r->entity_prepare_busy||r->entity_certificate_busy)
            return rfail(r,0,"map execution certificate callback reentered");
        r->entity_certificate_busy=1;current=map.current(map.receipt);r->entity_certificate_busy=0;
        if(!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during map execution certificate");
        if(r->error[0])return 0;
        if(current!=0&&current!=1)return rfail(r,0,"invalid map execution certificate result");
        if(!current){entity_task_retire(task);return 0;}
    }
    if(!task->certificate.serial)return 1;
    if(task->status==TASK_DEAD)return 0;
    if(r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,0,"entity execution certificate callback reentered");
    copy=task->certificate;r->entity_certificate_busy=1;
    current=copy.current(&copy.owner,copy.serial);
    r->entity_certificate_busy=0;
    if(!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during entity execution certificate");
    if(r->error[0])return 0;
    if(current!=0&&current!=1)return rfail(r,0,"invalid entity execution certificate result");
    if(!current)entity_task_retire(task);
    return current;
}
static int entity_task_current(GscRuntime *r) {
    if(r->state_arena_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,0,"entity access during guarded host preparation");
    return !r->executing_task||entity_certificate_current(r,r->executing_task);
}
static int entity_validate(GscRuntime *r,const GscEntityRef *ref) {
    GscEntityRef original,borrowed;int alive;
    if(!ref)return 0;
    original=*ref;borrowed=original;
    if(r->executing_task&&(r->anim_data_enabled||r->executing_task->certificate.serial||r->executing_task->map_certificate.receipt)&&
       !entity_certificate_current(r,r->executing_task))return 0;
    alive=r->entities.validate(r->entities.user,&borrowed);
    if(!state_ref_equal(&borrowed,&original))
        return rfail(r,0,"entity validator mutated its borrowed identity");
    if(r->executing_task&&(r->anim_data_enabled||r->executing_task->certificate.serial||r->executing_task->map_certificate.receipt))
        (void)entity_certificate_current(r,r->executing_task);
    return alive;
}
static int state_entity_live(GscRuntime *r,const GscEntityRef *ref) {
    GscEntityRef copy;int alive;
    if(!ref||!ref->epoch||!ref->generation)return 0;
    copy=*ref;alive=entity_validate(r,&copy)==1;
    if(r->executing_task&&!entity_certificate_current(r,r->executing_task)&&r->error[0])return -1;
    return r->error[0]?-1:alive;
}
static int refresh_state_context(GscRuntime *,Task *);
#include "gsc_vm_entity_fields.inc"
#include "gsc_vm_entity_methods.inc"
static int state_receiver(GscRuntime *r,const Value *receiver,int line) {
    int alive;
    if(!entity_task_current(r))return 0;
    if(!r->state_enabled||!receiver)return rfail(r,line,"state receiver is unavailable");
    if(receiver->kind==V_HUDELEM)return hud_receiver(r,receiver,line);
    if(receiver->kind==V_LEVEL)return 1;
    if(receiver->kind==V_ANIM)return anim_data_value_valid(r,receiver)&&anim_data_current(r,r->executing_task)?1:0;
    if(receiver->kind==V_STRUCT)return struct_value_valid(r,receiver)?1:
        rfail(r,line,"state structure receiver is invalid or belongs to another runtime");
    if(receiver->kind==V_NODE)return rfail(r,line,"node fields are read-only and node script receivers are unsupported");
    if(receiver->kind!=V_ENTITY)return rfail(r,line,"state field/script receiver requires an entity or level");
    alive=state_entity_live(r,&receiver->entity);
    if(alive!=1)return entity_task_current(r)?rfail(r,line,"state receiver is stale or missing"):0;
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    return 1;
}
static int state_field_index(GscRuntime *r,const Value *receiver,const char *name) {
    int n;
    for(n=0;n<r->state_field_capacity;n++) {
        const StateField *field=&r->state_fields[n];
        if(field->used&&field->owner_kind==receiver->kind&&
           (r->controls_enabled?control_field_equal(field->name,name):!strcmp(field->name,name))&&
           (receiver->kind==V_LEVEL||
            ((receiver->kind==V_STRUCT||receiver->kind==V_ANIM)?field->struct_owner==receiver->object:
             state_ref_equal(&field->owner,&receiver->entity))))return n;
    }
    return -1;
}
static int state_field_read(GscRuntime *r,const Value *receiver,const char *name,
                            int line,Value *out) {
    int index,property=control_property_descriptor(r->program,name);
    if(receiver&&receiver->kind==V_HUDELEM)return hud_field_read(r,receiver,name,line,out);
    if(receiver&&receiver->kind==V_NODE)return node_field_read(r,receiver,name,line,out);
    if(!state_receiver(r,receiver,line))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    if(receiver->kind==V_ENTITY) {
        int native=entity_field_classify(r,&receiver->entity,name,line);
        if(native<0)return 0;
        if(native)return entity_field_read(r,&receiver->entity,name,line,out);
    }
    if(r->controls_enabled&&receiver->kind==V_ENTITY&&control_metadata_name(name))
        return bridge_control_string(r,receiver,name,line,out);
    if(receiver->kind==V_ENTITY&&property>=0)
        return bridge_property_read(r,receiver,property,line,out);
    index=state_field_index(r,receiver,name);
    memset(out,0,sizeof(*out));
    if(index>=0)*out=r->state_fields[index].value;
    return 1;
}
static int state_field_write(GscRuntime *r,const Value *receiver,const char *name,
                             const Value *value,int line) {
    StateField copy;int n,index,property=control_property_descriptor(r->program,name);
    if(receiver&&receiver->kind==V_HUDELEM)return hud_field_write(r,receiver,name,value,line);
    if(receiver&&receiver->kind==V_NODE)return rfail(r,line,"node fields are read-only");
    if(value&&animation_namespace_kind(value->kind)&&!animation_namespace_value_valid(r,value))return rfail(r,line,"invalid animation metadata field value");
    if(value&&value->kind==V_ANIMATION&&!animation_value_valid(r,value))return rfail(r,line,"invalid animation reference field value");
    if(value&&value->kind==V_ANIM)return rfail(r,line,"whole anim owner field value is unsupported");
    if(!value||!name||!name[0]||strlen(name)>=sizeof(copy.name)||
       value->kind<V_UNDEFINED||
       (animation_namespace_kind(value->kind)?!animation_namespace_value_valid(r,value):value->kind==V_HUDELEM?!hud_value_valid(r,value):value->kind==V_NODE?!node_value_valid(r,value):value->kind==V_ANIMATION?!animation_value_valid(r,value):value->kind==V_STRUCT?!struct_value_valid(r,value):
        value->kind>(r->objectives_enabled?V_LOCALIZED_STRING:r->function_refs_enabled?V_FUNCTION:r->arrays_enabled?V_ARRAY:V_LEVEL)))
        return rfail(r,line,"invalid state field value or name");
    if(value->kind==V_FUNCTION&&!function_value_valid(r,value))return rfail(r,line,"invalid function reference state value");
    if(!state_receiver(r,receiver,line))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 1;
    if(receiver->kind==V_ENTITY) {
        int native=entity_field_classify(r,&receiver->entity,name,line);
        if(native<0)return r->executing_task&&r->executing_task->status==TASK_DEAD&&!r->error[0];
        if(native)return entity_field_write(r,&receiver->entity,name,value,line);
    }
    if(r->controls_enabled&&receiver->kind==V_ENTITY&&control_metadata_name(name))
        return rfail(r,line,"entity target/targetname metadata is read-only");
    if(receiver->kind==V_ENTITY&&property>=0)
        return bridge_property_write(r,receiver,property,value,line);
    /* Copy every input before a reserve may move the arena. */
    memset(&copy,0,sizeof(copy));copy.used=1;copy.owner_kind=receiver->kind;
    if(receiver->kind==V_ENTITY)copy.owner=receiver->entity;
    if(receiver->kind==V_STRUCT||receiver->kind==V_ANIM)copy.struct_owner=receiver->object;
    str_copy(copy.name,sizeof(copy.name),name);copy.value=*value;
    index=state_field_index(r,receiver,name);
    if(index<0){if(!state_fields_reserve_free(r,1,line))return 0;
     for(n=0;n<r->state_field_capacity;n++)if(!r->state_fields[n].used){index=n;break;}
     if(index<0)return rfail(r,line,"state arena reserved slot unavailable");}
    r->state_fields[index]=copy;return 1;
}
/* The event owner is a pending wait, not self. Keep both initial ownership and
 * temporary receiver-call contexts alive across sleep/events/frame unwinding.
 * A stale context retires this whole task, never another task or reused slot. */
void gsc_runtime_bind_readonly_context(GscRuntime *r, const GscReadOnlyContext *context,
    void *user, int (*validate)(void *, const GscEntityRef *)) {
    if (!r || !context || !validate || r->started || r->executing_task ||
        r->error[0] || r->entity_certificate_busy || r->entity_prepare_busy ||
        r->entities.user != user || r->entities.validate != validate ||
        !context->map.current || !context->map.receipt ||
        !context->animation.current || !context->animation.receipt ||
        !context->developer.current || !context->developer.receipt) return;
    r->readonly_context = *context;
    r->readonly_entity_user = user;
    r->readonly_entity_validate = validate;
}
static int readonly_certificate_matches(const GscMapExecutionCertificate *a,
    const GscMapExecutionCertificate *b) {
    return a->receipt == b->receipt && a->current == b->current;
}
static int readonly_context_selected(const GscRuntime *r, const Task *task) {
    const GscReadOnlyContext *c = &r->readonly_context;
    int actor=task->certificate.serial!=0;
    if(c->frame_active&&!c->frame_active())return 0;
    if(actor&&(!c->frame_active||!c->actor_current||task->certificate.current!=c->actor_current))return 0;
    return c->map.current && r->executing_task == task &&
        r->entities.user == r->readonly_entity_user && r->entities.validate == r->readonly_entity_validate &&
        !r->error[0] && r->entities_enabled && r->anim_data_enabled &&
        r->animation_namespace_enabled && !r->animation_refs_enabled &&
        r->program->developer_value_only && r->developer_admitted &&
        (readonly_certificate_matches(&task->map_certificate,&c->map) || (actor && !task->map_certificate.receipt)) &&
        readonly_certificate_matches(&r->anim_data,&c->animation) &&
        readonly_certificate_matches(&r->developer_admission.certificate,&c->developer);
}
/* The selected Session validator only reads its state and the actual GAME
 * allocation table. No script, trap or event dispatch occurs inside this
 * scan. Never retain a result across a VM expression, callback or yield. */
static int readonly_receivers_live(GscRuntime *r, Task *task, int *read_any) {
    int n;
    for (n = -1; n < task->depth; ++n) {
        const Value *self = n < 0 ? &task->initial_self : &task->frames[n].self;
        if (self->kind == V_ENTITY) {
            GscEntityRef ref = self->entity;
            *read_any = 1;
            if (!ref.epoch || !ref.generation || r->entities.validate(r->entities.user,&ref) != 1)
                return 0;
            if (!state_ref_equal(&ref,&self->entity))
                return rfail(r,0,"entity validator mutated its borrowed identity");
        }
    }
    return 1;
}
static int refresh_state_context(GscRuntime *r,Task *task) {
    int n,alive;
    if(!r->state_enabled||task->status==TASK_DEAD)return 1;
    if(!entity_certificate_current(r,task))return !r->error[0];
    if (readonly_context_selected(r,task)) {
        int read_any = 0;
        alive = readonly_receivers_live(r,task,&read_any);
        if (!read_any) return 1;
        if (r->error[0]) return 0;
        if (!entity_certificate_current(r,task)) return !r->error[0];
        if (!alive) goto retired;
        return 1;
    }
    if(task->initial_self.kind==V_ENTITY) {
        alive=state_entity_live(r,&task->initial_self.entity);
        if(alive<0)return 0;
        if(!alive)goto retired;
        if(!entity_certificate_current(r,task))return !r->error[0];
    }
    for(n=0;n<task->depth;n++)if(task->frames[n].self.kind==V_ENTITY) {
        alive=state_entity_live(r,&task->frames[n].self.entity);
        if(alive<0)return 0;
        if(!alive)goto retired;
        if(!entity_certificate_current(r,task))return !r->error[0];
    }
    return 1;
retired:
    entity_task_retire(task);
    return 1;
}
