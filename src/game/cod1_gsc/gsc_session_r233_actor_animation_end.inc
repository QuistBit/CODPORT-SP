/* No timer or clip implementation lives in Session. The current GAME owner
 * supplies a copied receipt and proves its registration/lifetime separately. */
int gsc_session_select_r233_actor_animation_end(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"actor end requires Session");if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->actor_animations_enabled||!s->event_payloads_enabled||s->actor_animation_end_enabled)
  return session_leave_rejected(s,"actor end requires idle selected actor animations and typed events",error,cap);
 if(!session_animation_resources_current(s))return session_leave_rejected(s,"actor end resources expired",error,cap);
 s->actor_animation_end_enabled=1;s->actor_animation_end_runtime=s->actor_animation_end_catalog=0;s->busy=0;return 1;
}
int gsc_session_notify_actor_animation_end(GscSession*s,uint64_t id,const GscActorAnimationEnd*input,int*awakened,char*error,size_t cap){
 GscActorAnimationEnd q;GscEventValue value;GscRuntime*runtime;uint64_t instance,catalog;int ok,now;
 if(!awakened||(cap&&!error)||session_animation_namespace_private(s,input,input?sizeof(*input):0)||session_animation_namespace_private(s,awakened,sizeof(*awakened))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,input?sizeof(*input):0,awakened,sizeof(*awakened))||session_animation_namespace_overlap(input,input?sizeof(*input):0,error,cap)||session_animation_namespace_overlap(awakened,sizeof(*awakened),error,cap))return 0;
 *awakened=0;if(!s||!input)return session_reject(error,cap,"actor end packet required");q=*input;
 if(!session_enter(s,error,cap))return 0;
 if(!session_id_matches(s,id)||!s->actor_animation_end_enabled||!s->actor_animations_enabled||!s->event_payloads_enabled||s->view.state!=GSC_SESSION_RUNNING||!s->runtime)
  return session_leave_rejected(s,"actor end Session or capability unavailable",error,cap);
 runtime=s->runtime;instance=gsc_runtime_instance_identity(runtime);catalog=s->actor_animation_end_catalog;now=s->view.logical_ms;
 if(!instance||s->actor_animation_end_runtime!=instance||q.runtime!=instance||!catalog||q.catalog!=catalog||!q.owner_identity||!q.generation||!q.ordinal||!s->animation_namespace_catalogs[0]||q.ordinal>gsc_animation_catalog_count(s->animation_namespace_catalogs[0])||q.produced_ms<0||!memchr(q.event,0,sizeof(q.event))||!q.event[0]||memcmp(q.payload,"end",4))
  return session_leave_rejected(s,"actor end identity or payload mismatch",error,cap);
 if(!session_animation_resources_current(s)||s->abort_requested||s->runtime!=runtime||s->view.session_id!=id||s->view.logical_ms!=now||memcmp(input,&q,sizeof(q))){
  if(s->abort_requested)return session_failed(s,"actor end resource check cancelled",error,cap);
  return session_leave_rejected(s,"actor end resources or packet retired",error,cap);
 }
 memset(&value,0,sizeof(value));value.kind=GSC_EVENT_STRING;strcpy(value.string,"end");
 ok=gsc_notify_entity_with_payload(runtime,&q.entity,q.event,&value,1,awakened);
 if(s->abort_requested||gsc_runtime_error(runtime)[0]){*awakened=0;return session_failed(s,gsc_runtime_error(runtime)[0]?gsc_runtime_error(runtime):"actor end notify cancelled",error,cap);}
 if(!ok)return session_leave_rejected(s,"actor end owner or waiter request refused",error,cap);
 s->busy=0;return 1;
}

int gsc_session_select_r233_flagged_all_restart(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;if(!s)return session_reject(error,cap,"flagged all restart requires Session");if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->actor_animations_enabled||!s->actor_animation_end_enabled||!s->native_animation_roots_enabled||s->flagged_all_restart_enabled)return session_leave_rejected(s,"flagged all restart requires idle selected actor/end/root capabilities",error,cap);
 if(!session_animation_resources_current(s))return session_leave_rejected(s,"flagged all restart resources expired",error,cap);s->flagged_all_restart_enabled=1;s->busy=0;return 1;
}
