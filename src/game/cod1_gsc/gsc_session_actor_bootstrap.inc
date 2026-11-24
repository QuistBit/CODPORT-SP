/* R203 explicit Session prelude. The facade context is a borrowed Session
 * lease, never a runtime pointer; every VM operation remains selected/typed. */
#include "gsc_placed_cohort_private.h"
#include "gsc_postcohort_private.h"
typedef struct {
    GscSession *session;GscRuntime *runtime;uint64_t id;
    int active,service_busy,hooks_attempted,hooks_returned,failed;uint32_t prepared;
    int late_attempted,late_invoked,late_graph_active,late_done,late_checking,late_native_call;uint64_t late_receipt;int(*late_current)(void*);void*late_context;
    int cohort_vm_pending,cohort_vm_active,cohort_vm_kind;GscEntityScriptEntry cohort_entry;GscEntityExecutionCertificate cohort_certificate;
    int split,construct_only,placed_init_attempted;uint32_t pop_row;int(*post_current)(void*);void*post_context;uint32_t constructed;GscEntityScriptEntry constructed_entries[GSC_AI_ARRAY_CAPACITY];
    const GscActorBootstrapServices *facade;GscActorBootstrapServices facade_seal;
    int pop_active,pop_checking,pop_collecting,pop_captured,pop_returned,pop_done;GscEntityScriptRun pop_run;const GscPlacedInitReturnRequest *pop_borrowed;GscPlacedInitReturnRequest pop_seal;
    GscActorInitReturnRequest vm_seal; /* Actual pop DATA, no retained VM callback/context. */
    GscEntityExecutionCertificate placed_certificate;GscMapExecutionCertificate placed_map;
} SessionActorBootstrapLease;
static SessionActorBootstrapLease*session_cohort_active;
static int session_actor_postcohort(void*,char*,size_t);
static int session_actor_placed_return_current(void *,const GscPlacedInitReturnRequest *);
static int session_actor_placed_init(void *,const GscEntityRef *,const GscEntityExecutionCertificate *,GscPlacedInitRun *,char *,size_t);
static int session_actor_cohort_post(void*,const GscPlacedInitRun*,char*,size_t,int(*)(void*),void*);
static int session_actor_capture_pop(void*,const GscActorInitReturnRequest*,char*,size_t,int(*)(void*),void*);
static int session_actor_node_target(void *,const GscEntityRef *,const char *,GscActorBootstrapNodeTarget *,char *,size_t);
static int session_actor_map_current(GscSession *s) {
    GscMapExecutionCertificate cert;int ok;
    if(!s||!s->actor_bootstrap_enabled||s->abort_requested)return 0;
    if(s->actor_map_busy){s->abort_requested=1;session_terminal(s,"actor bootstrap map certificate reentered");return 0;}
    cert=s->actor_bootstrap.map_certificate;s->actor_map_busy=1;ok=cert.current(cert.receipt);s->actor_map_busy=0;
    return ok==1&&!s->abort_requested;
}
static int session_actor_ready(GscSession *s) {
    return s&&s->busy&&!s->abort_requested&&s->runtime&&(s->view.state==GSC_SESSION_RUNNING||(s->actor_construct_running&&s->actor_bootstrap_running&&s->view.state==GSC_SESSION_PREPARED))&&session_actor_map_current(s);
}
static int session_actor_scope(SessionActorBootstrapLease *lease,int map) {
    GscSession *s=lease?lease->session:NULL;
    return lease&&lease->active&&s&&s->actor_bootstrap_running&&s->busy&&!s->abort_requested&&
        s->runtime==lease->runtime&&s->view.session_id==lease->id&&
        (lease->construct_only?(s->actor_construct_running&&s->view.state==GSC_SESSION_PREPARED):s->view.state==GSC_SESSION_RUNNING)&&
        s->view.logical_ms==0&&(!map||session_actor_map_current(s));
}
static int session_actor_service_current(void *context){SessionActorBootstrapLease*l=context;
 return session_actor_scope(l,1)&&(!l->session->actor_bootstrap.placed_cohort||!l->placed_init_attempted||gsc_placed_cohort_current(l->runtime,l->id));}
