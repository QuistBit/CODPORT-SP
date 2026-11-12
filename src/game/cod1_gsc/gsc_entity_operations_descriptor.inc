enum { ENTITY_OP_GETORIGIN=201,ENTITY_OP_GETANGLES,ENTITY_OP_SETANGLES,ENTITY_OP_DELETE };
static int entity_operation(int descriptor){return descriptor>=ENTITY_OP_GETORIGIN&&descriptor<=ENTITY_OP_DELETE;}
static int entity_operation_descriptor(const GscProgram *g,const char *name){
    if(!g->entity_operations_selected)return -1;
    if(callable_name_equal(g,name,"getorigin"))return ENTITY_OP_GETORIGIN;
    if(callable_name_equal(g,name,"getangles"))return ENTITY_OP_GETANGLES;
    if(callable_name_equal(g,name,"setangles"))return ENTITY_OP_SETANGLES;
    if(callable_name_equal(g,name,"delete"))return ENTITY_OP_DELETE;
    return -1;
}
