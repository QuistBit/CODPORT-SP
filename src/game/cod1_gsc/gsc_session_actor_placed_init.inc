/* Capture at actual pop; native dispatch follows returned entry cleanup.
 * No Task, VM current callback or borrowed VM stack context survives it. */
static int session_actor_pop_scope(SessionActorBootstrapLease*l){GscSession*s=l?l->session:NULL;
 return session_actor_scope(l,1)&&l->pop_active&&l->pop_captured&&l->pop_returned&&!l->pop_done&&!l->pop_collecting&&
  !l->failed&&l->service_busy&&l->placed_init_attempted&&l->split&&!l->construct_only&&l->hooks_returned&&
  l->prepared==l->pop_row+1&&l->constructed==s->actor_bootstrap.placed_count&&s->actor_bootstrap.placed_init_return&&
  l->facade&&!memcmp(l->facade,&l->facade_seal,sizeof(l->facade_seal))&&l->pop_borrowed&&
  !memcmp(l->pop_borrowed,&l->pop_seal,sizeof(l->pop_seal))&&!memcmp(&l->pop_seal.init,&l->vm_seal,sizeof(l->vm_seal))&&
  l->pop_run.status==GSC_ENTITY_ENTRY_RETURNED&&l->pop_run.task_serial==l->vm_seal.task_serial&&
  session_actor_ref_same(&l->pop_run.entity,&l->vm_seal.entity)&&session_actor_ref_same(&l->vm_seal.entity,&l->constructed_entries[l->pop_row].entity)&&
  !memcmp(&l->placed_map,&s->actor_bootstrap.map_certificate,sizeof(l->placed_map))&&
  session_actor_ref_same(&l->placed_certificate.owner,&l->vm_seal.entity)&&l->placed_certificate.serial&&l->placed_certificate.current&&
  l->pop_seal.session_id==l->id&&l->pop_seal.program_identity==gsc_runtime_program_identity(l->runtime)&&
  l->vm_seal.runtime_identity==gsc_runtime_instance_identity(l->runtime)&&s->view.origin_host_ms>=0&&s->view.logical_ms==l->vm_seal.now_ms&&
  s->view.origin_host_ms+s->view.logical_ms==l->pop_seal.host_now_ms;
}
static int session_actor_pop_current(void*ctx){SessionActorBootstrapLease*l=ctx;int ok;
 if(!session_actor_pop_scope(l)||(l->session->actor_bootstrap.placed_cohort&&(!l->post_current||!l->post_current(l->post_context))))return 0;if(l->pop_checking)return session_actor_bad_service(l,NULL,0,"placed root pop query reentered");l->pop_checking=1;
 ok=session_script_roots_current(l->session)&&session_script_roots_cache_current(l->session)&&session_actor_pop_scope(l)&&
  l->placed_certificate.current(&l->placed_certificate.owner,l->placed_certificate.serial)==1&&session_actor_pop_scope(l);
 l->pop_checking=0;return ok&&session_actor_pop_scope(l)&&(!l->session->actor_bootstrap.placed_cohort||l->post_current(l->post_context));
}
static int session_actor_placed_return_current(void*ctx,const GscPlacedInitReturnRequest*q){SessionActorBootstrapLease*l=ctx;
 return l&&q==l->pop_borrowed&&session_actor_pop_current(l);
}
static int session_actor_capture_pop(void*ctx,const GscActorInitReturnRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 SessionActorBootstrapLease*l=ctx;GscActorInitReturnRequest copy;int ok;
 if(!l||!input||!current||!session_actor_scope(l,1)||!l->pop_collecting||l->pop_captured||l->pop_active||
    !l->service_busy||!l->placed_init_attempted||l->prepared!=l->pop_row+1||!l->split||l->construct_only||!l->hooks_returned)
  return session_actor_bad_service(l,error,cap,"placed pop capture outside genuine first entry");
 copy=*input;
 ok=copy.task_serial&&copy.catalog_identity&&copy.now_ms==0&&copy.runtime_identity==gsc_runtime_instance_identity(l->runtime)&&
  session_actor_ref_same(&copy.entity,&l->constructed_entries[l->pop_row].entity)&&current(context)==1&&session_actor_scope(l,1)&&
  session_script_roots_current(l->session)&&session_script_roots_cache_current(l->session)&&current(context)==1&&
  session_actor_scope(l,1)&&!memcmp(input,&copy,sizeof(copy));
 if(!ok)return session_actor_bad_service(l,error,cap,"actual placed root pop capture retired");
 l->vm_seal=copy;l->pop_captured=1;return 1;
}
static int session_actor_dispatch_placed_return(SessionActorBootstrapLease*l,const GscPlacedInitRun*run,char*error,size_t cap){
 GscSession*s=l->session;GscPlacedInitReturnRequest q;int64_t now=s->view.origin_host_ms+s->view.logical_ms;int ok;
 if(!l->pop_captured||l->pop_returned||l->pop_done||l->pop_collecting||!run->root_pop_observed||!run->entry_scope_released||
    run->entry.status!=GSC_ENTITY_ENTRY_RETURNED||run->entry.task_serial!=l->vm_seal.task_serial||
    run->runtime_identity!=l->vm_seal.runtime_identity||run->catalog_identity!=l->vm_seal.catalog_identity||
    !session_actor_ref_same(&run->entry.entity,&l->vm_seal.entity)||now<0||now>INT_MAX)
  return session_actor_bad_service(l,error,cap,"placed return lacks captured normal pop and released entry");
 l->pop_run=run->entry;l->pop_returned=1;
 memset(&q,0,sizeof(q));q.init=l->vm_seal;q.program_identity=run->program_identity;q.session_id=l->id;q.host_now_ms=(int)now;
 l->pop_active=1;l->pop_borrowed=&q;l->pop_seal=q;
 ok=session_actor_pop_current(l);
 if(ok)ok=s->actor_bootstrap.placed_init_return(s->actor_bootstrap.user,&q,error,cap,session_actor_pop_current,l);
 if(!session_actor_pop_current(l))ok=0;
 l->pop_active=0;l->pop_borrowed=NULL;memset(&l->pop_seal,0,sizeof(l->pop_seal));
 if(ok!=1)return session_actor_bad_service(l,error,cap,"placed currentstate callback failed or retired");
 l->pop_done=1;return 1;
}
/* Only this actual VM post-entry callback performs native dispatch for the
 * cohort lane. No copied result or later public capture can create a witness. */
