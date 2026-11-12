int gsc_session_select_loop_fx(GscSession *s,const GscLoopFxHost *h,char *error,size_t cap){
 if(!s||!h||session_delete_overlap(h,sizeof(*h),s,sizeof(*s))||session_delete_overlap(error,cap,s,sizeof(*s))||
    session_delete_overlap(h,sizeof(*h),error,cap))return 0;
 if(!h->prepare||!h->finish||s->busy||s->runtime||s->program||s->loop_fx_enabled||
    !s->fx_enabled||!s->entities_enabled||!s->results_enabled||!s->lifetime_enabled)
  return session_reject(error,cap,"loop FX requires idle selected FX/entity/result/lifetime Session");
 s->loop_fx=*h;s->loop_fx_enabled=1;return 1;
}
typedef struct {GscSession *s;GscRuntime *r;uint64_t id;int active,now;GscLoopFxServices backend;
 const GscLoopFxRequest *request;GscLoopFxRequest expected;
 const GscLoopFxServices *facade;GscLoopFxServices original;} SessionLoopFxLease;
static int session_loop_fx_owner(SessionLoopFxLease *l){
 GscSession *s=l?l->s:NULL;
 return l&&l->active&&s&&s->busy&&!s->abort_requested&&s->runtime==l->r&&s->view.session_id==l->id&&
  s->view.state==GSC_SESSION_RUNNING&&s->view.logical_ms==l->now&&
  (!l->request||!memcmp(l->request,&l->expected,sizeof(l->expected)))&&
  !memcmp(l->facade,&l->original,sizeof(l->original));
}
static int session_loop_fx_current(void *p){
 SessionLoopFxLease *l=p;
 return session_loop_fx_owner(l)&&l->backend.current(l->backend.context)&&session_loop_fx_owner(l);
}
static int session_loop_fx_prepare(void *user,const GscLoopFxRequest *q,GscLoopFxTicket *out,
 const GscLoopFxServices *api,char *error,size_t cap){
 GscSession *s=user;SessionLoopFxLease l;GscLoopFxServices facade,original;GscLoopFxRequest copy,expected;int ok;
 if(!s||!s->loop_fx_enabled||!q||!out||!api||!api->current||!s->runtime)return 0;
 memset(&l,0,sizeof(l));memset(&facade,0,sizeof(facade));copy=*q;expected=copy;
 l.s=s;l.r=s->runtime;l.id=s->view.session_id;l.now=q->now_ms;l.active=1;l.backend=*api;
 facade.context=&l;facade.current=session_loop_fx_current;original=facade;
 l.request=&copy;l.expected=copy;l.facade=&facade;l.original=facade;
 if(!session_loop_fx_current(&l)){l.active=0;return 0;}
 ok=s->loop_fx.prepare(s->loop_fx.user,&copy,out,&facade,error,cap);
 if(memcmp(&copy,&expected,sizeof(copy))||memcmp(&facade,&original,sizeof(facade)))ok=0;
 if(!session_loop_fx_current(&l))ok=0;l.active=0;return ok==1;
}
static int session_loop_fx_finish(void *user,uint64_t runtime,uint64_t operation,int publish,const GscLoopFxServices *api,char *error,size_t cap){
 GscSession *s=user;SessionLoopFxLease l;GscLoopFxServices facade,original;int ok;
 if(!s||!s->loop_fx_enabled||!api||!api->current||!s->runtime||!operation||(publish!=0&&publish!=1))return 0;
 memset(&l,0,sizeof(l));memset(&facade,0,sizeof(facade));l.s=s;l.r=s->runtime;l.id=s->view.session_id;
 l.now=s->view.logical_ms;l.active=1;l.backend=*api;facade.context=&l;facade.current=session_loop_fx_current;original=facade;
 l.facade=&facade;l.original=facade;
 /* Abort is a reason to cancel, not a reason to skip the owner's cleanup. */
 if(publish&&!session_loop_fx_current(&l)){l.active=0;return 0;}
 ok=s->loop_fx.finish(s->loop_fx.user,runtime,operation,publish,&facade,error,cap);
 if(memcmp(&facade,&original,sizeof(facade))||(publish&&!session_loop_fx_current(&l)))ok=0;
 l.active=0;return ok==1;
}
