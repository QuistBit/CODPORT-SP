/* Native implicit source dependency, deliberately separate from method host. */
int gsc_session_select_r243_scripted_init_dependency(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"scripted init dependency requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    !s->modules_enabled||!s->function_admission_selected||!s->r243_thread_start_enabled||
    s->r243_scripted_init_dependency_enabled)
  return session_leave_rejected(s,"scripted init dependency requires idle EMPTY thread/function-admission Session",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"scripted init dependency selection cancelled",error,cap);
  return session_leave_rejected(s,"scripted init dependency resources retired",error,cap);
 }
 s->r243_scripted_init_dependency_enabled=1;s->busy=0;return 1;
}
int gsc_session_r243_scripted_init_dependency_selected(const GscSession*s){
 return s&&s->r243_scripted_init_dependency_enabled;
}
