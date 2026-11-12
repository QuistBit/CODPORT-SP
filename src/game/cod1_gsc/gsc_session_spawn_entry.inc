/* Factory prepare is already inside the Session's synchronous run scope.
 * This service never enters a second scheduler or exposes the runtime. */
int gsc_session_spawn_entry_storage_separate(const GscSession *s,const void *p,size_t n) {
    return s&&(!n||p)&&!session_animation_namespace_private(s,p,n)&&
        (!s->runtime||gsc_runtime_storage_separate(s->runtime,p,n));
}
uint64_t gsc_session_spawn_entry_program(const GscSession *s,uint64_t id,const GscEntityRef *source) {
    if(!s||!source||!gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
       !id||id!=s->view.session_id||!s->busy||!s->runtime||!s->actor_spawn_preparing||
       !s->actor_spawners_enabled||s->abort_requested||s->view.error[0]||
       s->view.state!=GSC_SESSION_RUNNING)return 0;
    return gsc_projected_spawn_entry_program(s->runtime,source);
}
int gsc_session_execute_projected_spawn_entry(GscSession *s,uint64_t id,
    const GscInheritedEntityEntry *q,const GscProjectedEntityEntryHost *h,
    GscProjectedEntityEntryRun *out,char *error,size_t cap) {
    int ok;
    if(!s||!q||!h||!out||
       !gsc_session_spawn_entry_storage_separate(s,q,sizeof(*q))||
       !gsc_session_spawn_entry_storage_separate(s,h,sizeof(*h))||
       !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
       !gsc_session_spawn_entry_storage_separate(s,error,cap)||
       session_animation_namespace_overlap(q,sizeof(*q),out,sizeof(*out))||
       session_animation_namespace_overlap(h,sizeof(*h),out,sizeof(*out))||
       session_animation_namespace_overlap(error,cap,q,sizeof(*q))||
       session_animation_namespace_overlap(error,cap,h,sizeof(*h))||
       session_animation_namespace_overlap(error,cap,out,sizeof(*out)))return 0;
    if(!id||id!=s->view.session_id||!s->busy||!s->runtime||
       !s->actor_spawn_preparing||!s->actor_spawners_enabled||
       s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING)
        return session_reject(error,cap,"projected spawn entry requires current Session prepare");
    ok=gsc_execute_projected_spawn_entry(s->runtime,q,h,out,error,cap);
    if(s->abort_requested||s->view.error[0])
        return session_reject(error,cap,"projected spawn entry cancelled with Session");
    return ok;
}

uint64_t gsc_session_spawn_issued_owner_program(const GscSession *s,uint64_t id,
    const GscEntityRef *source,const GscEntityRef *owner,uint64_t serial) {
    if(!s||!source||!owner||
       !gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
       !gsc_session_spawn_entry_storage_separate(s,owner,sizeof(*owner))||
       !id||id!=s->view.session_id||!s->busy||!s->runtime||!s->actor_spawn_preparing||
       !s->actor_spawners_enabled||s->abort_requested||s->view.error[0]||
       s->view.state!=GSC_SESSION_RUNNING)return 0;
    return gsc_spawn_issued_owner_program(s->runtime,source,owner,serial);
}

int gsc_session_execute_spawn_init(GscSession*s,uint64_t id,const GscEntityRef*source,
 const GscEntityRef*owner,uint64_t serial,GscEntityScriptRun*out,char*error,size_t cap){
 GscEntityRef src,dst;int ok;
 if(!s||!source||!owner||!out||
    !gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,owner,sizeof(*owner))||
    !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
    !gsc_session_spawn_entry_storage_separate(s,error,cap)||
    session_animation_namespace_overlap(source,sizeof(*source),out,sizeof(*out))||
    session_animation_namespace_overlap(owner,sizeof(*owner),out,sizeof(*out))||
    session_animation_namespace_overlap(error,cap,source,sizeof(*source))||
    session_animation_namespace_overlap(error,cap,owner,sizeof(*owner))||
    session_animation_namespace_overlap(error,cap,out,sizeof(*out)))return 0;
 src=*source;dst=*owner;
 if(!gsc_session_spawn_issued_owner_program(s,id,&src,&dst,serial)||!s->script_roots_enabled||
    !session_script_roots_current(s)||!session_script_roots_cache_current(s))
    return session_reject(error,cap,"spawn INIT requires current Session global-root cache and owner");
 ok=gsc_execute_spawn_init(s->runtime,&src,&dst,serial,out,error,cap);
 if(s->abort_requested||s->view.error[0])return session_reject(error,cap,"spawn INIT cancelled with Session");
 return ok;
}

uint64_t gsc_session_spawn_finish_program(const GscSession*s,uint64_t id,
 const GscActorSpawnTicket*t,int published){
 if(!s||!t||!id||id!=s->view.session_id||!s->busy||!s->runtime||
    !s->actor_spawners_enabled||!s->actor_spawn_finishing||
    s->actor_spawn_finish_published!=published||
    !gsc_session_spawn_entry_storage_separate(s,t,sizeof(*t))||
    memcmp(t,s->actor_spawn_finishing,sizeof(*t))||s->view.state!=GSC_SESSION_RUNNING)return 0;
 return gsc_actor_spawn_finish_program(s->runtime,t,published);
}

uint64_t gsc_session_spawn_prepare_program(const GscSession*s,uint64_t id,const GscActorSpawnRequest*q){
 if(!s||!q||!id||id!=s->view.session_id||!s->busy||!s->runtime||!s->actor_spawners_enabled||
    !s->actor_spawn_preparing||!s->actor_spawn_prepare_request||s->abort_requested||s->view.error[0]||
    s->view.state!=GSC_SESSION_RUNNING||!gsc_session_spawn_entry_storage_separate(s,q,sizeof(*q))||
    memcmp(q,s->actor_spawn_prepare_request,sizeof(*q)))return 0;
 return gsc_actor_spawn_prepare_program(s->runtime,q);
}

/* Delivery/cancellation scope is proof of what already happened; health is
 * separate so a late abort still consumes its actual published attempt. */
uint64_t gsc_session_spawn_commit_program(const GscSession*s,uint64_t id,const GscActorSpawnTicket*t){
 uint64_t program=gsc_session_spawn_finish_program(s,id,t,1);
 if(!program||s->abort_requested||s->view.error[0])return 0;
 return gsc_actor_spawn_commit_program(s->runtime,t)==program?program:0;
}
