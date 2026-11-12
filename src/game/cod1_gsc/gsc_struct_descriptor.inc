/* R148 selected pure spawnstruct. The original DLL wrapper tail-calls its
 * CreateStruct service without an argc guard or argument getters. */
static int struct_descriptor(const GscProgram *g,const char *name) {
    return g&&g->struct_mode&&callable_name_equal(g,name,"spawnstruct")?
        GSC_STRUCT_SPAWN:-1;
}
static unsigned struct_static_issues(int kind,int argc) {
    return kind==GSC_STRUCT_SPAWN&&argc>=0&&argc<=MAX_PARAMS?
        0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
