/* Explicit pre-prepare selection. No profile enables this implicitly. */
int gsc_session_select_r238_array_value_cow(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"R238 array COW requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    !s->arrays_enabled||!s->sparse_numeric_arrays_enabled||s->r238_array_value_cow_enabled)
  return session_leave_rejected(s,"R238 array COW requires idle selected sparse arrays",error,cap);
 if(!session_r237_frame_resources_current(s)){
  if(s->abort_requested)return session_failed(s,"R238 array COW selection cancelled",error,cap);
  return session_leave_rejected(s,"R238 array COW resources retired",error,cap);
 }
 s->r238_array_value_cow_enabled=1;s->busy=0;return 1;
}
int gsc_session_r238_array_value_cow_selected(const GscSession*s){
 return s&&s->r238_array_value_cow_enabled;
}
