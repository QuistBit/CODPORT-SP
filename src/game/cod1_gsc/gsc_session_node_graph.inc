/* Port-owned dependency lifetime, not a native graph/VM capability. The
 * receipt is opaque to the VM. Its retire callback must only release/cancel
 * that exact GAME owner; it must not call back into a Session. */
int gsc_session_node_graph_begin(GscSession*s,uint64_t id,uint64_t receipt,void(*retire)(uint64_t)){
 if(!s||!receipt||!retire||s->node_graph_pin||!session_node_script_query_begin(s,id))return 0;
 /* Pin object lifetime across synchronous GAME acquisition. Replacement or
  * unload may retire its runtime, but destroy cannot free the caller's s. */
 s->node_graph_pin=receipt;s->node_graph_pin_id=id;
 session_node_graph_retire(s);
 if(!session_node_scripts_current(s))return session_failed(s,"node graph acquisition map retired",NULL,0);
 s->node_graph_receipt=receipt;s->node_graph_id=id;s->node_graph_retire=retire;
 s->busy=0;return 1;
}
int gsc_session_node_graph_current(const GscSession*s,uint64_t id,uint64_t receipt){
 return s&&id&&receipt&&id==s->view.session_id&&id==s->node_graph_id&&
  receipt==s->node_graph_receipt&&s->node_graph_retire&&!s->busy&&!s->abort_requested&&!s->view.error[0]&&
  s->runtime&&s->node_script_phase==3&&
  (s->view.state==GSC_SESSION_PREPARED||s->view.state==GSC_SESSION_RUNNING||s->view.state==GSC_SESSION_COMPLETED);
}
int gsc_session_node_graph_check_begin(GscSession*s,uint64_t id,uint64_t receipt){
 if(!gsc_session_node_graph_current(s,id,receipt)||s->node_graph_pin)return 0;
 s->node_graph_pin=receipt;s->node_graph_pin_id=id;return 1;
}
int gsc_session_node_graph_end(GscSession*s,uint64_t id,uint64_t receipt,int keep){
 int valid;
 if(!s||!receipt||s->node_graph_pin!=receipt||s->node_graph_pin_id!=id||s->busy)return 0;
 valid=keep==1&&gsc_session_node_graph_current(s,id,receipt);
 s->node_graph_pin=s->node_graph_pin_id=0;
 if(!valid){if(s->node_graph_receipt==receipt)session_node_graph_retire(s);return 0;}
 /* No callbacks between the final validation and GAME ownership transfer. */
 return 1;
}
