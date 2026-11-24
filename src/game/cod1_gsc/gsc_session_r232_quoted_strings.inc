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
/* The same proven lexer decoder for an independently certified map program.
 * Selection grants no Actor/turret resource or executable native capability. */
int gsc_session_select_map_quoted_strings(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
 if(!s||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->root_map_enabled||!s->animation_namespace_enabled||!s->map_animation_names.count||s->actor_bootstrap_enabled||s->turret_animations_enabled||s->quoted_strings_enabled)
  return session_leave_rejected(s,"native quoted strings require idle independent map namespace",error,cap);
 if(!session_animation_resources_current(s)||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->quoted_strings_enabled)
  return session_leave_rejected(s,"native map quoted-string selection retired",error,cap);
 s->quoted_strings_enabled=1;s->busy=0;return 1;
}
