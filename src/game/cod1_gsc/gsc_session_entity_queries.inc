/* R149 copied replies belong to one current running session and logical clock. */
static int session_entity_query_overlap(const void *a,size_t as,const void *b,size_t bs) {
    uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
    if(!a||!b||!as||!bs)return 0;
    return aa<=bb?bb-aa<as:aa-bb<bs;
}
static int session_entity_number_query(void *user,int now,int32_t slot,
    GscEntityRef *out,int *found,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;int present=-1,accepted;uint64_t id;
    if(session_entity_query_overlap(out,sizeof(*out),error,size)||
       session_entity_query_overlap(found,sizeof(*found),error,size))return 0;
    if(!out||!found||session_entity_query_overlap(out,sizeof(*out),found,sizeof(*found)))
        return session_reject(error,size,"entity number query requires separate output copies");
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!s->entity_queries_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||s->view.logical_ms!=now||now<0||
       (uint32_t)slot>=2048||!s->entity_queries.by_number)
        return session_reject(error,size,"entity number query outside current running session");
    id=s->view.session_id;memset(&ref,0,sizeof(ref));
    accepted=s->entity_queries.by_number(s->entity_queries.user,id,now,slot,&ref,&present,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->view.session_id!=id||s->view.logical_ms!=now||!s->entity_queries_enabled)
        return session_reject(error,size,"entity number callback cancelled or changed session");
    if(accepted!=1)return 0;
    if((present!=0&&present!=1)||
       (present?(!ref.epoch||!ref.generation||ref.slot!=(uint32_t)slot):
        (ref.epoch||ref.slot||ref.generation)))
        return session_reject(error,size,"entity number callback returned invalid copied identity");
    *out=ref;*found=present;return 1;
}
static int session_spawner_team_collect(void *user,int now,unsigned int mask,
    GscEntityRef *out,size_t capacity,size_t *count,char *error,size_t size) {
    GscSession *s=user;GscEntityRef refs[GSC_ARRAY_CONTAINER_CAPACITY];
    size_t copied=(size_t)-1,n,bytes;uint64_t id;int accepted;
    bytes=capacity>SIZE_MAX/sizeof(*out)?SIZE_MAX:capacity*sizeof(*out);
    if(session_entity_query_overlap(out,bytes,error,size)||
       session_entity_query_overlap(count,sizeof(*count),error,size))return 0;
    if(!out||!count||session_entity_query_overlap(out,bytes,count,sizeof(*count)))
        return session_reject(error,size,"spawner team query requires separate output copies");
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!s->entity_queries_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||s->view.logical_ms!=now||now<0||
       !mask||(mask&~14u)||!s->entity_queries.collect_team||capacity!=GSC_ARRAY_CONTAINER_CAPACITY)
        return session_reject(error,size,"spawner team query outside current running session");
    id=s->view.session_id;memset(refs,0,sizeof(refs));
    accepted=s->entity_queries.collect_team(s->entity_queries.user,id,now,mask,refs,capacity,&copied,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->view.session_id!=id||s->view.logical_ms!=now||!s->entity_queries_enabled)
        return session_reject(error,size,"spawner team callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(copied>capacity)return session_reject(error,size,"spawner team callback exceeded complete output capacity");
    for(n=0;n<copied;n++)if(!refs[n].epoch||!refs[n].generation||refs[n].slot>=2048||
        (n&&(refs[n].epoch!=refs[0].epoch||refs[n].slot<=refs[n-1].slot)))
        return session_reject(error,size,"spawner team callback returned invalid ordered identities");
    if(copied)memcpy(out,refs,copied*sizeof(*out));
    *count=copied;return 1;
}
