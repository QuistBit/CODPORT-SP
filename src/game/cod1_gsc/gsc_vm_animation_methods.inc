int gsc_program_select_animation_methods(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->animation_methods_selected||!g->animation_refs_selected||!g->timed_arithmetic_selected||!g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->animation_methods_selected=1;return 1;
}
int gsc_runtime_select_animation_methods(GscRuntime *r,const GscAnimationMethodHost *input) {
    GscAnimationMethodHost host;
    if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->animation_methods_enabled||
       !r->program->animation_methods_selected||!r->animation_refs_enabled||!r->timed_arithmetic_enabled||
       r->entity_field_busy||r->entity_method_busy||r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||r->entity_sequence_busy||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    host=*input;if(!host.validate||!host.invoke||!anim_data_current(r,NULL))return 0;
    r->animation_methods=host;r->animation_methods_enabled=1;return 1;
}
int gsc_runtime_animation_delivery_available(const GscRuntime *r,uint64_t identity,int32_t next_ms) {
    return r&&identity&&r->array_token==identity&&r->started&&!r->error[0]&&r->animation_methods_enabled&&
        !r->animation_tick_serial&&!r->advancing&&!r->executing_task&&!r->entity_entry_depth&&!r->entity_field_busy&&!r->entity_method_busy&&
        !r->entity_prepare_busy&&!r->entity_certificate_busy&&!r->entity_sequence_busy&&!r->entity_inheritance_busy&&next_ms>=r->now_ms&&
        (!r->frame_cadence_enabled||next_ms<=INT32_C(0xffffff));
}
int gsc_runtime_animation_tick_begin(GscRuntime *r,uint64_t identity,int32_t next_ms,GscAnimationTickScope *out) {
    static uint64_t serial;GscAnimationTickScope scope;
    if(!out||serial==UINT64_MAX||!gsc_runtime_storage_separate(r,out,sizeof(*out))||!gsc_runtime_animation_delivery_available(r,identity,next_ms))return 0;
    scope.runtime_identity=identity;scope.serial=++serial;r->animation_tick_serial=scope.serial;r->animation_tick_ms=next_ms;r->entity_prepare_busy=1;*out=scope;return 1;
}
int gsc_runtime_animation_tick_commit(GscRuntime *r,const GscAnimationTickScope *input) {
    GscAnimationTickScope scope;
    if(!r||!input)return 0;
    scope=*input;
    if(!scope.serial||scope.runtime_identity!=r->array_token||scope.serial!=r->animation_tick_serial||r->entity_prepare_busy!=1||
       r->advancing||r->executing_task||r->error[0]||r->animation_tick_ms<r->now_ms)return 0;
    r->now_ms=r->animation_tick_ms;return 1;
}
int gsc_runtime_animation_tick_notify(GscRuntime *r,const GscAnimationTickScope *input,const GscEntityRef *owner,const char *name,const GscEventValue *values,size_t count,int *awakened) {
    GscAnimationTickScope scope;int result;
    if(!r||!input)return 0;
    scope=*input;
    if(!gsc_runtime_animation_tick_commit(r,&scope))return 0;
    r->entity_prepare_busy=0;result=gsc_notify_entity_with_payload(r,owner,name,values,count,awakened);r->entity_prepare_busy=1;return result;
}
void gsc_runtime_animation_tick_end(GscRuntime *r,const GscAnimationTickScope *input) {
    if(r&&input&&input->serial&&input->runtime_identity==r->array_token&&input->serial==r->animation_tick_serial){r->entity_prepare_busy=0;r->animation_tick_serial=0;r->animation_tick_ms=0;}
}
static int animation_method_context_current(void *context) {GscRuntime *r=(GscRuntime *)context;return !r->error[0]&&console_context(r)==1;}
static int animation_method_resource(GscRuntime *r,const Value *value,int line,GscAnimationLengthRequest *out) {
    int n;if(!animation_value_valid(r,value))return rfail(r,line,"animation method requires a current animation reference");
    memset(out,0,sizeof(*out));out->runtime_identity=r->array_token;
    for(n=0;n<r->animation_ref_count;++n)if(r->animation_bindings[n].entry==value->object){const AnimationBinding *b=&r->animation_bindings[n];strcpy(out->tree,b->tree);strcpy(out->name,b->name);out->ordinal=b->entry;out->flags=b->flags;return 1;}
    return rfail(r,line,"animation method resource binding unavailable");
}
static int animation_method_float(GscRuntime *r,const Value *value,int line,uint32_t *bits) {
    uint32_t magnitude;
    if(!float_arithmetic_number_bits(r,value,line,bits))return 0;
    magnitude=*bits&UINT32_C(0x7fffffff);
    if((*bits>>31)||((magnitude>>23)==255)||(magnitude&&!(magnitude>>23)))return rfail(r,line,"animation method GetFloat requires finite nonnegative normal values or zero");
    return 1;
}
static int animation_method_invoke(GscRuntime *r,const Value *receiver,int descriptor,const Value *args,int argc,int line) {
    GscAnimationMethodRequest request,original;GscEntityRef borrowed;char error[384];int accepted,finished,offset,all,flagged;
    if(!r->animation_methods_enabled||!receiver||receiver->kind!=V_ENTITY||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"animation method entity capability unavailable");
    if(!r->executing_task||!r->executing_task->certificate.serial)return rfail(r,line,"animation method requires a copied component execution certificate");
    if(!r->timed_arithmetic_enabled||!float_arithmetic_active(r))return rfail(r,line,"animation method requires the explicitly selected module arithmetic profile");
    if(r->entity_field_busy||r->entity_method_busy||r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy)return rfail(r,line,"animation method callback reentered");
    memset(&request,0,sizeof(request));request.entity=receiver->entity;request.now_ms=r->now_ms;request.precision_bits=r->program->float_arithmetic_profile.precision_bits;request.rounding=r->program->float_arithmetic_profile.rounding;
    request.method=descriptor==ENTITY_METHOD_SETANIMKNOB?GSC_ANIMATION_SET_KNOB:descriptor==ENTITY_METHOD_SETANIMKNOBALL?GSC_ANIMATION_SET_KNOB_ALL:GSC_ANIMATION_SET_FLAGGED_KNOB_ALL;
    r->entity_method_busy=1;finished=entity_method_finish(r,&request.entity,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
    borrowed=request.entity;error[0]=0;accepted=r->animation_methods.validate(r->animation_methods.user,&borrowed,r->array_token,r->now_ms,error,sizeof(error),animation_method_context_current,r);error[sizeof(error)-1]=0;
    finished=entity_method_finish(r,&request.entity,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
    if(!state_ref_equal(&borrowed,&request.entity)){r->entity_method_busy=0;return rfail(r,line,"animation host changed copied receiver identity");}
    if(accepted!=1){r->entity_method_busy=0;return rfail(r,line,"animation receiver refused: %s",error);}
    all=request.method!=GSC_ANIMATION_SET_KNOB;flagged=request.method==GSC_ANIMATION_SET_FLAGGED_KNOB_ALL;offset=flagged?1:0;
    if(flagged){if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str))){r->entity_method_busy=0;return rfail(r,line,"flagged animation requires an admitted plain notify string");}memcpy(request.notify,args[0].str,sizeof(request.notify));}
    if(argc<offset+1+all||!animation_method_resource(r,&args[offset],line,&request.animation)||(all&&!animation_method_resource(r,&args[offset+1],line,&request.root))){r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"animation method missing animation arguments");}
    if(all&&(!strcmp(request.animation.tree,request.root.tree)?request.animation.ordinal==request.root.ordinal:1)){r->entity_method_busy=0;return rfail(r,line,"animation and root require different entries in the same tree");}
    offset+=1+all;request.weight_bits=UINT32_C(0x3f800000);request.blend_bits=UINT32_C(0x3e4ccccd);request.rate_bits=UINT32_C(0x3f800000);
    if((argc>offset&&!animation_method_float(r,&args[offset],line,&request.weight_bits))||(argc>offset+1&&!animation_method_float(r,&args[offset+1],line,&request.blend_bits))||(argc>offset+2&&!animation_method_float(r,&args[offset+2],line,&request.rate_bits))){r->entity_method_busy=0;return 0;}
    if(flagged&&!(request.weight_bits&UINT32_C(0x7fffffff))){r->entity_method_busy=0;return rfail(r,line,"flagged animation weight must be positive");}
    original=request;error[0]=0;accepted=r->animation_methods.invoke(r->animation_methods.user,&request,error,sizeof(error),animation_method_context_current,r);error[sizeof(error)-1]=0;
    finished=entity_method_finish(r,&original.entity,line);r->entity_method_busy=0;if(finished<0)return finished==-2;
    if(memcmp(&request,&original,sizeof(request)))return rfail(r,line,"animation host changed copied method intent");
    if(accepted!=1)return rfail(r,line,"animation method refused: %s",error);
    return 1;
}
