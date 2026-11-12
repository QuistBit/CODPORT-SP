/* Read-only typed engine queries. No native pointer escapes the callback and
 * no result is published after cancellation or retirement of either entity. */
int gsc_program_select_spatial_natives(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->spatial_natives_selected||!g->names_syntax)return 0;
    g->spatial_natives_selected=1;return 1;
}
int gsc_runtime_select_spatial_natives(GscRuntime *r,const GscSpatialNativeHost *host) {
    if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
       !r->program->spatial_natives_selected||r->spatial_natives_enabled||
       !r->entities_enabled||!r->state_enabled||!r->results_enabled||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->spatial_natives=*host;r->spatial_natives_enabled=1;return 1;
}
static int spatial_native_invoke(GscRuntime *r,const Value *receiver,int descriptor,
                                 const Value *args,int argc,int line,Value *out) {
    GscSpatialNativeRequest request,expected;
    GscSpatialNativeResult result;Value staged;char error[384];int kind,finished,ok,j;
    kind=spatial_method_kind(descriptor);
    if(!r->spatial_natives_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||
       argc!=(kind==GSC_SPATIAL_IS_TOUCHING?1:0)||(argc&&!args))
        return rfail(r,line,"spatial native capability, receiver or arity unavailable");
    if(argc&&args[0].kind!=V_ENTITY)return rfail(r,line,"istouching requires an entity argument");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"spatial native callback reentered");
    memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));memset(&staged,0,sizeof(staged));
    request.receiver=receiver->entity;request.kind=kind;request.now_ms=r->now_ms;
    if(argc)request.other=args[0].entity;
    expected=request;r->entity_method_busy=1;
    finished=entity_method_finish(r,&expected.receiver,line);
    if(finished>=0&&argc)finished=entity_method_finish(r,&expected.other,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    error[0]=0;ok=r->spatial_natives.invoke(r->spatial_natives.user,&request,&result,
        error,sizeof(error),player_native_current,r);error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"spatial native refused: %s",error[0]?error:"host callback failed");}
    if(memcmp(&request,&expected,sizeof(request))){r->entity_method_busy=0;return rfail(r,line,"spatial native mutated its borrowed request");}
    finished=entity_method_finish(r,&expected.receiver,line);
    if(finished>=0&&argc)finished=entity_method_finish(r,&expected.other,line);
    r->entity_method_busy=0;if(finished<0)return finished==-2;
    if(kind==GSC_SPATIAL_IS_TOUCHING){
        if(result.kind!=GSC_SPATIAL_RESULT_INT||(result.value!=0&&result.value!=1))
            return rfail(r,line,"spatial native returned an invalid boolean");
        scalar_integer(&staged,result.value);
    }else{
        if(result.kind!=GSC_SPATIAL_RESULT_VECTOR)return rfail(r,line,"spatial native returned the wrong vector kind");
        staged.kind=V_VECTOR;
        for(j=0;j<3;++j){if(!isfinite(result.vector[j]))return rfail(r,line,"spatial native returned a nonfinite vector");staged.vector[j]=result.vector[j];}
    }
    if(out)*out=staged;return 1;
}
