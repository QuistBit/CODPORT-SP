int gsc_session_select_r232_actor_animations(GscSession *s,const GscActorAnimationHost *input,char *error,size_t cap) {
    GscActorAnimationHost copy;
    if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
    if(!s||!input)return session_reject(error,cap,"actor animation selection requires Session and real host");copy=*input;
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||s->actor_animations_enabled||!copy.validate||!copy.invoke)
        return session_leave_rejected(s,"actor animations require exclusive idle two-ATR configuration and real host",error,cap);
    if(!session_animation_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
        return session_leave_rejected(s,"actor animation selection expired",error,cap);
    s->actor_animations=copy;s->actor_animations_enabled=1;s->busy=0;return 1;
}
typedef struct {SessionTurretAnimationLease base;GscActorAnimationHost host;SessionSourceAnimationLease*source;int checking;} SessionActorAnimationLease;
static int session_actor_animation_scope(const SessionActorAnimationLease *l) {
    GscSession *s=l->base.session;
    return session_turret_animation_scope(&l->base)&&s->actor_animations_enabled&&!memcmp(&s->actor_animations,&l->host,sizeof(l->host));
}
static int session_actor_animation_current(void *context) {
    SessionActorAnimationLease *l=context;
    if(l->source){int ok;if(l->checking||!session_source_scope(l->source))return session_source_deny(l->source);
        l->checking=1;ok=session_actor_animation_scope(l)&&session_turret_animation_current(&l->base)&&session_actor_animation_scope(l)&&session_source_scope(l->source);
        l->checking=0;if(!ok)return session_source_deny(l->source);return 1;}
    return session_actor_animation_scope(l)&&session_turret_animation_current(&l->base)&&session_actor_animation_scope(l);
}
static void session_actor_animation_lease(GscSession *s,SessionActorAnimationLease *l,int(*current)(void*),void *context) {
    session_turret_animation_lease(s,&l->base,current,context);l->host=s->actor_animations;l->source=NULL;l->checking=0;
}
extern int R243CommandValidationCurrent(const GscEntityRef*,uint64_t,int,int(*)(void*),void*);
static int r243_command_host_time(GscSession*s,int logical,int*host){
 int64_t origin=s?s->view.origin_host_ms:-1;
 if(!s||origin<0||origin>INT32_MAX||logical<0||logical>INT32_MAX-origin||s->view.logical_ms!=logical)return 0;
 *host=(int)(origin+logical);return s->view.last_host_ms==*host;
}
static int session_actor_animation_validate(void *user,const GscEntityRef *input,uint64_t identity,int32_t now,
    char *error,size_t cap,int(*current)(void*),void *context) {
    GscSession *s=user;SessionActorAnimationLease l;GscEntityRef copy,original;int ok,hostNow=now,isCommand=0;int64_t origin=-1;GscActorAnimationHost provider;
    if(!s||!input||!current)return 0;copy=original=*input;session_actor_animation_lease(s,&l,current,context);
    if(identity!=l.base.instance||now!=l.base.now||!session_actor_animation_current(&l))return 0;
    isCommand=R243CommandValidationCurrent(input,identity,now,current,context);
    if(isCommand){origin=s->view.origin_host_ms;if(!r243_command_host_time(s,now,&hostNow))return 0;}
    provider=isCommand?s->command_provider:l.host;
    ok=provider.validate(provider.user,&copy,identity,hostNow,error,cap,session_actor_animation_current,&l);
    if(isCommand){int after;if(memcmp(&provider,&s->command_provider,sizeof(provider)))return 0;if(s->view.origin_host_ms!=origin||!r243_command_host_time(s,now,&after)||after!=hostNow||!R243CommandValidationCurrent(input,identity,now,current,context))return 0;}

    return ok==1&&session_actor_animation_current(&l)&&!memcmp(&copy,&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy));
}

