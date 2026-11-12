/* R216 copied native-field bridge. It does not shadow real item.count in VM
 * storage. A callback cannot retire/reload the owning Session and publish into
 * its replacement. No callback host pointers are retained from an operand. */
int gsc_session_select_r216_items(GscSession *s,const GscEntityFieldHost *host,
    char *error,size_t cap) {
    if(!s||!host||!host->classify||!host->read||!host->write||s->busy||s->program||
       s->runtime||s->grenade_items_enabled||s->actor_bootstrap_enabled||s->actor_program_enabled||
       !s->execution_extensions||!s->state_enabled||!s->scalar_switch_enabled||!s->names_enabled)
        return session_reject(error,cap,"grenade items require idle R198 native-field Session");
    s->grenade_item_fields=*host;s->grenade_items_enabled=1;return 1;
}
typedef struct {
    uint64_t id;int logical_ms;GscRuntime *runtime;GscEntityFieldHost host;
} SessionGrenadeItemLease;
static SessionGrenadeItemLease session_grenade_item_lease(GscSession *s) {
    SessionGrenadeItemLease l;l.id=s->view.session_id;l.logical_ms=s->view.logical_ms;
    l.runtime=s->runtime;l.host=s->grenade_item_fields;return l;
}
static int session_grenade_item_current(GscSession *s,const SessionGrenadeItemLease *l) {
    return s&&s->busy&&!s->abort_requested&&s->grenade_items_enabled&&s->runtime&&
        s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&
        s->view.logical_ms==l->logical_ms&&s->runtime==l->runtime&&
        !memcmp(&s->grenade_item_fields,&l->host,sizeof(l->host));
}
static int session_grenade_item_classify(void *user,const GscEntityRef *input,
    const char *name,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef ref,original;char field[128],expected[128];
    SessionGrenadeItemLease lease;int result;
    if(!s||!input||!name||strlen(name)>=sizeof(field))return -1;
    lease=session_grenade_item_lease(s);if(!session_grenade_item_current(s,&lease))return -1;
    ref=original=*input;strcpy(field,name);strcpy(expected,name);
    result=s->grenade_item_fields.classify(s->grenade_item_fields.user,&ref,field,error,cap);
    return session_grenade_item_current(s,&lease)&&!memcmp(&ref,&original,sizeof(ref))&&
        !memcmp(field,expected,strlen(name)+1)?result:-1;
}
static int session_grenade_item_read(void *user,const GscEntityRef *input,
    const char *name,GscStringCastValue *out,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef ref,original;char field[128],expected[128];GscStringCastValue result;
    SessionGrenadeItemLease lease;int ok;
    if(!s||!input||!name||!out||strlen(name)>=sizeof(field))return 0;
    lease=session_grenade_item_lease(s);if(!session_grenade_item_current(s,&lease))return 0;
    ref=original=*input;strcpy(field,name);strcpy(expected,name);memset(&result,0,sizeof(result));
    ok=s->grenade_item_fields.read(s->grenade_item_fields.user,&ref,field,&result,error,cap);
    if(ok!=1||!session_grenade_item_current(s,&lease)||memcmp(&ref,&original,sizeof(ref))||
       memcmp(field,expected,strlen(name)+1))return 0;
    *out=result;return 1;
}
static int session_grenade_item_write(void *user,const GscEntityRef *input,
    const char *name,const GscStringCastValue *value,char *error,size_t cap) {
    GscSession *s=user;GscEntityRef ref,original;char field[128],expected[128];GscStringCastValue copy,before;
    SessionGrenadeItemLease lease;int ok;
    if(!s||!input||!name||!value||strlen(name)>=sizeof(field))return 0;
    lease=session_grenade_item_lease(s);if(!session_grenade_item_current(s,&lease))return 0;
    ref=original=*input;copy=before=*value;strcpy(field,name);strcpy(expected,name);
    ok=s->grenade_item_fields.write(s->grenade_item_fields.user,&ref,field,&copy,error,cap);
    return ok==1&&session_grenade_item_current(s,&lease)&&!memcmp(&ref,&original,sizeof(ref))&&
        !memcmp(field,expected,strlen(name)+1)&&!memcmp(&copy,&before,sizeof(copy));
}
