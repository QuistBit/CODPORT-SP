/* Explicit pre-prepare selection. No profile enables this implicitly. */
int gsc_session_select_r239_mixed_scalar_equality(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"R239 mixed equality requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    s->r239_mixed_scalar_equality_enabled)
  return session_leave_rejected(s,"R239 mixed equality requires idle EMPTY Session",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"R239 mixed equality selection cancelled",error,cap);
  return session_leave_rejected(s,"R239 mixed equality resources retired",error,cap);
 }
 s->r239_mixed_scalar_equality_enabled=1;s->busy=0;return 1;
}
int gsc_session_r239_mixed_scalar_equality_selected(const GscSession*s){
 return s&&s->r239_mixed_scalar_equality_enabled;
}
