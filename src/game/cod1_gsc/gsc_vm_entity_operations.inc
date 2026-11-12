/* R198 aliases use the existing pose access provider. Deletion has its own
 * typed provider because a successful delete deliberately retires a receiver. */
static int delete_transaction_invoke(GscRuntime *,const Value *,int,Value *);
int gsc_program_select_entity_operations(GscProgram *g){
    if(!g||g->invalid||g->linked||g->entity_operations_selected||!g->names_syntax)return 0;
    g->entity_operations_selected=1;return 1;
}
int gsc_runtime_select_entity_operations(GscRuntime *r,const GscEntityDeleteHost *host){
    if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
       !r->program->entity_operations_selected||r->entity_operations_enabled||
       !r->entities_enabled||!r->state_enabled||!r->results_enabled||!r->access_enabled||
       !r->access.getorigin||!r->access.getangles||!r->access.setangles||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->entity_delete=*host;r->entity_operations_enabled=1;return 1;
}
static int entity_operation_invoke(GscRuntime *r,const Value *receiver,int descriptor,
                                    const Value *args,int argc,int line,Value *out){
    Value result;GscEntityRef owner,borrowed;char why[384];int ok,finished;
    if(!r->entity_operations_enabled||!entity_operation(descriptor)||!receiver||
       receiver->kind!=V_ENTITY||argc!=(descriptor==ENTITY_OP_SETANGLES?1:0)||(argc&&!args))
        return rfail(r,line,"entity operation capability, receiver or arity unavailable");
    if(descriptor==ENTITY_OP_GETORIGIN&&r->origin_method_enabled)
        return origin_method_invoke(r,receiver,line,out);
    if(descriptor==ENTITY_OP_GETORIGIN||descriptor==ENTITY_OP_GETANGLES){
        if(!bridge_property_read(r,receiver,descriptor==ENTITY_OP_GETORIGIN?ENTITY_PROPERTY_ORIGIN:ENTITY_PROPERTY_ANGLES,line,&result))return 0;
        if(out)*out=result;return 1;
    }
    if(descriptor==ENTITY_OP_SETANGLES)return bridge_property_write(r,receiver,ENTITY_PROPERTY_ANGLES,args,line);
    if(r->program->delete_transactions_selected)return delete_transaction_invoke(r,receiver,line,out);
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"entity delete callback reentered");
    owner=receiver->entity;r->entity_method_busy=1;
    finished=entity_method_finish(r,&owner,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    borrowed=owner;
    why[0]=0;ok=r->entity_delete.invoke(r->entity_delete.user,&borrowed,r->now_ms,why,sizeof(why),player_native_current,r);why[sizeof(why)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"entity delete refused: %s",why[0]?why:"host callback failed");}
    if(!entity_ref_equal(&borrowed,&owner)){r->entity_method_busy=0;return rfail(r,line,"entity delete mutated its borrowed owner");}
    /* A success that left the allocation alive is an invalid host result. */
    ok=entity_validate(r,&owner);r->entity_method_busy=0;
    if(r->error[0])return 0;
    if(ok!=0)return rfail(r,line,"entity delete host left the receiver alive");
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(out)memset(out,0,sizeof(*out));
    return 1;
}
