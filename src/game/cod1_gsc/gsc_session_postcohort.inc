/* Distinct busy graph lease. Ordinary graph/query APIs keep refusing busy. */
static int session_postcohort_exact(SessionActorBootstrapLease*l,GscRuntime*r,uint64_t id,uint32_t rows){
 GscSession*s=l?l->session:NULL;
 return l&&s&&l==session_cohort_active&&l->active&&!l->failed&&l->late_attempted&&l->service_busy&&
  l->cohort_vm_kind==3&&l->cohort_vm_active&&!l->cohort_vm_pending&&l->runtime==r&&l->id==id&&
  s->runtime==r&&s->program&&s->view.session_id==id&&s->busy&&s->actor_bootstrap_running&&!s->actor_construct_running&&
  s->view.state==GSC_SESSION_RUNNING&&!s->view.logical_ms&&!s->abort_requested&&!s->view.error[0]&&
  s->actor_bootstrap.placed_cohort&&rows==s->actor_bootstrap.placed_count&&rows&&
  l->prepared==rows&&l->constructed==rows&&l->placed_init_attempted==(int)rows&&l->pop_row==rows-1&&
  l->pop_captured&&l->pop_returned&&l->pop_done&&!l->pop_active&&!l->pop_collecting&&!l->post_current&&l->hooks_returned&&
  l->facade&&!memcmp(l->facade,&l->facade_seal,sizeof(l->facade_seal));
}
int gsc_session_postcohort_scope(const void*a,GscRuntime*r,uint64_t id,uint32_t rows){
 SessionActorBootstrapLease*l=session_cohort_active;return a==l&&session_postcohort_exact(l,r,id,rows);
}
int gsc_session_postcohort_admit(const void*a,GscRuntime*r,uint64_t id,uint32_t rows){
 SessionActorBootstrapLease*l=session_cohort_active;
 if(!l||a!=l)return 0;
 if(!l->cohort_vm_pending||l->cohort_vm_active||l->cohort_vm_kind!=3)return session_actor_bad_service(l,NULL,0,"post-cohort replay or reentry");
 l->cohort_vm_pending=0;l->cohort_vm_active=1;
 return session_postcohort_exact(l,r,id,rows)?1:session_actor_bad_service(l,NULL,0,"post-cohort differs from complete native returned plan");
}
static int session_postcohort_pure(SessionActorBootstrapLease*l){GscSession*s=l?l->session:NULL;
 return s&&session_postcohort_exact(l,l->runtime,l->id,l->constructed)&&l->late_invoked&&l->late_graph_active&&
  l->late_current&&l->late_current(l->late_context)==1&&gsc_placed_postcohort_current(l->runtime,l->id)&&
  l->late_receipt&&s->node_graph_id==l->id&&s->node_graph_receipt==l->late_receipt&&
  s->node_graph_pin==l->late_receipt&&s->node_graph_pin_id==l->id&&s->node_graph_retire&&
  s->node_scripts_enabled&&s->node_script_phase==3&&!s->actor_graph_query;
}
int gsc_session_postcohort_graph_current(GscSession*s,uint64_t id,uint64_t receipt){
 SessionActorBootstrapLease*l=session_cohort_active;GscRuntime*r;GscProgram*p;int ok;
 if(!l||s!=l->session||id!=l->id||receipt!=l->late_receipt||!session_postcohort_pure(l))return 0;
 if(l->late_checking)return session_actor_bad_service(l,NULL,0,"post-cohort graph current reentered");
 r=s->runtime;p=s->program;l->late_checking=1;
 ok=session_actor_map_current(s)&&session_postcohort_pure(l)&&session_actor_program_current(s)&&
  session_postcohort_pure(l)&&session_node_scripts_current(s)&&session_postcohort_pure(l);
 l->late_checking=0;return ok&&s->runtime==r&&s->program==p&&session_postcohort_pure(l);
}
int gsc_session_postcohort_native_scope(GscSession*s,uint64_t id,uint64_t receipt,const void*a,int step){
 SessionActorBootstrapLease*l=session_cohort_active;
 if(!l||a!=l||s!=l->session||id!=l->id||receipt!=l->late_receipt||l->late_native_call!=step||
  !l->active||!l->late_attempted||!l->late_invoked||!l->service_busy||!l->cohort_vm_active||l->cohort_vm_pending||l->cohort_vm_kind!=3||
  !gsc_placed_postcohort_call_scope(l->runtime,l->id,step))return 0;
 return step==3?1:((step==1||step==2)&&session_postcohort_pure(l));
}
int gsc_session_postcohort_graph_map_matches(GscSession*s,uint64_t id,uint64_t receipt,const GscMapExecutionCertificate*map){
 GscMapExecutionCertificate copy;if(!s||!map||session_animation_namespace_private(s,map,sizeof(*map)))return 0;copy=*map;
 return copy.receipt==s->node_script_selection.map_certificate.receipt&&copy.current==s->node_script_selection.map_certificate.current&&
  gsc_session_postcohort_graph_current(s,id,receipt);
}
int gsc_session_postcohort_graph_script_current(GscSession*s,uint64_t id,uint64_t receipt,GscNodeScriptRef ref){
 return gsc_session_postcohort_graph_current(s,id,receipt)&&gsc_runtime_node_script_current(s->runtime,ref)&&gsc_session_postcohort_graph_current(s,id,receipt);
}
int gsc_session_postcohort_native_begin(const void*a,GscRuntime*r,uint64_t id,uint32_t rows,int(*current)(void*),void*ctx,char*error,size_t cap){
 SessionActorBootstrapLease*l=session_cohort_active;GscSession*s;int ok;
 if(!l||a!=l||!session_postcohort_exact(l,r,id,rows)||l->late_native_call||!gsc_placed_postcohort_call_scope(r,id,1)||l->late_invoked||l->late_done||!current||
  !gsc_placed_postcohort_current(r,id)||current(ctx)!=1)return 0;
 l->late_invoked=1;s=l->session;l->late_current=current;l->late_context=ctx;l->late_receipt=s->node_graph_receipt;
 if(!l->late_receipt||s->node_graph_id!=id||s->node_graph_pin||s->actor_graph_query)return 0;
 s->node_graph_pin=l->late_receipt;s->node_graph_pin_id=id;l->late_graph_active=1;
 ok=gsc_session_postcohort_graph_current(s,id,l->late_receipt)&&session_script_roots_current(s)&&
  session_postcohort_pure(l)&&session_script_roots_cache_current(s)&&session_postcohort_pure(l);
 #if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
 if(ok){l->late_native_call=1;ok=G_Cod1ActorPostCohortNative(s,id,l->late_receipt,l,error,cap);l->late_native_call=0;}
 #else
 /* This GAME build has no fixed native provider. Refuse, never simulate it. */
 ok=session_actor_bad_service(l,error,cap,"post-cohort native bridge is unavailable in this GAME build");
 #endif
 return ok==1&&session_postcohort_pure(l);
}
int gsc_session_postcohort_native_check(const void*a,GscRuntime*r,uint64_t id,uint32_t rows){
 SessionActorBootstrapLease*l=session_cohort_active;GscSession*s;
 if(!l||a!=l)return 0;
 if(l->late_native_call||!gsc_placed_postcohort_call_scope(r,id,2))return session_actor_bad_service(l,NULL,0,"post-cohort native check outside VM call stage");
 if(!session_postcohort_exact(l,r,id,rows)||!session_postcohort_pure(l))return 0;s=l->session;
 if(!gsc_session_postcohort_graph_current(s,id,l->late_receipt)||!session_script_roots_current(s)||
  !session_postcohort_pure(l)||!session_script_roots_cache_current(s)||!session_postcohort_pure(l))return 0;
 /* The fixed GAME check ends with the fresh complete world fence. */
 #if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
 l->late_native_call=2;{int ok=G_Cod1ActorPostCohortNativeCheck(s,id,l->late_receipt,l)==1&&session_postcohort_pure(l);l->late_native_call=0;return ok;}
 #else
 return 0;
 #endif
}
int gsc_session_postcohort_native_finish(const void*a,GscRuntime*r,uint64_t id,uint32_t rows,int keep){
 SessionActorBootstrapLease*l=session_cohort_active;GscSession*s;int ok,ended;
 if(!l||a!=l||l->runtime!=r||l->id!=id||l->cohort_vm_kind!=3||!l->late_attempted)return 0;
 if(l->late_native_call||!gsc_placed_postcohort_call_scope(r,id,3))return session_actor_bad_service(l,NULL,0,"post-cohort native finish outside VM call stage");s=l->session;
 ok=keep==1&&session_postcohort_exact(l,r,id,rows)&&session_postcohort_pure(l);
 #if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
 l->late_native_call=3;ended=G_Cod1ActorPostCohortNativeFinish(s,id,l->late_receipt,l,ok);l->late_native_call=0;
 #else
 ended=0;
 #endif
 ok=ok&&ended==1&&!s->abort_requested&&!s->view.error[0]&&s->busy&&s->runtime==r;
 if(l->late_graph_active&&s->node_graph_pin==l->late_receipt&&s->node_graph_pin_id==id)s->node_graph_pin=s->node_graph_pin_id=0;else ok=0;
 l->late_graph_active=0;l->late_current=NULL;l->late_context=NULL;l->late_done=ok;return ok;
}
static int session_actor_postcohort(void*context,char*error,size_t cap){
 SessionActorBootstrapLease*l=context;GscSession*s;int ok;
 if(!session_actor_scope(l,1))return 0;s=l->session;
 if(!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"post-cohort error buffer aliases private storage");
 if(l->construct_only||!l->split||l->failed||l->service_busy||l->late_attempted||l->late_done||session_cohort_active||
  !s->actor_bootstrap.placed_cohort||!l->hooks_returned||l->prepared!=l->constructed||l->constructed!=s->actor_bootstrap.placed_count||
  l->placed_init_attempted!=(int)l->constructed||!l->pop_done||!gsc_placed_cohort_current(l->runtime,l->id))return session_actor_bad_service(l,error,cap,"post-cohort requires every genuine INIT/native return exactly once");
 l->late_attempted=1;l->service_busy=1;l->cohort_vm_pending=1;l->cohort_vm_kind=3;session_cohort_active=l;
 ok=gsc_execute_placed_postcohort(l->runtime,l,l->id,l->constructed,error,cap);
 session_cohort_active=NULL;l->cohort_vm_pending=l->cohort_vm_active=l->cohort_vm_kind=0;l->service_busy=0;
 if(!ok||!l->late_done)return session_actor_bad_service(l,error,cap,"post-cohort late native owner phase did not finish");
 return session_actor_scope(l,1)&&gsc_placed_cohort_current(l->runtime,l->id);
}
