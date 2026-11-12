/* One genuine resource dependency, explicitly selected before strict link. */
#define START_ORIGIN_STATEMENT_TARGET 13002
static int start_origin_descriptor(const GscProgram *g,const char *name) {
    return (g->start_origin_selected||g->namespace_start_origin_selected)&&callable_name_equal(g,name,"getstartorigin");
}
static int start_origin_guy(const char *name) {
    const char *expected="training_guy_practicejump";
    while(*expected) {
        unsigned char c=(unsigned char)*name++;
        if(c>='A'&&c<='Z')c+=32;
        if(c!=(unsigned char)*expected++)return 0;
    }
    return !*name;
}
static int start_origin_gear(const char *name){const char *expected="training_gear_practicejump";while(*expected){unsigned char c=(unsigned char)*name++;if(c>='A'&&c<='Z')c+=32;if(c!=(unsigned char)*expected++)return 0;}return !*name;}
static unsigned start_origin_static_issues(const GscProgram *g,const int *args,int argc) {
    int i,type;
    if(argc!=3)return GSC_LINK_ISSUE_BUILTIN_ARITY;
    for(i=0;args&&i<3;++i) {
        const Expr *e;
        if(args[i]<0||args[i]>=g->expr_count)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
        e=&g->expr[args[i]];type=known_value_kind(g,args[i]);
        if(i<2) {
            if(type>=0&&type!=V_VECTOR)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
            if(e->kind==E_ANIM_REF||(g->namespace_start_origin_selected&&e->kind==E_ANIMTREE_VALUE))return GSC_LINK_ISSUE_BUILTIN_LITERAL;
        } else {
            if(type>=0)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
            if(g->namespace_start_origin_selected){
                if(e->kind==E_ANIMTREE_VALUE||e->kind==E_CAST||
                   (e->kind==E_ANIM_REF&&!start_origin_guy(e->name)&&!start_origin_gear(e->name)))return GSC_LINK_ISSUE_BUILTIN_LITERAL;
            } else if(e->kind==E_ANIM_REF&&!start_origin_guy(e->name))return GSC_LINK_ISSUE_BUILTIN_LITERAL;
        }
    }
    return 0;
}
