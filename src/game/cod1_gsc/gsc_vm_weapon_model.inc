/* Separate selected query: all arguments have already evaluated left-to-right. */
int gsc_program_select_weapon_model_query(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->weapon_model_selected||!g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->weapon_model_selected=1;return 1;
}
int gsc_runtime_select_weapon_model(GscRuntime *r,const GscWeaponModelHost *host) {
    if(!r||!host||r->started||r->advancing||r->error[0]||r->weapon_model_enabled||
       !r->program->weapon_model_selected||!r->names_enabled||!r->scalar_switch_enabled||!host->query||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->weapon_model=*host;r->weapon_model_enabled=1;return 1;
}
/* Synchronous scope only; providers must not retain this callback/context. */
static int weapon_model_context_current(void *user){GscRuntime *r=(GscRuntime *)user;return !r->error[0]&&console_context(r)==1;}
static int weapon_model_call(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    char name[128],original[128],model[128],error[384];int accepted,context;Value value;
    if(!r->weapon_model_enabled||!out||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"weapon model capability unavailable");
    if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str)))return rfail(r,line,"getWeaponModel GetString0 requires an admitted plain string");
    if(r->weapon_model_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"weapon model callback reentered");
    memcpy(name,args[0].str,sizeof(name));memcpy(original,name,sizeof(original));memset(model,0xff,sizeof(model));error[0]=0;r->weapon_model_busy=1;
    context=console_context(r);if(context<=0){r->weapon_model_busy=0;return 0;}
    accepted=r->weapon_model.query(r->weapon_model.user,name,model,error,sizeof(error),weapon_model_context_current,r);error[sizeof(error)-1]=0;
    r->weapon_model_busy=0;if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    context=console_context(r);if(context<=0)return 0;
    if(memcmp(name,original,sizeof(name)))return rfail(r,line,"weapon model host changed copied query name");
    if(accepted!=1)return rfail(r,line,"weapon model host refused: %s",error[0]?error:"unspecified host failure");
    if(!memchr(model,0,sizeof(model)))return rfail(r,line,"weapon model reply exceeds owned text capacity");
    memset(&value,0,sizeof(value));value.kind=V_STRING;memcpy(value.str,model,sizeof(value.str));*out=value;return 1;
}
