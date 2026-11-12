/* R211 exactly three copied catalogs and one genuine GAME bank certificate.
 * No query/effect provider, native ID, or borrowed catalog is published. */
static int session_animation_namespace_overlap(const void *a,size_t na,
    const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int session_animation_namespace_private(const GscSession *s,
    const void *p,size_t n) {
    return s&&(r243_critical_session_private(p,n)||r243_command_session_private(p,n)||r243_move_delta_session_private(p,n)||r243_actor_float_session_private(p,n)||r243_stance_session_private(p,n)||r243_animtime_session_private(p,n)||session_animation_namespace_overlap(p,n,s,sizeof(*s))||session_source_private(s,p,n)||
        session_animation_namespace_overlap(p,n,s->turret_program_bytes[0],s->turret_program_sizes[0])||
        session_animation_namespace_overlap(p,n,s->turret_program_bytes[1],s->turret_program_sizes[1])||
        session_animation_namespace_overlap(p,n,s->turret_program_bytes[2],s->turret_program_sizes[2])||
        session_animation_namespace_overlap(p,n,s->turret_program_bytes[3],s->turret_program_sizes[3]));
}
static int session_animation_namespace_certificate(GscSession *s,
    const GscMapExecutionCertificate *input) {
    GscMapExecutionCertificate copy;GscRuntime *runtime;uint64_t id;
    int state,now,ok;
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!input)return 0;
    if(s->animation_namespace_map_busy) {
        s->abort_requested=1;session_terminal(s,"animation namespace certificate reentered");return 0;
    }
    copy=*input;if(!copy.receipt||!copy.current)return 0;
    runtime=s->runtime;id=s->view.session_id;state=s->view.state;now=s->view.logical_ms;
    s->animation_namespace_map_busy=1;
    ok=copy.current(copy.receipt);
    s->animation_namespace_map_busy=0;
    return ok==1&&s->busy&&!s->abort_requested&&!s->view.error[0]&&
        s->runtime==runtime&&s->view.session_id==id&&s->view.state==state&&
        s->view.logical_ms==now&&input->receipt==copy.receipt&&input->current==copy.current;
}
static int session_animation_namespace_map_current(GscSession *s) {
    int n;
    if(!s)return 0;
    if(!s->animation_namespace_enabled)return 1;
    for(n=0;n<(s->training_rigs?4:s->turret_animations_enabled?2:3);n++)if(!s->animation_namespace_catalogs[n])return 0;
    if(s->turret_program_enabled){
        if(!session_turret_program_current(s))return 0;
        if(!s->turret_program_admitted)return !s->runtime&&
            (s->view.state==GSC_SESSION_EMPTY||s->view.state==GSC_SESSION_COMPILED);
    }
    return session_animation_namespace_certificate(s,&s->animation_namespace_map);
}
/* Earlier selected scopes keep their original callback count/order. */
static int session_animation_resources_current(GscSession *s) {
    return session_start_origin_map_current(s)&&session_animation_namespace_map_current(s)&&
        (!s->developer_selected||(s->turret_program_enabled&&!s->turret_program_admitted)||session_animation_namespace_certificate(s,&s->developer.certificate));
}
static void session_animation_namespace_destroy(GscSession *s) {
    int n;
    if(!s)return;
    for(n=0;n<4;n++) {
        gsc_animation_catalog_free(s->animation_namespace_catalogs[n]);
        s->animation_namespace_catalogs[n]=NULL;
    }
    memset(&s->animation_namespace_map,0,sizeof(s->animation_namespace_map));
    s->animation_namespace_enabled=0;
    for(n=0;n<4;n++){GSC_SESSION_FREE(s->turret_program_bytes[n]);s->turret_program_bytes[n]=NULL;s->turret_program_sizes[n]=0;}
    s->training_rigs=0;memset(&s->training_rig_map,0,sizeof(s->training_rig_map));
    s->turret_program_enabled=0;
}
int gsc_session_select_r211_animation_namespace(GscSession *s,
    const GscSessionAnimationNamespaceSelection *input,char *error,size_t cap) {
    static const char *const names[3]={"generic_human",
        "animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"};
    GscSessionAnimationNamespaceSelection copy;
    GscAnimationCatalog *catalogs[3]={NULL,NULL,NULL};
    char why[GSC_SESSION_ERROR_CAP];void *bytes=NULL;int n,ok=0;
    if((cap&&!error)||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_private(s,input,sizeof(*input))||
       session_animation_namespace_overlap(error,cap,input,sizeof(*input)))return 0;
    if(!s||!input)return session_reject(error,cap,"animation namespace requires Session and catalog inputs");
    memcpy(&copy,input,sizeof(copy));
    for(n=0;n<3;n++) {
        if(session_animation_namespace_private(s,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size)||
           session_animation_namespace_overlap(error,cap,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size)||
           session_animation_namespace_overlap(input,sizeof(*input),copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size))return 0;
    }
    if(!session_enter(s,error,cap))return 0;
    if(!s->host.enabled||!s->dynamic_randomfloat_enabled||s->view.state!=GSC_SESSION_EMPTY||
       s->program||s->runtime||s->animation_namespace_enabled||s->start_origin_enabled)
        return session_leave_rejected(s,"animation namespace requires idle EMPTY mode40 and exclusive immutable selection",error,cap);
    if(!copy.map_certificate.receipt||!copy.map_certificate.current)
        return session_leave_rejected(s,"animation namespace requires genuine map certificate",error,cap);
    for(n=0;n<3;n++)if(!copy.catalogs[n].atr_bytes||!copy.catalogs[n].atr_size||
       copy.catalogs[n].atr_size>GSC_SESSION_MAX_SOURCE)
        return session_leave_rejected(s,"animation namespace requires three bounded ATR byte inputs",error,cap);
    why[0]=0;
    for(n=0;n<3;n++) {
        bytes=GSC_SESSION_MALLOC(copy.catalogs[n].atr_size);
        if(!bytes)goto done;
        memcpy(bytes,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size);
        catalogs[n]=gsc_animation_catalog_parse(names[n],bytes,copy.catalogs[n].atr_size,why,sizeof(why));
        GSC_SESSION_FREE(bytes);bytes=NULL;
        if(!catalogs[n]||!gsc_animation_catalog_count(catalogs[n]))goto done;
    }
    /* All payloads and descriptor fields are owned before the first external
     * callback. The provider is a real GAME receipt, never a Session pointer. */
    if(!session_animation_namespace_certificate(s,&copy.map_certificate)||
       s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
       s->animation_namespace_enabled||s->start_origin_enabled||
       memcmp(input,&copy,sizeof(copy)))goto done;
    for(n=0;n<3;n++) {
        s->animation_namespace_catalogs[n]=catalogs[n];catalogs[n]=NULL;
    }
    s->animation_namespace_map=copy.map_certificate;
    s->animation_namespace_enabled=1;ok=1;
done:
    GSC_SESSION_FREE(bytes);
    for(n=0;n<3;n++)gsc_animation_catalog_free(catalogs[n]);
    s->busy=0;
    if(!ok)return session_reject(error,cap,why[0]?why:"animation namespace selection refused");
    return 1;
}

