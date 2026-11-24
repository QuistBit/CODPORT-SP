/* Included by g_spawn.c after its registry/domain/prefix owners. The native
 * origin setter 20038980 copies words and links only an already linked body.
 * This selected lane admits only an unlinked prototype, so every check and
 * the complete commit are pure. No caller/engine callback is hidden here. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "../g_cod1_spawner_pose.h"
static GscEntityHost cod1SpawnerPoseHost, cod1SpawnerPoseBase;
static GscEntityControlHost cod1SpawnerPoseControl, cod1SpawnerPoseControlBase;
static int cod1SpawnerPoseHostReady;

static int SpawnerPosePrivate(const void *p,size_t n) {
    return G_Cod1EntityStorageProtected(p,n) || Cod1ActorPrefixPrivate(p,n) ||
        Cod1SpawnerOverlap(p,n,&g_gametype,sizeof(g_gametype)) ||
        Cod1SpawnerOverlap(p,n,&cod1AiDomainAccounted,sizeof(cod1AiDomainAccounted)) ||
        Cod1SpawnerOverlap(p,n,&cod1AiCoverage,sizeof(cod1AiCoverage)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerStorage,sizeof(cod1SpawnerStorage)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerEpoch,sizeof(cod1SpawnerEpoch)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerIntakeSerial,sizeof(cod1SpawnerIntakeSerial)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerPoseHost,sizeof(cod1SpawnerPoseHost)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerPoseBase,sizeof(cod1SpawnerPoseBase)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerPoseControl,sizeof(cod1SpawnerPoseControl)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerPoseControlBase,sizeof(cod1SpawnerPoseControlBase)) ||
        Cod1SpawnerOverlap(p,n,&cod1SpawnerPoseHostReady,sizeof(cod1SpawnerPoseHostReady));
}
/* Route by the exact retained record before considering mutable eligibility.
 * A known record whose body/class was changed must never fall back to the
 * ordinary script_origin writer and bypass this lane's stronger checks. */
