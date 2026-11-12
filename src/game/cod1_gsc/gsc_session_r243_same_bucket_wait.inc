int gsc_session_select_r243_same_bucket_wait(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"R243 same-bucket wait requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    !s->r243_thread_start_enabled||s->r243_same_bucket_wait_enabled)
  return session_leave_rejected(s,"R243 same-bucket wait requires idle EMPTY thread Session",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"R243 same-bucket wait selection cancelled",error,cap);
  return session_leave_rejected(s,"R243 same-bucket wait resources retired",error,cap);
 }
 s->r243_same_bucket_wait_enabled=1;s->busy=0;return 1;
}
int gsc_session_r243_same_bucket_wait_selected(const GscSession*s){return s&&s->r243_same_bucket_wait_enabled;}