static int session_actor_bad_service(SessionActorBootstrapLease *lease,char *error,size_t cap,const char *text) {
    if(lease&&lease->session){lease->failed=1;lease->session->abort_requested=1;session_terminal(lease->session,text);}
    return session_reject(error,cap,text);
}
static int session_actor_host_copy(GscSessionActorBootstrapHost*out,const GscSessionActorBootstrapHost*input,char*error,size_t cap){
 GscSessionActorBootstrapHost copy;uint32_t n,k;char canonical[128];
 if(!input)return session_reject(error,cap,"null actor bootstrap host");
    copy=*input;
    if((copy.placed_cohort!=0&&copy.placed_cohort!=1)||(copy.placed_cohort&&(!copy.placed_init_initial||!copy.placed_init_return)))return session_reject(error,cap,"cohort requires genuine INIT and native return selection");
    if(copy.placed_init_return&&!copy.placed_init_initial)return session_reject(error,cap,"placed return requires initial invocation selection");
    if(copy.placed_init_initial!=0&&copy.placed_init_initial!=1)return session_reject(error,cap,"invalid placed INIT selection");
    if((copy.node_target!=NULL)!=(copy.node_target_end!=NULL))return session_reject(error,cap,"placed graph query requires its paired pure finish");
    if(!copy.run||!copy.module_count||copy.module_count>GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY||
       !copy.placed_count||copy.placed_count>GSC_AI_ARRAY_CAPACITY||!copy.map_certificate.receipt||!copy.map_certificate.current||
       !copy.access.setmodel||!copy.access.getorigin||!copy.access.getangles||!copy.access.setangles||
       !copy.fields.classify||!copy.fields.read||!copy.fields.write||!copy.methods.invoke||!copy.attachments.invoke||
       !copy.spawner_team.validate||!copy.spawner_team.set)
        return session_reject(error,cap,"actor bootstrap requires bounded roots, placed identities and complete typed providers");
    for(n=0;n<copy.module_count;n++) {
        if(!memchr(copy.module_roots[n],0,sizeof(copy.module_roots[n]))||
           !gsc_module_identity_canonical(copy.module_roots[n],canonical,sizeof(canonical)))
            return session_reject(error,cap,"invalid actor bootstrap module root");
        for(k=0;k<n;k++)if(!strcmp(copy.module_roots[k],canonical))return session_reject(error,cap,"duplicate actor bootstrap module root");
        memset(copy.module_roots[n],0,sizeof(copy.module_roots[n]));strcpy(copy.module_roots[n],canonical);
    }
    for(n=0;n<copy.placed_count;n++) {
        const GscEntityRef *r=&copy.placed[n];
        if(!r->epoch||!r->generation||r->slot>=2048||(n&&r->epoch!=copy.placed[0].epoch))
            return session_reject(error,cap,"invalid actor bootstrap placed identity");
        for(k=0;k<n;k++)if(r->epoch==copy.placed[k].epoch&&r->slot==copy.placed[k].slot&&r->generation==copy.placed[k].generation)
            return session_reject(error,cap,"duplicate actor bootstrap placed identity");
    }
 *out=copy;return 1;
}
int gsc_session_select_r203_actor_bootstrap(GscSession *s,const GscSessionActorBootstrapHost *input,char *error,size_t cap) {
    GscSessionActorBootstrapHost copy;uint32_t n,k;char canonical[128];
    if(!s||!input||s->busy||s->program||s->runtime||s->actor_bootstrap_enabled||s->actor_program_enabled||s->grenade_items_enabled||!s->execution_extensions||
       !s->modules_enabled||!s->structs_enabled||!s->resources_enabled||!s->precache_item_enabled||
       !s->access_enabled||!s->state_enabled||!s->names_enabled)
        return session_reject(error,cap,"actor bootstrap requires an idle selected R198 resource/struct Session");
    if(!session_actor_host_copy(&copy,input,error,cap))return 0;
    s->actor_bootstrap=copy;s->access=copy.access;s->actor_bootstrap_enabled=1;return 1;
}
#include "gsc_session_actor_cache.inc"
static int session_actor_program_current(GscSession*s){
 GscSessionActorProgramPlan copy;int ok;
 if(!s||s->abort_requested)return 0;if(!s->actor_program_enabled)return 1;
 if(s->actor_program_busy){s->abort_requested=1;session_terminal(s,"actor program declaration reentered");return 0;}
 copy=s->actor_program;s->actor_program_busy=1;ok=copy.current(copy.user,copy.epoch,copy.receipt);s->actor_program_busy=0;
 return ok==1&&!s->abort_requested&&!memcmp(&copy,&s->actor_program,sizeof(copy));
}
int gsc_session_select_actor_program(GscSession*s,const GscSessionActorProgramPlan*input,char*error,size_t cap){
 GscSessionActorProgramPlan copy;uint32_t n,k;char canonical[128];
 if(!s||!input||s->busy||s->program||s->runtime||s->view.error[0]||s->actor_program_enabled||s->grenade_items_enabled||
    !s->execution_extensions||!s->modules_enabled||!s->structs_enabled||!s->resources_enabled||!s->precache_item_enabled||!s->access_enabled||!s->state_enabled||!s->names_enabled)
  return session_reject(error,cap,"actor program plan requires idle selected resource/struct Session");
 copy=*input;if(!copy.epoch||!copy.receipt||!copy.current||!copy.module_count||copy.module_count>GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY)
  return session_reject(error,cap,"actor program plan requires current declarations and bounded roots");
 for(n=0;n<copy.module_count;n++){
  if(!memchr(copy.module_roots[n],0,128)||!gsc_module_identity_canonical(copy.module_roots[n],canonical,sizeof(canonical)))return session_reject(error,cap,"invalid actor program root");
  for(k=0;k<n;k++)if(!strcmp(copy.module_roots[k],canonical))return session_reject(error,cap,"duplicate actor program root");
  memset(copy.module_roots[n],0,128);strcpy(copy.module_roots[n],canonical);
 }
 /* The existing GAME profile may have selected other actor capabilities first.
  * Detach its cohort before compile; late admission must freshly obtain it. */
 if(s->actor_bootstrap_enabled&&(s->actor_bootstrap.module_count!=copy.module_count||
    memcmp(s->actor_bootstrap.module_roots,copy.module_roots,copy.module_count*128)||
    s->actor_bootstrap.map_certificate.receipt!=copy.receipt||s->actor_bootstrap.placed[0].epoch!=copy.epoch))
  return session_reject(error,cap,"actor program plan differs from selected profile");
 if(!session_enter(s,error,cap))return 0;
 s->actor_program=copy;s->actor_program_base_access=s->access;s->actor_program_enabled=1;
 if(!session_actor_program_current(s))return session_failed(s,"actor program declaration unavailable",error,cap);
 memset(&s->actor_bootstrap,0,sizeof(s->actor_bootstrap));s->actor_bootstrap_enabled=0;s->actor_admission_attempted=0;s->busy=0;return 1;
}
/* Explicit composition of copied map resources and the real early Actor
 * declaration plan. It never substitutes a map-only root for constructors. */
