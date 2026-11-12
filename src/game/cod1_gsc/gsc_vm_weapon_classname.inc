int gsc_program_select_weapon_classname(GscProgram*g){
 if(!g||g->invalid||g->linked||g->weapon_classname_selected||!g->names_syntax||!g->scalar_switch_syntax)return 0;
 g->weapon_classname_selected=1;return 1;
}
int gsc_runtime_select_weapon_classname(GscRuntime*r,const GscWeaponClassnameHost*h){
 if(!r||!h||!h->query||r->started||r->advancing||r->error[0]||r->weapon_classname_enabled||!r->program->weapon_classname_selected||
    !r->names_enabled||!r->scalar_switch_enabled||!r->results_enabled||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->weapon_classname=*h;r->weapon_classname_enabled=1;return 1;
}
static int weapon_classname_current(void*ctx){GscRuntime*r=ctx;return !r->error[0]&&console_context(r)==1;}
static int weapon_classname_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 char name[128],expected[128],error[384];GscWeaponClassnameResult result;Value value;int ok;
 if(!r->weapon_classname_enabled||!out||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"weapon classname capability unavailable");
 if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str)))return rfail(r,line,"getWeaponClassname GetString0 requires plain string");
 if(r->weapon_classname_busy||r->weapon_model_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"weapon classname callback reentered");
 memcpy(name,args[0].str,128);memcpy(expected,name,128);memset(&result,0xff,sizeof(result));error[0]=0;r->weapon_classname_busy=1;
 if(console_context(r)!=1){r->weapon_classname_busy=0;return 0;}
 ok=r->weapon_classname.query(r->weapon_classname.user,name,r->now_ms,&result,error,sizeof(error),weapon_classname_current,r);error[sizeof(error)-1]=0;
 r->weapon_classname_busy=0;
 if(r->error[0]||!entity_task_current(r)||console_context(r)!=1)return 0;
 if(memcmp(name,expected,128))return rfail(r,line,"weapon classname host changed borrowed input");
 if(ok!=1)return rfail(r,line,"weapon classname refused: %s",error[0]?error:"host failed");
 if((result.defined!=0&&result.defined!=1)||!memchr(result.classname,0,128)||
    (result.defined?!result.classname[0]:result.classname[0]!=0))return rfail(r,line,"weapon classname host returned invalid string/undefined reply");
 memset(&value,0,sizeof(value));if(result.defined){value.kind=V_STRING;memcpy(value.str,result.classname,128);}*out=value;return 1;
}
