/* Observes the actual selected ControlHost path; the GAME primitive stays pure. */
#include "../g_cod1_spawner.h"
static unsigned gscTestSpawnerPoseWrites;
static int GscTestSpawnerPoseScope(void) {
    GscSessionSnapshot snap;
    return gscTestEntityMode==45 && gscTestBusy && gscTestSession && gscTestId &&
        !gscTestCompleted && GscTestModuleReadCurrent() &&
        gsc_session_snapshot(gscTestSession,&snap) && snap.busy &&
        snap.state==GSC_SESSION_RUNNING && snap.session_id==gscTestId;
}
static int GscTestSpawnerRawHash(const GscEntityRef *ref,int count,uint32_t *out) {
    char key[1024],value[4096],why[256];uint32_t hash=2166136261u;
    int n;const unsigned char *p;
    if(count<1 || count>256)return 0;
    for(n=0;n<count;++n) {
        if(!G_Cod1SpawnerRawField(ref,(size_t)n,key,sizeof(key),value,sizeof(value),why,sizeof(why)))return 0;
        for(p=(const unsigned char *)key;*p;++p)hash=(hash^*p)*16777619u;
        hash=(hash^0u)*16777619u;
        for(p=(const unsigned char *)value;*p;++p)hash=(hash^*p)*16777619u;
        hash=(hash^0u)*16777619u;
    }
    *out=hash;return 1;
}
static int GscTestSpawnerSetOrigin(void *user,const GscEntityRef *ref,
    const float origin[3],char *error,size_t size) {
    const GscEntityControlHost *real=G_Cod1SpawnerPoseControlHost();
    cod1SpawnerSnapshot_t before,after,normalized;
    cod1EntityNativeStaticSnapshot_t staticBefore,staticAfter,staticNormalized;
    uint32_t rawBefore,rawAfter,beforeBits[3],wanted[3],body[3],snapshot[3];
    uint64_t intake;float observed[3];const GscEntityAccessHost *access=G_Cod1EntityAccessHost();
    (void)user;
    if(!ref || !origin || !GscTestSpawnerPoseScope() || !real || !real->setorigin ||
       !G_Cod1SpawnerRead(ref,&before,error,size) ||
       !(intake=G_Cod1SpawnerIntakeIdentity(ref)) ||
       !G_Cod1EntityCaptureNativeStaticSource(ref,&staticBefore,error,size) ||
       !GscTestSpawnerRawHash(ref,before.rawFieldCount,&rawBefore))return 0;
    memcpy(wanted,origin,sizeof(wanted));memcpy(beforeBits,before.origin,sizeof(beforeBits));
    if(!real->setorigin(real->user,ref,origin,error,size) || !GscTestSpawnerPoseScope() ||
       !G_Cod1SpawnerRead(ref,&after,error,size) || G_Cod1SpawnerIntakeIdentity(ref)!=intake ||
       !G_Cod1EntityCaptureNativeStaticSource(ref,&staticAfter,error,size) ||
       !GscTestSpawnerRawHash(ref,after.rawFieldCount,&rawAfter) || rawAfter!=rawBefore ||
       !access || !access->getorigin || !access->getorigin(access->user,ref,observed,error,size))return 0;
    memcpy(body,observed,sizeof(body));memcpy(snapshot,after.origin,sizeof(snapshot));
    normalized=after;memcpy(normalized.origin,before.origin,sizeof(normalized.origin));
    staticNormalized=staticAfter;memcpy(staticNormalized.originBits,staticBefore.originBits,sizeof(staticNormalized.originBits));
    if(memcmp(body,wanted,sizeof(body)) || memcmp(snapshot,wanted,sizeof(snapshot)) ||
       memcmp(staticAfter.originBits,wanted,sizeof(wanted)) ||
       memcmp(&normalized,&before,sizeof(before)) ||
       memcmp(&staticNormalized,&staticBefore,sizeof(staticBefore)) ||
       g_entities[ref->slot].r.linked || !GscTestSpawnerPoseScope())return 0;
    ++gscTestSpawnerPoseWrites;
    G_Printf("[R210] SPAWNER_ORIGIN_WRITE index=%u slot=%u intake=%llu before=%08x,%08x,%08x result=%08x,%08x,%08x raw=%08x fields=%d count=%d team=%d last=%d linked=0 static=1 immutable=1\n",
        gscTestSpawnerPoseWrites,ref->slot,intake,beforeBits[0],beforeBits[1],beforeBits[2],
        body[0],body[1],body[2],rawAfter,after.rawFieldCount,after.count,after.team,after.lastSpawnTime);
    return 1;
}
static const GscEntityControlHost *GscTestSpawnerPoseControlHost(void) {
    static GscEntityControlHost host;
    host=*G_Cod1SpawnerPoseControlHost();host.setorigin=GscTestSpawnerSetOrigin;
    return &host;
}
