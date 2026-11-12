int gsc_program_select_console_int32_display(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->console_int32_selected||!g->console_syntax||!g->scalar_switch_syntax||!g->names_syntax)return 0;
    g->console_int32_selected=1;return 1;
}
int gsc_runtime_select_console_int32_display(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->console_int32_enabled||
       !r->program->console_int32_selected||!r->console_enabled||!r->scalar_switch_enabled||!r->names_enabled||
       !r->console.query_gate||(!r->console.print&&!r->console.println)||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->console_int32_enabled=1;return 1;
}
/* R110 print/println wrapper bridge. The original engine display getter is an
 * external boundary: this bounded stage only displays ordinary VM strings.
 * Output remains per argument and a println newline is a separate sink action.
 * Already accepted output is never rolled back after a later refusal. */
static GscConsoleCallback console_callback(const GscConsoleHost *host,int kind) {
    if(!host)return NULL;
    return kind==GSC_CONSOLE_PRINT?host->print:kind==GSC_CONSOLE_PRINTLN?host->println:NULL;
}
static int console_runtime_preflight(const GscProgram *g,const GscConsoleHost *host,
    char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-CONSOLE_TARGET_BASE||i->target<-1-HUD_TARGET_BASE)continue;
        kind=-1-CONSOLE_TARGET_BASE-i->target;
        if(!g->console_mode||i->module[0]||console_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||console_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid console binding",i->line);
            return 0;
        }
        if(!host||!host->query_gate||!console_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: console requires gate and selected sink capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_CONSOLE_NATIVE)continue;
        kind=e->target;
        if(!g->console_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           console_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           console_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid console expression binding",e->line);
            return 0;
        }
        if(!host||!host->query_gate||!console_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: console expression requires gate and selected sink capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int console_context(GscRuntime *r) {
    if(r->error[0])return -1;
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return -1;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    return 1;
}
static int console_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscConsoleEvent event;GscConsoleCallback callback;char error[256];int enabled=-1,ok,n,context;
    const char *end;
    if(!r->console_enabled||!r->names_enabled||r->error[0]||
       console_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid console invocation");
    callback=console_callback(&r->console,kind);
    if(!r->console.query_gate||!callback)return rfail(r,line,"console host capability is unavailable");
    /* Original println outer gate precedes print's separate body gate. Once
     * the outer gate admits println, a suppressed inner print still permits
     * its newline; output callbacks never cause an extra gate reread. */
    if(kind==GSC_CONSOLE_PRINTLN) {
        if(r->console_int32_enabled)r->entity_prepare_busy=1;
        error[0]=0;ok=r->console.query_gate(r->console.user,r->now_ms,kind,
            GSC_CONSOLE_GATE_ENTRY,&enabled,error,sizeof(error));error[sizeof(error)-1]=0;
        if(r->console_int32_enabled)r->entity_prepare_busy=0;
        if(r->error[0])return 0;
        if(r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
        if(ok!=1)return rfail(r,line,"console host rejected entry gate: %s",error[0]?error:"unspecified host failure");
        if(!r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
        if(enabled!=0&&enabled!=1)return rfail(r,line,"console gate returned invalid enabled state");
        if(!enabled)return 1;
    }
    enabled=-1;error[0]=0;
    if(r->console_int32_enabled)r->entity_prepare_busy=1;
    ok=r->console.query_gate(r->console.user,r->now_ms,kind,
        GSC_CONSOLE_GATE_ARGUMENTS,&enabled,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->console_int32_enabled)r->entity_prepare_busy=0;
    if(r->error[0])return 0;
    if(r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
    if(ok!=1)return rfail(r,line,"console host rejected argument gate: %s",error[0]?error:"unspecified host failure");
    if(!r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
    if(enabled!=0&&enabled!=1)return rfail(r,line,"console gate returned invalid enabled state");
    if(!enabled&&kind==GSC_CONSOLE_PRINT)return 1;
    for(n=0;enabled&&n<argc;n++) {
        memset(&event,0,sizeof(event));event.kind=kind;event.phase=GSC_CONSOLE_ARGUMENT;
        event.argument_index=n;
        if(r->console_int32_enabled&&args[n].kind==V_NUMBER&&args[n].integer) {
            int32_t integer;
            if(!scalar_get_integer(r,&args[n],line,&integer))return 0;
            Cod1ModelIntText(integer,event.text);event.length=strlen(event.text);
        } else if(r->program->carbine_items_selected&&args[n].kind==V_NUMBER&&!args[n].integer) {
            GscStringCastValue value;GscStringCastResult text;
            memset(&value,0,sizeof(value));value.type=4;value.number=args[n].number;
            if(GscStringCast(&value,&text,error,sizeof(error))!=1)return rfail(r,line,"unsupported console float display");
            strcpy(event.text,text.string);event.length=strlen(event.text);
        } else {
            /* Native DebugString 469f80 raw2 fallback at46a056 jumps to
             * SL_ConvertToString45ccc0: copy the token, without localization. */
            if(args[n].kind!=V_STRING&&!(r->program->carbine_items_selected&&r->objectives_enabled&&args[n].kind==V_LOCALIZED_STRING))
                return rfail(r,line,"console display getter admits ordinary strings only in this stage");
            end=memchr(args[n].str,0,sizeof(args[n].str));
            if(!end)return rfail(r,line,"unterminated console display string");
            event.length=(size_t)(end-args[n].str);memcpy(event.text,args[n].str,event.length+1);
        }
        if(r->console_int32_enabled)r->entity_prepare_busy=1;
        error[0]=0;ok=callback(r->console.user,r->now_ms,&event,error,sizeof(error));
        error[sizeof(error)-1]=0;
        if(r->console_int32_enabled)r->entity_prepare_busy=0;
        if(r->error[0])return 0;
        if(r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
        if(ok!=1)return rfail(r,line,"console host rejected argument: %s",error[0]?error:"unspecified host failure");
        if(!r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
    }
    if(kind==GSC_CONSOLE_PRINTLN) {
        memset(&event,0,sizeof(event));event.kind=kind;event.phase=GSC_CONSOLE_NEWLINE;
        event.argument_index=-1;event.length=1;event.text[0]='\n';
        if(r->console_int32_enabled)r->entity_prepare_busy=1;
        error[0]=0;ok=callback(r->console.user,r->now_ms,&event,error,sizeof(error));
        error[sizeof(error)-1]=0;
        if(r->console_int32_enabled)r->entity_prepare_busy=0;
        if(r->error[0])return 0;
        if(r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
        if(ok!=1)return rfail(r,line,"console host rejected newline: %s",error[0]?error:"unspecified host failure");
        if(!r->console_int32_enabled){context=console_context(r);if(context<0)return 0;if(!context)return 1;}
    }
    return 1;
}
