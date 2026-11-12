/* R202 getaiarray is an explicit native capability, never a classname scan. */
#define AI_ARRAY_STATEMENT_TARGET 13001
static int ai_array_descriptor(const GscProgram *g,const char *name) {
    return g->ai_arrays_selected&&callable_name_equal(g,name,"getaiarray");
}
static unsigned ai_array_static_issues(const GscProgram *g,const int *args,int argc) {
    int n,type;
    if(argc<0||argc>MAX_PARAMS)return GSC_LINK_ISSUE_BUILTIN_ARITY;
    for(n=0;args&&n<argc;n++) {
        type=known_value_kind(g,args[n]);
        if(type>=0&&type!=V_STRING)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return 0;
}