int gsc_session_select_map_actor_program(GscSession*s,const GscSessionActorProgramPlan*input,char*error,size_t cap){
 GscSessionActorProgramPlan copy;
 if(!s||!input||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 if(s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->map_actor_program_enabled||
    !s->root_map_enabled||s->root_map.receipt!=copy.epoch||!s->root_map.current||!s->map_animation_names.count||!s->animation_namespace_enabled||!s->map_source_float_enabled)
  return session_reject(error,cap,"map Actor program requires same-epoch map resources and fresh declarations");
 if(!gsc_session_select_actor_program(s,&copy,error,cap))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(memcmp(input,&copy,sizeof(copy))||!session_animation_resources_current(s)||!session_actor_program_current(s))return session_failed(s,"map Actor declaration or resources changed during selection",error,cap);
 s->map_actor_program_enabled=1;s->busy=0;return 1;
}
int gsc_session_admit_actor_bootstrap(GscSession*s,uint64_t id,const GscSessionActorBootstrapHost*input,char*error,size_t cap){
 GscSessionActorBootstrapHost copy;char why[GSC_SESSION_ERROR_CAP];int ok;
 if(s&&s->bsp_runtime&&(!gsc_runtime_storage_separate(s->bsp_runtime,input,input?sizeof(*input):0)||!gsc_runtime_storage_separate(s->bsp_runtime,error,cap)||session_actor_overlap(error,cap,s,sizeof(*s))))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",error,cap);
 if(s->view.state!=GSC_SESSION_COMPILED||!s->program||s->runtime||!s->actor_program_enabled||s->actor_bootstrap_enabled||s->actor_admission_attempted)
  return session_leave_rejected(s,"actor cohort admission outside compiled program boundary",error,cap);
 s->actor_admission_attempted=1;why[0]=0;
 if(!session_actor_cache_current(s)||!session_actor_program_current(s)||!session_actor_host_copy(&copy,input,why,sizeof(why)))return session_failed(s,why[0]?why:"actor program declaration retired before cohort admission",error,cap);
 if(copy.module_count!=s->actor_program.module_count||memcmp(copy.module_roots,s->actor_program.module_roots,copy.module_count*128)||
    copy.map_certificate.receipt!=s->actor_program.receipt||copy.placed[0].epoch!=s->actor_program.epoch)
  return session_failed(s,"real actor cohort differs from compiled program plan",error,cap);
 ok=copy.map_certificate.current(copy.map_certificate.receipt);
 if(ok!=1||s->abort_requested||!session_actor_program_current(s))return session_failed(s,"real actor cohort retired during admission",error,cap);
 if(s->bsp_runtime){uint32_t n;
  if(s->bsp_scope||s->bsp_promoted||s->bsp_count!=copy.placed_count)return session_failed(s,"BSP cohort count changed",error,cap);
  for(n=0;n<s->bsp_count;n++)if((s->bsp_entries[n].entity.epoch!=copy.placed[n].epoch||s->bsp_entries[n].entity.slot!=copy.placed[n].slot||s->bsp_entries[n].entity.generation!=copy.placed[n].generation))return session_failed(s,"BSP cohort order or allocation changed",error,cap);
 }
 s->actor_bootstrap=copy;s->access=copy.access;s->actor_bootstrap_enabled=1;s->busy=0;return 1;
}
static int session_actor_name(const char *source,char out[128]) {
    size_t n;if(!source)return 0;memset(out,0,128);for(n=0;n<128&&source[n];n++)out[n]=source[n];
    if(!n||n==128)return 0;out[n]=0;return 1;
}
static int session_actor_ref_same(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int session_actor_classify(void *user,const GscEntityRef *ref,const char *name,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;char field[128],expected[128];int ok;
    if(!ref||!session_actor_name(name,field)||!session_actor_ready(s))return -1;
    copy=*ref;memcpy(expected,field,sizeof(field));ok=s->actor_bootstrap.fields.classify(s->actor_bootstrap.fields.user,&copy,field,error,cap);
    if(!session_actor_ready(s)||!session_actor_ref_same(&copy,ref)||memcmp(field,expected,sizeof(field)))return -1;
    return ok;
}
static int session_script_state_field(const char*name){
    static const char*const names[]={"scriptstate","lastscriptstate","statechangereason"};
    unsigned i,n;for(i=0;i<3;i++){for(n=0;name[n]&&names[i][n];n++){
        unsigned char c=(unsigned char)name[n];if(c>='A'&&c<='Z')c+=32;
        if(c!=(unsigned char)names[i][n])break;
    }if(!name[n]&&!names[i][n])return 1;}return 0;
}
static int session_actor_read(void *user,const GscEntityRef *ref,const char *name,GscStringCastValue *out,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;GscStringCastValue result;char field[128],expected[128];int ok;
    if(!ref||!out||!session_actor_name(name,field)||!session_actor_ready(s))return session_reject(error,cap,"actor field read outside current Session");
    copy=*ref;memcpy(expected,field,sizeof(field));memset(&result,0,sizeof(result));result.type=-1;
    if(s->r243_script_state_enabled&&session_script_state_field(field)){
        GscActorScriptStateHost host=s->r243_script_state;
        ok=host.read(host.user,&copy,field,&result,error,cap);
        if(memcmp(&s->r243_script_state,&host,sizeof(host)))return 0;
    }else ok=s->actor_bootstrap.fields.read(s->actor_bootstrap.fields.user,&copy,field,&result,error,cap);
    if(ok!=1||!session_actor_ready(s))return 0;
    if(!session_actor_ref_same(&copy,ref)||memcmp(field,expected,sizeof(field)))return session_reject(error,cap,"actor field reader mutated borrowed identity or name");
    *out=result;return 1;
}
static int session_actor_write(void *user,const GscEntityRef *ref,const char *name,const GscStringCastValue *value,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;GscStringCastValue borrowed,original;char field[128],expected[128];int ok;
    if(!ref||!value||!session_actor_name(name,field)||!session_actor_ready(s))return session_reject(error,cap,"actor field write outside current Session");
    copy=*ref;borrowed=*value;original=borrowed;memcpy(expected,field,sizeof(field));
    ok=s->actor_bootstrap.fields.write(s->actor_bootstrap.fields.user,&copy,field,&borrowed,error,cap);
    if(ok!=1||!session_actor_ready(s))return 0;
    if(!session_actor_ref_same(&copy,ref)||memcmp(field,expected,sizeof(field))||memcmp(&borrowed,&original,sizeof(borrowed)))return session_reject(error,cap,"actor field writer mutated borrowed input");
    return 1;
}
static int session_actor_method(void *user,const GscEntityRef *ref,uint32_t method,const GscStringCastValue *args,int argc,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;GscStringCastValue borrowed[GSC_DYNAMIC_INFO_MAX_ARGS],expected[GSC_DYNAMIC_INFO_MAX_ARGS];int ok;
    if(!ref||argc<0||argc>GSC_DYNAMIC_INFO_MAX_ARGS||(argc&&!args)||!session_actor_ready(s))return session_reject(error,cap,"actor method outside current Session");
    copy=*ref;memset(borrowed,0,sizeof(borrowed));if(argc)memcpy(borrowed,args,(size_t)argc*sizeof(*args));memcpy(expected,borrowed,sizeof(expected));
    ok=s->actor_bootstrap.methods.invoke(s->actor_bootstrap.methods.user,&copy,method,borrowed,argc,error,cap);
    if(ok!=1||!session_actor_ready(s))return 0;
    if(!session_actor_ref_same(&copy,ref)||memcmp(borrowed,expected,sizeof(borrowed)))return session_reject(error,cap,"actor method mutated borrowed input");
    return 1;
}
static int session_actor_team_validate(void *user,const GscEntityRef *ref,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;int ok;if(!ref||!session_actor_ready(s))return session_reject(error,cap,"spawner validation outside current Session");
    copy=*ref;ok=s->actor_bootstrap.spawner_team.validate(s->actor_bootstrap.spawner_team.user,&copy,error,cap);
    return ok==1&&session_actor_ready(s)&&session_actor_ref_same(&copy,ref);
}
static int session_actor_team_set(void *user,const GscEntityRef *ref,const char *team,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef copy;char text[128],expected[128];int ok;
    if(!ref||!session_actor_name(team,text)||!session_actor_ready(s))return session_reject(error,cap,"spawner team outside current Session");
    copy=*ref;memcpy(expected,text,sizeof(text));ok=s->actor_bootstrap.spawner_team.set(s->actor_bootstrap.spawner_team.user,&copy,text,error,cap);
    return ok==1&&session_actor_ready(s)&&session_actor_ref_same(&copy,ref)&&!memcmp(text,expected,sizeof(text));
}
static int session_actor_attachment_current(void *context){SessionPlayerLease *lease=context;return session_player_current(lease)&&session_actor_ready(lease->session);}
static int session_actor_attachment(void *user,const GscAttachmentQueryRequest *request,GscAttachmentQueryResult *out,char *error,size_t cap,int (*current)(void *),void *context){
 GscSession *s=user;GscAttachmentQueryRequest copy,expected;GscAttachmentQueryResult staged;SessionPlayerLease lease;int ok;
 if(!request||!out||!current||!session_actor_ready(s))return session_reject(error,cap,"attachment query outside selected Session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 copy=*request;expected=copy;memset(&staged,0,sizeof(staged));staged.kind=-1;
 if(!session_actor_attachment_current(&lease))return session_reject(error,cap,"attachment query lease expired");
 ok=s->actor_bootstrap.attachments.invoke(s->actor_bootstrap.attachments.user,&copy,&staged,error,cap,session_actor_attachment_current,&lease);
 if(ok!=1||!session_actor_attachment_current(&lease))return 0;
 if(memcmp(&copy,&expected,sizeof(copy)))return session_reject(error,cap,"attachment query host mutated borrowed input");
 *out=staged;return 1;
}
static int session_actor_overlap(const void *a,size_t na,const void *b,size_t nb){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);}
static int session_actor_external(SessionActorBootstrapLease *l,const void *p,size_t size){return gsc_runtime_storage_separate(l->runtime,p,size)&&!session_actor_overlap(p,size,l,sizeof(*l))&&!session_actor_overlap(p,size,l->session,sizeof(*l->session))&&!session_actor_overlap(p,size,l->facade,l->facade?sizeof(*l->facade):0)&&!(l->pop_active&&session_actor_overlap(p,size,l->pop_borrowed,sizeof(*l->pop_borrowed)));}
static uint64_t session_actor_program(void *context){SessionActorBootstrapLease *l=context;return session_actor_scope(l,1)?gsc_runtime_program_identity(l->runtime):0;}
static int session_actor_function(void *context,const char *module,const char *function){SessionActorBootstrapLease *l=context;return session_actor_scope(l,1)&&session_actor_cached_function(l->session,module,function)&&gsc_runtime_script_function_available(l->runtime,module,function);}
static int session_actor_storage(void *context,const void *p,size_t size){SessionActorBootstrapLease *l=context;return session_actor_scope(l,1)&&session_actor_external(l,p,size);}
static int session_actor_available(void *context,int kind){SessionActorBootstrapLease *l=context;if(!session_actor_scope(l,1))return 0;return kind==GSC_ACTOR_BOOTSTRAP_AVAILABLE_RESOURCES?gsc_runtime_resource_bootstrap_available(l->runtime):kind==GSC_ACTOR_BOOTSTRAP_AVAILABLE_SPAWNER_TEAM?gsc_runtime_spawner_team_available(l->runtime):0;}
static int session_actor_copy_error(void *context,char *out,size_t cap){SessionActorBootstrapLease *l=context;if(!out||!cap||!session_actor_scope(l,0)||!session_actor_external(l,out,cap))return 0;session_message(out,cap,gsc_runtime_error(l->runtime));return 1;}
static int session_actor_hooks(void *context,const GscEntityHookBootstrapEntry *entries,size_t count,const GscEntityHookBootstrapHost *host,GscEntityHookBootstrapResult *out,char *error,size_t cap) {
    SessionActorBootstrapLease *l=context;GscEntityHookBootstrapResult result;size_t n;int ok;
    if(!session_actor_scope(l,1))return session_reject(error,cap,"actor hooks outside bootstrap scope");
    if(!out||!entries||!count||count>GSC_ENTITY_HOOK_BOOTSTRAP_MAX)return session_actor_bad_service(l,NULL,0,"invalid actor hook request");
    if(!session_actor_external(l,entries,count*sizeof(*entries))||!session_actor_external(l,host,host?sizeof(*host):0)||
       !session_actor_external(l,out,sizeof(*out))||!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"actor hook service aliases private storage");
    if(l->construct_only)return session_actor_bad_service(l,error,cap,"hooks cannot run during explicit construction");
    if(l->split&&l->constructed!=l->session->actor_bootstrap.placed_count)return session_actor_bad_service(l,error,cap,"hooks require the complete constructed cohort");
    if(l->failed||l->service_busy||l->hooks_attempted)return session_actor_bad_service(l,error,cap,"actor hooks already attempted or reentered");
    l->hooks_attempted=1;l->service_busy=1;memset(&result,0,sizeof(result));ok=gsc_start_entity_hooks(l->runtime,entries,count,host,&result,error,cap);l->service_busy=0;
    if(!session_actor_scope(l,1))return 0;
    *out=result;if(!ok){l->failed=1;return 0;}
    if(result.attempted!=count||gsc_active_threads(l->runtime)){l->failed=1;return session_reject(error,cap,"actor bootstrap hooks must all return without children");}
    for(n=0;n<count;n++)if(result.runs[n].status!=GSC_ENTITY_ENTRY_RETURNED){l->failed=1;return session_reject(error,cap,"actor bootstrap hook did not return");}
    l->hooks_returned=1;return 1;
}
static int session_actor_prepared(void *context,const GscPreparedEntityEntry *entry,const GscPreparedEntityEntryHost *host,GscPreparedEntityEntryRun *out,char *error,size_t cap) {
    SessionActorBootstrapLease *l=context;GscPreparedEntityEntry copy,expected;GscPreparedEntityEntryRun result;char canonical[128];uint32_t n;int ok;
    if(!session_actor_scope(l,1))return session_reject(error,cap,"prepared actor outside bootstrap scope");
    if(!entry||!host||!out)return session_actor_bad_service(l,NULL,0,"invalid prepared actor request");
    if(!session_actor_external(l,entry,sizeof(*entry))||!session_actor_external(l,host,sizeof(*host))||
       !session_actor_external(l,out,sizeof(*out))||!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"prepared actor service aliases private storage");
    if(l->construct_only||l->split||l->service_busy||l->failed||!l->hooks_returned||l->prepared>=l->session->actor_bootstrap.placed_count)return session_actor_bad_service(l,error,cap,"prepared actor order or bootstrap scope violated");
    copy=*entry;expected=copy;
    if(!session_actor_ref_same(&copy.entry.entity,&l->session->actor_bootstrap.placed[l->prepared])||
       !memchr(copy.entry.module,0,sizeof(copy.entry.module))||!memchr(copy.entry.function,0,sizeof(copy.entry.function))||
       strcmp(copy.entry.function,"main")||!gsc_module_identity_canonical(copy.entry.module,canonical,sizeof(canonical)))
        return session_actor_bad_service(l,error,cap,"prepared actor identity or main entry differs from selected plan");
    for(n=0;n<l->session->actor_bootstrap.module_count;n++)if(!strcmp(canonical,l->session->actor_bootstrap.module_roots[n]))break;
    if(n==l->session->actor_bootstrap.module_count)return session_actor_bad_service(l,error,cap,"prepared actor module is outside selected roots");
    /* Latch attempt before invoking any field/native callback. Failure is not
     * replayable; the underlying GAME result keeps its exact retained lease. */
    l->prepared++;l->service_busy=1;memset(&result,0,sizeof(result));ok=gsc_execute_prepared_entity_entry(l->runtime,&copy,host,&result,error,cap);l->service_busy=0;
    if(!session_actor_scope(l,1))return 0;
    if(memcmp(&copy,&expected,sizeof(copy)))return session_actor_bad_service(l,error,cap,"prepared actor request mutated");
    *out=result;
    if(!ok){l->failed=1;return 0;}
    if(result.stage!=GSC_PREPARED_ENTRY_ENTERED||result.entry.status!=GSC_ENTITY_ENTRY_RETURNED||gsc_active_threads(l->runtime))
        {l->failed=1;return session_reject(error,cap,"prepared actor must return without active children");}
    return 1;
}

static int session_actor_construct(void *context,const GscPreparedEntityEntry *entry,const GscPreparedEntityEntryHost *host,GscPreparedEntityEntryRun *out,char *error,size_t cap){
 SessionActorBootstrapLease*l=context;GscPreparedEntityEntry copy,expected;GscPreparedEntityEntryRun result;uint32_t n,index;char canonical[128];int ok;
 if(!session_actor_scope(l,1))return session_reject(error,cap,"construction outside bootstrap scope");
 if(!entry||!host||!out||!session_actor_external(l,entry,sizeof(*entry))||!session_actor_external(l,host,sizeof(*host))||
    !session_actor_external(l,out,sizeof(*out))||!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"construction aliases private storage");
 if(l->session->bsp_promoted||(l->session->actor_bootstrap.construct&&!l->construct_only)||l->service_busy||l->failed||l->hooks_attempted||l->prepared||l->constructed>=l->session->actor_bootstrap.placed_count)return session_actor_bad_service(l,error,cap,"construction phase or order violated");
 copy=*entry;expected=copy;index=l->constructed;
 if(!session_actor_ref_same(&copy.entry.entity,&l->session->actor_bootstrap.placed[index])||
    !memchr(copy.entry.module,0,sizeof(copy.entry.module))||!memchr(copy.entry.function,0,sizeof(copy.entry.function))||
    strcmp(copy.entry.function,"main")||!gsc_module_identity_canonical(copy.entry.module,canonical,sizeof(canonical)))return session_actor_bad_service(l,error,cap,"construction differs from selected placed plan");
 for(n=0;n<l->session->actor_bootstrap.module_count;n++)if(!strcmp(canonical,l->session->actor_bootstrap.module_roots[n]))break;
 if(n==l->session->actor_bootstrap.module_count)return session_actor_bad_service(l,error,cap,"construction module outside roots");
 l->split=1;l->constructed++;l->service_busy=1;memset(&result,0,sizeof(result));
 ok=gsc_construct_prepared_entity(l->runtime,&copy,host,&result,error,cap);l->service_busy=0;
 if(!session_actor_scope(l,1))return 0;
 if(memcmp(&copy,&expected,sizeof(copy)))return session_actor_bad_service(l,error,cap,"construction request mutated");
 *out=result;
 if(!ok||result.stage!=GSC_PREPARED_ENTRY_PREPARED||result.entry.task_serial||gsc_active_threads(l->runtime)){
  l->failed=1;if(!ok)return 0;return session_reject(error,cap,"construction executed a script task or returned an invalid stage");}
 l->constructed_entries[index]=copy.entry;return 1;
}
static int session_actor_adopt_bsp(void*context,const GscPreparedEntityEntry*input,char*error,size_t cap){
 SessionActorBootstrapLease*l=context;GscPreparedEntityEntry q;uint32_t i;
 if(!session_actor_scope(l,1))return session_reject(error,cap,"BSP adoption outside current construction scope");
 if(!input||!session_actor_external(l,input,sizeof(*input))||!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"BSP adoption aliases private storage");
 if(!l->construct_only||!l->session->bsp_promoted||l->service_busy||l->failed||l->hooks_attempted||l->prepared||l->constructed>=l->session->bsp_count)return session_actor_bad_service(l,error,cap,"BSP adoption phase or order changed");
 q=*input;i=l->constructed;
 if(memcmp(&q.entry,&l->session->bsp_entries[i],sizeof(q.entry))||
    !session_actor_ref_same(&q.entry.entity,&l->session->actor_bootstrap.placed[i])||
    !gsc_bsp_fields_current(l->runtime,&q))return session_actor_bad_service(l,error,cap,"BSP adoption entry or retained private fields changed");
 l->split=1;l->constructed_entries[i]=q.entry;l->constructed++;return 1;
}
static int session_actor_enter_constructed(void *context,const GscEntityScriptEntry *entry,const GscEntityExecutionCertificate *certificate,GscEntityScriptRun *out,char *error,size_t cap){
 SessionActorBootstrapLease*l=context;GscEntityScriptEntry copy;GscEntityExecutionCertificate cert;GscEntityScriptRun result;int ok;
 if(!session_actor_scope(l,1))return session_reject(error,cap,"constructed entry outside bootstrap scope");
 if(!entry||!certificate||!out||!session_actor_external(l,entry,sizeof(*entry))||!session_actor_external(l,certificate,sizeof(*certificate))||
    !session_actor_external(l,out,sizeof(*out))||!session_actor_external(l,error,cap))return session_actor_bad_service(l,NULL,0,"constructed entry aliases private storage");
 if(!l->split||l->service_busy||l->failed||!l->hooks_returned||l->constructed!=l->session->actor_bootstrap.placed_count||l->prepared>=l->constructed)return session_actor_bad_service(l,error,cap,"constructed entry phase or order violated");
 copy=*entry;cert=*certificate;
 if(memcmp(&copy,&l->constructed_entries[l->prepared],sizeof(copy))||!session_actor_ref_same(&cert.owner,&copy.entity)||!cert.serial||!cert.current)return session_actor_bad_service(l,error,cap,"constructed entry identity or fresh certificate invalid");
 l->prepared++;l->service_busy=1;memset(&result,0,sizeof(result));
 if(l->session->actor_bootstrap.placed_cohort&&l->prepared>1){
  if(session_cohort_active){l->failed=1;ok=0;}else{
   l->cohort_vm_pending=1;l->cohort_vm_kind=2;l->cohort_entry=copy;l->cohort_certificate=cert;session_cohort_active=l;
   ok=l->placed_init_attempted==(int)l->prepared-1&&l->pop_done&&gsc_execute_placed_cohort_aitype(l->runtime,l,l->id,l->prepared-1,&copy,&cert,&result,error,cap);
   session_cohort_active=NULL;l->cohort_vm_pending=l->cohort_vm_active=l->cohort_vm_kind=0;
  }
 }
 else ok=gsc_execute_constructed_entity(l->runtime,&copy,&cert,&result);
 l->service_busy=0;
 if(!session_actor_scope(l,1))return 0;
 *out=result;
 if(!ok||result.status!=GSC_ENTITY_ENTRY_RETURNED||
    ((l->session->actor_bootstrap.placed_cohort&&l->prepared>1)?!gsc_placed_cohort_current(l->runtime,l->id):gsc_active_threads(l->runtime))){l->failed=1;return session_reject(error,cap,"constructed entry did not return without children");}
 return 1;
}

static int session_actor_prelude(GscSession *s,char *error,size_t cap) {
    SessionActorBootstrapLease lease;GscActorBootstrapServices api,expected;int ok;
    memset(&lease,0,sizeof(lease));memset(&api,0,sizeof(api));lease.session=s;lease.runtime=s->runtime;lease.id=s->view.session_id;lease.active=1;lease.facade=&api;
    api.context=&lease;api.current=session_actor_service_current;api.program_identity=session_actor_program;api.script_function_available=session_actor_function;
    api.storage_separate=session_actor_storage;api.available=session_actor_available;api.copy_error=session_actor_copy_error;api.start_entity_hooks=session_actor_hooks;api.execute_prepared_entity_entry=session_actor_prepared;api.construct_entity=session_actor_construct;api.adopt_bsp_entity=session_actor_adopt_bsp;api.enter_constructed_entity=session_actor_enter_constructed;api.node_target=s->actor_bootstrap.node_target?session_actor_node_target:NULL;api.placed_init_initial=s->actor_bootstrap.placed_init_initial?session_actor_placed_init:NULL;api.placed_init_return_current=s->actor_bootstrap.placed_init_return?session_actor_placed_return_current:NULL;api.postcohort_node_owners=s->actor_bootstrap.placed_cohort?session_actor_postcohort:NULL;api.placed_cohort=s->actor_bootstrap.placed_cohort;expected=api;lease.facade_seal=api;
    if(s->actor_bootstrap_attempted)return session_reject(error,cap,"actor bootstrap was already attempted");
    if(s->actor_bootstrap.construct){
        if(!s->actor_construct_complete||s->actor_construct_program!=gsc_runtime_program_identity(s->runtime)||
           s->actor_construct_count!=s->actor_bootstrap.placed_count)return session_reject(error,cap,"constructed cohort program or count changed");
        lease.split=1;lease.constructed=s->actor_construct_count;
        memcpy(lease.constructed_entries,s->actor_construct_entries,sizeof(lease.constructed_entries));
    }
    s->actor_bootstrap_attempted=1;s->actor_bootstrap_running=1;
    if(!session_actor_scope(&lease,1)){ok=0;session_message(error,cap,"actor bootstrap map retired before prelude");}
    else ok=s->actor_bootstrap.run(s->actor_bootstrap.user,&api,error,cap);
    if(ok==1&&(!session_actor_scope(&lease,1)||memcmp(&api,&expected,sizeof(api))||lease.failed||!lease.hooks_returned||
       lease.prepared!=s->actor_bootstrap.placed_count||
       (s->actor_bootstrap.placed_cohort?(!lease.late_done||!gsc_placed_cohort_current(s->runtime,lease.id)):gsc_active_threads(s->runtime))||gsc_runtime_error(s->runtime)[0])) {
        ok=0;session_message(error,cap,"actor bootstrap did not complete its selected roots and placed plan");
    }
    lease.active=0;s->actor_bootstrap_running=0;return ok==1;
}

/* Construction runs in a fresh synchronous scope before the script clock
 * starts. Persistent ownership consists only of this Session's program ID and
 * copied exact entries. The borrowed facade/lease/certificates die on return. */
int gsc_session_construct_actors(GscSession*s,uint64_t id,char*error,size_t cap){
 SessionActorBootstrapLease lease;GscActorBootstrapServices api,expected;int ok,logical;
 if(!session_enter(s,error,cap))return 0;
 if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",error,cap);
 if(s->view.state!=GSC_SESSION_PREPARED||!s->actor_bootstrap_enabled||!s->actor_bootstrap.construct)
  return session_leave_rejected(s,"explicit actor construction requires a prepared selected Session",error,cap);
 if(s->actor_construct_attempted||s->actor_bootstrap_attempted)
  return session_failed(s,"actor construction was already attempted",error,cap);
 if(!session_script_roots_current(s)||!session_script_roots_cache_current(s))return session_failed(s,"global/map declarations retired before construction",error,cap);
 s->actor_construct_attempted=1;
 if(!s->runtime||gsc_active_threads(s->runtime)||gsc_runtime_error(s->runtime)[0])
  return session_failed(s,"actor construction runtime is not fresh",error,cap);
 memset(&lease,0,sizeof(lease));memset(&api,0,sizeof(api));lease.session=s;lease.runtime=s->runtime;
 lease.id=id;lease.active=1;lease.construct_only=1;lease.facade=&api;
 api.context=&lease;api.current=session_actor_service_current;api.program_identity=session_actor_program;
 api.script_function_available=session_actor_function;api.storage_separate=session_actor_storage;
 api.available=session_actor_available;api.copy_error=session_actor_copy_error;api.start_entity_hooks=session_actor_hooks;
 api.execute_prepared_entity_entry=session_actor_prepared;api.construct_entity=session_actor_construct;api.adopt_bsp_entity=session_actor_adopt_bsp;
 api.enter_constructed_entity=session_actor_enter_constructed;api.node_target=s->actor_bootstrap.node_target?session_actor_node_target:NULL;api.placed_init_initial=s->actor_bootstrap.placed_init_initial?session_actor_placed_init:NULL;api.placed_init_return_current=s->actor_bootstrap.placed_init_return?session_actor_placed_return_current:NULL;api.postcohort_node_owners=s->actor_bootstrap.placed_cohort?session_actor_postcohort:NULL;api.placed_cohort=s->actor_bootstrap.placed_cohort;expected=api;lease.facade_seal=api;
 logical=s->view.logical_ms;s->view.logical_ms=0;s->actor_construct_running=s->actor_bootstrap_running=1;
 ok=session_actor_scope(&lease,1)&&s->actor_bootstrap.construct(s->actor_bootstrap.user,&api,error,cap)==1;
 if(ok&&(!session_actor_scope(&lease,1)||memcmp(&api,&expected,sizeof(api))||lease.failed||lease.hooks_attempted||lease.prepared||
    !lease.split||lease.constructed!=s->actor_bootstrap.placed_count||gsc_active_threads(s->runtime)||gsc_runtime_error(s->runtime)[0])){
  ok=0;session_message(error,cap,"explicit actor construction did not complete its selected cohort without script tasks");
 }
 if(ok){s->actor_construct_count=lease.constructed;s->actor_construct_program=gsc_runtime_program_identity(s->runtime);
  memcpy(s->actor_construct_entries,lease.constructed_entries,sizeof(s->actor_construct_entries));s->actor_construct_complete=1;}
 lease.active=0;s->actor_construct_running=s->actor_bootstrap_running=0;s->view.logical_ms=logical;
 if(!ok)return session_failed(s,error&&cap&&error[0]?error:"explicit actor construction failed",error,cap);
 session_capture(s);s->busy=0;return 1;
}
