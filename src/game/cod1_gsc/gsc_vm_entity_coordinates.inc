/* R198 portable entity-coordinate methods. The engine AnglesToAxis convention
 * is forward, -right, up: local +Y is left, and zero angles are identity.
 * Origin and angles are two synchronous validated reads from the actual pose
 * access provider. Its API carries no atomic pose version, so no snapshot or
 * retail arithmetic/FPU parity is claimed. No result is published on refusal. */
int gsc_program_select_entity_coordinates(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->entity_coordinates_selected||!g->names_syntax)return 0;
    g->entity_coordinates_selected=1;return 1;
}
int gsc_runtime_select_entity_coordinates(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->entity_coordinates_enabled||!r->program->entity_coordinates_selected||
       !r->entities_enabled||!r->state_enabled||!r->access_enabled||!r->results_enabled||
       !r->names_enabled||!r->access.getorigin||!r->access.getangles||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->entity_coordinates_enabled=1;return 1;
}
static int entity_coordinate_guard(GscRuntime *r,const GscEntityRef *owner,int line) {
    GscEntityRef borrowed;int alive;
    if(r->error[0])return 0;
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    borrowed=*owner;alive=entity_validate(r,&borrowed);
    if(!state_ref_equal(owner,&borrowed))return rfail(r,line,"coordinate validator changed the borrowed receiver identity");
    if(r->error[0]||!entity_task_current(r))return 0;
    if(alive!=1)return rfail(r,line,"coordinate receiver is stale or missing");
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    return 1;
}
static int entity_coordinate_read(GscRuntime *r,const GscEntityRef *owner,int angles,int line,float output[3]) {
    GscEntityRef borrowed;float staged[3]={NAN,NAN,NAN};char why[256];int ok,k;
    if(!entity_coordinate_guard(r,owner,line))return 0;
    borrowed=*owner;why[0]=0;
    ok=angles?r->access.getangles(r->access.user,&borrowed,staged,why,sizeof(why)):
        r->access.getorigin(r->access.user,&borrowed,staged,why,sizeof(why));
    why[sizeof(why)-1]=0;
    if(!state_ref_equal(owner,&borrowed))return rfail(r,line,"coordinate pose provider changed the borrowed receiver identity");
    if(!entity_coordinate_guard(r,owner,line))return 0;
    if(ok!=1)return rfail(r,line,"coordinate %s read refused: %s",angles?"angles":"origin",why[0]?why:"host callback failed");
    for(k=0;k<3;k++)if(!isfinite(staged[k]))return rfail(r,line,"coordinate pose provider returned a nonfinite or incomplete vector");
    memcpy(output,staged,sizeof(staged));return 1;
}
static int entity_coordinate_invoke(GscRuntime *r,const Value *receiver,int descriptor,
                                      const Value *args,int argc,int line,Value *out) {
    GscEntityRef owner;Value result;float origin[3],angles[3];double input[3],axis[3][3];
    double sp,cp,sy,cy,sr,cr,pitch,yaw,roll,value;int k,j,ok=0;
    const double radians=0.017453292519943295769236907684886;
    if(!r->entity_coordinates_enabled||!entity_coordinate(descriptor)||!receiver||
       receiver->kind!=V_ENTITY||!receiver->entity.epoch||!receiver->entity.generation||
       argc!=1||!args||args[0].kind!=V_VECTOR)
        return rfail(r,line,"coordinate method requires the selected capability, entity receiver and one vector");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"coordinate method callback reentered");
    owner=receiver->entity;
    for(k=0;k<3;k++) {
        if(!isfinite(args[0].vector[k]))return rfail(r,line,"coordinate argument must be a finite three-component vector");
        input[k]=(double)args[0].vector[k];
    }
    r->entity_method_busy=1;
    if(!entity_coordinate_read(r,&owner,0,line,origin)||!entity_coordinate_read(r,&owner,1,line,angles))goto done;
    pitch=fmod((double)angles[0],360.0)*radians;yaw=fmod((double)angles[1],360.0)*radians;roll=fmod((double)angles[2],360.0)*radians;
    sp=sin(pitch);cp=cos(pitch);sy=sin(yaw);cy=cos(yaw);sr=sin(roll);cr=cos(roll);
    axis[0][0]=cp*cy;axis[0][1]=cp*sy;axis[0][2]=-sp;
    axis[1][0]=sr*sp*cy-cr*sy;axis[1][1]=sr*sp*sy+cr*cy;axis[1][2]=sr*cp;
    axis[2][0]=cr*sp*cy+sr*sy;axis[2][1]=cr*sp*sy-sr*cy;axis[2][2]=cr*cp;
    memset(&result,0,sizeof(result));result.kind=V_VECTOR;
    for(k=0;k<3;k++) {
        if(descriptor==ENTITY_COORD_LOCAL_TO_WORLD) {
            value=origin[k];for(j=0;j<3;j++)value+=input[j]*axis[j][k];
        } else {
            value=0;for(j=0;j<3;j++)value+=(input[j]-(double)origin[j])*axis[k][j];
        }
        result.vector[k]=(float)value;
        if(!isfinite(result.vector[k])){rfail(r,line,"coordinate result exceeds finite binary32 range");goto done;}
    }
    if(!entity_coordinate_guard(r,&owner,line))goto done;
    if(out)*out=result;ok=1;
done:
    r->entity_method_busy=0;return ok;
}
