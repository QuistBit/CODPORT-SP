/* R149 selected entity number and named-team spawner queries. */
static int known_value_kind(const GscProgram *,int);
static int entity_query_descriptor(const GscProgram *g,const char *name) {
    if(!g||!g->entity_queries_mode)return -1;
    if(callable_name_equal(g,name,"getentbynum"))return GSC_ENTITY_QUERY_BY_NUMBER;
    if(callable_name_equal(g,name,"getspawnerteamarray"))return GSC_ENTITY_QUERY_SPAWNER_TEAM;
    return -1;
}
static unsigned entity_query_static_issues(const GscProgram *g,int kind,
    const int *args,int argc) {
    int index,type;
    if((kind!=GSC_ENTITY_QUERY_BY_NUMBER&&kind!=GSC_ENTITY_QUERY_SPAWNER_TEAM)||
       argc<0||argc>MAX_PARAMS)return GSC_LINK_ISSUE_BUILTIN_ARITY;
    /* Number lookup admits exact int32 values. Unproved float/string GetInt
     * coercion remains outside this selector; dynamic values are checked at use.
     * Team getters and missing parameters report errors only when executed. */
    if(kind==GSC_ENTITY_QUERY_BY_NUMBER&&argc&&args) {
        index=args[0];type=known_value_kind(g,index);
        if((type>=0&&type!=V_NUMBER)||
           (index>=0&&index<g->expr_count&&g->expr[index].kind==E_NUMBER&&
            !g->expr[index].integer))return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return 0;
}
