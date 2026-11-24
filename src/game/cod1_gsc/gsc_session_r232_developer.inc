int gsc_session_select_r232_developer_script(GscSession *s,const GscDeveloperScriptSelection *input,char *error,size_t cap) {
    GscDeveloperScriptSelection copy;
    if((cap&&!error)||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_private(s,input,sizeof(*input))||
       session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
    if(!s||!input)return session_reject(error,cap,"developer selection requires Session and captured setting");
    copy=*input;
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->function_admission_selected||s->developer_selected||
       (copy.enabled!=0&&copy.enabled!=1)||!copy.certificate.receipt||!copy.certificate.current)
        return session_leave_rejected(s,"developer selection requires idle admitted profile and exact setting0/1",error,cap);
    if(!session_animation_resources_current(s)||!session_animation_namespace_certificate(s,&copy.certificate)||
       s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->developer_selected||memcmp(input,&copy,sizeof(copy)))
        return session_leave_rejected(s,"developer setting selection expired",error,cap);
    s->developer=copy;s->developer_selected=1;s->busy=0;return 1;
}

int gsc_session_select_map_developer_script(GscSession*s,const GscDeveloperScriptSelection*input,char*error,size_t cap){
 GscDeveloperScriptSelection copy;
 if((cap&&!error)||session_animation_namespace_private(s,error,cap)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input||!session_enter(s,error,cap))return 0;copy=*input;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->root_map_enabled||!s->animation_namespace_enabled||!s->map_animation_names.count||s->actor_bootstrap_enabled||s->turret_animations_enabled||s->function_admission_selected||s->developer_selected||(copy.enabled!=0&&copy.enabled!=1)||!copy.certificate.receipt||!copy.certificate.current)
  return session_leave_rejected(s,"developer selection requires idle independent map and captured setting0/1",error,cap);
 if(!session_animation_resources_current(s)||!session_animation_namespace_certificate(s,&copy.certificate)||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->developer_selected||memcmp(input,&copy,sizeof(copy)))
  return session_leave_rejected(s,"map developer selection retired",error,cap);
 s->developer=copy;s->developer_selected=s->map_developer_selected=1;s->busy=0;return 1;
}
