/* R96 syntax stays typed. Output arguments are bindings, never expressions to
 * evaluate. Preflight every retained event, including dead functions/modules. */
static int payload_output_valid(const GscProgram *g,int expression) {
    const Expr *e;
    if(expression<0||expression>=g->expr_count)return 0;
    e=&g->expr[expression];
    return e->kind==E_VAR&&e->name[0]&&strlen(e->name)<64&&
        !literal_name(e->name)&&strcmp(e->name,"self")&&strcmp(e->name,"level");
}
static int event_payload_preflight(const GscProgram *g,char *error,size_t size) {
    int n,k,wait,argc;const int *args;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind==I_WAITER||i->kind==I_NOTIFY) {
            if(!g->event_payload_syntax)continue;
            wait=i->kind==I_WAITER;argc=i->argc;args=i->args;
        } else if(i->kind==I_METHOD&&!i->module[0]&&
                  entity_event_descriptor(g,i->name)>=0&&
                  find_func(g,i->owner_module,i->name)<0) {
            if(!g->event_payload_mode)continue; /* old arity policy is unchanged */
            if(entity_event_descriptor(g,i->name)==ENTITY_EVENT_ENDON&&i->argc!=1) {
                if(error&&size)snprintf(error,size,"line %d: endon requires exactly one event name",i->line);
                return 0;
            }
            if(entity_event_descriptor(g,i->name)==ENTITY_EVENT_WAITTILLMATCH) {
                if(i->argc<2||i->argc>MAX_PARAMS) {
                    if(error&&size)snprintf(error,size,"line %d: waittillmatch requires event and 1..15 filters",i->line);
                    return 0;
                }
                for(k=1;k<i->argc;k++) {
                    int type=known_value_kind(g,i->args[k]);
                    if(type>=0&&type!=V_UNDEFINED&&type!=V_NUMBER&&type!=V_STRING&&type!=V_VECTOR&&type!=V_ENTITY) {
                        if(error&&size)snprintf(error,size,"line %d: unsupported waittillmatch filter type",i->line);
                        return 0;
                    }
                }
            }
            wait=entity_event_descriptor(g,i->name)==ENTITY_EVENT_WAITTILL;
            argc=i->argc;args=i->args;
        } else continue;
        if(argc<1||argc>MAX_PARAMS||(!g->event_payload_mode&&argc!=1)) {
            if(error&&size)snprintf(error,size,"line %d: event payloads require explicit mode and 1..16 arguments",i->line);
            return 0;
        }
        if(g->event_payload_mode) {
            int kind=known_value_kind(g,args[0]);
            if((kind>=0&&kind!=V_STRING)||
               (kind==V_STRING&&!g->expr[args[0]].name[0])) {
                if(error&&size)snprintf(error,size,"line %d: event requires a nonempty string",i->line);
                return 0;
            }
            if(!wait)for(k=1;k<argc;k++) {
                const Expr *e=&g->expr[args[k]];
                if(e->kind==E_VAR&&!strcmp(e->name,"level")) {
                    if(error&&size)snprintf(error,size,"line %d: whole level event payload is unsupported",i->line);
                    return 0;
                }
            }
        }
        if(wait)for(k=1;k<argc;k++)if(!payload_output_valid(g,args[k])) {
            if(error&&size)snprintf(error,size,"line %d: waittill output must be a simple nonreserved local name",i->line);
            return 0;
        }
    }
    return 1;
}
