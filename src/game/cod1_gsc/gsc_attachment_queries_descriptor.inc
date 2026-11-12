enum { ATTACH_QUERY_SIZE=601,ATTACH_QUERY_MODEL=602,ATTACH_QUERY_TAG=603 };
static int attachment_query_kind(int d){return d>=ATTACH_QUERY_SIZE&&d<=ATTACH_QUERY_TAG?d-ATTACH_QUERY_SIZE+1:0;}
static int attachment_query_descriptor(const GscProgram *g,const char *name){
 if(!g->attachment_queries_selected)return -1;
 if(callable_name_equal(g,name,"getattachsize"))return ATTACH_QUERY_SIZE;
 if(callable_name_equal(g,name,"getattachmodelname"))return ATTACH_QUERY_MODEL;
 if(callable_name_equal(g,name,"getattachtagname"))return ATTACH_QUERY_TAG;
 return -1;
}
