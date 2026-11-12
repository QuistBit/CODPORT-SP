int gsc_program_select_r216_yaw(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->yaw_motion_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->yaw_motion_selected=1;return 1;
}
int gsc_runtime_select_r216_yaw(GscRuntime *r,const GscYawMotionHost *host) {
    if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
       !r->program->yaw_motion_selected||r->yaw_motion_enabled||
       !r->entities_enabled||!r->state_enabled||!r->scalar_switch_enabled||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->yaw_motion=*host;r->yaw_motion_enabled=1;return 1;
}
static int yaw_motion_current(void *context) {
    GscRuntime *r=context;return r&&!r->error[0]&&entity_task_current(r);
}
static int yaw_motion_invoke(GscRuntime *r,const Value *receiver,const Value *args,
    int argc,int line) {
    GscYawMotionRequest request,original;float values[4];int i,ok,finished;char error[384];
    if(!r->yaw_motion_enabled||!receiver||receiver->kind!=V_ENTITY||argc!=4||!args)
        return rfail(r,line,"yaw motion capability, receiver or arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"yaw motion callback reentered");
    for(i=0;i<4;++i) {
        if(!scalar_float(r,&args[i],line,&values[i]))return 0;
        if(!isfinite(values[i]))return rfail(r,line,"yaw motion requires finite scalars");
    }
    if(values[1]<=0.f||values[2]<0.f||values[3]<0.f||
       (double)values[2]+values[3]>(double)values[1])
        return rfail(r,line,"yaw motion requires positive duration and bounded ramp times");
    memset(&request,0,sizeof(request));request.entity=receiver->entity;
    request.yaw=values[0];request.seconds=values[1];request.accel=values[2];request.decel=values[3];
    request.now_ms=r->now_ms;original=request;
    r->entity_method_busy=1;finished=entity_method_finish(r,&request.entity,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    error[0]=0;ok=r->yaw_motion.invoke(r->yaw_motion.user,&request,error,sizeof(error),yaw_motion_current,r);
    error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"yaw motion refused: %s",error[0]?error:"host failed");}
    finished=entity_method_finish(r,&original.entity,line);r->entity_method_busy=0;
    if(finished<0)return finished==-2;
    if(memcmp(&request,&original,sizeof(request)))return rfail(r,line,"yaw motion mutated its borrowed request");
    return 1;
}
