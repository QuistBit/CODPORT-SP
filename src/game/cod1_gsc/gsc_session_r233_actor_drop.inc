
int gsc_session_select_r233_actor_drop(GscSession*s,const GscActorDropHost*input,char*error,size_t cap){GscActorDropHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;if(!s||!input)return session_reject(error,cap,"DropWeapon requires real Session host");h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||s->actor_drop_enabled||!h.invoke)return session_leave_rejected(s,"DropWeapon requires exclusive idle selected turret resources",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"DropWeapon selection expired",error,cap);s->actor_drop=h;s->actor_drop_enabled=1;s->busy=0;return 1;
}
typedef struct {SessionTurretAnimationLease base;GscActorDropHost host;GscActorDropRequest*input,copy;} SessionActorDropLease;
static int session_actor_drop_scope(const SessionActorDropLease*l){return session_turret_animation_scope(&l->base)&&l->base.session->actor_drop_enabled&&!memcmp(&l->base.session->actor_drop,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));}
static int session_actor_drop_current(void*opaque){SessionActorDropLease*l=opaque;return session_actor_drop_scope(l)&&session_animation_resources_current(l->base.session)&&session_actor_drop_scope(l)&&session_turret_animation_current(&l->base)&&session_actor_drop_scope(l);}
static int session_actor_drop_invoke(void*user,const GscActorDropRequest*input,char*error,size_t cap,int(*current)(void*),void*context){GscSession*s=user;SessionActorDropLease l;GscActorDropRequest q;int ok;
 if(!s||!input||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 memset(&l,0,sizeof(l));q=*input;l.input=&q;l.copy=q;session_turret_animation_lease(s,&l.base,current,context);l.host=s->actor_drop;
 if(q.runtime_identity!=l.base.instance||q.now_ms!=l.base.now||!session_actor_drop_current(&l))return 0;ok=l.host.invoke(l.host.user,&q,error,cap,session_actor_drop_current,&l);
 return ok==1&&session_actor_drop_current(&l)&&!memcmp(input,&l.copy,sizeof(l.copy));
}
