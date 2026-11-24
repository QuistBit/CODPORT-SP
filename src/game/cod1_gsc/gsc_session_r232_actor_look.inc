int gsc_session_select_r232_actor_look(GscSession *s,const GscActorLookHost *input,char *error,size_t cap) {
    GscActorLookHost copy;
    if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
    if(!s||!input)return session_reject(error,cap,"actor animation selection requires Session and real host");copy=*input;
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||s->actor_look_enabled||!copy.validate||!copy.invoke)
        return session_leave_rejected(s,"actor animations require exclusive idle two-ATR configuration and real host",error,cap);
    if(!session_animation_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
        return session_leave_rejected(s,"actor animation selection expired",error,cap);
    s->actor_look=copy;s->actor_look_enabled=1;s->busy=0;return 1;
}
int gsc_session_select_r402_source_look(GscSession*s,const GscActorLookHost*input,char*error,size_t cap){
 GscActorLookHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!h.validate||!h.invoke||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->actor_look_enabled||s->source_actor_look_enabled)
  return session_leave_rejected(s,"source animation provider requires idle exclusive Actor configuration",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"source animation provider retired",error,cap);
 s->source_actor_look=h;s->source_actor_look_enabled=1;s->busy=0;return 1;
}

typedef struct {SessionTurretAnimationLease base;GscActorLookHost host;SessionSourceLookLease*source;int checking;} SessionActorLookLease;
static int session_actor_look_scope(const SessionActorLookLease *l) {
    GscSession *s=l->base.session;
    return session_turret_animation_scope(&l->base)&&s->actor_look_enabled&&!memcmp(&s->actor_look,&l->host,sizeof(l->host));
}
static int session_actor_look_current(void *context) {
    SessionActorLookLease *l=context;
    if(l->source){int ok;if(l->checking||!session_source_look_scope(l->source))return session_source_look_deny(l->source);
        l->checking=1;ok=session_actor_look_scope(l)&&session_turret_animation_current(&l->base)&&session_actor_look_scope(l)&&session_source_look_scope(l->source);
        l->checking=0;if(!ok)return session_source_look_deny(l->source);return 1;}
    return session_actor_look_scope(l)&&session_turret_animation_current(&l->base)&&session_actor_look_scope(l);
}
static void session_actor_look_lease(GscSession *s,SessionActorLookLease *l,int(*current)(void*),void *context) {
    session_turret_animation_lease(s,&l->base,current,context);l->host=s->actor_look;l->source=NULL;l->checking=0;
}
static int session_actor_look_validate(void *user,const GscEntityRef *input,uint64_t identity,int32_t now,
    char *error,size_t cap,int(*current)(void*),void *context) {
    GscSession *s=user;SessionActorLookLease l;GscEntityRef copy,original;int ok;
    if(!s||!input||!current)return 0;copy=original=*input;session_actor_look_lease(s,&l,current,context);
    if(identity!=l.base.instance||now!=l.base.now||!session_actor_look_current(&l))return 0;
    ok=l.host.validate(l.host.user,&copy,identity,now,error,cap,session_actor_look_current,&l);
    return ok==1&&session_actor_look_current(&l)&&!memcmp(&copy,&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy));
}
static int session_actor_look_invoke(void *user,const GscActorLookRequest *input,GscActorLookReply*out,char *error,size_t cap,int(*current)(void*),void *context) {
    GscSession *s=user;SessionActorLookLease l;GscActorLookRequest copy,original;GscActorLookReply reply;int ok=0,owned;
    SessionSourceLookLease source;
    if(!s||!input||!out||!current)return 0;memset(&reply,0xff,sizeof(reply));
    owned=session_source_look_begin(s,input,current,context,&source);if(owned<0)return 0;
    copy=original=*input;session_actor_look_lease(s,&l,current,context);
    if(owned)copy=source.gameQ;
    if(owned&&(!session_source_look_bind(&source,&l,&copy)||(cap&&!error)||
       session_animation_namespace_private(s,error,cap)||session_animation_namespace_private(s,out,sizeof(*out))||
       session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(out,sizeof(*out),error,cap)||
       !gsc_runtime_r243_controller_storage_separate(s->runtime,out,sizeof(*out))||!gsc_runtime_r243_controller_storage_separate(s->runtime,error,cap)))goto done;
    if(original.runtime_identity!=l.base.instance||original.now_ms!=l.base.now||!original.catalog_identity||!session_actor_look_current(&l))goto done;
    if(owned&&!session_source_look_enter_game(&source))goto done;
    if(owned)ok=source.provider.invoke(source.provider.user,&copy,&reply,error,cap,session_actor_look_current,&l);
    else ok=l.host.invoke(l.host.user,&copy,&reply,error,cap,session_actor_look_current,&l);
    if(!(ok==1&&session_actor_look_current(&l)&&!memcmp(&copy,owned?&source.gameQ:&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy)))){ok=0;goto done;}
done:if(owned)ok=session_source_look_end(&source,ok);if(ok==1)*out=reply;return ok==1;
}
