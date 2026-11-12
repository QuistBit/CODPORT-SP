typedef struct {
 GscSession*s;GscRuntime*r;const GscProgram*g;uint64_t id;
 R243StanceRequest q;const R243StanceRequest*source;
 GscMapExecutionCertificate map;R243StanceHost provider;
 int(*current)(void*);void*context;
} R243StanceSessionIdentity;
typedef struct {R243StanceSessionIdentity id,seal;R243StanceReply reply;char error[384];} R243StanceSessionFrame;
static R243StanceSessionFrame*r243StanceSessionActive;
static int r243_stance_session_private(const void*p,size_t n){
 return session_animation_namespace_overlap(p,n,&r243StanceSessionActive,sizeof(r243StanceSessionActive))||
  (r243StanceSessionActive&&session_animation_namespace_overlap(p,n,r243StanceSessionActive,sizeof(*r243StanceSessionActive)));
}
static int r243_stance_session_scope(R243StanceSessionFrame*f){
 const R243StanceSessionIdentity*i;GscSession*s;
 if(!f||r243StanceSessionActive!=f||memcmp(&f->id,&f->seal,sizeof(f->id)))return 0;i=&f->seal;s=i->s;
 return s&&s->stance_query_enabled&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==i->r&&s->program==i->g&&s->view.session_id==i->id&&i->id&&s->view.logical_ms==i->q.now&&
  i->q.runtime==gsc_runtime_instance_identity(i->r)&&!memcmp(&s->animation_namespace_map,&i->map,sizeof(i->map))&&
  !memcmp((&s->stance_provider),&i->provider,sizeof(i->provider))&&!memcmp(i->source,&i->q,sizeof(i->q));
}
static int r243_stance_session_current(R243StanceSessionFrame*f){
 return r243_stance_session_scope(f)&&R243StanceExactCallCurrent(&f->seal.q,f->seal.current,f->seal.context)&&
  r243_stance_session_scope(f)&&session_animation_resources_current(f->seal.s)&&r243_stance_session_scope(f);
}
int R243StanceSessionBuffersCurrent(const R243StanceRequest*q,const R243StanceReply*out,const char*error,size_t cap,int(*current)(void*),void*context){
 R243StanceSessionFrame*f=r243StanceSessionActive;
 return f&&q==&f->id.q&&out==&f->reply&&error==f->error&&cap==sizeof(f->error)&&
  f->seal.current==current&&f->seal.context==context&&r243_stance_session_current(f);
}
static int r243_stance_session_validate(void*user,const GscEntityRef*entity){
 GscSession*s=user;const R243StanceHost*provider;if(!s)return 0;provider=&s->stance_provider;
 return s&&s->stance_query_enabled&&s->runtime&&s->busy&&!s->abort_requested&&s->view.state==GSC_SESSION_RUNNING&&
  !s->view.error[0]&&provider&&provider->validate&&session_animation_resources_current(s)&&provider->validate(provider->user,entity)==1&&session_animation_resources_current(s);
}
static int r243_stance_session_query(void*user,const R243StanceRequest*input,R243StanceReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;R243StanceSessionFrame f;int accepted=0,ok=0;
 if(!s||!input||!out||!current||r243StanceSessionActive||!s->stance_query_enabled)return 0;
 memset(&f,0,sizeof(f));f.id.s=s;f.id.r=s->runtime;f.id.g=s->program;f.id.id=s->view.session_id;f.id.q=*input;f.id.source=input;
 f.id.map=s->animation_namespace_map;f.id.provider=s->stance_provider;f.id.current=current;f.id.context=context;
 f.seal=f.id;f.reply.allowed=2;r243StanceSessionActive=&f;
 if(!r243_stance_session_current(&f))goto done;
 /* Preserve the genuine VM Current and copied intent through the pure GAME
  * read. Session's own sealed context is checked before and after publication. */
 accepted=f.seal.provider.query(f.seal.provider.user,&f.id.q,&f.reply,f.error,sizeof(f.error),current,context);f.error[sizeof(f.error)-1]=0;
 if(!r243_stance_session_current(&f)||accepted!=1||(f.reply.allowed!=0&&f.reply.allowed!=1))goto done;ok=1;
done:
 r243StanceSessionActive=NULL;
 if(!ok){if(error&&cap)snprintf(error,cap,"stance Session query refused: %s",f.error[0]?f.error:"caller/map/context retired");return 0;}
 *out=f.reply;return 1;
}
int gsc_session_select_r243_stance(GscSession*s,const R243StanceHost*input,char*error,size_t cap){
 R243StanceHost copy;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap))return 0;
 copy=*input;if(!copy.validate||!copy.query||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->animation_namespace_enabled||s->stance_query_enabled||!session_animation_resources_current(s))return session_leave_rejected(s,"R243 stance needs idle live namespace Session",error,cap);
 s->stance_provider=copy;s->stance_query_enabled=1;s->busy=0;return 1;
}
static int r243_stance_program_select(GscSession*s){return !s->stance_query_enabled||(R243ProgramSelectStance(s->program)&&(!s->stance_provider.allowWrite||Cod1ProgramSelectStanceWrite(s->program)));}
int R243StanceSessionHostTimeCurrent(const R243StanceRequest*q,int hostNow){
 R243StanceSessionFrame*f=r243StanceSessionActive;GscSession*s;
 if(!f||!q||memcmp(q,&f->seal.q,sizeof(*q))||!r243_stance_session_current(f))return 0;
 s=f->seal.s;
 return hostNow>=0&&s->view.origin_host_ms>=0&&f->seal.q.now>=0&&s->view.origin_host_ms<=INT_MAX-f->seal.q.now&&hostNow==s->view.origin_host_ms+f->seal.q.now&&r243_stance_session_current(f);
}
static int r243_stance_runtime_bind(GscSession*s){
 R243StanceHost host;if(!s->stance_query_enabled)return 1;
 memset(&host,0,sizeof(host));host.user=s;host.validate=r243_stance_session_validate;host.query=r243_stance_session_query;host.allowWrite=s->stance_provider.allowWrite;
 if(!session_animation_resources_current(s)||!R243RuntimeSelectStance(s->runtime,&host)||!session_animation_resources_current(s))return 0;
 puts("R243 STANCE_SESSION_BOUND selectedTypedActorPredicate1 nativePathOwnerNotInferred1 liveCourseProfile1");return 1;
}
