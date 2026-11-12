int gsc_session_select_entity_links(GscSession*s,const GscEntityLinkHost*h,char*error,size_t cap){
 if(s&&(session_entity_query_overlap(h,sizeof(*h),s,sizeof(*s))||session_entity_query_overlap(error,cap,s,sizeof(*s))||session_entity_query_overlap(h,sizeof(*h),error,cap)))return 0;
 if(!s||!h||!h->invoke||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||!s->execution_extensions||s->entity_links_enabled||
    session_entity_query_overlap(h,sizeof(*h),s,sizeof(*s)))return session_reject(error,cap,"entity links require idle execution session and own typed host");
 s->entity_links=*h;s->entity_links_enabled=1;return 1;
}
static int session_entity_link(void*user,const GscEntityLinkRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;GscEntityLinkRequest q,expected;SessionPlayerLease lease;int ok;
 if(!s||!input||!s->entity_links_enabled||!s->entity_links.invoke)return session_reject(error,cap,"entity link outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(error,cap,"entity link session expired");
 q=*input;expected=q;
 if(q.now_ms!=s->view.logical_ms)return session_reject(error,cap,"entity link clock differs from session");
 ok=s->entity_links.invoke(s->entity_links.user,&q,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 return !memcmp(&q,&expected,sizeof(q))?1:session_reject(error,cap,"entity link host changed request");
}
