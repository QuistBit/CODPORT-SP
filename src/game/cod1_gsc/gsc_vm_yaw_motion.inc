int gsc_program_select_r216_yaw(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->yaw_motion_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->yaw_motion_selected=1;return 1;
}
int gsc_program_select_map_moveto(GscProgram *g){
 if(!g||g->invalid||g->linked||g->module_count||!g->yaw_motion_selected||g->map_moveto_selected)return 0;
 g->map_moveto_selected=1;return 1;
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
static int yaw_motion_invoke(GscRuntime *r,int method,const Value *receiver,const Value *args,
    int argc,int line) {
    GscYawMotionRequest request,original;int i,ok,finished;char error[384];
    if(!r->yaw_motion_enabled||!receiver||receiver->kind!=V_ENTITY||!entity_method_argc_valid(method,argc)||!args)
        return rfail(r,line,"yaw motion capability, receiver or arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"yaw motion callback reentered");
    memset(&request,0,sizeof(request));request.entity=receiver->entity;
    if(method==COD1_LAUNCH){
        request.method=method;request.operandsValid=args[0].kind==V_VECTOR;
        if(request.operandsValid)for(i=0;i<3;i++){
            request.velocity[i]=args[0].vector[i];
            if(!isfinite(request.velocity[i]))request.operandsValid=0;
        }
    }else if(method==502||method==COD1_ROTATE_ROLL){
        request.method=method;request.operandsValid=1;
        for(i=0;i<argc&&i<4;i++){
            float *out=i==0?&request.yaw:i==1?&request.seconds:i==2?&request.accel:&request.decel;
            if(args[i].kind!=V_NUMBER){request.operandsValid=0;break;}
            if(!scalar_float(r,args+i,line,out))return 0;
        }
    }else if(method!=502){
        request.method=method;
        if(args[0].kind==V_VECTOR && args[1].kind==V_NUMBER){
            if(!scalar_float(r,&args[1],line,&request.seconds))return 0;
            request.operandsValid=1;
            if(method==COD1_MOVE_TO||method==COD1_ROTATE_TO){
                for(i=2;i<argc && i<4;i++){
                    float *out=i==2?&request.accel:&request.decel;
                    if(args[i].kind!=V_NUMBER){request.operandsValid=0;break;}
                    if(!scalar_float(r,args+i,line,out))return 0;
                }
            }
            for(i=0;i<3;i++){
                request.velocity[i]=args[0].vector[i];
                if(!isfinite(request.velocity[i]))request.operandsValid=0;
            }
        }
    }
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
