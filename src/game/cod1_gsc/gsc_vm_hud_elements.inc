/* R200 typed service-owned HUD allocations. Values retain identity across
 * script copies and waits; destroyed allocations never become new elements. */
typedef struct {GscRuntime *runtime;GscHudElemHost host;uint64_t owner;int now;} HudElemLease;
static int hud_ref_equal(const GscHudElemRef *a,const GscHudElemRef *b){
    return a->epoch==b->epoch&&a->owner==b->owner&&a->index==b->index&&a->generation==b->generation;
}
static int hud_value_valid(const GscRuntime *r,const Value *v){
    return r&&r->hud_elements_enabled&&!r->hud_elements_retired&&v&&v->kind==V_HUDELEM&&
        v->hud.owner==r->array_token&&v->hud.epoch&&v->hud.epoch==r->hud_elements_epoch&&
        v->hud.index<GSC_HUDELEM_CAPACITY&&v->hud.generation&&
        v->hud.generation<=r->hud_refs[v->hud.index].generation;
}
static int hud_lease_current(void *context){
    HudElemLease *lease=context;GscRuntime *r=lease->runtime;
    return r&&!r->error[0]&&r->hud_elements_enabled&&!r->hud_elements_retired&&r->hud_elements_busy&&
        r->array_token==lease->owner&&r->now_ms==lease->now&&
        !memcmp(&r->hud_elements,&lease->host,sizeof(lease->host))&&entity_task_current(r);
}
static int hud_begin(GscRuntime *r,int line,HudElemLease *lease){
    if(!r->hud_elements_enabled||r->hud_elements_retired||!r->program->hud_elements_selected||r->error[0])
        return rfail(r,line,"HUD element capability is unavailable");
    if(r->hud_elements_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"HUD element callback reentered");
    if(!entity_task_current(r))return 0;
    lease->runtime=r;lease->host=r->hud_elements;lease->owner=r->array_token;lease->now=r->now_ms;
    r->hud_elements_busy=1;r->entity_method_busy=1;return 1;
}
static int hud_finish(GscRuntime *r,HudElemLease *lease,int line){
    int current=hud_lease_current(lease);r->hud_elements_busy=0;r->entity_method_busy=0;
    if(r->error[0])return 0;
    if(!current){if(!entity_task_current(r))return 0;return rfail(r,line,"HUD callback changed its clock or capability");}
    return 1;
}
static int hud_value_live(GscRuntime *r,const Value *v,int line,int *out){
    HudElemLease lease;GscHudElemRef copy,expected;char error[256];int ok,alive=-1;
    if(!hud_value_valid(r,v))return rfail(r,line,"invalid or foreign HUD element reference");
    if(!entity_task_current(r))return 0;
    if(!r->hud_live[v->hud.index]||!hud_ref_equal(&v->hud,&r->hud_refs[v->hud.index])){*out=0;return 1;}
    copy=v->hud;expected=copy;if(!hud_begin(r,line,&lease))return 0;
    error[0]=0;ok=lease.host.validate(lease.host.user,&copy,lease.now,&alive,error,sizeof(error),hud_lease_current,&lease);
    error[sizeof(error)-1]=0;if(!hud_finish(r,&lease,line))return 0;
    if(!hud_ref_equal(&copy,&expected))return rfail(r,line,"HUD validator mutated its borrowed identity");
    if(ok!=1)return rfail(r,line,"HUD validation refused: %s",error[0]?error:"host callback failed");
    if(alive!=0&&alive!=1)return rfail(r,line,"HUD validator returned invalid alive flag");
    if(!alive)r->hud_live[v->hud.index]=0;*out=alive;return 1;
}
static int hud_receiver(GscRuntime *r,const Value *v,int line){
    int alive;if(!hud_value_live(r,v,line,&alive))return 0;
    return alive?1:rfail(r,line,"HUD element receiver is destroyed or stale");
}
static int hud_compare(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out){
    if(a->kind!=V_HUDELEM||b->kind!=V_HUDELEM||(op!=T_EQ&&op!=T_NE))
        return rfail(r,line,"HUD elements support only current identity equality");
    if(!hud_receiver(r,a,line)||!hud_receiver(r,b,line))return 0;
    scalar_integer(out,op==T_EQ?hud_ref_equal(&a->hud,&b->hud):!hud_ref_equal(&a->hud,&b->hud));return 1;
}
int gsc_runtime_select_hud_elements(GscRuntime *r,const GscHudElemHost *host){
    if(!r||!host||!host->invoke||!host->validate||!host->retire||r->started||r->advancing||
       r->executing_task||r->task_count||r->error[0]||r->hud_elements_enabled||
       !r->program->hud_elements_selected||!r->state_enabled||!r->arrays_enabled||!r->array_token||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_method_busy||r->entity_sequence_busy||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->hud_elements=*host;r->hud_elements_enabled=1;r->hud_elements_operation=1;return 1;
}
int gsc_runtime_retire_hud_elements(GscRuntime *r,char *error,size_t cap){
    GscHudElemHost host;uint64_t owner;int ok;char why[256];
    if(error&&cap)error[0]=0;
    if(!r||!r->hud_elements_enabled||r->hud_elements_retired)return 1;
    if(r->advancing||r->executing_task||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy){
        if(error&&cap)snprintf(error,cap,"HUD retirement during active execution");return 0;
    }
    host=r->hud_elements;owner=r->array_token;r->hud_elements_busy=1;why[0]=0;
    ok=host.retire(host.user,owner,why,sizeof(why));why[sizeof(why)-1]=0;r->hud_elements_busy=0;
    if(ok!=1||r->array_token!=owner||memcmp(&r->hud_elements,&host,sizeof(host))){
        if(error&&cap)snprintf(error,cap,"HUD cleanup refused: %s",why[0]?why:"host callback failed");return 0;
    }
    r->hud_elements_retired=1;memset(r->hud_live,0,sizeof(r->hud_live));return 1;
}
static int hud_service(GscRuntime *r,GscHudElemRequest *request,int line,GscHudElemResult *out){
    HudElemLease lease;GscHudElemRequest expected;GscHudElemResult staged;char error[256];int ok;
    if(!r->hud_elements_operation)return rfail(r,line,"HUD operation identity exhausted");
    if(!hud_begin(r,line,&lease))return 0;
    request->owner=lease.owner;request->now_ms=lease.now;request->operation_id=r->hud_elements_operation;
    r->hud_elements_operation=r->hud_elements_operation==UINT64_MAX?0:r->hud_elements_operation+1;
    expected=*request;memset(&staged,0,sizeof(staged));staged.kind=-1;staged.alive=-1;error[0]=0;
    ok=lease.host.invoke(lease.host.user,request,&staged,error,sizeof(error),hud_lease_current,&lease);
    error[sizeof(error)-1]=0;if(!hud_finish(r,&lease,line))return 0;
    if(memcmp(request,&expected,sizeof(expected)))return rfail(r,line,"HUD host mutated its borrowed request");
    if(ok!=1)return rfail(r,line,"HUD service refused: %s",error[0]?error:"host callback failed");
    if(staged.alive!=(request->kind==GSC_HUDELEM_DESTROY?0:1))return rfail(r,line,"HUD service returned invalid lifecycle state");
    if(request->kind!=GSC_HUDELEM_CREATE&&!hud_ref_equal(&staged.element,&request->element))
        return rfail(r,line,"HUD service changed element identity");
    *out=staged;return 1;
}
static int hud_create(GscRuntime *r,int argc,int line,Value *out){
    GscHudElemRequest request;GscHudElemResult result;Value staged;int alive;
    if(argc||!out)return rfail(r,line,"newHudElem requires no arguments");
    memset(&request,0,sizeof(request));request.kind=GSC_HUDELEM_CREATE;
    if(!hud_service(r,&request,line,&result))return 0;
    if(result.kind!=GSC_HUDELEM_RESULT_REF||!result.element.epoch||result.element.owner!=r->array_token||
       result.element.index>=GSC_HUDELEM_CAPACITY||!result.element.generation||
       r->hud_live[result.element.index]||
       (r->hud_elements_epoch&&result.element.epoch!=r->hud_elements_epoch)||
       result.element.generation<=r->hud_refs[result.element.index].generation)
        return rfail(r,line,"newHudElem returned malformed, duplicate or foreign identity");
    r->hud_elements_epoch=result.element.epoch;r->hud_refs[result.element.index]=result.element;
    r->hud_live[result.element.index]=1;memset(&staged,0,sizeof(staged));staged.kind=V_HUDELEM;staged.hud=result.element;
    if(!hud_value_live(r,&staged,line,&alive))return 0;
    if(!alive)return rfail(r,line,"newHudElem allocation retired before publication");
    *out=staged;return 1;
}
static int hud_alignment(int field,const char *text){
    return field==GSC_HUDELEM_FIELD_ALIGN_X?(!strcmp(text,"left")||!strcmp(text,"center")||!strcmp(text,"right")):
        field==GSC_HUDELEM_FIELD_ALIGN_Y?(!strcmp(text,"top")||!strcmp(text,"middle")||!strcmp(text,"bottom")):0;
}
static int hud_field_write(GscRuntime *r,const Value *receiver,const char *name,const Value *value,int line){
    GscHudElemRequest request;GscHudElemResult result;int field=hud_elem_field(name);float number;
    if(!field)return rfail(r,line,"unsupported HUD element field %s",name);
    memset(&request,0,sizeof(request));request.kind=GSC_HUDELEM_WRITE;request.field=field;
    if(field==GSC_HUDELEM_FIELD_X||field==GSC_HUDELEM_FIELD_Y||field>=GSC_HUDELEM_FIELD_ALPHA){
        if(!scalar_float(r,value,line,&number))return 0;
        if(number<-8192||number>8192||(field==GSC_HUDELEM_FIELD_FONT_SCALE&&(number<=0||number>16)))return rfail(r,line,"HUD position outside [-8192,8192]");
        request.value_kind=GSC_HUDELEM_VALUE_NUMBER;request.number=number;
    }else{
        if(value->kind!=V_STRING||!memchr(value->str,0,sizeof(value->str))||!hud_alignment(field,value->str))
            return rfail(r,line,"HUD alignment requires an admitted ordinary string");
        request.value_kind=GSC_HUDELEM_VALUE_STRING;memcpy(request.text,value->str,sizeof(request.text));
    }
    if(!hud_receiver(r,receiver,line))return 0;request.element=receiver->hud;
    if(!hud_service(r,&request,line,&result))return 0;
    if(result.kind!=GSC_HUDELEM_RESULT_VOID)return rfail(r,line,"HUD write returned wrong result kind");
    return hud_receiver(r,receiver,line);
}
static int hud_field_read(GscRuntime *r,const Value *receiver,const char *name,int line,Value *out){
    GscHudElemRequest request;GscHudElemResult result;Value staged;int field=hud_elem_field(name);
    if(!field)return rfail(r,line,"unsupported HUD element field %s",name);
    if(!hud_receiver(r,receiver,line))return 0;
    memset(&request,0,sizeof(request));request.kind=GSC_HUDELEM_READ;request.field=field;request.element=receiver->hud;
    if(!hud_service(r,&request,line,&result)||!hud_receiver(r,receiver,line))return 0;
    memset(&staged,0,sizeof(staged));
    if(field==GSC_HUDELEM_FIELD_X||field==GSC_HUDELEM_FIELD_Y||field>=GSC_HUDELEM_FIELD_ALPHA){
        if(result.kind!=GSC_HUDELEM_RESULT_NUMBER||!isfinite(result.number)||result.number<-8192||result.number>8192)
            return rfail(r,line,"HUD read returned invalid position");
        staged.kind=V_NUMBER;staged.number=result.number;
    }else{
        if(result.kind!=GSC_HUDELEM_RESULT_STRING||!memchr(result.text,0,sizeof(result.text))||!hud_alignment(field,result.text))
            return rfail(r,line,"HUD read returned invalid alignment");
        staged.kind=V_STRING;memcpy(staged.str,result.text,sizeof(staged.str));
    }
    *out=staged;return 1;
}
static int hud_method_invoke(GscRuntime *r,const Value *receiver,int method,const Value *args,int argc,int line){
    GscHudElemRequest request;GscHudElemResult result;float remaining,total;int32_t width,height;
    if(argc!=hud_elem_arity(method)||(argc&&!args))return rfail(r,line,"HUD method argument count is invalid");
    memset(&request,0,sizeof(request));request.kind=method;
    if(method==GSC_HUDELEM_SET_CLOCK){
        if(!scalar_float(r,&args[0],line,&remaining)||!scalar_float(r,&args[1],line,&total))return 0;
        if(remaining<0||total<=0||remaining>total||total>86400)return rfail(r,line,"HUD clock seconds outside admitted range");
        if(args[2].kind!=V_STRING||!memchr(args[2].str,0,sizeof(args[2].str))||strcmp(args[2].str,"hudStopwatch"))
            return rfail(r,line,"HUD clock requires the supported hudStopwatch shader");
        if(!scalar_get_integer(r,&args[3],line,&width)||!scalar_get_integer(r,&args[4],line,&height))return 0;
        if(width<1||width>1024||height<1||height>1024)return rfail(r,line,"HUD clock dimensions outside 1..1024");
        request.remaining_seconds=remaining;request.total_seconds=total;request.width=width;request.height=height;
        memcpy(request.text,args[2].str,sizeof(request.text));
    }
    if(method==GSC_HUDELEM_SET_SHADER) {
        if(args[0].kind!=V_STRING||!args[0].str[0]||!memchr(args[0].str,0,sizeof(request.text))||
           !scalar_get_integer(r,&args[1],line,&width)||!scalar_get_integer(r,&args[2],line,&height))
            return rfail(r,line,"HUD shader requires a bounded name and integer dimensions");
        if(width<0||width>1024||height<0||height>1024)return rfail(r,line,"HUD shader dimensions outside selected domain");
        strcpy(request.text,args[0].str);request.width=width;request.height=height;
    }
    if(method==GSC_HUDELEM_SET_TEXT) {
        if(args[0].kind!=V_LOCALIZED_STRING||!memchr(args[0].str,0,sizeof(args[0].str)))
            return rfail(r,line,"HUD setText requires selected localized token");
        memcpy(request.text,args[0].str,sizeof(request.text));
    }
    if(method==GSC_HUDELEM_FADE) {
        if(!scalar_float(r,&args[0],line,&request.number)||request.number<=0||request.number>60)
            return rfail(r,line,"HUD fade seconds must be >0 and <=60");
    }
    if(!hud_receiver(r,receiver,line))return 0;request.element=receiver->hud;
    if(!hud_service(r,&request,line,&result))return 0;
    if(result.kind!=GSC_HUDELEM_RESULT_VOID)return rfail(r,line,"HUD method returned wrong result kind");
    if(method==GSC_HUDELEM_DESTROY){r->hud_live[receiver->hud.index]=0;return 1;}
    return hud_receiver(r,receiver,line);
}
