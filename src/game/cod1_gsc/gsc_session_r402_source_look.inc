/* Session takes the exact VM offer before any of its own callbacks. The
 * forwarding and GAME_ENTRY transitions below are callback-free. */
static int session_source_look_private(const GscSession*s,const void*p,size_t n){
 const SessionSourceLookLease*d=s?s->r402_source_active:NULL;
 return s&&((s->r243_source_controller&&session_animation_namespace_overlap(p,n,s->r243_source_controller,sizeof(SessionControllerLease)))||
  (d&&(session_animation_namespace_overlap(p,n,d,sizeof(*d))||
       session_animation_namespace_overlap(p,n,d->actorLease,d->actorLease?sizeof(SessionActorLookLease):0)||
       session_animation_namespace_overlap(p,n,d->forwarded,d->forwarded?sizeof(d->q):0))));
}
static int session_source_look_deny(SessionSourceLookLease*d){
 if(d){d->cancelled=1;if(d->controller)((SessionControllerLease*)d->controller)->base.cancelled=1;}return 0;
}
/* Convert the clock at the owning Session boundary. The VM offer stays
 * logical; the single forwarded GAME request and callback stay exact. */
static int session_source_look_mapping_current(SessionSourceLookLease*d){
 GscActorLookRequest expected;int now;GscSession*s=d?d->s:NULL;
 if(!s||d->mapped!=s->source_actor_look_enabled)return 0;
 expected=d->q;
 if(d->mapped){
  if(memcmp(&d->provider,&s->source_actor_look,sizeof(d->provider))||
     d->origin!=s->view.origin_host_ms||!r243_command_host_time(s,d->q.now_ms,&now)||now!=d->hostNow)return 0;
  expected.now_ms=now;
 }else if(memcmp(&d->provider,&d->host,sizeof(d->provider)))return 0;
 return !memcmp(&expected,&d->gameQ,sizeof(expected));
}
static int session_source_look_scope(SessionSourceLookLease*d){
 SessionControllerLease*c=d?d->controller:NULL;GscSession*s=d?d->s:NULL;
 SessionActorLookLease*a=d?d->actorLease:NULL;
 return d&&s&&c&&!s->r243_source_active&&!d->cancelled&&s->r402_source_active==d&&s->r243_source_controller==c&&
  session_controller_scope(c)&&s->busy&&!memcmp(&s->actor_look,&d->host,sizeof(d->host))&&session_source_look_mapping_current(d)&&
  d->input&&!memcmp(d->input,&d->q,sizeof(d->q))&&
  (!d->forwarded||!memcmp(d->forwarded,&d->gameQ,sizeof(d->gameQ)))&&
  (!a||(a->source==d&&session_actor_look_scope(a)));
}
static int session_source_look_begin(GscSession*s,const GscActorLookRequest*input,
 int(*current)(void*),void*context,SessionSourceLookLease*d){
 SessionControllerLease*c=s?s->r243_source_controller:NULL;const GscActorSourceLookBroker*b;
 if(!c)return 0;
 if(input&&input->method!=GSC_ACTOR_LOOK_AIM_AT_POS&&!s->r243_source_active&&!s->r402_source_active)return 0;
 if(s->r243_source_active||s->r402_source_active||!input||!current||!session_controller_scope(c)){
  c->base.cancelled=1;return -1;}
 memset(d,0,sizeof(*d));d->s=s;d->controller=c;d->input=input;
 d->current=current;d->context=context;d->host=s->actor_look;d->phase=1;
 b=&c->expectedOriginalApi.sourceLook;
 if(!b->take||!b->claim||!b->current||
    b->take(b->context,input,current,context,&d->receipt)!=1){c->base.cancelled=1;return -1;}
 d->q=*input;d->gameQ=d->q;d->mapped=s->source_actor_look_enabled;
 d->provider=d->mapped?s->source_actor_look:d->host;
 if(d->mapped){d->origin=s->view.origin_host_ms;
  if(!r243_command_host_time(s,d->q.now_ms,&d->hostNow)){session_source_look_deny(d);return -1;}
  d->gameQ.now_ms=d->hostNow;
 }
 s->r402_source_active=d;
 if(!session_source_look_scope(d)){session_source_look_deny(d);s->r402_source_active=NULL;return -1;}
 return 1;
}
static int session_source_look_bind(SessionSourceLookLease*d,void*actorLease,const GscActorLookRequest*forwarded){
 SessionActorLookLease*a=actorLease;
 if(!d||!a||!forwarded||!session_source_look_scope(d)||memcmp(forwarded,&d->gameQ,sizeof(d->gameQ)))return session_source_look_deny(d);
 d->actorLease=a;d->forwarded=forwarded;a->source=d;return session_source_look_scope(d);
}
static int session_source_look_enter_game(SessionSourceLookLease*d){
 SessionActorLookLease*a=d?d->actorLease:NULL;
 if(!session_source_look_scope(d)||!a||a->checking||d->phase!=1)return session_source_look_deny(d);
 d->phase=2;return 1;
}
static int session_source_look_end(SessionSourceLookLease*d,int accepted){
 int ok=accepted&&session_source_look_scope(d)&&d->phase==3;
 if(d&&d->s&&d->s->r402_source_active==d)d->s->r402_source_active=NULL;
 if(!ok)session_source_look_deny(d);return ok;
}
static int session_source_look_take_unavailable(void*u,const GscActorLookRequest*q,int(*f)(void*),void*x,GscActorSourceLookReceipt*out){
 SessionControllerLease*c=u;(void)q;(void)f;(void)x;(void)out;if(c)c->base.cancelled=1;return 0;
}
static int session_source_look_claim(void*opaque,const GscActorLookRequest*q,
 int(*current)(void*),void*context,GscActorSourceLookReceipt*out){
 SessionControllerLease*c=opaque;SessionSourceLookLease*d=c?c->base.session->r402_source_active:NULL;
 SessionActorLookLease*a=d?d->actorLease:NULL;const GscActorSourceLookBroker*b;
 if(!c||!d||!out||!session_source_look_scope(d)||d->controller!=c||!a||a->checking||
    d->phase!=2||q!=d->forwarded||current!=session_actor_look_current||context!=a||
    session_animation_namespace_private(d->s,out,sizeof(*out))||
    !gsc_runtime_r243_controller_storage_separate(d->s->runtime,out,sizeof(*out)))return session_source_look_deny(d);
 b=&c->expectedOriginalApi.sourceLook;
 /* Consume Session GAME_ENTRY before the pure VM claim. No callback can
  * observe an armed reusable grant, including inside a forged broker. */
 d->phase=3;
 if(b->claim(b->context,d->input,d->current,d->context,&d->receipt)!=1||!session_source_look_scope(d))return session_source_look_deny(d);
 *out=d->receipt;return 1;
}
static int session_source_look_current(void*opaque,const GscActorSourceLookReceipt*receipt,
 const GscActorLookRequest*q,int(*current)(void*),void*context){
 SessionControllerLease*c=opaque;SessionSourceLookLease*d=c?c->base.session->r402_source_active:NULL;
 SessionActorLookLease*a=d?d->actorLease:NULL;const GscActorSourceLookBroker*b;
 if(!c||!d||!receipt||!session_source_look_scope(d)||d->controller!=c||!a||a->checking||d->phase!=3||
    q!=d->forwarded||current!=session_actor_look_current||context!=a||memcmp(receipt,&d->receipt,sizeof(*receipt)))return session_source_look_deny(d);
 b=&c->expectedOriginalApi.sourceLook;
 if(b->current(b->context,receipt,d->input,d->current,d->context)!=1||!session_source_look_scope(d))return session_source_look_deny(d);
 return 1;
}
