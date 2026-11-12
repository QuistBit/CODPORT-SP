enum { ENTITY_LINK_METHOD=13001,ENTITY_UNLINK_METHOD=13002 };
static int entity_link_method(int d){return d==ENTITY_LINK_METHOD||d==ENTITY_UNLINK_METHOD;}
static int entity_link_descriptor(const GscProgram*g,const char*n){
 if(!g->entity_links_selected)return -1;
 return callable_name_equal(g,n,"linkto")?ENTITY_LINK_METHOD:callable_name_equal(g,n,"unlink")?ENTITY_UNLINK_METHOD:-1;
}
