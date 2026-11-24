/* One native-field provider for a composed map. Item.count has its actual
 * item allocation/lifecycle owner; Actor and other native fields retain the
 * Actor bootstrap facade. The legacy mutually exclusive selectors stay so. */
int gsc_session_select_map_items(GscSession*s,const GscEntityFieldHost*input,char*error,size_t cap){
 GscEntityFieldHost copy;
 if(!s||!input||session_animation_namespace_private(s,input,sizeof(*input)))return 0;
 if(!s->map_actor_program_enabled)return gsc_session_select_r216_items(s,input,error,cap);
 if(session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 if(!copy.classify||!copy.read||!copy.write||s->busy||s->program||s->runtime||s->view.state!=GSC_SESSION_EMPTY||
    s->view.error[0]||s->abort_requested||s->grenade_items_enabled||s->composed_map_items_enabled||s->carbine_items_enabled||
    !s->actor_program_enabled||s->actor_bootstrap_enabled||!s->root_map_enabled||
    !s->execution_extensions||!s->state_enabled||!s->scalar_switch_enabled||!s->names_enabled)
  return session_reject(error,cap,"composed map items require idle same-map Actor plan");
 if(!session_enter(s,error,cap))return 0;
 if(!session_animation_resources_current(s)||!session_actor_program_current(s)||!session_animation_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
  return session_failed(s,"composed item/Actor map retired during selection",error,cap);
 s->grenade_item_fields=copy;s->grenade_items_enabled=1;s->composed_map_items_enabled=1;s->busy=0;return 1;
}
typedef struct {uint64_t id;int now;GscRuntime*runtime;GscEntityFieldHost item;GscMapExecutionCertificate map;} SessionMapItemActorLease;
static SessionMapItemActorLease session_map_item_actor_lease(GscSession*s){
 SessionMapItemActorLease l;l.id=s->view.session_id;l.now=s->view.logical_ms;l.runtime=s->runtime;l.item=s->grenade_item_fields;l.map=s->root_map;return l;
}
static int session_map_item_actor_current(GscSession*s,const SessionMapItemActorLease*l){
 return s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->composed_map_items_enabled&&s->grenade_items_enabled&&
  s->map_actor_program_enabled&&s->actor_program_enabled&&s->actor_bootstrap_enabled&&s->root_map_enabled&&
  l->id&&s->view.session_id==l->id&&s->view.logical_ms==l->now&&s->runtime==l->runtime&&l->runtime&&
  !memcmp(&s->grenade_item_fields,&l->item,sizeof(l->item))&&!memcmp(&s->root_map,&l->map,sizeof(l->map))&&
  (s->view.state==GSC_SESSION_RUNNING||(s->view.state==GSC_SESSION_PREPARED&&s->actor_construct_running&&s->actor_bootstrap_running));
}
static int session_map_item_count(const char*n){
 static const char name[]="count";unsigned i;if(!n)return 0;
 for(i=0;i<5;i++){unsigned char c=n[i];if(c>='A'&&c<='Z')c+=32;if(c!=name[i])return 0;}return n[5]==0;
}
static int session_map_item_actor_classify(void*u,const GscEntityRef*e,const char*n,char*error,size_t cap){
 GscSession*s=u;SessionMapItemActorLease l;GscEntityRef ref;char field[128];int result;if(!s||!e||!n)return -1;l=session_map_item_actor_lease(s);
 if(!session_map_item_actor_current(s,&l)||!session_actor_name(n,field))return -1;ref=*e;
 result=session_map_item_count(field)?session_grenade_item_classify(s,&ref,"count",error,cap):session_actor_classify(s,&ref,field,error,cap);
 return session_map_item_actor_current(s,&l)&&!memcmp(e,&ref,sizeof(ref))&&!memcmp(n,field,strlen(field)+1)?result:-1;
}
static int session_map_item_actor_read(void*u,const GscEntityRef*e,const char*n,GscStringCastValue*out,char*error,size_t cap){
 GscSession*s=u;SessionMapItemActorLease l;GscEntityRef ref;char field[128];GscStringCastValue result;int ok;if(!s||!e||!n||!out)return 0;l=session_map_item_actor_lease(s);
 if(!session_map_item_actor_current(s,&l)||!session_actor_name(n,field))return 0;ref=*e;memset(&result,0,sizeof(result));
 ok=session_map_item_count(field)?session_grenade_item_read(s,&ref,"count",&result,error,cap):session_actor_read(s,&ref,field,&result,error,cap);
 if(ok!=1||!session_map_item_actor_current(s,&l)||memcmp(e,&ref,sizeof(ref))||memcmp(n,field,strlen(field)+1))return 0;*out=result;return 1;
}
static int session_map_item_actor_write(void*u,const GscEntityRef*e,const char*n,const GscStringCastValue*v,char*error,size_t cap){
 GscSession*s=u;SessionMapItemActorLease l;GscEntityRef ref;char field[128];int ok;if(!s||!e||!n||!v)return 0;l=session_map_item_actor_lease(s);
 if(!session_map_item_actor_current(s,&l)||!session_actor_name(n,field))return 0;ref=*e;
 ok=session_map_item_count(field)?session_grenade_item_write(s,&ref,"count",v,error,cap):session_actor_write(s,&ref,field,v,error,cap);
 return ok==1&&session_map_item_actor_current(s,&l)&&!memcmp(e,&ref,sizeof(ref))&&!memcmp(n,field,strlen(field)+1);
}
