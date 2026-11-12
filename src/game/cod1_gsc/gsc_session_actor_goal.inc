int gsc_session_select_r222_carbine_items(GscSession *s,char *error,size_t size) {
 if(!s||s->busy||s->program||s->runtime||!s->execution_extensions||(!s->actor_bootstrap_enabled&&!s->actor_program_enabled)||s->grenade_items_enabled||s->carbine_items_enabled)
  return session_reject(error,size,"carbine native item selection requires idle execution and Actor prefix");
 s->carbine_items_enabled=1;return 1;
}
int gsc_session_select_r228_training_opening(GscSession *s,char *error,size_t size){
 if(!s||s->busy||s->program||s->runtime||s->training_opening_enabled||s->carbine_items_enabled!=2)
  return session_reject(error,size,"opening requires idle selected training followup");
 s->training_opening_enabled=1;return 1;
}
int gsc_session_select_r225_training_followup(GscSession *s,char *error,size_t size){
 if(!s||s->busy||s->program||s->runtime||(!s->actor_bootstrap_enabled&&!s->actor_program_enabled)||!s->execution_extensions||s->carbine_items_enabled!=1)return session_reject(error,size,"followup hints require idle selected training Actor/item Session");
 s->carbine_items_enabled=2;return 1;
}
int gsc_session_select_r222_actor_goal(GscSession *s,const GscActorGoalHost *host,
    char *error,size_t size) {
    if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||s->actor_goal_enabled||
       !s->execution_extensions||!s->state_enabled||!s->node_queries_enabled||(!s->actor_bootstrap_enabled&&!s->actor_program_enabled))
        return session_reject(error,size,"goal assignment requires idle selected execution Session");
    s->actor_goal=*host;s->actor_goal_enabled=1;return 1;
}
typedef struct {
    GscSession *session;uint64_t id;int logical_ms;GscRuntime *runtime;
    GscActorGoalHost host;int (*current)(void *);void *context;
} SessionActorGoalLease;
static int session_actor_goal_scope(const SessionActorGoalLease *l) {
    GscSession *s=l->session;
    return s&&s->busy&&!s->abort_requested&&s->actor_goal_enabled&&
        s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&
        s->view.logical_ms==l->logical_ms&&s->runtime==l->runtime&&
        !memcmp(&s->actor_goal,&l->host,sizeof(l->host));
}
static int session_actor_goal_current(void *context) {
    SessionActorGoalLease *l=context;
    return session_actor_goal_scope(l)&&l->current&&l->current(l->context)==1&&
        session_actor_goal_scope(l);
}
static int session_actor_goal_invoke(void *user,const GscActorGoalRequest *input,
    char *error,size_t size,int (*current)(void *),void *context) {
    GscSession *s=user;SessionActorGoalLease l;GscActorGoalRequest request,original;int ok;
    if(!s||!input||!current)return 0;
    l.session=s;l.id=s->view.session_id;l.logical_ms=s->view.logical_ms;l.runtime=s->runtime;
    l.host=s->actor_goal;l.current=current;l.context=context;
    if(!session_actor_goal_scope(&l))return 0;
    request=original=*input;
    ok=l.host.invoke(l.host.user,&request,error,size,session_actor_goal_current,&l);
    return ok==1&&session_actor_goal_scope(&l)&&!memcmp(&request,&original,sizeof(request));
}

int gsc_session_training_appearance(GscSession *s,uint64_t id,const GscEntityRef *ref,int kind,GscTrainingAppearance *out){
 GscEntityRef owner;GscTrainingAppearance result={0};
 if(!s || !ref || !out || s->busy || s->abort_requested || !s->training_opening_enabled || s->view.error[0] ||
    s->view.state!=GSC_SESSION_RUNNING || s->view.session_id!=id || !s->runtime ||
    session_animation_namespace_private(s,ref,sizeof(*ref)) || session_animation_namespace_private(s,out,sizeof(*out)) ||
    session_animation_namespace_overlap(ref,sizeof(*ref),out,sizeof(*out)))return 0;
 owner=*ref;if(!gsc_runtime_training_appearance(s->runtime,&owner,kind,&result))return 0;
 *out=result;return 1;
}