/* The two extra declarations come from the winning VFS before spawning.
 * Exact mounted ATR bytes must match again after real clip intake. */
int gsc_session_training_rigs(GscSession *s,const GscSessionAnimationNamespaceSelection *input,int admit,char *error,size_t cap){
 static const char *const names[2]={"animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"};
 unsigned char *bytes[2]={NULL,NULL};GscAnimationCatalog *parsed[2]={NULL,NULL};
 GscSessionAnimationNamespaceSelection copy;int i,ok=0;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 for(i=0;i<3;i++){
  const void *p=copy.catalogs[i].atr_bytes;size_t n=copy.catalogs[i].atr_size;
  if(!p||!n||n>GSC_SESSION_MAX_SOURCE||session_animation_namespace_private(s,p,n)||
     session_animation_namespace_overlap(input,sizeof(*input),p,n)||session_animation_namespace_overlap(error,cap,p,n))return 0;
 }
 if(!session_enter(s,error,cap))return 0;
 if(!s->turret_program_enabled||s->runtime||copy.catalogs[0].atr_size!=s->turret_program_sizes[0]||
    memcmp(copy.catalogs[0].atr_bytes,s->turret_program_bytes[0],s->turret_program_sizes[0])||
    !session_turret_program_current(s))goto done;
 if(admit){
  if(!s->training_rigs||s->view.state!=GSC_SESSION_COMPILED||!s->turret_program_admitted||
     s->training_rig_map.receipt||!session_animation_namespace_certificate(s,&copy.map_certificate))goto done;
  for(i=0;i<2;i++)if(copy.catalogs[i+1].atr_size!=s->turret_program_sizes[i+2]||
    memcmp(copy.catalogs[i+1].atr_bytes,s->turret_program_bytes[i+2],s->turret_program_sizes[i+2]))goto done;
  if(memcmp(input,&copy,sizeof(copy)))goto done;
  s->training_rig_map=copy.map_certificate;
 }else{
  if(s->training_rigs||s->view.state!=GSC_SESSION_EMPTY||s->program||s->turret_program_admitted)goto done;
  for(i=0;i<2;i++){
   size_t n=copy.catalogs[i+1].atr_size;
   bytes[i]=GSC_SESSION_MALLOC(n);if(!bytes[i])goto done;
   memcpy(bytes[i],copy.catalogs[i+1].atr_bytes,n);
   parsed[i]=gsc_animation_catalog_parse(names[i],bytes[i],n,error,cap);
   if(!parsed[i]||!gsc_animation_catalog_count(parsed[i]))goto done;
  }
  if(memcmp(input,&copy,sizeof(copy))||!session_turret_program_current(s))goto done;
  for(i=0;i<2;i++){s->animation_namespace_catalogs[i+2]=parsed[i];parsed[i]=NULL;
   s->turret_program_sizes[i+2]=copy.catalogs[i+1].atr_size;s->turret_program_bytes[i+2]=bytes[i];bytes[i]=NULL;}
  s->training_rigs=1;
 }
 ok=1;
done:
 for(i=0;i<2;i++){GSC_SESSION_FREE(bytes[i]);gsc_animation_catalog_free(parsed[i]);}
 s->busy=0;return ok?1:session_reject(error,cap,"Training rig declaration or mounted bytes unavailable");
}
