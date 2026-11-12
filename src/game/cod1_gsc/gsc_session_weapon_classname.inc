int gsc_session_select_weapon_classname(GscSession*s,const GscWeaponClassnameHost*input,char*error,size_t cap){
 GscWeaponClassnameHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"weapon classname selection requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->weapon_classname_enabled||!s->names_enabled||!s->scalar_switch_enabled||!h.query)
  return session_leave_rejected(s,"weapon classname requires idle named scalar Session",error,cap);
 if(memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"weapon classname provider changed",error,cap);
 s->weapon_classname=h;s->weapon_classname_enabled=1;s->busy=0;return 1;
}
static int session_weapon_classname(void*user,const char*input,int now,GscWeaponClassnameResult*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscWeaponClassnameHost host;GscWeaponClassnameResult result;char name[128],expected[128];int ok;
 if(!s||!input||!out||!current||!s->weapon_classname_enabled||!memchr(input,0,128)||
    session_animation_namespace_private(s,input,128)||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,128,out,sizeof(*out))||session_animation_namespace_overlap(input,128,error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->weapon_classname;
 if(!session_player_current(&lease)||now!=s->view.logical_ms)return 0;
 memcpy(name,input,128);memcpy(expected,name,128);memset(&result,0xff,sizeof(result));
 ok=host.query(host.user,name,now,&result,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease)||now!=s->view.logical_ms||memcmp(&host,&s->weapon_classname,sizeof(host))||memcmp(name,expected,128)||memcmp(input,expected,128))return 0;
 *out=result;return 1;
}
