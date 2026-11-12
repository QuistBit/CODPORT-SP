int gsc_session_select_r232_quoted_strings(GscSession *s,char *error,size_t cap) {
    if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
    if(!s)return session_reject(error,cap,"native quoted strings require Session");
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
       !s->turret_animations_enabled||s->quoted_strings_enabled)
        return session_leave_rejected(s,"native quoted strings require idle two-ATR Session",error,cap);
    if(!session_animation_resources_current(s)||s->view.state!=GSC_SESSION_EMPTY||
       s->program||s->runtime||s->quoted_strings_enabled)
        return session_leave_rejected(s,"native quoted-string resource scope retired",error,cap);
    s->quoted_strings_enabled=1;s->busy=0;return 1;
}
