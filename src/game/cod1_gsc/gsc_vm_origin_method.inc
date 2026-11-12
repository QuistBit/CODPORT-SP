/* GetOrigin method-only capability. The unselected R198 alias remains a
 * legacy restoration profile, not a native brush-getter parity certificate. */
int gsc_runtime_select_origin_method(GscRuntime *r,const GscEntityOriginMethodHost *host) {
    if(!r||!host||runtime_private_overlap(r,host,sizeof(*host))||!host->read||
       r->started||r->advancing||r->executing_task||r->error[0]||
       !r->entity_operations_enabled||!r->access_enabled||r->origin_method_enabled)
        return 0;
    r->origin_method=*host;r->origin_method_enabled=1;return 1;
}
static int origin_method_current(GscRuntime *r,const GscEntityRef *owner,int line) {
    GscEntityRef borrowed;int alive;
    if(r->error[0]||!entity_task_current(r))return 0;
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    borrowed=*owner;alive=entity_validate(r,&borrowed);
    if(!state_ref_equal(owner,&borrowed))return rfail(r,line,"GetOrigin validator changed borrowed identity");
    if(r->error[0]||!entity_task_current(r))return 0;
    if(alive!=1)return rfail(r,line,"GetOrigin receiver is stale or missing");
    return 1;
}
static int origin_method_invoke(GscRuntime *r,const Value *receiver,int line,Value *out) {
    GscEntityRef owner,borrowed;float staged[3]={NAN,NAN,NAN};
    Value result;char why[256]={0};int ok=0,status,k;
    if(!r->origin_method_enabled||!r->origin_method.read||!receiver||receiver->kind!=V_ENTITY||
       !receiver->entity.epoch||!receiver->entity.generation||r->origin_method_busy||
       r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"GetOrigin method capability, owner or scope unavailable");
    owner=receiver->entity;r->origin_method_busy=1;
    if(!origin_method_current(r,&owner,line))goto done;
    borrowed=owner;status=r->origin_method.read(r->origin_method.user,&borrowed,staged,why,sizeof(why));
    why[sizeof(why)-1]=0;
    if(!state_ref_equal(&owner,&borrowed)) {rfail(r,line,"GetOrigin provider changed borrowed identity");goto done;}
    if(!origin_method_current(r,&owner,line))goto done;
    if(status!=1){rfail(r,line,"GetOrigin method refused: %s",why[0]?why:"host read failed");goto done;}
    for(k=0;k<3;k++)if(!isfinite(staged[k])){rfail(r,line,"GetOrigin method returned incomplete or nonfinite vector");goto done;}
    memset(&result,0,sizeof(result));result.kind=V_VECTOR;
    memcpy(result.vector,staged,sizeof(staged));if(out)*out=result;ok=1;
done:r->origin_method_busy=0;return ok;
}
