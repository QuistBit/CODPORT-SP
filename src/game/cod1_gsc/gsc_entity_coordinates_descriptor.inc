/* Private IDs are separate from model/player/entity-operation namespaces. */
enum { ENTITY_COORD_LOCAL_TO_WORLD=301,ENTITY_COORD_WORLD_TO_LOCAL=302 };
static int entity_coordinate(int descriptor) {
    return descriptor==ENTITY_COORD_LOCAL_TO_WORLD||descriptor==ENTITY_COORD_WORLD_TO_LOCAL;
}
static int entity_coordinate_descriptor(const GscProgram *g,const char *name) {
    if(!g->entity_coordinates_selected)return -1;
    if(callable_name_equal(g,name,"localtoworldcoords"))return ENTITY_COORD_LOCAL_TO_WORLD;
    if(callable_name_equal(g,name,"worldtolocalcoords"))return ENTITY_COORD_WORLD_TO_LOCAL;
    return -1;
}
