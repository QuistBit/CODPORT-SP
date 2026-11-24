/* R168 selected native field namespace. Included before private state access.
 * Native refusal cannot create a shadow dynamic field or indexed root. */
int gsc_runtime_select_entity_fields(GscRuntime *r,const GscEntityFieldHost *host) {
    if(!r||!host||r->started||r->advancing||r->error[0]||r->entity_fields_enabled||
       !r->state_enabled||!r->scalar_switch_enabled||!r->names_enabled||
       !host->classify||!host->read||!host->write||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->entity_fields=*host;r->entity_fields_enabled=1;return 1;
}
/* -2 means cancellation without a script error. Callback inputs are owned
 * local copies; the compiler's names and caller Value records stay untouched. */
/* Called immediately after the field host returned and entity_task_current
 * succeeded. In native Training these allocation reads cannot dispatch VM
 * code. A refusal falls through to the ordered retirement/error path. */
static int entity_field_native_receivers(GscRuntime *r,const GscEntityRef *ref){
    Task *task=r->executing_task;
    GscEntityRef borrowed;
    int read=0;
    if(!r->native_readonly_scan || !r->state_enabled || !task || task->status==TASK_DEAD ||
       !ref || !ref->epoch || !ref->generation || !r->readonly_context.frame_active ||
       !readonly_context_selected(r,task))return 0;
    if(!readonly_receivers_live(r,task,&read) || r->error[0])return 0;
    borrowed=*ref;
    return r->entities.validate(r->entities.user,&borrowed)==1 &&
           state_ref_equal(&borrowed,ref) && !r->error[0];
}
static int entity_field_finish(GscRuntime *r,const GscEntityRef *ref,int line) {
    int alive;
    if(r->error[0])return -1;
    if(entity_field_native_receivers(r,ref))return 1;
    if(r->error[0])return -1;
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return -1;
        if(r->executing_task->status==TASK_DEAD)return -2;
    }
    alive=state_entity_live(r,ref);
    if(!entity_task_current(r))return r->error[0]?-1:-2;
    if(alive!=1){rfail(r,line,"native field callback retired its receiver");return -1;}
    return 1;
}
static int entity_field_begin(GscRuntime *r,int line) {
    if(r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"native field during guarded host callback");
    if(r->entity_field_busy||r->entity_method_busy)return rfail(r,line,"native entity field callback reentered");
    r->entity_field_busy=1;return 1;
}
static int entity_field_classify(GscRuntime *r,const GscEntityRef *owner,const char *name,int line) {
    GscEntityRef ref;char copied[128],error[256];int native,finished;
    if(!r->entity_fields_enabled)return 0;
    if(!name||!name[0]||strlen(name)>=sizeof(copied))return -1;
    ref=*owner;str_copy(copied,sizeof(copied),name);memset(error,0,sizeof(error));
    if(!entity_field_begin(r,line))return -1;
    native=r->entity_fields.classify(r->entity_fields.user,&ref,copied,error,sizeof(error));
    r->entity_field_busy=0;error[sizeof(error)-1]=0;
    if(r->error[0])return -1;
    if(!entity_task_current(r))return r->error[0]?-1:-2;
    if(native!=0&&native!=1){rfail(r,line,"native field namespace unavailable: %s",error);return -1;}
    finished=entity_field_finish(r,owner,line);return finished<0?finished:native;
}
static int entity_field_export(const Value *v,GscStringCastValue *out) {
    memset(out,0,sizeof(*out));
    switch(v->kind) {
    case V_UNDEFINED:out->type=0;break;
    case V_STRING:case V_LOCALIZED_STRING:
        out->type=v->kind==V_STRING?1:2;memcpy(out->string,v->str,sizeof(out->string));
        if(!memchr(out->string,0,sizeof(out->string)))return 0;
        break;
    case V_VECTOR:out->type=3;memcpy(out->vector,v->vector,sizeof(out->vector));
        if(!isfinite(out->vector[0])||!isfinite(out->vector[1])||!isfinite(out->vector[2]))return 0;
        break;
    case V_NUMBER:
        if(v->integer!=0&&v->integer!=1)return 0;
        if(v->integer){out->type=5;if(v->integer_exact)out->integer=v->integer_value;
            else if(!Cod1ModelLegacyInt(v->number,&out->integer))return 0;}
        else{out->type=4;out->number=v->number;if(!isfinite(out->number))return 0;}break;
    case V_ENTITY:case V_ARRAY:case V_STRUCT:out->type=7;break;
    case V_FUNCTION:out->type=9;break;
    default:return 0;
    }
    return 1;
}
static int entity_field_import(const GscStringCastValue *v,Value *out) {
    memset(out,0,sizeof(*out));
    switch(v->type) {
    case 0:break;
    case 1:case 2:
        if(!memchr(v->string,0,sizeof(v->string)))return 0;
        out->kind=v->type==1?V_STRING:V_LOCALIZED_STRING;memcpy(out->str,v->string,sizeof(out->str));break;
    case 3:if(!isfinite(v->vector[0])||!isfinite(v->vector[1])||!isfinite(v->vector[2]))return 0;
        out->kind=V_VECTOR;memcpy(out->vector,v->vector,sizeof(out->vector));break;
    case 4:if(!isfinite(v->number))return 0;out->kind=V_NUMBER;out->number=v->number;break;
    case 5:scalar_integer(out,v->integer);break;
    default:return 0;
    }
    return 1;
}
static int entity_field_read(GscRuntime *r,const GscEntityRef *owner,const char *name,int line,Value *out) {
    GscEntityRef ref=*owner;GscStringCastValue value;Value copy;char copied[128],error[256];int ok,finished;
    str_copy(copied,sizeof(copied),name);memset(error,0,sizeof(error));memset(&value,0xa5,sizeof(value));
    if(!entity_field_begin(r,line))return 0;
    ok=r->entity_fields.read(r->entity_fields.user,&ref,copied,&value,error,sizeof(error));
    r->entity_field_busy=0;error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(ok!=1)return rfail(r,line,"native field read refused: %s (field=%s slot=%u)",error,copied,(unsigned)ref.slot);
    finished=entity_field_finish(r,owner,line);if(finished<0)return 0;
    if(!entity_field_import(&value,&copy))return rfail(r,line,"native field read returned an invalid value");
    *out=copy;return 1;
}
static int entity_field_write(GscRuntime *r,const GscEntityRef *owner,const char *name,const Value *input,int line) {
    GscEntityRef ref=*owner;GscStringCastValue value;char copied[128],error[256];int ok,finished;
    if(!animation_value_exportable(r,input)||!entity_field_export(input,&value))return rfail(r,line,"native field operand is outside the admitted domain");
    str_copy(copied,sizeof(copied),name);memset(error,0,sizeof(error));
    if(!entity_field_begin(r,line))return 0;
    ok=r->entity_fields.write(r->entity_fields.user,&ref,copied,&value,error,sizeof(error));
    r->entity_field_busy=0;error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return !r->error[0];
    if(ok!=1)return rfail(r,line,"native field write refused: %s",error);
    finished=entity_field_finish(r,owner,line);return finished==1||finished==-2;
}