extern int R243CommandExactCurrent(const GscActorAnimationRequest*,int(*)(void*),void*);
typedef struct {
 SessionActorAnimationLease lease;GscActorAnimationHost provider;
 GscActorAnimationRequest q,expected,gameExpected;int64_t origin;int hostNow;const GscActorAnimationRequest *upstream;
 int(*current)(void*);void*context;uintptr_t nonce;int checking,cancelled,taken;
} R243CommandSessionCall;
static R243CommandSessionCall*r243CommandSessionActive;
static uintptr_t r243CommandSessionNonce;
static int r243_command_session_private(const void*p,size_t n){
 return session_animation_namespace_overlap(p,n,&r243CommandSessionActive,sizeof(r243CommandSessionActive))||
 session_animation_namespace_overlap(p,n,&r243CommandSessionNonce,sizeof(r243CommandSessionNonce))||
 (r243CommandSessionActive&&session_animation_namespace_overlap(p,n,r243CommandSessionActive,sizeof(*r243CommandSessionActive)));
}
static int r243_command_session_scope(R243CommandSessionCall*c){
 int host;
 return c&&c==r243CommandSessionActive&&!c->cancelled&&
 c->lease.base.session->flagged_all_method_enabled&&!memcmp(&c->provider,&c->lease.base.session->command_provider,sizeof(c->provider))&&session_actor_animation_scope(&c->lease)&&
 c->q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL&&c->origin==c->lease.base.session->view.origin_host_ms&&
 r243_command_host_time(c->lease.base.session,c->expected.now_ms,&host)&&host==c->hostNow&&
 !memcmp(&c->q,&c->gameExpected,sizeof(c->q))&&
 c->upstream&&!memcmp(c->upstream,&c->expected,sizeof(c->q));
}
static int r243_command_session_current(void*opaque){
 R243CommandSessionCall*c=r243CommandSessionActive;int ok;
 if(!c||(uintptr_t)opaque!=c->nonce||!r243_command_session_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;
 ok=R243CommandExactCurrent(c->upstream,c->current,c->context)&&r243_command_session_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243CommandSessionTake(const GscActorAnimationRequest*q,int(*current)(void*),void*context){
 R243CommandSessionCall*c=r243CommandSessionActive;
 if(!c||!q||c->taken||current!=r243_command_session_current||(uintptr_t)context!=c->nonce||
 memcmp(q,&c->gameExpected,sizeof(*q))||!current(context))return 0;
 c->taken=1;return 1;
}
static int r243_command_session_invoke(GscSession*s,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 R243CommandSessionCall c;int ok=0;
 if(!s||!s->flagged_all_method_enabled||r243CommandSessionActive||r243CommandSessionNonce==UINTPTR_MAX||
 (cap&&!error)||session_animation_namespace_private(s,error,cap)||!R243CommandExactCurrent(input,current,context))return 0;
 memset(&c,0,sizeof(c));session_actor_animation_lease(s,&c.lease,current,context);
 c.provider=s->command_provider;c.q=c.expected=*input;c.upstream=input;c.current=current;c.context=context;c.nonce=++r243CommandSessionNonce;
 if(c.q.runtime_identity!=c.lease.base.instance||c.q.now_ms!=c.lease.base.now||!c.q.catalog_identity)return 0;
 c.origin=s->view.origin_host_ms;if(!r243_command_host_time(s,c.expected.now_ms,&c.hostNow))return 0;
 c.gameExpected=c.expected;c.gameExpected.now_ms=c.hostNow;c.q=c.gameExpected;
 r243CommandSessionActive=&c;
 if(r243_command_session_current((void*)c.nonce)){
  ok=c.provider.invoke(c.provider.user,&c.q,error,cap,r243_command_session_current,(void*)c.nonce);
  ok=ok==1&&c.taken&&r243_command_session_current((void*)c.nonce);
 }
 if(ok&&s->actor_animation_end_enabled){s->actor_animation_end_runtime=c.lease.base.instance;s->actor_animation_end_catalog=c.expected.catalog_identity;}
 r243CommandSessionActive=NULL;return ok;
}
static int session_actor_animation_invoke(void *user,const GscActorAnimationRequest *input,char *error,size_t cap,int(*current)(void*),void *context) {
    GscSession *s=user;SessionActorAnimationLease l;GscActorAnimationRequest copy,original;int ok=0,owned;
    SessionSourceAnimationLease source;
    if(!s||!input||!current)return 0;
    owned=session_source_begin(s,input,current,context,&source);if(owned<0)return 0;
    if(input->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL){if(owned)return session_source_end(&source,0);return r243_command_session_invoke(s,input,error,cap,current,context);}
    copy=original=*input;session_actor_animation_lease(s,&l,current,context);
    if(owned&&(!session_source_bind(&source,&l,&copy)||(cap&&!error)||
       session_animation_namespace_private(s,error,cap)||!gsc_runtime_r243_controller_storage_separate(s->runtime,error,cap)))goto done;
    if(copy.runtime_identity!=l.base.instance||copy.now_ms!=l.base.now||!copy.catalog_identity||!session_actor_animation_current(&l))goto done;
    if(owned&&!session_source_enter_game(&source))goto done;
    ok=l.host.invoke(l.host.user,&copy,error,cap,session_actor_animation_current,&l);
    if(!(ok==1&&session_actor_animation_current(&l)&&!memcmp(&copy,&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy)))){ok=0;goto done;}
    if(s->actor_animation_end_enabled&&original.method!=GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){s->actor_animation_end_runtime=l.base.instance;s->actor_animation_end_catalog=original.catalog_identity;}
done:if(owned)ok=session_source_end(&source,ok);return ok==1;
}