static cod1SpawnerRecord_t *SpawnerPoseRecord(const GscEntityRef *ref) {
    cod1SpawnerRecord_t *record;
    if(!ref || ref->slot>=MAX_GENTITIES)return NULL;
    record=&cod1Spawners[ref->slot];
    return ref->epoch && ref->generation &&
        record->snapshot.entity.epoch==ref->epoch &&
        record->snapshot.entity.slot==ref->slot &&
        record->snapshot.entity.generation==ref->generation ? record : NULL;
}
static int SpawnerPoseClass(const cod1SpawnerRecord_t *record,const char *name) {
    const char *original=NULL;int i;
    if(!record || !name || strncmp(name,"actor_",6) ||
       record->snapshot.rawFieldCount<0 || record->snapshot.rawFieldCount>MAX_SPAWN_VARS ||
       record->snapshot.rawBytes<0 || record->snapshot.rawBytes>MAX_SPAWN_VARS_CHARS)return 0;
    for(i=0;i<record->snapshot.rawFieldCount;++i) {
        size_t key=record->keyOffset[i],value=record->valueOffset[i];
        if(key>=(size_t)record->snapshot.rawBytes || value>=(size_t)record->snapshot.rawBytes ||
           !memchr(record->raw+key,0,(size_t)record->snapshot.rawBytes-key) ||
           !memchr(record->raw+value,0,(size_t)record->snapshot.rawBytes-value))return 0;
        if(!Q_stricmp(record->raw+key,"classname"))original=record->raw+value;
    }
    return original && !strcmp(original,name);
}
static int SpawnerPoseWrite(const GscEntityRef *input,
    const float origin[3],char *error,size_t size,int control) {
    GscEntityRef ref;uint32_t bits[3];cod1SpawnerRecord_t *record;
    cod1EntityNativeStaticSnapshot_t source;gentity_t *ent;int i;
    /* An invalid active-client bound cannot safely classify diagnostics as
     * external storage. Refuse silently before any possible error write. */
    if(level.maxclients<0 || level.maxclients>MAX_CLIENTS ||
       ((level.maxclients>0)!=(level.clients!=NULL)))return 0;
    if((size&&!error) || SpawnerPosePrivate(error,size) ||
       Cod1SpawnerOverlap(input,sizeof(*input),error,size) ||
       Cod1SpawnerOverlap(origin,sizeof(bits),error,size) ||
       SpawnerPosePrivate(input,sizeof(*input)) || SpawnerPosePrivate(origin,sizeof(bits)))return 0;
    if(!input || !origin)return Cod1SpawnerError(error,size,"spawner pose requires copied identity and vector");
    ref=*input;memcpy(bits,origin,sizeof(bits));
    record=SpawnerPoseRecord(&ref);
    if(!record) {
        if(!cod1SpawnerPoseHostReady)return 0;
        if(control)return cod1SpawnerPoseControlBase.setorigin(cod1SpawnerPoseControlBase.user,input,origin,error,size);
        return cod1SpawnerPoseBase.setorigin(cod1SpawnerPoseBase.user,input,origin,error,size);
    }
    for(i=0;i<3;++i)if((bits[i]&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))
        return Cod1SpawnerError(error,size,"spawner origin requires three finite binary32 values");
    if(!record->intakeIdentity || !ref.epoch || ref.epoch==UINT64_MAX ||
       !G_Cod1EntityCollectionReady(ref.epoch,level.num_entities) || level.restarted || level.time<0 ||
       level.maxclients<0 || level.maxclients>MAX_CLIENTS || (level.maxclients&&!level.clients) ||
       !Cod1SpawnerFind(&ref))goto refuse;
    ent=&g_entities[ref.slot];
    if(!(ent->spawnflags&1) || ent->s.solid || ent->neverFree || ent->takedamage || ent->physicsObject ||
       ent->r.svFlags!=(SVF_USE_CURRENT_ORIGIN|SVF_NOCLIENT) ||
       ent->s.modelindex2 || ent->teamchain || ent->teammaster || ent->team ||
       ent->tagParent || ent->tagName || ent->touch || ent->use || ent->think || ent->nextthink)
        goto refuse;
    for(i=0;i<level.num_entities;++i)
        if(g_entities[i].inuse && g_entities[i].tagParent==ent)goto refuse;
    /* This pure capture additionally excludes Actor/Sentient/Ammo/foreign-AI
     * ownership and requires genuine current R175 native-static authority.
     * The actual training prototype has that complete export27 authority. */
    if(!G_Cod1EntityCaptureNativeStaticSource(&ref,&source,NULL,0) ||
       !source.classname.present || !SpawnerPoseClass(record,source.classname.text) ||
       source.modelByte || source.modelByteAuthoritative!=1 ||
       memcmp(ent->s.origin,record->snapshot.origin,sizeof(bits)) ||
       memcmp(ent->r.currentOrigin,record->snapshot.origin,sizeof(bits)) ||
       memcmp(ent->s.pos.trBase,record->snapshot.origin,sizeof(bits)))goto refuse;
    /* Word copies preserve signed zero/subnormals and caller x87/MXCSR state.
     * s.origin and registry pose are explicit portable coherence caches; raw
     * ordered BSP fields and initial extras are deliberately not rewritten. */
    ent->s.pos.trType=TR_STATIONARY;ent->s.pos.trTime=0;ent->s.pos.trDuration=0;
    memset(ent->s.pos.trDelta,0,sizeof(ent->s.pos.trDelta));
    memcpy(ent->s.pos.trBase,bits,sizeof(bits));
    memcpy(ent->r.currentOrigin,bits,sizeof(bits));
    memcpy(ent->s.origin,bits,sizeof(bits));
    memcpy(record->snapshot.origin,bits,sizeof(bits));
    return 1;
refuse:
    return Cod1SpawnerError(error,size,"spawner origin requires its current hidden unreserved typed prototype");
}
static int SpawnerPoseSetOrigin(void *user,const GscEntityRef *ref,
    const float vector[3],char *error,size_t size) {
    (void)user;return SpawnerPoseWrite(ref,vector,error,size,0);
}
static int SpawnerPoseControlOrigin(void *user,const GscEntityRef *ref,
    const float vector[3],char *error,size_t size) {
    (void)user;return SpawnerPoseWrite(ref,vector,error,size,1);
}
static int SpawnerPoseInitialize(void) {
    if(!cod1SpawnerPoseHostReady) {
        const GscEntityHost *base=G_Cod1EntityHost();
        const GscEntityControlHost *controls=G_Cod1EntityControlHost();
        if(!base || !base->setorigin || !controls || !controls->setorigin)return 0;
        cod1SpawnerPoseBase=*base;cod1SpawnerPoseHost=*base;
        cod1SpawnerPoseControlBase=*controls;cod1SpawnerPoseControl=*controls;
        cod1SpawnerPoseControl.setorigin=SpawnerPoseControlOrigin;
        cod1SpawnerPoseHost.setorigin=SpawnerPoseSetOrigin;cod1SpawnerPoseHostReady=1;
    }
    return 1;
}
const GscEntityHost *G_Cod1SpawnerPoseEntityHost(void) {
    return SpawnerPoseInitialize()?&cod1SpawnerPoseHost:NULL;
}
const GscEntityControlHost *G_Cod1SpawnerPoseControlHost(void) {
    return SpawnerPoseInitialize()?&cod1SpawnerPoseControl:NULL;
}
#endif