static int session_actor_cohort_post(void*ctx,const GscPlacedInitRun*run,char*error,size_t cap,int(*current)(void*),void*context){
 SessionActorBootstrapLease*l=ctx;int ok;
 if(!l||!current||!l->pop_collecting||l->post_current||!l->session->actor_bootstrap.placed_cohort||current(context)!=1)return 0;
 l->post_current=current;l->post_context=context;l->pop_collecting=0;
 ok=session_actor_dispatch_placed_return(l,run,error,cap)&&current(context)==1&&session_actor_scope(l,1)&&
  session_script_roots_current(l->session)&&current(context)==1&&session_script_roots_cache_current(l->session)&&current(context)==1;
 l->post_current=NULL;l->post_context=NULL;return ok;
}
/* Fixed placed INIT under the actual synchronous returned-row lease. */
static int session_actor_placed_init(void*context,const GscEntityRef*input,
 const GscEntityExecutionCertificate*authority,GscPlacedInitRun*out,char*error,size_t cap){
 SessionActorBootstrapLease*l=context;GscSession*s;GscEntityRef row;
 GscEntityExecutionCertificate cert;GscMapExecutionCertificate map;GscPlacedInitRun result;int ok;char why[384];
 if(!session_actor_scope(l,1))return 0;s=l->session;
 if(!input||!authority||!out||!session_actor_external(l,input,sizeof(*input))||
    !session_actor_external(l,authority,sizeof(*authority))||!session_actor_external(l,out,sizeof(*out))||
    !session_actor_external(l,error,cap)||session_actor_overlap(out,sizeof(*out),input,sizeof(*input))||
    session_actor_overlap(out,sizeof(*out),authority,sizeof(*authority))||session_actor_overlap(out,sizeof(*out),error,cap)||
    session_actor_overlap(error,cap,input,sizeof(*input))||session_actor_overlap(error,cap,authority,sizeof(*authority)))
  return session_actor_bad_service(l,NULL,0,"placed INIT aliases private or borrowed storage");
 if(!s->actor_bootstrap.placed_init_initial||l->construct_only||!l->split||l->failed||l->service_busy||
    !l->hooks_returned||!l->prepared||(s->actor_bootstrap.placed_cohort?l->placed_init_attempted!=(int)l->prepared-1:(l->prepared!=1||l->placed_init_attempted))||l->constructed!=s->actor_bootstrap.placed_count||
    !s->script_roots_enabled||(!s->actor_bootstrap.placed_cohort&&gsc_active_threads(l->runtime)))
  return session_actor_bad_service(l,error,cap,"placed INIT requires unused first returned row; cohort continuation remains open");
 row=*input;cert=*authority;
 if(!cert.serial||!cert.current||!session_actor_ref_same(&cert.owner,&row)||
    !session_actor_ref_same(&row,&l->constructed_entries[l->prepared-1].entity))
  return session_actor_bad_service(l,error,cap,"placed INIT row or allocation authority changed");
 if(s->actor_bootstrap.placed_cohort&&l->prepared>1){
  if(!l->pop_done||!gsc_placed_cohort_current(l->runtime,l->id))return session_actor_bad_service(l,error,cap,"cohort previous witness retired");
  l->pop_active=l->pop_checking=l->pop_collecting=l->pop_captured=l->pop_returned=l->pop_done=0;
  memset(&l->pop_run,0,sizeof(l->pop_run));memset(&l->vm_seal,0,sizeof(l->vm_seal));
 }
 l->pop_row=l->prepared-1;l->placed_init_attempted++;l->service_busy=1;memset(&result,0,sizeof(result));
 map=s->actor_bootstrap.map_certificate;l->placed_certificate=cert;l->placed_map=map;
 ok=session_script_roots_current(s)&&session_script_roots_cache_current(s)&&session_actor_scope(l,1);
 if(ok){GscPlacedInitReturnHost host={l,session_actor_capture_pop};
  l->pop_collecting=s->actor_bootstrap.placed_init_return!=NULL;
  if(s->actor_bootstrap.placed_cohort){GscPlacedCohortPostHost post={l,session_actor_cohort_post};
   if(session_cohort_active)ok=0;else{
    l->cohort_vm_pending=1;l->cohort_vm_kind=1;l->cohort_certificate=cert;l->cohort_entry=l->constructed_entries[l->pop_row];session_cohort_active=l;
    ok=gsc_execute_placed_cohort_init(l->runtime,l,l->id,l->pop_row,&row,&cert,&map,&host,&post,&result,error,cap);
    session_cohort_active=NULL;l->cohort_vm_pending=l->cohort_vm_active=l->cohort_vm_kind=0;
   }
  }else ok=s->actor_bootstrap.placed_init_return?gsc_execute_placed_init_with_return(l->runtime,&row,&cert,&map,&host,&result,error,cap):gsc_execute_placed_init_initial(l->runtime,&row,&cert,&map,&result,error,cap);
  l->pop_collecting=0;
  if(ok&&s->actor_bootstrap.placed_init_return&&!s->actor_bootstrap.placed_cohort){ok=session_actor_dispatch_placed_return(l,&result,error,cap);result.currentstate_completed=ok==1;}
 }
 /* Preserve genuine partial work, including original fields/children, on a
  * failed postcheck. Session terminal cleanup owns it; attempts never refund. */
 *out=result;
 if(ok)ok=session_actor_scope(l,1)&&session_script_roots_current(s)&&session_script_roots_cache_current(s)&&
  session_actor_scope(l,1)&&!memcmp(input,&row,sizeof(row))&&!memcmp(authority,&cert,sizeof(cert))&&
  !memcmp(&s->actor_bootstrap.map_certificate,&map,sizeof(map));
 l->service_busy=0;
 if(!ok){session_message(why,sizeof(why),error&&cap&&error[0]?error:"placed INIT owner retired during initial invocation");return session_actor_bad_service(l,error,cap,why);}
 return 1;
}

