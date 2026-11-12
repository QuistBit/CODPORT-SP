typedef struct {GscSession*s;uint64_t id,program;GscPreparedEntityEntry request;GscSessionBspConstructionHost host;int callback;} SessionBspScope;
static int session_bsp_current(SessionBspScope*c){int ok;GscEntityRef ref;
 if(!c||c->s->bsp_scope!=c||!c->s->busy||c->s->abort_requested||c->s->view.error[0]||
    c->s->view.session_id!=c->id||c->s->view.state!=GSC_SESSION_COMPILED||c->s->runtime||
    gsc_program_identity(c->s->program)!=c->program||!session_actor_cache_current(c->s)||!session_actor_program_current(c->s))return 0;
 if(c->callback){c->s->abort_requested=1;session_terminal(c->s,"BSP construction current callback reentered");return 0;}
 ref=c->request.entry.entity;c->callback=1;ok=c->host.current(c->host.user,&ref);c->callback=0;
 return ok==1&&session_actor_ref_same(&ref,&c->request.entry.entity)&&c->s->bsp_scope==c&&!c->s->abort_requested&&c->s->view.session_id==c->id&&gsc_program_identity(c->s->program)==c->program;
}
static int session_bsp_validate(void*u,const GscEntityRef*r){GscSession*s=u;SessionBspScope*c=s?s->bsp_scope:NULL;
 return c&&r&&session_actor_ref_same(r,&c->request.entry.entity)&&session_bsp_current(c);
}
static int session_bsp_classify(void*u,const GscEntityRef*r,const char*name,char*e,size_t n){GscSession*s=u;SessionBspScope*c=s?s->bsp_scope:NULL;uint32_t i;
 if(!c||!name||!session_bsp_validate(u,r)){session_reject(e,n,"BSP constructor field classification outside its record");return -1;}
 for(i=0;i<c->request.field_count;i++)if(!strcmp(name,c->request.fields[i].name))return 0;
 session_reject(e,n,"BSP constructor requested an undeclared private field");return -1;
}
static int session_bsp_commit(void*u,const GscEntityScriptEntry*q,char*e,size_t n){SessionBspScope*c=u;int ok;GscEntityScriptEntry copy;
 if(!q||memcmp(q,&c->request.entry,sizeof(*q))||!session_bsp_current(c))return 0;
 copy=*q;ok=c->host.native.commit_fields(c->host.native.user,&copy,e,n);
 /* The native commit's last one-shot mark must be followed by VM publication
  * without another host callback. Only pure exact Session checks follow it. */
 return ok==1&&!memcmp(&copy,q,sizeof(copy))&&c->s->bsp_scope==c&&c->s->busy&&!c->s->abort_requested&&
  c->s->view.session_id==c->id&&gsc_program_identity(c->s->program)==c->program;
}
static int session_bsp_prepare(void*u,const GscEntityScriptEntry*q,GscEntityExecutionCertificate*out,char*e,size_t n){SessionBspScope*c=u;int ok;GscEntityScriptEntry copy;
 if(!q||memcmp(q,&c->request.entry,sizeof(*q))||!session_bsp_current(c)||!gsc_bsp_fields_current(c->s->bsp_runtime,&c->request))return 0;
 copy=*q;ok=c->host.native.prepare(c->host.native.user,&copy,out,e,n);return ok==1&&!memcmp(&copy,q,sizeof(copy))&&session_bsp_current(c);
}
int gsc_session_bsp_fields_current(const GscSession*s,uint64_t id,const GscPreparedEntityEntry*q){
 return s&&id&&!s->abort_requested&&!s->view.error[0]&&s->view.session_id==id&&s->program&&q&&((s->view.state==GSC_SESSION_COMPILED&&s->bsp_runtime&&gsc_bsp_fields_current(s->bsp_runtime,q))||(s->bsp_promoted&&s->view.state==GSC_SESSION_PREPARED&&s->runtime&&gsc_bsp_fields_current(s->runtime,q)));
}
int gsc_session_construct_bsp_actor(GscSession*s,uint64_t id,const GscPreparedEntityEntry*input,
 const GscSessionBspConstructionHost*host,GscPreparedEntityEntryRun*out,char*e,size_t n){
 SessionBspScope c;uint32_t i;GscEntityHost entities;GscEntityFieldHost fields;GscPreparedEntityEntryHost native;int ok;
 if(!s||!input||!host||!out||!host->current||!host->native.commit_fields||!host->native.prepare||
    session_actor_overlap(input,sizeof(*input),s,sizeof(*s))||session_actor_overlap(out,sizeof(*out),s,sizeof(*s))||
    session_actor_overlap(host,sizeof(*host),s,sizeof(*s))||session_actor_overlap(e,n,s,sizeof(*s))||
    session_actor_overlap(input,sizeof(*input),out,sizeof(*out))||session_actor_overlap(host,sizeof(*host),out,sizeof(*out))||
    session_actor_overlap(e,n,input,sizeof(*input))||session_actor_overlap(e,n,out,sizeof(*out))||session_actor_overlap(e,n,host,sizeof(*host)))return 0;
 if(s->bsp_runtime&&(!gsc_runtime_storage_separate(s->bsp_runtime,input,sizeof(*input))||!gsc_runtime_storage_separate(s->bsp_runtime,host,sizeof(*host))||
    !gsc_runtime_storage_separate(s->bsp_runtime,out,sizeof(*out))||!gsc_runtime_storage_separate(s->bsp_runtime,e,n)))return 0;
 if(s->runtime&&(!gsc_runtime_storage_separate(s->runtime,input,sizeof(*input))||!gsc_runtime_storage_separate(s->runtime,host,sizeof(*host))||
    !gsc_runtime_storage_separate(s->runtime,out,sizeof(*out))||!gsc_runtime_storage_separate(s->runtime,e,n)))return 0;
 if(!session_enter(s,e,n))return 0;
 if(!session_id_matches(s,id))return session_leave_rejected(s,"stale BSP constructor Session",e,n);
 if(s->view.state!=GSC_SESSION_COMPILED||!s->program||s->runtime||!s->actor_program_enabled||s->actor_bootstrap_enabled||s->bsp_scope||s->bsp_promoted||s->bsp_count>=GSC_AI_ARRAY_CAPACITY)
  return session_leave_rejected(s,"BSP construction requires current early program without late cohort",e,n);
 for(i=0;i<s->bsp_count;i++)if(session_actor_ref_same(&s->bsp_entries[i].entity,&input->entry.entity))return session_failed(s,"BSP allocation was already constructed",e,n);
 memset(&c,0,sizeof(c));c.s=s;c.id=id;c.program=gsc_program_identity(s->program);c.request=*input;c.host=*host;
 if(!memchr(c.request.entry.module,0,sizeof(c.request.entry.module))||strncmp(c.request.entry.module,"aitype/",7)||
    !memchr(c.request.entry.function,0,sizeof(c.request.entry.function))||strcmp(c.request.entry.function,"main")||c.request.field_count>GSC_ENTITY_INITIAL_FIELDS_MAX||
    c.request.entry.entity.epoch!=s->actor_program.epoch||!session_actor_cached_function(s,c.request.entry.module,"main"))
  return session_failed(s,"BSP constructor differs from early aitype function cache",e,n);
 s->bsp_scope=&c;
 if(!session_bsp_current(&c)){s->bsp_scope=NULL;return session_failed(s,"BSP constructor record owner unavailable",e,n);}
 if(!s->bsp_runtime){memset(&entities,0,sizeof(entities));entities.user=s;entities.validate=session_bsp_validate;
  memset(&fields,0,sizeof(fields));fields.user=s;fields.classify=session_bsp_classify;
  s->bsp_runtime=gsc_runtime_new_bsp_constructor(s->program,session_fog,s,&entities,&fields,e,n);
  if(s->bsp_runtime&&s->execution_extensions&&!gsc_runtime_select_owned_state_arena(s->bsp_runtime,GSC_STATE_FIELD_OWNED_LIMIT)){s->bsp_scope=NULL;return session_failed(s,"BSP shared state arena selection refused",e,n);}
 }
 if(!s->bsp_runtime){s->bsp_scope=NULL;return session_failed(s,"BSP construction runtime allocation refused",e,n);}
 native.user=&c;native.commit_fields=session_bsp_commit;native.prepare=session_bsp_prepare;
 ok=gsc_construct_bsp_entity(s->bsp_runtime,&c.request,&native,out,e,n);
 if(ok&&!session_bsp_current(&c))ok=0;s->bsp_scope=NULL;
 if(!ok)return session_failed(s,e&&n&&e[0]?e:"BSP construction failed",e,n);
 s->bsp_entries[s->bsp_count++]=c.request.entry;s->busy=0;return 1;
}
