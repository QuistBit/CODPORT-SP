int gsc_session_select_owned_wait_delivery(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"native wait delivery requires Session");
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    s->owned_wait_delivery_enabled||!s->actor_program_enabled||!s->execution_extensions||
    !s->events_enabled||!s->event_payloads_enabled||!s->state_enabled||
    !s->results_enabled||!s->lifetime_enabled)
  return session_leave_rejected(s,"native wait delivery requires idle map-owned full execution profile",error,cap);
 s->owned_wait_delivery_enabled=1;s->busy=0;return 1;
}
