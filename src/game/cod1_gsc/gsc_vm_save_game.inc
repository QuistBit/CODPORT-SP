/* R123 original GAME 20035790: signed player health gate precedes mapname
 * registration and getters. Arg0 GetString, optional Arg1 GetIString ONLY,
 * optional Arg2 GetString; absent image registers ui_campaign. Extras ignored
 * after evaluation. Paths/default images truncate to 63 bytes. Selected finite
 * GetString formatting follows R121; native argument evaluation remains the
 * inherited LTR port policy. Publication means EXEC_APPEND request only. */
static int save_game_runtime_preflight(const GscProgram *g,const GscSaveGameHost *host,char *error,size_t size) {
    int n,kind,needed=0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-SAVE_GAME_TARGET_BASE)continue;
        if(g->tag_fx_mode&&i->target<-1-SAVE_GAME_TARGET_BASE-GSC_SAVE_GAME_REQUEST)continue;
        kind=-1-SAVE_GAME_TARGET_BASE-i->target;
        if(!g->save_game_mode||i->module[0]||save_game_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||save_game_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid saveGame binding",i->line);
            return 0;
        }
        needed=1;
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_SAVE_GAME_NATIVE)continue;
        if(!g->save_game_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           save_game_descriptor(g,e->name)!=e->target||find_func(g,e->owner_module,e->name)>=0||
           save_game_static_issues(g,e->target,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid saveGame expression binding",e->line);
            return 0;
        }
        needed=1;
    }
    if(needed&&(!host||!host->query||!host->publish)) {
        if(error&&size)snprintf(error,size,"saveGame requires selected query and request host capabilities");
        return 0;
    }
    return 1;
}
static int save_game_query(GscRuntime *r,int kind,uint64_t operation,int line,GscSaveGameQuery *out) {
    GscSaveGameQuery copy;char error[256];int accepted,context;
    if(!r->save_game.query)return rfail(r,line,"saveGame query host capability is unavailable");
    memset(&copy,0,sizeof(copy));copy.kind=kind;copy.operation_id=operation;error[0]=0;
    accepted=r->save_game.query(r->save_game.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"saveGame host rejected query: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    if(copy.kind!=kind||copy.operation_id!=operation||
       (kind!=GSC_SAVE_GAME_PLAYER_HEALTH&&!memchr(copy.text,0,sizeof(copy.text))))
        return rfail(r,line,"saveGame host returned invalid query payload");
    *out=copy;return 1;
}
static int save_game_get_string(GscRuntime *r,const Value *value,int index,int line,char *out,size_t size) {
    int length,n;int32_t integer;size_t bytes;
    if(value->kind==V_STRING) {
        if(!memchr(value->str,0,sizeof(value->str)))
            return rfail(r,line,"saveGame GetString argument %d exceeds selected string domain",index);
        bytes=strlen(value->str);if(bytes>=size)return rfail(r,line,"saveGame GetString argument %d exceeds selected string domain",index);
        memcpy(out,value->str,bytes+1);return 1;
    }
    if(value->kind==V_NUMBER) {
        if(value->integer) {
            if(!scalar_get_integer(r,value,line,&integer))return 0;
            length=snprintf(out,size,"%ld",(long)integer);
        } else {
            if(!isfinite(value->number))return rfail(r,line,"saveGame GetString argument %d is outside finite domain",index);
            length=snprintf(out,size,"%g",(double)value->number);
        }
    } else if(value->kind==V_VECTOR) {
        for(n=0;n<3;n++)if(!isfinite(value->vector[n]))
            return rfail(r,line,"saveGame GetString argument %d is outside finite domain",index);
        length=snprintf(out,size,"(%.2f, %.2f, %.2f)",
            (double)value->vector[0],(double)value->vector[1],(double)value->vector[2]);
    } else return rfail(r,line,"saveGame GetString argument %d cannot cast admitted value to string",index);
    if(length<0||(size_t)length>=size)
        return rfail(r,line,"saveGame GetString argument %d exceeds selected formatted string domain",index);
    return 1;
}
static int save_game_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscSaveGameEvent event,copy;GscSaveGameQuery health,map,campaign;
    char name[128],formatted[300],error[256];int accepted,context;
    uint64_t operation;
    if(!r->save_game_enabled||!r->names_enabled||r->error[0]||!out||
       save_game_static_issues(r->program,kind,NULL,argc))
        return rfail(r,line,"invalid saveGame structural invocation");
    if(!r->next_save_game_operation||r->next_save_game_operation==UINT64_MAX)
        return rfail(r,line,"saveGame operation token exhausted");
    operation=r->next_save_game_operation++;
    if(!save_game_query(r,GSC_SAVE_GAME_PLAYER_HEALTH,operation,line,&health))return 0;
    if(health.player_health<=0) {memset(out,0,sizeof(*out));return 1;}
    if(!save_game_query(r,GSC_SAVE_GAME_MAPNAME,operation,line,&map))return 0;
    if(argc<1||!args)return rfail(r,line,"saveGame GetString argument 0 is missing");
    memset(&event,0,sizeof(event));event.operation_id=operation;
    if(!save_game_get_string(r,&args[0],0,line,name,sizeof(name)))return 0;
    if(!name[0])strcpy(name,"auto");
    if(argc>1) {
        if(args[1].kind!=V_LOCALIZED_STRING||!memchr(args[1].str,0,sizeof(args[1].str)))
            return rfail(r,line,"saveGame GetIString argument 1 requires a localized string");
        memcpy(event.description,args[1].str,strlen(args[1].str)+1);
    }
    if(argc>2) {
        if(!save_game_get_string(r,&args[2],2,line,event.image,sizeof(event.image)))return 0;
    } else {
        if(!save_game_query(r,GSC_SAVE_GAME_CAMPAIGN,operation,line,&campaign))return 0;
        snprintf(formatted,sizeof(formatted),"levelshots/loadscreen/loading_%s.tga",campaign.text);
        str_copy(event.image,64,formatted);
    }
    if(callable_name_equal(r->program,name,"levelstart"))
        snprintf(formatted,sizeof(formatted),"autosave\\%s",map.text);
    else snprintf(formatted,sizeof(formatted),"autosave\\%s-%s",map.text,name);
    str_copy(event.filename,sizeof(event.filename),formatted);
    if(!save_game_event_valid(&event))return rfail(r,line,"saveGame request is outside selected payload domain");
    if(!r->save_game.publish)return rfail(r,line,"saveGame request host capability is unavailable");
    copy=event;error[0]=0;
    accepted=r->save_game.publish(r->save_game.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"saveGame host rejected request: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
