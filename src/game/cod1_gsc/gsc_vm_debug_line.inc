/* R119 proven line wrapper order, raw optional integer argument4 retained.
 * argc2 endpoint1,endpoint0; argc3 RGB2,endpoint1,endpoint0; argc4 alpha3,
 * RGB2,endpoint1,endpoint0; every other count Int4,alpha3,RGB2,end1,start0.
 * Original syscall0x52 adds an immutable final0; its engine semantics and the
 * optional word meaning remain unproved. Strict int32/finite data/undefined
 * result and bounded parser16 are selected port policies. */
static int debug_line_runtime_preflight(const GscProgram *g,const GscDebugLineHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-DEBUG_LINE_TARGET_BASE||i->target<-1-AMBIENT_TARGET_BASE)continue;
        kind=-1-DEBUG_LINE_TARGET_BASE-i->target;
        if(!g->debug_line_mode||i->module[0]||debug_line_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||debug_line_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid debug line binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: debug line requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_DEBUG_LINE_NATIVE)continue;
        kind=e->target;
        if(!g->debug_line_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           debug_line_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           debug_line_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid debug line expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: debug line expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int debug_line_integer(GscRuntime *r,const Value *args,int argc,int index,int line,int32_t *out) {
    if(argc<=index)return rfail(r,line,"line missing GetInt argument %d",index);
    if(args[index].kind!=V_NUMBER||args[index].integer!=1)
        return rfail(r,line,"line GetInt argument %d requires selected int32 scalar domain",index);
    return scalar_get_integer(r,&args[index],line,out);
}
static int debug_line_float(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    int32_t integer;
    if(argc<=index)return rfail(r,line,"line missing GetFloat argument %d",index);
    if(args[index].kind!=V_NUMBER)return rfail(r,line,"line GetFloat argument %d requires a numeric scalar",index);
    if(args[index].integer) {
        if(!scalar_get_integer(r,&args[index],line,&integer))return 0;
        *out=(float)integer;
    } else *out=args[index].number;
    return 1;
}
static int debug_line_vector(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    if(argc<=index)return rfail(r,line,"line missing GetVector argument %d",index);
    if(args[index].kind!=V_VECTOR)return rfail(r,line,"line GetVector argument %d requires a vector",index);
    memcpy(out,args[index].vector,3*sizeof(*out));return 1;
}
static int debug_line_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscDebugLineEvent event,copy;char error[256];int accepted,context,n;
    if(!r->debug_line_enabled||!r->names_enabled||r->error[0]||!out||
       debug_line_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid debug line structural invocation");
    memset(&event,0,sizeof(event));
    event.color[0]=event.color[1]=event.color[2]=event.color[3]=1.0f;
    if(argc!=2&&argc!=3&&argc!=4&&
       !debug_line_integer(r,args,argc,4,line,&event.optional_integer))return 0;
    if(argc!=2&&argc!=3&&!debug_line_float(r,args,argc,3,line,&event.color[3]))return 0;
    if(argc!=2&&!debug_line_vector(r,args,argc,2,line,event.color))return 0;
    if(!debug_line_vector(r,args,argc,1,line,event.end)||
       !debug_line_vector(r,args,argc,0,line,event.start))return 0;
    for(n=0;n<3;n++)if(!isfinite(event.start[n])||!isfinite(event.end[n])||!isfinite(event.color[n]))
        return rfail(r,line,"line vector is outside finite transport domain");
    if(!isfinite(event.color[3]))return rfail(r,line,"line scalar is outside finite transport domain");
    if(!r->debug_line.publish)return rfail(r,line,"debug line publish host capability is unavailable");
    if(!r->next_debug_line_operation)return rfail(r,line,"debug line operation token exhausted");
    event.operation_id=r->next_debug_line_operation;
    r->next_debug_line_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->debug_line.publish(r->debug_line.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"debug line host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
