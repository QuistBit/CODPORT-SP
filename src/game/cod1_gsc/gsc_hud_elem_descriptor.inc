/* R200 HUD objects have a separate receiver namespace and service. */
#define HUDELEM_TARGET_BASE 12000
static int hud_elem_global(const GscProgram *g,const char *name){
    return g->hud_elements_selected&&callable_name_equal(g,name,"newHudElem");
}
static int hud_elem_method(const GscProgram *g,const char *name){
    if(!g->hud_elements_selected)return 0;
    return callable_name_equal(g,name,"setClock")?GSC_HUDELEM_SET_CLOCK:
        callable_name_equal(g,name,"destroy")?GSC_HUDELEM_DESTROY:
        callable_name_equal(g,name,"setShader")?GSC_HUDELEM_SET_SHADER:
        callable_name_equal(g,name,"setText")?GSC_HUDELEM_SET_TEXT:
        callable_name_equal(g,name,"fadeOverTime")?GSC_HUDELEM_FADE:0;
}
static int hud_elem_arity(int method){return method==GSC_HUDELEM_SET_CLOCK?5:method==GSC_HUDELEM_DESTROY?0:method==GSC_HUDELEM_SET_SHADER?3:(method==GSC_HUDELEM_SET_TEXT||method==GSC_HUDELEM_FADE)?1:-1;}
static int hud_elem_field(const char *name){
    return !strcmp(name,"x")?GSC_HUDELEM_FIELD_X:!strcmp(name,"y")?GSC_HUDELEM_FIELD_Y:
        !strcmp(name,"alpha")?GSC_HUDELEM_FIELD_ALPHA:!strcmp(name,"fontScale")?GSC_HUDELEM_FIELD_FONT_SCALE:!strcmp(name,"sort")?GSC_HUDELEM_FIELD_SORT:
        !strcmp(name,"alignX")?GSC_HUDELEM_FIELD_ALIGN_X:!strcmp(name,"alignY")?GSC_HUDELEM_FIELD_ALIGN_Y:0;
}
