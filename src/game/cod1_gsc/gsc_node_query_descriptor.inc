/* R199 node queries use a separate value and host lane. No gentity fallback. */
#define NODE_QUERY_TARGET_BASE 11000
static int node_query_descriptor(const GscProgram *g,const char *name) {
    if(g->vehicle_nodes_selected){
        if(callable_name_equal(g,name,"getvehiclenode"))return GSC_VEHICLE_QUERY_ONE;
        if(callable_name_equal(g,name,"getvehiclenodearray"))return GSC_VEHICLE_QUERY_ARRAY;
    }
    if(!g->node_queries_selected)return 0;
    if(callable_name_equal(g,name,"getnode"))return GSC_NODE_QUERY_ONE;
    if(callable_name_equal(g,name,"getnodearray"))return GSC_NODE_QUERY_ARRAY;
    if(callable_name_equal(g,name,"getallnodes"))return GSC_NODE_QUERY_ALL;
    return 0;
}
static int node_query_arity(int kind) {
    return kind==GSC_NODE_QUERY_ALL?0:
        kind==GSC_NODE_QUERY_ONE||kind==GSC_NODE_QUERY_ARRAY||kind==GSC_VEHICLE_QUERY_ONE||kind==GSC_VEHICLE_QUERY_ARRAY?2:-1;
}
static unsigned node_query_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    int n,type;
    if(node_query_arity(kind)<0||argc!=node_query_arity(kind))return GSC_LINK_ISSUE_BUILTIN_ARITY;
    for(n=0;args&&n<argc;n++) {
        type=known_value_kind(g,args[n]);
        if(type>=0&&type!=V_STRING)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return 0;
}
static int node_field_descriptor(const char *name) {
    static const char *fields[]={"origin","angles","targetname","target","classname","animscript","spawnflags",
        "type","script_noteworthy","script_delay","script_fallback","radius"};
    int n;for(n=0;n<(int)(sizeof(fields)/sizeof(fields[0]));n++)if(!strcmp(fields[n],name))return n+1;
    return 0;
}
