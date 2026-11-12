/* R172 narrow runtime-owned game cell. Original a6cfcc stores a mutable
 * variable-cell ID, whose initial Value is an array object. Native lifecycle
 * can retain/prune that cell across changelevel. This selected port capability
 * owns an ARRAY-only root for one runtime, with no native persistence claim. */
static int game_array_selected(const GscRuntime *r,const char *name) {
    return r&&r->game_array_enabled&&name&&!strcmp(name,"game");
}
/* object_token is unused by ordinary array Values in older profiles. Selected
 * array creation stamps the existing private runtime cookie so replacement
 * cannot adopt a foreign/forged numeric handle through this internal boundary.
 * Public input schemas do not expose a raw array Value in the first place. */
static int game_array_value_valid(const GscRuntime *r,const Value *value) {
    return r&&value&&value->kind==V_ARRAY&&r->array_token&&
        value->object_token==r->array_token&&array_value_valid(r,value->array);
}
static int game_array_read(GscRuntime *r,int line,Value *out) {
    if(!r->game_array_enabled||!game_array_value_valid(r,&r->game_array_root))
        return rfail(r,line,"selected game root is unavailable");
    *out=r->game_array_root;return 1;
}
static int game_array_root_matches(GscRuntime *r,const Value *captured,int line) {
    if(!game_array_value_valid(r,captured)||!game_array_value_valid(r,&r->game_array_root)||
       captured->array!=r->game_array_root.array)
        return rfail(r,line,"captured array root changed during expression");
    return 1;
}
static int game_array_selection_ready(const GscRuntime *r) {
    int n,k;
    if(!r||r->started||r->advancing||r->error[0]||r->game_array_enabled||
       !r->program||!r->program->linked||!r->state_enabled||!r->arrays_enabled||
       !r->scalar_switch_enabled||!r->names_enabled||!r->array_token||
       r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY)return 0;
    /* Existing formal/output binding machinery writes locals directly. Refuse
     * shadow declarations in every retained function before allocating a root. */
    for(n=0;n<r->program->func_count;n++) {
        const Func *f=&r->program->funcs[n];
        for(k=0;k<f->argc;k++)if(!strcmp(f->params[k],"game"))return 0;
    }
    for(n=0;n<r->program->code_count;n++) {
        const Instr *i=&r->program->code[n];
        if(i->kind!=I_WAITER&&!(i->kind==I_METHOD&&
           i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT&&
           i->receiver_event_target==ENTITY_EVENT_WAITTILL))continue;
        for(k=1;k<i->argc;k++)if(i->args[k]>=0&&i->args[k]<r->program->expr_count&&
           r->program->expr[i->args[k]].kind==E_VAR&&
           !strcmp(r->program->expr[i->args[k]].name,"game"))return 0;
    }
    return 1;
}
int gsc_runtime_select_game_array(GscRuntime *r) {
    Value root;
    if(!game_array_selection_ready(r))return 0;
    /* All refusal checks precede this non-callback allocation/publication. */
    if(!array_new(r,0,&root))return 0;
    root.object_token=r->array_token;r->game_array_root=root;r->game_array_enabled=1;
    return 1;
}
