/* Explicit R198 player methods. Native IDs are private to this adapter. */
#define ENTITY_METHOD_PLAYER_BASE 100
static int player_method_kind(int descriptor) {
    int kind=descriptor-ENTITY_METHOD_PLAYER_BASE;
    return kind>=GSC_PLAYER_GET_CURRENT_WEAPON&&kind<=GSC_PLAYER_GIVE_MAX_AMMO?kind:0;
}
static int player_method_descriptor(const GscProgram *g,const char *name) {
    static const char *const names[]={"getcurrentweapon","hasweapon","giveweapon",
        "takeweapon","takeallweapons","switchtoweapon","setweaponammoclip",
        "setweaponammostock","attackbuttonpressed","usebuttonpressed",
        "reloadbuttonpressed","meleebuttonpressed","freezecontrols"};
    int i;
    if(!g->player_natives_selected)return -1;
    if(g->player_max_ammo_selected&&callable_name_equal(g,name,"givemaxammo"))return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_GIVE_MAX_AMMO;
    if(g->player_viewmodel_selected&&callable_name_equal(g,name,"setviewmodel"))return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_SET_VIEWMODEL;
    if(g->player_normal_health_selected) {
        if(callable_name_equal(g,name,"setnormalhealth"))return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_SET_NORMAL_HEALTH;
        if(callable_name_equal(g,name,"getnormalhealth"))return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_GET_NORMAL_HEALTH;
    }
    if(g->training_opening_selected&&callable_name_equal(g,name,"getstance"))return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_GET_STANCE;
    if((g->grenade_items_selected||g->carbine_items_selected)&&callable_name_equal(g,name,"getfractionmaxammo"))
        return ENTITY_METHOD_PLAYER_BASE+GSC_PLAYER_GET_FRACTION_MAX_AMMO;
    for(i=0;i<(int)(sizeof(names)/sizeof(names[0]));i++)
        if(callable_name_equal(g,name,names[i]))return ENTITY_METHOD_PLAYER_BASE+i+1;
    return -1;
}
static int player_method_arity(int descriptor) {
    int kind=player_method_kind(descriptor);
    if(kind==GSC_PLAYER_SET_NORMAL_HEALTH||kind==GSC_PLAYER_SET_VIEWMODEL||kind==GSC_PLAYER_GIVE_MAX_AMMO)return 1;
    if(kind==GSC_PLAYER_SET_AMMO_CLIP||kind==GSC_PLAYER_SET_AMMO_STOCK)return 2;
    if(kind==GSC_PLAYER_HAS_WEAPON||kind==GSC_PLAYER_GIVE_WEAPON||
       kind==GSC_PLAYER_TAKE_WEAPON||kind==GSC_PLAYER_SWITCH_TO_WEAPON||
       kind==GSC_PLAYER_FREEZE_CONTROLS||kind==GSC_PLAYER_GET_FRACTION_MAX_AMMO)return 1;
    return 0;
}
static int player_method_result(int descriptor) {
    int kind=player_method_kind(descriptor);
    if(kind==GSC_PLAYER_GET_CURRENT_WEAPON||kind==GSC_PLAYER_GET_STANCE)return GSC_PLAYER_RESULT_STRING;
    if(kind==GSC_PLAYER_GET_FRACTION_MAX_AMMO||kind==GSC_PLAYER_GET_NORMAL_HEALTH)return GSC_PLAYER_RESULT_FLOAT;
    if(kind==GSC_PLAYER_HAS_WEAPON||
       (kind>=GSC_PLAYER_ATTACK_PRESSED&&kind<=GSC_PLAYER_MELEE_PRESSED))return GSC_PLAYER_RESULT_INT;
    return GSC_PLAYER_RESULT_VOID;
}
