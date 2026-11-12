/* Explicit real table query bridge; legacy Session profiles stay unselected. */
int gsc_session_select_r232_weapon_model(GscSession *s,const GscWeaponModelHost *input,char *error,size_t cap){
 GscWeaponModelHost copy;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"weapon model selection needs Session and real provider");
 copy=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->weapon_model_enabled||
    !s->turret_animations_enabled||!s->names_enabled||!s->scalar_switch_enabled||!copy.query)
  return session_leave_rejected(s,"weapon model query requires idle selected two-ATR Session and real provider",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
  return session_leave_rejected(s,"weapon model selection retired or changed",error,cap);
 s->weapon_model=copy;s->weapon_model_enabled=1;s->busy=0;return 1;
}
typedef struct {SessionTurretAnimationLease base;GscWeaponModelHost host;} SessionWeaponModelLease;
static int session_weapon_model_scope(const SessionWeaponModelLease *lease){
 GscSession *s=lease->base.session;
 return session_turret_animation_scope(&lease->base)&&s->weapon_model_enabled&&!memcmp(&s->weapon_model,&lease->host,sizeof(lease->host));
}
static int session_weapon_model_current(void *context){
 SessionWeaponModelLease *lease=context;
 return session_weapon_model_scope(lease)&&session_turret_animation_current(&lease->base)&&session_weapon_model_scope(lease);
}
static int session_weapon_model_query(void *user,const char *input,char output[128],char *error,size_t cap,int (*current)(void*),void *ctx){
 GscSession *s=user;SessionWeaponModelLease lease;char name[128],original[128],model[128];int ok;
 if(!s||!input||!output||!current||!memchr(input,0,128)||
    session_animation_namespace_private(s,input,128)||session_animation_namespace_private(s,output,128)||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,128,output,128)||
    session_animation_namespace_overlap(input,128,error,cap)||session_animation_namespace_overlap(output,128,error,cap))return 0;
 memcpy(name,input,128);memcpy(original,input,128);session_turret_animation_lease(s,&lease.base,current,ctx);lease.host=s->weapon_model;
 if(!session_weapon_model_current(&lease))return 0;
 memset(model,0xff,sizeof(model));
 ok=lease.host.query(lease.host.user,name,model,error,cap,session_weapon_model_current,&lease);
 if(ok!=1||!session_weapon_model_current(&lease)||memcmp(name,original,128)||memcmp(input,original,128)||!memchr(model,0,sizeof(model)))return 0;
 memcpy(output,model,sizeof(model));return 1;
}
