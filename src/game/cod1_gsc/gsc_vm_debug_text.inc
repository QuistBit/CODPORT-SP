/* R118 print3d wrapper bridge. Proven intrinsic order: argc2 defaults;
 * argc3 RGB2; argc4 alpha3,RGB2; all other counts scale4,alpha3,RGB2;
 * then text1,origin0. In particular argc0/1 first fail GetFloat4. No native
 * upper arity guard: MAX_PARAMS is existing parser/VM structure policy.
 * Bounded printable ordinary text, finite numeric conversion, finite vectors
 * and undefined return are selected port policy, not original getter parity. */
static int debug_text_runtime_preflight(const GscProgram *g,const GscDebugTextHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-DEBUG_TEXT_TARGET_BASE||i->target<-1-DEBUG_LINE_TARGET_BASE)continue;
        kind=-1-DEBUG_TEXT_TARGET_BASE-i->target;
        if(!g->debug_text_mode||i->module[0]||debug_text_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||debug_text_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid debug text binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: debug text requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE)continue;
        kind=e->target;
        if(!g->debug_text_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           debug_text_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           debug_text_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid debug text expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: debug text expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int debug_text_float(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    int32_t integer;
    if(argc<=index)return rfail(r,line,"print3d missing GetFloat argument %d",index);
    if(args[index].kind!=V_NUMBER)return rfail(r,line,"print3d GetFloat argument %d requires a numeric scalar",index);
    if(args[index].integer) {
        if(!scalar_get_integer(r,&args[index],line,&integer))return 0;
        *out=(float)integer;
    } else *out=args[index].number;
    return 1;
}
static int debug_text_vector(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    if(argc<=index)return rfail(r,line,"print3d missing GetVector argument %d",index);
    if(args[index].kind!=V_VECTOR)return rfail(r,line,"print3d GetVector argument %d requires a vector",index);
    memcpy(out,args[index].vector,3*sizeof(*out));return 1;
}
static int debug_text_string(GscRuntime *r,const Value *args,int argc,int line,GscDebugTextEvent *event) {
    const char *end;int32_t integer;int length;size_t n;
    if(argc<2)return rfail(r,line,"print3d missing GetString argument 1");
    if(args[1].kind==V_STRING) {
        end=memchr(args[1].str,0,sizeof(args[1].str));
        if(!end)return rfail(r,line,"print3d unterminated text");
        event->length=(size_t)(end-args[1].str);
        if(event->length>=sizeof(event->text))return rfail(r,line,"print3d oversized text");
        memcpy(event->text,args[1].str,event->length+1);
    } else if(args[1].kind==V_NUMBER) {
        /* Explicit port conversion for authored numeric diagnostic labels.
         * This is independent of the original external Scr_GetString rules. */
        if(args[1].integer) {
            if(!scalar_get_integer(r,&args[1],line,&integer))return 0;
            length=snprintf(event->text,sizeof(event->text),"%ld",(long)integer);
        } else {
            if(!isfinite(args[1].number))return rfail(r,line,"print3d numeric text is outside finite domain");
            length=snprintf(event->text,sizeof(event->text),"%.9g",(double)args[1].number);
        }
        if(length<0||(size_t)length>=sizeof(event->text))return rfail(r,line,"print3d numeric text conversion failed");
        event->length=(size_t)length;
    } else return rfail(r,line,"print3d GetString argument 1 requires selected ordinary string or numeric text domain");
    for(n=0;n<event->length;n++) {
        unsigned char c=(unsigned char)event->text[n];
        if(c<32||c>126)return rfail(r,line,"print3d text outside printable ASCII transport domain");
    }
    return 1;
}
static int debug_text_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscDebugTextEvent event,copy;char error[256];int accepted,context,n;
    if(!r->debug_text_enabled||!r->names_enabled||r->error[0]||!out||
       debug_text_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid debug text structural invocation");
    memset(&event,0,sizeof(event));
    event.color[0]=event.color[1]=event.color[2]=event.color[3]=event.scale=1.0f;
    if(argc!=2&&argc!=3&&argc!=4&&
       !debug_text_float(r,args,argc,4,line,&event.scale))return 0;
    if(argc!=2&&argc!=3&&
       !debug_text_float(r,args,argc,3,line,&event.color[3]))return 0;
    if(argc!=2&&!debug_text_vector(r,args,argc,2,line,event.color))return 0;
    if(!debug_text_string(r,args,argc,line,&event)||
       !debug_text_vector(r,args,argc,0,line,event.origin))return 0;
    for(n=0;n<3;n++)if(!isfinite(event.origin[n])||!isfinite(event.color[n]))
        return rfail(r,line,"print3d vector is outside finite transport domain");
    if(!isfinite(event.color[3])||!isfinite(event.scale))
        return rfail(r,line,"print3d scalar is outside finite transport domain");
    if(!r->debug_text.publish)return rfail(r,line,"debug text publish host capability is unavailable");
    if(!r->next_debug_text_operation)return rfail(r,line,"debug text operation token exhausted");
    event.operation_id=r->next_debug_text_operation;
    r->next_debug_text_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->debug_text.publish(r->debug_text.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"debug text host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    /* Retirement retains the accepted external prefix and cancels the task. */
    memset(out,0,sizeof(*out));return 1;
}
