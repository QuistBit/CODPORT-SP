/* Exact stack-facade service, independent node map/cache/resource authority.
 * busy stays set for the entire operation; the old generic graph APIs refuse. */
int gsc_session_actor_graph_current(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row){
 GscRuntime*r;GscProgram*p;int ok;
 if(!s||!row||session_animation_namespace_private(s,row,sizeof(*row))||!id||!receipt||
    !s->busy||!s->actor_bootstrap_running||s->actor_construct_running||!s->actor_graph_query||
    s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.logical_ms||
    id!=s->view.session_id||id!=s->node_graph_id||receipt!=s->node_graph_receipt||
    s->node_graph_pin!=receipt||s->node_graph_pin_id!=id||!s->node_graph_retire||
    !s->node_scripts_enabled||s->node_script_phase!=3||!s->runtime||!s->program||
    !session_actor_ref_same(row,&s->actor_graph_row)||
    s->actor_graph_program!=gsc_runtime_program_identity(s->runtime))return 0;
 if(s->actor_graph_check_busy){s->abort_requested=1;session_terminal(s,"placed graph owner check reentered");return 0;}
 r=s->runtime;p=s->program;s->actor_graph_check_busy=1;
 ok=session_actor_map_current(s)&&session_actor_program_current(s)&&session_node_scripts_current(s);
 s->actor_graph_check_busy=0;
 return ok&&!s->abort_requested&&!s->view.error[0]&&s->runtime==r&&s->program==p&&
  s->busy&&s->actor_bootstrap_running&&s->actor_graph_query&&s->view.state==GSC_SESSION_RUNNING&&
  id==s->view.session_id&&id==s->node_graph_id&&receipt==s->node_graph_receipt&&
  s->node_graph_pin==receipt&&s->node_graph_pin_id==id&&session_actor_ref_same(row,&s->actor_graph_row)&&
  s->actor_graph_program==gsc_runtime_program_identity(r)&&!gsc_runtime_error(r)[0];
}
int gsc_session_actor_graph_map_matches(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,const GscMapExecutionCertificate*map){
 GscMapExecutionCertificate copy;
 if(!s||!map||session_animation_namespace_private(s,map,sizeof(*map)))return 0;copy=*map;
 return copy.receipt==s->node_script_selection.map_certificate.receipt&&copy.current==s->node_script_selection.map_certificate.current&&
  gsc_session_actor_graph_current(s,id,receipt,row);
}
int gsc_session_actor_graph_script_current(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,GscNodeScriptRef ref){
 return gsc_session_actor_graph_current(s,id,receipt,row)&&gsc_runtime_node_script_current(s->runtime,ref)&&
  gsc_session_actor_graph_current(s,id,receipt,row);
}
static int session_actor_node_target(void*context,const GscEntityRef*input,const char*name,GscActorBootstrapNodeTarget*out,char*error,size_t cap){
 SessionActorBootstrapLease*l=context;GscSession*s;GscEntityRef row,owned;GscActorBootstrapNodeTarget value;
 char text[128],expected[128],original[128];uint64_t receipt;int ok,called=0,finished;
 if(!session_actor_scope(l,1))return 0;s=l->session;
 if(!input||!name||!out||!session_actor_external(l,input,sizeof(*input))||!session_actor_external(l,name,128)||
    !session_actor_external(l,out,sizeof(*out))||!session_actor_external(l,error,cap)||
    session_actor_overlap(out,sizeof(*out),input,sizeof(*input))||session_actor_overlap(out,sizeof(*out),name,128)||
    session_actor_overlap(out,sizeof(*out),error,cap)||session_actor_overlap(error,cap,input,sizeof(*input))||
    session_actor_overlap(error,cap,name,128))return session_actor_bad_service(l,NULL,0,"placed node query aliases private or borrowed storage");
 if(!l->split||l->construct_only||l->failed||l->service_busy||!l->hooks_returned||!l->prepared||l->prepared>l->constructed||
    (!s->actor_bootstrap.node_target||!s->actor_bootstrap.node_target_end)||s->node_graph_pin||s->actor_graph_query||!session_actor_name(name,text))
  return session_actor_bad_service(l,error,cap,"placed node query outside returned split row");
 row=*input;owned=row;memcpy(expected,text,sizeof(text));
 if(!session_actor_ref_same(&row,&l->constructed_entries[l->prepared-1].entity))return session_actor_bad_service(l,error,cap,"placed node query row changed");
 receipt=s->node_graph_receipt;if(!receipt||s->node_graph_id!=l->id)return session_actor_bad_service(l,error,cap,"placed target requires this Session's published graph");
 l->service_busy=1;s->node_graph_pin=receipt;s->node_graph_pin_id=l->id;
 s->actor_graph_query=1;s->actor_graph_row=row;s->actor_graph_program=gsc_runtime_program_identity(l->runtime);
 memset(&value,0,sizeof(value));
 ok=gsc_session_actor_graph_current(s,l->id,receipt,&row)&&
  (called=1,s->actor_bootstrap.node_target(s->actor_bootstrap.user,s,l->id,receipt,&row,text,&value))==1&&
  !memcmp(text,expected,sizeof(text))&&session_actor_name(name,original)&&!memcmp(original,expected,sizeof(original))&&session_actor_scope(l,1)&&
  gsc_session_actor_graph_current(s,l->id,receipt,&row)&&session_actor_ref_same(&row,input);
 /* This paired finish invokes NO callbacks. GAME retains its busy/cancelled
  * operation through the preceding map/program/cache checks. No authority
  * callback may follow finish and precede copying the owned DATA result. */
 finished=called?s->actor_bootstrap.node_target_end(s->actor_bootstrap.user,s,l->id,receipt,&owned,ok):0;
 ok=ok&&finished==1&&!s->abort_requested&&!s->view.error[0]&&s->runtime==l->runtime&&
  s->view.session_id==l->id&&s->actor_graph_query&&s->busy&&s->actor_bootstrap_running;
 s->actor_graph_query=0;s->actor_graph_program=0;memset(&s->actor_graph_row,0,sizeof(s->actor_graph_row));
 s->node_graph_pin=s->node_graph_pin_id=0;l->service_busy=0;
 if(!ok||(value.status!=1&&value.status!=2))return session_actor_bad_service(l,error,cap,"placed graph target owner retired or query failed");
 *out=value;return 1;
}