/* Fixed bridge: only the currently executing Session service can consume its
 * own exact row once. Native hosts are the real private Session functions. */
int gsc_session_placed_cohort_scope(const void*authority,GscRuntime*r,uint64_t id,uint32_t row,int kind){
 SessionActorBootstrapLease*l=session_cohort_active;
 return l&&authority==l&&l->active&&!l->failed&&l->service_busy&&l->cohort_vm_active&&!l->cohort_vm_pending&&
  l->cohort_vm_kind==kind&&l->runtime==r&&l->id==id&&l->session->runtime==r&&l->session->view.session_id==id&&
  !l->session->abort_requested&&l->session->actor_bootstrap_running&&l->session->busy&&l->session->view.logical_ms==0&&
  l->session->actor_bootstrap.placed_cohort&&l->prepared==row+1&&row<l->constructed&&l->facade&&
  !memcmp(l->facade,&l->facade_seal,sizeof(l->facade_seal));
}
int gsc_session_placed_cohort_admit(const void*authority,GscRuntime*r,uint64_t id,uint32_t row,int kind,
 const GscEntityScriptEntry*entry,const GscEntityExecutionCertificate*cert,const GscMapExecutionCertificate*map,
 const GscPlacedInitReturnHost*pop,const GscPlacedCohortPostHost*post){
 SessionActorBootstrapLease*l=session_cohort_active;int ok;
 if(!l||authority!=l)return 0;
 if(!l->cohort_vm_pending||l->cohort_vm_active||l->cohort_vm_kind!=kind)return session_actor_bad_service(l,NULL,0,"cohort VM admission replay or reentry");
 l->cohort_vm_pending=0;l->cohort_vm_active=1;
 ok=gsc_session_placed_cohort_scope(authority,r,id,row,kind)&&cert&&
  !memcmp(cert,&l->cohort_certificate,sizeof(*cert))&&
  !memcmp(&l->cohort_entry,&l->constructed_entries[row],sizeof(l->cohort_entry));
 if(kind==2)ok=ok&&row&&entry&&!memcmp(entry,&l->cohort_entry,sizeof(*entry))&&!map&&!pop&&!post&&
  l->pop_done&&l->placed_init_attempted==(int)row;
 else if(kind==1)ok=ok&&!entry&&map&&pop&&post&&l->pop_collecting&&!l->pop_captured&&!l->pop_returned&&!l->pop_done&&
  l->placed_init_attempted==(int)row+1&&l->pop_row==row&&
  !memcmp(map,&l->session->actor_bootstrap.map_certificate,sizeof(*map))&&
  pop->user==l&&pop->invoke==session_actor_capture_pop&&post->user==l&&post->invoke==session_actor_cohort_post;
 else ok=0;
 if(!ok)return session_actor_bad_service(l,NULL,0,"cohort VM admission differs from exact selected row");
 return 1;
}
