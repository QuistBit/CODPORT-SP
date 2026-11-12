/* R91 private, explicitly selected native metadata and visibility bridge.
 * These are bounded port capabilities, not original engine value tags. */
static const char *control_metadata_name(const char *name) {
    if(control_field_equal(name,"target"))return "target";
    if(control_field_equal(name,"targetname"))return "targetname";
    return NULL;
}
static int control_property_descriptor(const GscProgram *g,const char *name) {
    if(g->controls_mode) {
        if(control_field_equal(name,"origin"))return ENTITY_PROPERTY_ORIGIN;
        if(control_field_equal(name,"angles"))return ENTITY_PROPERTY_ANGLES;
    }
    return entity_property_descriptor(name);
}
static int control_mutation_valid(const GscProgram *g,const Instr *i) {
    const Expr *field;
    return g->controls_mode&&i->kind==I_MUTATE&&i->op==GSC_MUT_ADD&&
        i->dest>=0&&i->dest<g->expr_count&&i->expr>=0&&i->expr<g->expr_count&&
        (field=&g->expr[i->dest])->kind==E_FIELD&&
        control_field_equal(field->name,"origin")&&field->a>=0&&field->a<i->dest;
}
static int bridge_control_string(GscRuntime *r,const Value *receiver,
                                  const char *name,int line,Value *out) {
    GscEntityRef ref;char text[128],error[256];int found=-1,ok;Value copy;
    if(!r->controls_enabled||!r->controls.getstring||!receiver||
       receiver->kind!=V_ENTITY||!out||!control_metadata_name(name))
        return rfail(r,line,"metadata string capability is unavailable");
    ref=receiver->entity;
    if(entity_validate(r,&ref)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"metadata receiver is stale or missing"):0;
    if(!entity_task_current(r))return 0;
    memset(text,0xa5,sizeof(text));memset(error,0,sizeof(error));
    ok=r->controls.getstring(r->controls.user,&ref,control_metadata_name(name),
        text,sizeof(text),&found,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!entity_task_current(r))return 0;
    if(!ok||r->error[0])return rfail(r,line,"metadata read: %s",error[0]?error:"host callback failed");
    if((found!=0&&found!=1)||(found&&!memchr(text,0,sizeof(text))))
        return rfail(r,line,"metadata getter returned malformed string or found flag");
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    if(entity_validate(r,&ref)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"metadata getter retired the receiver"):0;
    memset(&copy,0,sizeof(copy));
    if(found){copy.kind=V_STRING;memcpy(copy.str,text,strlen(text)+1);}
    *out=copy;return 1;
}
static int bridge_visible_callback(void *user,const GscNativeValue *receiver,
    const GscNativeValue *args,int argc,GscNativeValue *out,int visible,
    char *error,size_t size) {
    GscRuntime *r=user;int ok;
    (void)args;
    if(!r||!r->controls_enabled||!r->controls.setvisible||!receiver||argc||out||
       receiver->kind!=GSC_NATIVE_ENTITY)
        return bridge_entity_error(error,size,"invalid visibility callback state");
    if(entity_validate(r,&receiver->entity)!=1||r->error[0])
        return bridge_entity_error(error,size,"visibility receiver is stale or missing");
    if(!entity_task_current(r))return !r->error[0];
    ok=r->controls.setvisible(r->controls.user,&receiver->entity,visible,error,size);
    if(!entity_task_current(r))return !r->error[0];
    if(!ok||r->error[0])return 0;
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 1;
    }
    if(entity_validate(r,&receiver->entity)!=1||r->error[0])
        return bridge_entity_error(error,size,"visibility host retired the receiver");
    return 1;
}
static int bridge_hide_callback(void *user,const GscNativeValue *receiver,
    const GscNativeValue *args,int argc,GscNativeValue *out,char *error,size_t size) {
    return bridge_visible_callback(user,receiver,args,argc,out,0,error,size);
}
static int bridge_show_callback(void *user,const GscNativeValue *receiver,
    const GscNativeValue *args,int argc,GscNativeValue *out,char *error,size_t size) {
    return bridge_visible_callback(user,receiver,args,argc,out,1,error,size);
}
static int bridge_add_controls(GscNativeRegistry *registry,GscRuntime *r,
                                char *error,size_t size) {
    GscNativeContract c;int n,index=-1;
    if(!r->controls.setvisible)return 1;
    for(n=0;n<2;n++) {
        memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_METHOD;
        str_copy(c.name,sizeof(c.name),n?"show":"hide");
        str_copy(c.provenance,sizeof(c.provenance),
            "R91 bounded entity hide/show capability; original DLL methods observed, not native VM ABI");
        c.known=GSC_NATIVE_KNOWN_METHOD;c.contexts=GSC_NATIVE_STATEMENT;
        c.min_args=c.max_args=0;c.receiver_types=GSC_NATIVE_TYPE_ENTITY;c.result_types=0;
        if(!gsc_native_registry_add(registry,&c,&index,error,size)||
           !gsc_native_registry_bind_method(registry,index,n?bridge_show_callback:bridge_hide_callback,
                                            r,error,size))return 0;
        if(n)r->native_show_import=index;else r->native_hide_import=index;
    }
    return 1;
}
static int bridge_setvisible(GscRuntime *r,const Value *receiver,int descriptor,
                              int argc,int line) {
    GscNativeValue recv;GscNativeResult result;char error[256];int index;
    index=descriptor==ENTITY_METHOD_HIDE?r->native_hide_import:r->native_show_import;
    if(!r->controls_enabled||argc||index<0||
       (descriptor!=ENTITY_METHOD_HIDE&&descriptor!=ENTITY_METHOD_SHOW))
        return rfail(r,line,"invalid visibility binding or arity");
    if(!bridge_export_value(r,receiver,&recv,line))return 0;
    if(!gsc_native_invoke_method(r->native_registry,index,GSC_NATIVE_STATEMENT,
        &recv,NULL,0,&result,error,sizeof(error)))
        return entity_task_current(r)?rfail(r,line,"visibility native dispatch: %s",error):0;
    if(r->error[0])return 0;
    if(result.has_value)return rfail(r,line,"visibility method must return VOID");
    return 1;
}
