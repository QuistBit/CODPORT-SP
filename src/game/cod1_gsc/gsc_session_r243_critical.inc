
extern int R243CriticalVMExact(const GscEntityRef*,uint32_t,const GscActorCriticalCall*);
typedef struct {
 SessionTurretAnimationLease lease;GscActorCriticalHost host;GscEntityRef owner,entity;uint32_t method;
 GscActorCriticalCall original,forwarded,expected;const GscEntityRef*upEntity;const GscActorCriticalCall*upCall;
 uintptr_t nonce;int checking,cancelled,taken;
} R243CriticalSessionCall;
static R243CriticalSessionCall*r243CriticalSessionActive;static uintptr_t r243CriticalSessionNonce;
static int r243_critical_session_private(const void*p,size_t n){return session_animation_namespace_overlap(p,n,&r243CriticalSessionActive,sizeof(r243CriticalSessionActive))||session_animation_namespace_overlap(p,n,&r243CriticalSessionNonce,sizeof(r243CriticalSessionNonce))||(r243CriticalSessionActive&&session_animation_namespace_overlap(p,n,r243CriticalSessionActive,sizeof(*r243CriticalSessionActive)));}
static int r243_critical_session_scope(R243CriticalSessionCall*c){
 return c&&c==r243CriticalSessionActive&&!c->cancelled&&session_turret_animation_scope(&c->lease)&&c->lease.session->critical_selected&&
 !memcmp(&c->host,&c->lease.session->critical_provider,sizeof(c->host))&&!memcmp(&c->entity,&c->owner,sizeof(c->owner))&&
 !memcmp(&c->forwarded,&c->expected,sizeof(c->expected))&&!memcmp(c->upEntity,&c->owner,sizeof(c->owner))&&!memcmp(c->upCall,&c->original,sizeof(c->original));
}
static int r243_critical_session_current(void*token){
 R243CriticalSessionCall*c=r243CriticalSessionActive;int ok;
 if(!c||(uintptr_t)token!=c->nonce)return 0;
 if(!r243_critical_session_scope(c)){c->cancelled=1;return 0;}
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;
 ok=R243CriticalVMExact(c->upEntity,c->method,c->upCall)&&r243_critical_session_scope(c);c->checking=0;if(!ok)c->cancelled=1;return ok;
}
static int r243_critical_session_label(void*token,char*out,size_t cap,char*error,size_t size){
 R243CriticalSessionCall*c=r243CriticalSessionActive;int ok;
 if(!c||!r243_critical_session_current(token)||!c->original.has_argument||session_animation_namespace_private(c->lease.session,out,cap)||session_animation_namespace_private(c->lease.session,error,size)||session_animation_namespace_overlap(out,cap,error,size))return 0;
 ok=c->original.read_label(c->original.context,out,cap,error,size);
 return ok==1&&r243_critical_session_current(token);
}
int R243CriticalSessionTake(const GscEntityRef*e,uint32_t method,const GscActorCriticalCall*q){
 R243CriticalSessionCall*c=r243CriticalSessionActive;
 if(!c||!e||!q||c->taken||method!=c->method||memcmp(e,&c->owner,sizeof(*e))||memcmp(q,&c->expected,sizeof(*q))||!r243_critical_session_current(q->context))return 0;
 c->taken=1;return 1;
}
static int r243_critical_session_invoke(void*user,const GscEntityRef*e,uint32_t method,const GscActorCriticalCall*q,int32_t*out,char*error,size_t cap){
 GscSession*s=user;R243CriticalSessionCall c;int ok;int32_t result=-1;
 if(!s||!e||!q||!out||!s->critical_selected||r243CriticalSessionActive||r243CriticalSessionNonce==UINTPTR_MAX||!R243CriticalVMExact(e,method,q)||
 session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 memset(&c,0,sizeof(c));session_turret_animation_lease(s,&c.lease,q->current,q->context);c.host=s->critical_provider;c.owner=c.entity=*e;c.method=method;c.original=*q;c.upCall=q;c.upEntity=e;c.nonce=++r243CriticalSessionNonce;
 c.forwarded=*q;c.forwarded.context=(void*)c.nonce;c.forwarded.current=r243_critical_session_current;c.forwarded.read_label=r243_critical_session_label;c.expected=c.forwarded;
 r243CriticalSessionActive=&c;
 ok=r243_critical_session_current((void*)c.nonce)&&c.host.invoke(c.host.user,&c.entity,method,&c.forwarded,&result,error,cap)==1;
 ok=ok&&c.taken&&r243_critical_session_current((void*)c.nonce);r243CriticalSessionActive=NULL;
 if(ok)*out=result;return ok;
}
int gsc_session_select_r243_critical(GscSession*s,const GscActorCriticalHost*input,char*error,size_t cap){
 GscActorCriticalHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->actor_animations_enabled||!s->native_animation_roots_enabled||s->critical_selected||!h.invoke)return session_leave_rejected(s,"critical selection requires idle configured Actor Session",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"critical selection retired",error,cap);
 s->critical_provider=h;s->critical_selected=1;s->busy=0;return 1;
}
