/* R191 isAI: the native body checks entity+0x108 for nonzero, not readiness.
 * This sidecar represents that pointer by a validated Actor allocation serial.
 * Extra CoD map/identity/foreign-owner checks are bounded port policy. */
int G_Cod1ActorPresenceRead(const GscEntityRef *input,uint64_t *out,
    char *error,size_t size) {
    GscEntityRef ref;cod1ActorLease_t lease;const cod1ActorRecord_t *a;
    const gentity_t *e;uint32_t i;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
    if(!input||!out){EntityError(error,size,"isAI requires an identity and serial output");return 0;}
    ref=*input;
    if(!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted){
        EntityError(error,size,"isAI requires a ready fresh CoD SP map");return 0;}
    if(!G_Cod1EntitySameAllocation(&ref)){*out=0;return 1;}
    e=&g_entities[ref.slot];
    if(cod1ActorByEntity[ref.slot]){
        i=cod1ActorByEntity[ref.slot]-1;
        if(i>=COD1_ACTOR_CAPACITY){EntityError(error,size,"isAI Actor index is invalid");return 0;}
        a=&cod1Actors[i];memset(&lease,0,sizeof(lease));lease.entity=ref;
        lease.serial=a->serial;lease.actorIndex=i;lease.sentientIndex=a->sentientIndex;
        lease.kind=COD1_ACTOR_LEASE_ACTOR;
        if(!ActorLeaseValid(&lease)){EntityError(error,size,"isAI Actor ownership is inconsistent");return 0;}
        *out=lease.serial;return 1;
    }
    /* A foreign RTCW Actor cannot establish presence OR absence of CoD Actor.
     * Classname, health, eType and arbitrary client pointers are not evidence. */
    if((e->r.svFlags&(SVF_CASTAI|SVF_BOT))||e->aiCharacter||e->aiAttributes||
       e->AIScript_AlertEntity||e->aiInactive||(e->aiName&&strcmp(e->aiName,"player"))){
        EntityError(error,size,"isAI cannot classify retained RTCW AI ownership");return 0;}
    if(e->client||ref.slot<MAX_CLIENTS){
        if(!level.clients||level.maxclients<=0||level.maxclients>MAX_CLIENTS||
           ref.slot>=MAX_CLIENTS||ref.slot>=(uint32_t)level.maxclients||
           e->client!=&level.clients[ref.slot]||e->client->pers.connected!=CON_CONNECTED||
           e->client->ps.aiChar){EntityError(error,size,"isAI player binding is unconfirmed");return 0;}
    }
    *out=0;return 1;
}
static int ActorPresenceRead(void *user,const GscEntityRef *ref,uint64_t *serial,
    char *error,size_t size) {
    (void)user;return G_Cod1ActorPresenceRead(ref,serial,error,size);
}
static int ActorPresenceCurrent(void *user,const GscEntityRef *ref,uint64_t serial) {
    uint64_t now=0;char error[128];(void)user;
    return G_Cod1EntitySameAllocation(ref)&&G_Cod1ActorPresenceRead(ref,&now,error,sizeof(error))&&now==serial;
}
const GscActorPresenceHost *G_Cod1ActorPresenceHost(void) {
    static const GscActorPresenceHost host={NULL,ActorPresenceRead,ActorPresenceCurrent};return &host;
}
