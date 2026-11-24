enum {SPATIAL_METHOD_ISTOUCHING=401,SPATIAL_METHOD_GETEYE,SPATIAL_METHOD_STANDING_NODE};
static int spatial_method_kind(int descriptor) {
    return descriptor==SPATIAL_METHOD_ISTOUCHING?GSC_SPATIAL_IS_TOUCHING:
        descriptor==SPATIAL_METHOD_GETEYE?GSC_SPATIAL_GET_EYE:descriptor==SPATIAL_METHOD_STANDING_NODE?GSC_SPATIAL_STANDING_NODE:0;
}
static int spatial_method_descriptor(const GscProgram *g,const char *name) {
    if(!g->spatial_natives_selected)return -1;
    if(callable_name_equal(g,name,"istouching"))return SPATIAL_METHOD_ISTOUCHING;
    if(callable_name_equal(g,name,"geteye"))return SPATIAL_METHOD_GETEYE;
    if(g->node_queries_selected && callable_name_equal(g,name,"getstandingnode"))return SPATIAL_METHOD_STANDING_NODE;
    return -1;
}
