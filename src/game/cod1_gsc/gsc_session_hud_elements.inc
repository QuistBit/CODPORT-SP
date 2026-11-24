/* R200: independent copied HUD and brush collision services. Retirement is
 * deliberately independent of a running Session/player lease. */
int gsc_session_select_startup_hud(GscSession *s,const GscHudElemHost *h,char *err,size_t cap){
 if(!s||!h||!h->invoke||!h->validate||!h->retire||s->busy||s->program||s->runtime||
    s->view.error[0]||s->abort_requested||!s->execution_extensions||s->hud_elements_enabled)
  return session_reject(err,cap,"startup HUD requires idle selected execution and typed service");
 s->hud_elements=*h;s->hud_elements_enabled=1;return 1;
}
int gsc_session_select_r200_hud_elements(GscSession *s,const GscHudElemHost *h,char *err,size_t cap){
 if(!s||!h||!h->invoke||!h->validate||!h->retire||s->busy||s->program||s->runtime||
    !s->node_queries_enabled||!s->execution_extensions||s->hud_elements_enabled)
  return session_reject(err,cap,"HUD elements require idle R199 node execution and a typed service");
 s->hud_elements=*h;s->hud_elements_enabled=1;return 1;
}
int gsc_session_select_r200_entity_collision(GscSession *s,const GscEntityCollisionHost *h,char *err,size_t cap){
 if(!s||!h||!h->notsolid||s->busy||s->program||s->runtime||!s->hud_elements_enabled||s->entity_collision_enabled)
  return session_reject(err,cap,"collision service requires idle R200 HUD profile");
 s->entity_collision=*h;s->entity_collision_enabled=1;return 1;
}
static int session_hud_element_invoke(void *user,const GscHudElemRequest *request,GscHudElemResult *out,
 char *err,size_t cap,int (*current)(void *),void *context){
 GscSession *s=user;GscHudElemRequest copy,expected;GscHudElemResult result;SessionPlayerLease lease;int ok;
 if(!s||!request||!out||!s->hud_elements_enabled)return session_reject(err,cap,"HUD invocation outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(err,cap,"HUD Session lease expired");
 copy=*request;memcpy(&expected,&copy,sizeof(copy));memset(&result,0,sizeof(result));result.kind=-1;result.alive=-1;
 ok=s->hud_elements.invoke(s->hud_elements.user,&copy,&result,err,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(memcmp(&copy,&expected,sizeof(copy)))return session_reject(err,cap,"HUD host mutated its borrowed request");
 *out=result;return 1;
}
static int session_hud_element_validate(void *user,const GscHudElemRef *ref,int now,int *alive,
 char *err,size_t cap,int (*current)(void *),void *context){
 GscSession *s=user;GscHudElemRef copy,expected;SessionPlayerLease lease;int result=-1,ok;
 if(!s||!ref||!alive||!s->hud_elements_enabled)return session_reject(err,cap,"HUD validation outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(err,cap,"HUD Session lease expired");
 copy=*ref;expected=copy;
 ok=s->hud_elements.validate(s->hud_elements.user,&copy,now,&result,err,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(copy.epoch!=expected.epoch||copy.owner!=expected.owner||copy.index!=expected.index||copy.generation!=expected.generation||
    (result!=0&&result!=1))return session_reject(err,cap,"HUD validator mutated identity or returned invalid boolean");
 *alive=result;return 1;
}
static int session_hud_element_retire(void *user,uint64_t owner,char *err,size_t cap){
 GscSession *s=user;
 if(!s||!owner||!s->hud_elements_enabled||!s->hud_elements.retire)return session_reject(err,cap,"HUD retirement service unavailable");
 return s->hud_elements.retire(s->hud_elements.user,owner,err,cap)==1;
}
static int session_entity_world_action(GscEntityWorldAction action,void *user,const GscEntityRef *ref,int now,char *err,size_t cap,
 int (*current)(void *),void *context){
 GscSession *s=user;GscEntityRef copy,expected;SessionPlayerLease lease;int ok;
 if(!s||!ref||!action||!s->entity_collision_enabled)return session_reject(err,cap,"collision service outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(err,cap,"collision Session lease expired");
 copy=*ref;expected=copy;
 ok=action(s->entity_collision.user,&copy,now,err,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(copy.epoch!=expected.epoch||copy.slot!=expected.slot||copy.generation!=expected.generation)
  return session_reject(err,cap,"collision host mutated its borrowed identity");
 return 1;
}

static int session_entity_notsolid(void *user,const GscEntityRef *ref,int now,char *err,size_t cap,int (*current)(void *),void *ctx){
 GscSession *s=user;
 return s && session_entity_world_action(s->entity_collision.notsolid,user,ref,now,err,cap,current,ctx);
}
static int session_entity_solid(void *user,const GscEntityRef *ref,int now,char *err,size_t cap,int (*current)(void *),void *ctx){
 GscSession *s=user;
 return s && session_entity_world_action(s->entity_collision.solid,user,ref,now,err,cap,current,ctx);
}
static int session_entity_grenade_touch(void *user,const GscEntityRef *ref,int now,char *err,size_t cap,int (*current)(void *),void *ctx){
 GscSession *s=user;
 return s && session_entity_world_action(s->entity_collision.grenade_touch,user,ref,now,err,cap,current,ctx);
}
static int session_entity_grenade_bounce(void *user,const GscEntityRef *ref,int now,char *err,size_t cap,int (*current)(void *),void *ctx){
 GscSession *s=user;
 return s && session_entity_world_action(s->entity_collision.grenade_bounce,user,ref,now,err,cap,current,ctx);
}
static int session_entity_connectpaths(void *user,const GscEntityRef *ref,int now,char *err,size_t cap,int (*current)(void *),void *ctx){
 GscSession *s=user;
 return s && session_entity_world_action(s->connectpaths,user,ref,now,err,cap,current,ctx);
}
int gsc_session_select_connectpaths(GscSession *s,GscEntityWorldAction action){
 if(!s||!action||s->busy||s->program||s->runtime||!s->entity_collision_enabled||s->connectpaths)return 0;
 s->connectpaths=action;return 1;
}

static int session_entity_disconnectpaths(void*user,const GscEntityRef*ref,int now,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;return s&&session_entity_world_action(s->disconnectpaths,user,ref,now,error,cap,current,ctx);
}
int gsc_session_select_disconnectpaths(GscSession*s,GscEntityWorldAction action){
 if(!s||!action||s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->entity_collision_enabled||!s->connectpaths||s->disconnectpaths)return 0;
 s->disconnectpaths=action;return 1;
}
int gsc_session_running_path_graph(const GscSession *s,uint64_t id,uint64_t receipt){
 return s && s->connectpaths && id && receipt && s->busy && !s->abort_requested &&
    s->view.state==GSC_SESSION_RUNNING && !s->view.error[0] && s->view.session_id==id &&
    s->node_graph_id==id && s->node_graph_receipt==receipt && !s->node_graph_pin && s->node_graph_retire;
}
