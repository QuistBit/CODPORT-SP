int gsc_session_select_r232_function_admission(GscSession *s,char *error,size_t cap) {
    if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
    if(!s)return session_reject(error,cap,"function admission requires Session");
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||s->function_admission_selected)
        return session_leave_rejected(s,"function admission requires idle selected two-ATR configuration",error,cap);
    if(!session_animation_resources_current(s)||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->function_admission_selected)
        return session_leave_rejected(s,"function admission selection expired",error,cap);
    s->function_admission_selected=1;s->busy=0;return 1;
}
