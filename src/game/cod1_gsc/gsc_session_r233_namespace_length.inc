/* Session lifetime fence around read-only, typed native clip metadata. */
int gsc_session_select_r233_namespace_animation_length(GscSession*s,const GscAnimationLengthHost*input,char*error,size_t cap){GscAnimationLengthHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"namespace length requires Session host");h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->native_animation_roots_enabled||s->namespace_length_enabled||!h.query)return session_leave_rejected(s,"namespace length requires exclusive idle native-root namespace",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"namespace length resources expired",error,cap);
 s->namespace_length=h;s->namespace_length_enabled=1;s->busy=0;return 1;
}
typedef struct {SessionTurretAnimationLease base;GscAnimationLengthHost host;GscAnimationLengthRequest *input,copy;int checking,cancelled;} SessionNamespaceLengthLease;
static int session_namespace_length_scope(const SessionNamespaceLengthLease*l){return !l->cancelled&&session_turret_animation_scope(&l->base)&&l->base.session->namespace_length_enabled&&!memcmp(&l->host,&l->base.session->namespace_length,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));}
static int session_namespace_length_current(void*opaque){SessionNamespaceLengthLease*l=opaque;int ok;
 if(!session_namespace_length_scope(l))return 0;if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_animation_resources_current(l->base.session)&&session_namespace_length_scope(l)&&session_turret_animation_current(&l->base)&&session_namespace_length_scope(l);l->checking=0;return ok;
}
static int session_namespace_length_query(void*user,const GscAnimationLengthRequest*input,GscAnimationLengthReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionNamespaceLengthLease l;GscAnimationLengthRequest q;GscAnimationLengthReply reply;int ok;
 if(!s||!input||!out||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 memset(&l,0,sizeof(l));q=*input;l.input=&q;l.copy=q;session_turret_animation_lease(s,&l.base,current,context);l.host=s->namespace_length;
 if(q.runtime_identity!=l.base.instance||!session_namespace_length_current(&l))return 0;
 memset(&reply,0xff,sizeof(reply));ok=l.host.query(l.host.user,&q,&reply,error,cap,session_namespace_length_current,&l);
 if(ok!=1||!session_namespace_length_current(&l)||memcmp(input,&l.copy,sizeof(l.copy)))return 0;*out=reply;return 1;
}

int gsc_session_select_map_animation_length(GscSession*s,const GscAnimationLengthHost*input,char*error,size_t cap){
 GscAnimationLengthHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->root_map_enabled||!s->animation_namespace_enabled||!s->map_animation_names.count||!s->map_source_float_enabled||s->native_animation_roots_enabled||s->turret_animations_enabled||s->actor_bootstrap_enabled||s->namespace_length_enabled||!h.query)return session_leave_rejected(s,"map length requires exclusive idle map namespace and metadata provider",error,cap);
 if(!session_animation_resources_current(s)||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->namespace_length_enabled||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"map length selection retired",error,cap);
 s->namespace_length=h;s->namespace_length_enabled=1;s->map_animation_length_enabled=1;s->busy=0;return 1;
}
typedef struct{GscSession*s;GscRuntime*r;uint64_t id,instance;int now,checking,cancelled;GscAnimationLengthHost host;GscAnimationLengthRequest*input,copy;GscMapExecutionCertificate map;int(*current)(void*);void*context;}SessionMapLengthLease;
static int session_map_length_scope(const SessionMapLengthLease*l){GscSession*s=l->s;
 return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&l->id&&s->runtime==l->r&&gsc_runtime_instance_identity(s->runtime)==l->instance&&s->view.logical_ms==l->now&&s->namespace_length_enabled&&s->map_animation_length_enabled&&s->root_map_enabled&&s->map_animation_names.count&&s->animation_namespace_enabled&&(!s->actor_bootstrap_enabled||s->map_actor_program_enabled)&&s->root_map.receipt==l->map.receipt&&s->root_map.current==l->map.current&&s->animation_namespace_map.receipt==l->map.receipt&&s->animation_namespace_map.current==l->map.current&&!memcmp(&s->namespace_length,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));
}
static int session_map_length_current(void*opaque){SessionMapLengthLease*l=opaque;int ok;
 if(!session_map_length_scope(l)||!l->current)return 0;if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_animation_resources_current(l->s)&&session_map_length_scope(l)&&l->current(l->context)==1&&session_map_length_scope(l);l->checking=0;return ok;
}
static int session_map_animation_length_query(void*user,const GscAnimationLengthRequest*input,GscAnimationLengthReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionMapLengthLease l;GscAnimationLengthRequest q;GscAnimationLengthReply reply;GscAnimationCatalogEntry entry;unsigned i;int found=0,ok;
 if(!s||!input||!out||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;if(!q.runtime_identity||!q.ordinal||q.flags||!memchr(q.tree,0,64)||!memchr(q.name,0,64))return 0;
 memset(&l,0,sizeof(l));l.s=s;l.r=s->runtime;l.id=s->view.session_id;l.instance=gsc_runtime_instance_identity(s->runtime);l.now=s->view.logical_ms;l.host=s->namespace_length;l.map=s->animation_namespace_map;l.current=current;l.context=context;l.input=&q;l.copy=q;
 if(q.runtime_identity!=l.instance||!session_map_length_scope(&l))return 0;
 for(i=0;i<s->map_animation_names.count;i++)if(!strcmp(q.tree,s->map_animation_names.names[i])&&gsc_animation_catalog_entry(s->animation_namespace_catalogs[i],q.ordinal-1,&entry)&&!entry.flags&&entry.ordinal==q.ordinal&&!strcmp(entry.name,q.name))found++;
 if(found!=1||!session_map_length_current(&l))return 0;
 memset(&reply,0xff,sizeof(reply));ok=l.host.query(l.host.user,&q,&reply,error,cap,session_map_length_current,&l);
 if(ok!=1||!session_map_length_current(&l)||memcmp(input,&l.copy,sizeof(l.copy)))return 0;*out=reply;return 1;
}

int gsc_session_map_animation_metadata_request(const GscSession*s,uint64_t id,unsigned index,GscAnimationLengthRequest*out){
 if(!s||!id||!out||s->busy||s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_PREPARED||s->view.session_id!=id||!s->map_animation_length_enabled||!s->runtime||session_animation_namespace_private(s,out,sizeof(*out)))return -1;
 return gsc_runtime_map_animation_metadata_request(s->runtime,index,out);
}
