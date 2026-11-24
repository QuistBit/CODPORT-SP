/* Native class3 query values, independent from class2/GAME identities.
 * Host callbacks return copied data under current map/Session ownership. */
typedef struct {GscRuntime *runtime;int now_ms;GscVehicleNodeHost host;} VehicleNodeLease;
static int vehicle_node_ref_equal(const GscVehicleNodeRef *a,const GscVehicleNodeRef *b) {
    return a->epoch==b->epoch&&a->index==b->index&&a->generation==b->generation;
}
static int vehicle_node_value_valid(const GscRuntime *r,const Value *value) {
    return r&&r->vehicle_nodes_enabled&&value&&value->kind==V_VEHICLE_NODE&&
        value->vehicle_node.epoch&&value->vehicle_node.generation&&value->vehicle_node.index<GSC_VEHICLE_NODE_CAPACITY;
}
static int vehicle_node_query_current(void *context) {
    VehicleNodeLease *lease=context;GscRuntime *r=lease->runtime;
    if(!r||r->error[0]||!r->vehicle_nodes_enabled||!r->vehicle_node_busy||
       r->now_ms!=lease->now_ms||memcmp(&r->vehicle_nodes,&lease->host,sizeof(lease->host)))return 0;
    return entity_task_current(r);
}
static int vehicle_node_query_begin(GscRuntime *r,int line,VehicleNodeLease *lease) {
    if(!r->vehicle_nodes_enabled||!r->program->vehicle_nodes_selected||!r->arrays_enabled||
       !r->state_enabled||r->error[0])return rfail(r,line,"node query capability is unavailable");
    if(r->vehicle_node_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"node query callback reentered");
    if(!entity_task_current(r))return 0;
    lease->runtime=r;lease->now_ms=r->now_ms;lease->host=r->vehicle_nodes;
    r->vehicle_node_busy=1;r->entity_method_busy=1;return 1;
}
static int vehicle_node_query_finish(GscRuntime *r,VehicleNodeLease *lease,int line) {
    int current=vehicle_node_query_current(lease);
    r->vehicle_node_busy=0;r->entity_method_busy=0;
    if(r->error[0])return 0;
    if(!current) {
        if(!entity_task_current(r))return 0;
        return rfail(r,line,"node callback changed its clock or capability");
    }
    return 1;
}
static int vehicle_node_ref_live(GscRuntime *r,const GscVehicleNodeRef *ref,int line,int *out) {
    VehicleNodeLease lease;GscVehicleNodeRef borrowed=*ref,expected=*ref;char error[256];int ok,alive=-1;
    if(!ref->epoch||!ref->generation||ref->index>=GSC_VEHICLE_NODE_CAPACITY)
        return rfail(r,line,"malformed node identity");
    if(!vehicle_node_query_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.validate(lease.host.user,&borrowed,lease.now_ms,&alive,
        error,sizeof(error),vehicle_node_query_current,&lease);error[sizeof(error)-1]=0;
    if(!vehicle_node_query_finish(r,&lease,line))return 0;
    if(!vehicle_node_ref_equal(&borrowed,&expected))return rfail(r,line,"node validator mutated its borrowed identity");
    if(ok!=1)return rfail(r,line,"node validation refused: %s",error[0]?error:"host callback failed");
    if(alive!=0&&alive!=1)return rfail(r,line,"node validator returned an invalid alive flag");
    *out=alive;return 1;
}
static int vehicle_node_value_live(GscRuntime *r,const Value *value,int line,int *alive) {
    if(!vehicle_node_value_valid(r,value))return rfail(r,line,"invalid selected node value");
    return vehicle_node_ref_live(r,&value->vehicle_node,line,alive);
}
static int vehicle_node_compare(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out) {
    int alive,equal;
    if(a->kind!=V_VEHICLE_NODE||b->kind!=V_VEHICLE_NODE||(op!=T_EQ&&op!=T_NE))
        return rfail(r,line,"node values support only identity equality");
    if(!vehicle_node_value_live(r,a,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node comparison identity is stale");
    if(!vehicle_node_value_live(r,b,line,&alive))return 0;
    if(!alive)return rfail(r,line,"node comparison identity is stale");
    equal=vehicle_node_ref_equal(&a->vehicle_node,&b->vehicle_node);scalar_integer(out,op==T_EQ?equal:!equal);return 1;
}
int gsc_runtime_select_vehicle_nodes(GscRuntime *r,const GscVehicleNodeHost *host) {
    if(!r||!host||!host->query||!host->validate||!host->read||r->started||r->advancing||
       r->executing_task||r->task_count||r->error[0]||r->vehicle_nodes_enabled||
       !r->program->vehicle_nodes_selected||!r->state_enabled||!r->arrays_enabled||
       r->vehicle_node_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->vehicle_nodes=*host;r->vehicle_nodes_enabled=1;return 1;
}
static int vehicle_node_query_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscVehicleNodeQuery request,expected;GscVehicleNodeRef refs[GSC_VEHICLE_NODE_CAPACITY];
    VehicleNodeLease lease;Value staged,member;char error[256];size_t count=(size_t)-1,n;int ok,alive,defined=-1;
    if(!out||node_query_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid node query arity");
    memset(&request,0,sizeof(request));memset(&staged,0,sizeof(staged));
    request.kind=kind;request.now_ms=r->now_ms;
    if(argc) {
        if(args[0].kind!=V_STRING||args[1].kind!=V_STRING||
           !memchr(args[0].str,0,sizeof(args[0].str))||!memchr(args[1].str,0,sizeof(args[1].str)))
            return rfail(r,line,"node query requires two ordinary strings and a nonempty field");
        memcpy(request.value,args[0].str,sizeof(request.value));memcpy(request.field,args[1].str,sizeof(request.field));
    }
    expected=request;memset(refs,0,sizeof(refs));
    if(!vehicle_node_query_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.query(lease.host.user,&request,refs,GSC_VEHICLE_NODE_CAPACITY,&count,&defined,
        error,sizeof(error),vehicle_node_query_current,&lease);error[sizeof(error)-1]=0;
    if(!vehicle_node_query_finish(r,&lease,line))return 0;
    if(memcmp(&request,&expected,sizeof(request)))return rfail(r,line,"node query mutated its borrowed request");
    if(ok!=1)return rfail(r,line,"node query refused: %s",error[0]?error:"host callback failed");
    if((defined!=0&&defined!=1)||(!defined&&count))return rfail(r,line,"invalid vehicle-node query definition flag");
    if(!defined){*out=staged;return 1;}
    if(count>GSC_VEHICLE_NODE_CAPACITY||(kind==GSC_VEHICLE_QUERY_ONE&&count>1))
        return rfail(r,line,"node query returned an invalid count or overflow");
    for(n=0;n<count;n++) {
        if(!vehicle_node_ref_live(r,&refs[n],line,&alive))return 0;
        if(!alive)return rfail(r,line,"node query returned a stale identity");
        if(n&&(refs[n].epoch!=refs[0].epoch||refs[n].index<=refs[n-1].index))
            return rfail(r,line,"node query returned mixed map epochs or unordered duplicate identities");
    }
    for(n=0;n<count;n++) {
        if(!vehicle_node_ref_live(r,&refs[n],line,&alive))return 0;
        if(!alive)return rfail(r,line,"node query identity retired before publication");
    }
    if(kind==GSC_VEHICLE_QUERY_ONE) {
        if(count){staged.kind=V_VEHICLE_NODE;staged.vehicle_node=refs[0];}
    } else {
        if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||count>(size_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
            return rfail(r,line,"node query array arena limit");
        /* All callback work precedes allocation/publication. */
        if(!array_new(r,line,&staged))return 0;
        for(n=0;n<count;n++) {
            ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)staged.array;
            entry.key.kind=V_NUMBER;entry.key.number=(int)n;
            memset(&member,0,sizeof(member));member.kind=V_VEHICLE_NODE;member.vehicle_node=refs[n];entry.value=member;
            r->array_entries[r->array_entry_count++]=entry;
        }
        r->arrays[staged.array-1].count=r->arrays[staged.array-1].numeric_count=(int)count;
    }
    *out=staged;return 1;
}

#include "gsc_vehicle_field_types.inc"
static int vehicle_node_field_builtin(const char*name){
 static const char*names[]={"targetname","target","origin","angles","speed","lookahead"};unsigned i;
 if(!name)return 0;for(i=0;i<6;i++)if(control_field_equal(name,names[i]))return i+1;return 0;
}
static int vehicle_node_field_kind(const char*name){
 unsigned i;int builtin=vehicle_node_field_builtin(name);
 if(builtin)return builtin<=2?GSC_VEHICLE_VALUE_STRING:builtin<=4?GSC_VEHICLE_VALUE_VECTOR:GSC_VEHICLE_VALUE_FLOAT;
 for(i=0;i<sizeof(vehicleCustomFields)/sizeof(vehicleCustomFields[0]);i++)if(control_field_equal(name,vehicleCustomFields[i].name))return vehicleCustomFields[i].kind;
 return GSC_VEHICLE_VALUE_UNDEFINED;
}
static int vehicle_node_field_read(GscRuntime*r,const Value*receiver,const char*name,int line,Value*out){
 VehicleNodeLease lease;GscVehicleNodeRef ref,expected;GscVehicleNodeField field;Value result;
 char fieldName[128],expectedName[128],error[256];int alive,ok,kind,i;
 if(!name||!name[0]||strlen(name)>=sizeof(fieldName))return rfail(r,line,"invalid vehicle-node field name");
 if(!vehicle_node_value_live(r,receiver,line,&alive))return 0;
 if(!alive)return rfail(r,line,"vehicle-node field identity is stale");
 ref=receiver->vehicle_node;expected=ref;memset(fieldName,0,sizeof(fieldName));str_copy(fieldName,sizeof(fieldName),name);memcpy(expectedName,fieldName,sizeof(fieldName));
 memset(&field,0,sizeof(field));field.kind=-1;memset(&result,0,sizeof(result));
 if(!vehicle_node_query_begin(r,line,&lease))return 0;
 error[0]=0;ok=lease.host.read(lease.host.user,&ref,fieldName,lease.now_ms,&field,error,sizeof(error),vehicle_node_query_current,&lease);error[sizeof(error)-1]=0;
 if(!vehicle_node_query_finish(r,&lease,line))return 0;
 if(memcmp(&ref,&expected,sizeof(ref))||memcmp(fieldName,expectedName,sizeof(fieldName)))return rfail(r,line,"vehicle-node reader mutated borrowed input");
 if(ok!=1)return rfail(r,line,"vehicle-node field read refused: %s",error[0]?error:"host failed");
 if(!vehicle_node_value_live(r,receiver,line,&alive))return 0;
 if(!alive)return rfail(r,line,"vehicle-node retired before field publication");
 kind=vehicle_node_field_kind(fieldName);
 if(field.kind==GSC_VEHICLE_VALUE_UNDEFINED){
  if(vehicle_node_field_builtin(fieldName)>=3)return rfail(r,line,"vehicle-node builtin numeric field is missing");
  *out=result;return 1;
 }
 if(!kind||field.kind!=kind)return rfail(r,line,"vehicle-node field has wrong type");
 if(kind==GSC_VEHICLE_VALUE_INT32)scalar_integer(&result,field.integer);
 else if(kind==GSC_VEHICLE_VALUE_STRING){if(!memchr(field.text,0,sizeof(field.text)))return rfail(r,line,"unterminated vehicle-node field string");result.kind=V_STRING;memcpy(result.str,field.text,sizeof(result.str));}
 else if(kind==GSC_VEHICLE_VALUE_FLOAT){if(!isfinite(field.number))return rfail(r,line,"nonfinite vehicle-node float");result.kind=V_NUMBER;result.number=field.number;}
 else{result.kind=V_VECTOR;for(i=0;i<3;i++){if(!isfinite(field.vector[i]))return rfail(r,line,"nonfinite vehicle-node vector");result.vector[i]=field.vector[i];}}
 *out=result;return 1;
}
