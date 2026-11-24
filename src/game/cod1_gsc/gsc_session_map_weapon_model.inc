/* Real loaded-weapon data query owned by a map Session. No actor/ATR
 * bootstrap is manufactured for a weapon definition lookup. */
int gsc_session_select_map_weapon_model(GscSession*s,const GscWeaponModelHost*input,char*error,size_t cap){
 GscWeaponModelHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"map weapon model requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->weapon_model_enabled||!s->root_map_enabled||!s->names_enabled||!s->scalar_switch_enabled||!h.query)
  return session_leave_rejected(s,"map weapon model requires idle named map Session",error,cap);
 s->weapon_model=h;s->weapon_model_enabled=1;s->map_weapon_model_enabled=1;s->busy=0;return 1;
}
static int session_map_weapon_model_query(void*user,const char*input,char output[128],char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscWeaponModelHost host;char name[128],expected[128],model[128];GscRuntime*runtime;int now,ok;
 if(!s||!input||!output||!current||(cap&&!error)||!s->map_weapon_model_enabled||!s->weapon_model_enabled||!memchr(input,0,128)||
    session_animation_namespace_private(s,input,128)||session_animation_namespace_private(s,output,128)||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,128,output,128)||session_animation_namespace_overlap(input,128,error,cap)||session_animation_namespace_overlap(output,128,error,cap))return 0;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->weapon_model;runtime=s->runtime;now=s->view.logical_ms;
 if(!session_player_current(&lease))return 0;
 memcpy(name,input,128);memcpy(expected,name,128);memset(model,0xff,sizeof(model));
 ok=host.query(host.user,name,model,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease)||s->runtime!=runtime||s->view.logical_ms!=now||memcmp(&host,&s->weapon_model,sizeof(host))||memcmp(name,expected,128)||memcmp(input,expected,128)||!memchr(model,0,128))return 0;
 memcpy(output,model,128);return 1;
}
