/* EMPTY-only opt-in; existing profiles remain deferred until selected. */
int gsc_session_select_configured_precache_interval(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"configured precache requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    !s->r243_thread_start_enabled||!s->precache_string_enabled||s->configured_precache_interval_enabled)
  return session_leave_rejected(s,"configured precache requires idle EMPTY string/R243 Session",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"configured precache selection cancelled",error,cap);
  return session_leave_rejected(s,"configured precache resources retired",error,cap);
 }
 s->configured_precache_interval_enabled=1;s->busy=0;return 1;
}
int gsc_session_select_r243_thread_start(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"R243 thread start requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    !s->r237_frame_buckets_enabled||s->r243_thread_start_enabled)
  return session_leave_rejected(s,"R243 thread start requires idle EMPTY frame Session",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"R243 thread start selection cancelled",error,cap);
  return session_leave_rejected(s,"R243 thread start resources retired",error,cap);
 }
 s->r243_thread_start_enabled=1;s->busy=0;return 1;
}
int gsc_session_r243_thread_start_selected(const GscSession*s){return s&&s->r243_thread_start_enabled;}
int gsc_session_select_r243_thread_entities(GscSession*s,const GscThreadEntityAuthorityHost*input,char*error,size_t cap){
 GscThreadEntityAuthorityHost copy;
 if((cap&&!error)||session_r237_frame_private(s,input,sizeof(*input))||session_r237_frame_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"R243 thread entities require Session and real host");
 copy=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->r243_thread_start_enabled||
    s->r243_thread_entities_enabled||!copy.resolve)
  return session_leave_rejected(s,"R243 thread entities require idle EMPTY selected Session",error,cap);
 if(!session_r237_frame_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
  return session_leave_rejected(s,"R243 thread entity selection expired",error,cap);
 s->r243_thread_entities=copy;s->r243_thread_entities_enabled=1;s->busy=0;return 1;
}
static int session_r243_thread_resolve(void*user,const GscEntityRef*input,
 GscEntityExecutionCertificate*out,char*error,size_t cap){
 GscSession*s=user;GscRuntime*runtime;uint64_t id;int now,ok;GscEntityRef ref,borrowed;
 GscThreadEntityAuthorityHost host;GscEntityExecutionCertificate certificate;
 if(!s||!input||!out||!s->busy||!s->runtime||s->abort_requested||
    s->view.state!=GSC_SESSION_RUNNING||!s->r243_thread_entities_enabled)return 0;
 ref=*input;borrowed=ref;host=s->r243_thread_entities;runtime=s->runtime;id=s->view.session_id;now=s->view.logical_ms;
 if(!session_r237_frame_resources_current(s))return 0;
 memset(&certificate,0,sizeof(certificate));ok=host.resolve(host.user,&borrowed,&certificate,error,cap);
 if(ok!=1||!s->busy||s->runtime!=runtime||s->view.session_id!=id||s->view.logical_ms!=now||
    s->view.state!=GSC_SESSION_RUNNING||s->abort_requested||!s->r243_thread_entities_enabled||
    memcmp(&host,&s->r243_thread_entities,sizeof(host))||memcmp(&ref,&borrowed,sizeof(ref))||
    !session_r237_frame_resources_current(s))return 0;
 *out=certificate;return 1;
}
