int gsc_program_select_r232_randomfloat_range(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||g->random_range_selected)return 0;
    g->random_range_selected=1;return 1;
}
int gsc_runtime_select_r232_randomfloat_range(GscRuntime *r) {
    if(!animation_namespace_quiescent(r)||!r->program->random_range_selected||!r->turret_animations_enabled||
       !r->randomfloat_enabled||!r->dynamic_randomfloat_enabled||!r->randomfloat.draw||r->random_range_enabled||
       !animation_namespace_scope(r)||!anim_data_current(r,NULL))return 0;
    r->random_range_enabled=1;return 1;
}
typedef struct {GscRuntime*r;Task*task;uint64_t serial,instance,catalog;int now,checking;GscRandomFloatHost host;} R232RangeLease;
static int random_range_scope(const R232RangeLease*l){GscRuntime*r=l->r;
    return r&&!r->error[0]&&r->random_range_enabled&&r->randomfloat_enabled&&r->dynamic_randomfloat_enabled&&
        r->entity_method_busy&&r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&
        r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&r->now_ms==l->now&&
        (r->map_source_float_enabled?(l->task->map_certificate.receipt&&l->task->map_certificate.current):animation_namespace_scope(r))&&!memcmp(&r->randomfloat,&l->host,sizeof(l->host));
}
static int random_range_current(R232RangeLease*l){int ok;
    if(!random_range_scope(l))return 0;if(l->checking)return rfail(l->r,0,"randomFloatRange current reentered");l->checking=1;
    ok=entity_task_current(l->r)&&random_range_scope(l)&&(l->r->map_source_float_enabled?entity_task_current(l->r):anim_data_current(l->r,l->task))&&random_range_scope(l)&&entity_task_current(l->r)&&random_range_scope(l);
    l->checking=0;return ok;
}
static int random_range_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
    uint32_t low,high,bits;GscRandomFloatRequest q,original;R232RangeLease l;Value result;char error[256];int ok;
    if(!r||!r->random_range_enabled||!args||argc!=2||!out)return rfail(r,line,"randomFloatRange capability or exact two arguments unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy)return rfail(r,line,"randomFloatRange reentered");
    if(!float_arithmetic_number_bits(r,args,line,&low)||!float_arithmetic_number_bits(r,args+1,line,&high)||!r232_range_bounds(low,high))return rfail(r,line,"randomFloatRange requires ordered finite GetFloat bounds within +/-2^24");
    if(!r->randomfloat.draw||!r->next_randomfloat_operation)return rfail(r,line,"randomFloatRange shared draw owner unavailable or exhausted");
    memset(&l,0,sizeof(l));l.r=r;l.task=r->executing_task;l.serial=l.task?l.task->serial:0;l.instance=r->array_token;l.catalog=r->animation_namespace.identity;l.now=r->now_ms;l.host=r->randomfloat;
    memset(&q,0,sizeof(q));q.operation_id=r->next_randomfloat_operation;q.bound_bits=UINT32_C(0x3f800000);q.draw15=UINT32_MAX;original=q;
    r->entity_method_busy=1;if(!random_range_current(&l))goto retired;
    r->next_randomfloat_operation=q.operation_id==UINT64_MAX?0:q.operation_id+1;error[0]=0;
    ok=l.host.draw(l.host.user,l.now,&q,error,sizeof(error));error[sizeof(error)-1]=0;
    if(!random_range_current(&l))goto retired;
    if(ok!=1||!randomfloat_reply_valid(&original,&q)){r->entity_method_busy=0;return rfail(r,line,"randomFloatRange shared owner refused or changed draw: %s",error);}
    if(!r232_range_project(low,high,q.draw15,&bits)){r->entity_method_busy=0;return rfail(r,line,"randomFloatRange native arithmetic domain unavailable");}
    if(!random_range_current(&l))goto retired;
    memset(&result,0,sizeof(result));result.kind=V_NUMBER;memcpy(&result.number,&bits,4);r->entity_method_busy=0;*out=result;return 1;
retired:r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"randomFloatRange task, clock, provider or resource certificate retired");
}
