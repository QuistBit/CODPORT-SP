/* A map-only entry needs no fabricated animation namespace or actor cohort. */
int gsc_session_select_root_map(GscSession*s,const GscMapExecutionCertificate*input,char*error,size_t cap){
 GscMapExecutionCertificate copy;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"map root requires Session/certificate");copy=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->root_map_enabled||s->actor_bootstrap_enabled||!copy.receipt||!copy.current)
  return session_leave_rejected(s,"map root requires idle Session and genuine authority",error,cap);
 s->root_map=copy;s->root_map_enabled=1;s->busy=0;return 1;
}
typedef struct {GscSession*s;GscRuntime*r;uint64_t id;GscMapExecutionCertificate map;}SessionRootMapLease;
static int session_root_map_start_current(void*u){SessionRootMapLease*l=u;GscSession*s=l->s;
 return s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&s->runtime==l->r&&s->root_map_enabled&&!s->actor_bootstrap_enabled&&!memcmp(&s->root_map,&l->map,sizeof(l->map));
}

int gsc_session_select_map_source_float(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->root_map_enabled||!s->root_map.receipt||!s->root_map.current||s->actor_bootstrap_enabled||s->map_source_float_enabled||s->r232_source_float_enabled||s->random_range_enabled||!s->execution_extensions||!s->dynamic_randomfloat_enabled||!s->randomfloat_enabled||!s->randomfloat.draw)
  return session_leave_rejected(s,"map arithmetic requires idle map-certified Session and shared RNG",error,cap);
 s->map_source_float_enabled=1;s->busy=0;return 1;
}
