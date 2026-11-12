typedef struct {
 GscSession*s;GscRuntime*r;const GscProgram*g;uint64_t id;
 R243MoveDeltaRequest q;const R243MoveDeltaRequest*source;
 GscMapExecutionCertificate map;R243MoveDeltaHost provider;
 int(*current)(void*);void*context;
} R243MoveDeltaSessionIdentity;
typedef struct {R243MoveDeltaSessionIdentity id,seal;R243MoveDeltaReply reply;char error[384];} R243MoveDeltaSessionFrame;
static R243MoveDeltaSessionFrame*r243MoveDeltaSessionActive;
static int r243_move_delta_session_private(const void*p,size_t n){return session_animation_namespace_overlap(p,n,&r243MoveDeltaSessionActive,sizeof(r243MoveDeltaSessionActive))||(r243MoveDeltaSessionActive&&session_animation_namespace_overlap(p,n,r243MoveDeltaSessionActive,sizeof(*r243MoveDeltaSessionActive)));}
static int r243_move_delta_session_scope(R243MoveDeltaSessionFrame*f){
 const R243MoveDeltaSessionIdentity*i;GscSession*s;
 if(!f||r243MoveDeltaSessionActive!=f||memcmp(&f->id,&f->seal,sizeof(f->id)))return 0;i=&f->seal;s=i->s;
 return s&&s->move_delta_query_enabled&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==i->r&&s->program==i->g&&s->view.session_id==i->id&&i->id&&s->view.logical_ms==i->q.now&&
  i->q.runtime==gsc_runtime_instance_identity(i->r)&&i->q.resource_owner==i->provider.resource_owner&&i->q.resource_owner&&
  !memcmp(&s->animation_namespace_map,&i->map,sizeof(i->map))&&!memcmp(&s->move_delta_provider,&i->provider,sizeof(i->provider))&&!memcmp(i->source,&i->q,sizeof(i->q));
}
static int r243_move_delta_session_current(R243MoveDeltaSessionFrame*f){return r243_move_delta_session_scope(f)&&R243MoveDeltaExactCallCurrent(&f->seal.q,f->seal.current,f->seal.context)&&r243_move_delta_session_scope(f)&&session_animation_resources_current(f->seal.s)&&r243_move_delta_session_scope(f);}
int R243MoveDeltaSessionBuffersCurrent(const R243MoveDeltaRequest*q,const R243MoveDeltaReply*out,const char*error,size_t cap,int(*current)(void*),void*context){R243MoveDeltaSessionFrame*f=r243MoveDeltaSessionActive;return f&&q==&f->id.q&&out==&f->reply&&error==f->error&&cap==sizeof(f->error)&&f->seal.current==current&&f->seal.context==context&&r243_move_delta_session_current(f);}
int R243MoveDeltaSessionHostTimeCurrent(const R243MoveDeltaRequest*q,int hostNow){R243MoveDeltaSessionFrame*f=r243MoveDeltaSessionActive;GscSession*s;if(!f||!q||memcmp(q,&f->seal.q,sizeof(*q))||!r243_move_delta_session_current(f))return 0;s=f->seal.s;return hostNow>=0&&s->view.origin_host_ms>=0&&q->now>=0&&s->view.origin_host_ms<=INT_MAX-q->now&&hostNow==s->view.origin_host_ms+q->now&&r243_move_delta_session_current(f);}
/* Exact whole declaration catalog, excluding independently allocated parser
 * tokens. Called only during the named Session query; no packed native IDs. */
int R243MoveDeltaSessionCatalogCurrent(const R243MoveDeltaRequest*q,const GscAnimationCatalog*loaded){R243MoveDeltaSessionFrame*f=r243MoveDeltaSessionActive;GscAnimationCatalog*owned;GscAnimationCatalogEntry a,b;size_t n,count;int slot;
 if(!f||!q||!loaded||memcmp(q,&f->seal.q,sizeof(*q))||!r243_move_delta_session_current(f))return 0;
 slot=!strcmp(q->tree,"generic_human")?0:!strcmp(q->tree,"animation_rig_gear_trainingjump")?1:!strcmp(q->tree,"animation_rig_wire_trainingjump")?2:-1;
 if(slot<0||(owned=f->seal.s->animation_namespace_catalogs[slot])==NULL)return 0;
 count=gsc_animation_catalog_count(owned);if(!count||count!=gsc_animation_catalog_count(loaded))return 0;
 for(n=0;n<count;n++)if(!gsc_animation_catalog_entry(owned,n,&a)||!gsc_animation_catalog_entry(loaded,n,&b)||memcmp(&a,&b,sizeof(a)))return 0;
 return r243_move_delta_session_current(f);
}
static int r243_move_delta_session_query(void*user,const R243MoveDeltaRequest*input,R243MoveDeltaReply*out,char*error,size_t cap,int(*current)(void*),void*context){GscSession*s=user;R243MoveDeltaSessionFrame f;int n,accepted=0,ok=0;
 if(!s||!input||!out||!current||r243MoveDeltaSessionActive||!s->move_delta_query_enabled)return 0;
 memset(&f,0,sizeof(f));f.id.s=s;f.id.r=s->runtime;f.id.g=s->program;f.id.id=s->view.session_id;f.id.q=*input;f.id.source=input;f.id.map=s->animation_namespace_map;f.id.provider=s->move_delta_provider;f.id.current=current;f.id.context=context;f.seal=f.id;
 for(n=0;n<3;n++){uint32_t poison=0x7fc00001u;memcpy(&f.reply.vector[n],&poison,4);}r243MoveDeltaSessionActive=&f;
 if(!r243_move_delta_session_current(&f))goto done;
 accepted=f.seal.provider.query(f.seal.provider.user,&f.id.q,&f.reply,f.error,sizeof(f.error),current,context);f.error[sizeof(f.error)-1]=0;
 if(!r243_move_delta_session_current(&f)||accepted!=1)goto done;
 for(n=0;n<3;n++)if(!isfinite(f.reply.vector[n]))goto done;ok=1;
done:r243MoveDeltaSessionActive=NULL;if(!ok){if(error&&cap)snprintf(error,cap,"GetMoveDelta Session query refused: %s",f.error[0]?f.error:"caller/resource/map retired");return 0;}*out=f.reply;return 1;
}
int gsc_session_select_r243_move_delta(GscSession*s,const R243MoveDeltaHost*input,char*error,size_t cap){R243MoveDeltaHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!h.query||!h.resource_owner||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->animation_namespace_enabled||!s->execution_extensions||s->move_delta_query_enabled||!session_animation_resources_current(s))return session_leave_rejected(s,"GetMoveDelta needs EMPTY current namespace/executable Session",error,cap);
 s->move_delta_provider=h;s->move_delta_query_enabled=1;s->busy=0;return 1;
}
static int r243_move_delta_program_select(GscSession*s){return !s->move_delta_query_enabled||R243ProgramSelectMoveDelta(s->program);}
static int r243_move_delta_runtime_bind(GscSession*s){R243MoveDeltaHost h;if(!s->move_delta_query_enabled)return 1;memset(&h,0,sizeof(h));h.user=s;h.query=r243_move_delta_session_query;h.resource_owner=s->move_delta_provider.resource_owner;return session_animation_resources_current(s)&&R243RuntimeSelectMoveDelta(s->runtime,&h)&&session_animation_resources_current(s);}
