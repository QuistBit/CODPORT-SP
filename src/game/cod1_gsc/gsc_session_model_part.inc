int gsc_session_select_model_part(GscSession*s,const GscModelPartHost*input,char*error,size_t cap){
 GscModelPartHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"model part selection requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->model_part_enabled||!s->names_enabled||!s->scalar_switch_enabled||!h.query)
  return session_leave_rejected(s,"model part requires idle named scalar Session",error,cap);
 if(memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"model part provider changed",error,cap);
 s->model_part=h;s->model_part_enabled=1;s->busy=0;return 1;
}
static int session_model_part(void*user,const GscModelPartRequest*input,char out[128],char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscModelPartHost host;GscModelPartRequest q,expected;char result[128];int ok;
 if(!s||!input||!out||!current||!s->model_part_enabled||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,128)||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,128)||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,128,error,cap))return 0;
 q=*input;expected=q;if(!memchr(q.model,0,128))return 0;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->model_part;
 if(!session_player_current(&lease)||q.now_ms!=s->view.logical_ms)return 0;
 memset(result,0xff,sizeof(result));
 ok=host.query(host.user,&q,result,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease)||q.now_ms!=s->view.logical_ms||memcmp(&host,&s->model_part,sizeof(host))||memcmp(&q,&expected,sizeof(q))||memcmp(input,&expected,sizeof(q))||!memchr(result,0,128)||!result[0])return 0;
 memcpy(out,result,128);return 1;
}
