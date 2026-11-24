int gsc_session_select_bullet_trace(GscSession*s,const GscBulletTraceHost*input,char*error,size_t cap){
 GscBulletTraceHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"bulletTrace requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->bullet_trace_enabled||!s->execution_extensions||!s->root_map_enabled||!h.query)
  return session_leave_rejected(s,"bulletTrace requires idle extended map Session",error,cap);
 s->bullet_trace=h;s->bullet_trace_enabled=1;s->busy=0;return 1;
}
static int session_bullet_trace(void*user,const GscBulletTraceRequest*input,GscBulletTraceReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscBulletTraceHost host;GscBulletTraceRequest q,expected;GscBulletTraceReply result;int ok;
 if(!s||!input||!out||!current||(cap&&!error)||!s->bullet_trace_enabled||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;expected=q;lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->bullet_trace;
 if(!session_player_current(&lease)||q.now_ms!=s->view.logical_ms||q.program!=gsc_runtime_program_identity(s->runtime))return 0;
 memset(&result,0xff,sizeof(result));ok=host.query(host.user,&q,&result,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease)||q.now_ms!=s->view.logical_ms||q.program!=gsc_runtime_program_identity(s->runtime)||memcmp(&host,&s->bullet_trace,sizeof(host))||memcmp(&q,&expected,sizeof(q))||memcmp(input,&expected,sizeof(q)))return 0;
 *out=result;return 1;
}
