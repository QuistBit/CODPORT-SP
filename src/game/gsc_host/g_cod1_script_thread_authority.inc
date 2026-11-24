static int TrainingRigReceiverCurrent(const GscEntityRef *,uint64_t);
extern int G_Cod1SignThreadCurrent(const GscEntityRef *,uint64_t);
extern int G_Cod1TouchThreadCurrent(const GscEntityRef *,uint64_t);
extern int G_Cod1BombWallThreadCurrent(const GscEntityRef *, uint64_t);
extern int G_Cod1GrenadeTargetThreadCurrent(const GscEntityRef *, uint64_t);
static int ScriptThreadTurretCurrent(const GscEntityRef *, uint64_t);
#include "../g_cod1_player_lifetime.h"
/* A script receiver's identity survives ordinary input and death. It does
 * not certify path flags, geometry, native VM references, or any operation
 * performed by the child. The private ClientBegin owner revokes completion
 * on observed allocation/client/role/spawn/lifecycle mismatch. */
static int ScriptThreadPlayerRead(const GscEntityRef *ref,
                                 cod1PlayerLifetime_t *receipt) {
    if(!G_Cod1PlayerLifetimeRead(ref,receipt))return 0;
    /* Read must observe a broken private lifetime before a broader host
     * predicate can reject it. A host-only domain failure is terminal too. */
    if(!PlayerNativePlayer(ref)) {
        G_Cod1PlayerLifetimeRetire(g_entities+ref->slot);
        return 0;
    }
    return 1;
}
static int ScriptThreadPlayerCurrent(const GscEntityRef *input,uint64_t serial) {
    GscEntityRef ref;cod1PlayerLifetime_t receipt;
    if(!input||!serial)return 0;
    ref=*input;
    return ScriptThreadPlayerRead(&ref,&receipt)&&receipt.completion==serial;
}
/* A brush thread belongs to this allocation through hide/show, motion and
 * waits. Reuse of the slot must never resume the former owner's task. */
static int ScriptBrushThreadCurrent(const GscEntityRef *ref,uint64_t generation) {
    const gentity_t *e;
    if (!ref || !generation || generation != ref->generation ||
        !ActorOwnerAllowed(ref,0)) return 0;
    e = &g_entities[ref->slot];
    return e->classname && !strcmp(e->classname,"script_brushmodel") &&
        e->s.eType == ET_MOVER && e->r.bmodel && EntityControlInlineModel(e);
}
static int ScriptThreadResolve(void *user,const GscEntityRef *input,
                              GscEntityExecutionCertificate *out,
                              char *error,size_t size) {
    GscEntityRef ref;cod1PlayerLifetime_t receipt;
    GscEntityExecutionCertificate certificate;(void)user;
    if(!input||!out||
       !ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
    ref=*input;
    if(ScriptThreadPlayerRead(&ref,&receipt)) {
        memset(&certificate,0,sizeof(certificate));certificate.owner=ref;
        certificate.serial=receipt.completion;
        certificate.current=ScriptThreadPlayerCurrent;
        *out=certificate;return 1;
    }
    /* A human player without a current completion cannot obtain a different
     * kind of certificate through Actor/spawner fallback. Those resolvers
     * retain their own complete allocation/lease/intake checks. */
    if(PlayerNativePlayer(&ref)||
       (ref.slot<MAX_GENTITIES&&G_Cod1EntitySameAllocation(&ref)&&
        g_entities[ref.slot].client))return 0;
    if(TrainingRigReceiverCurrent(&ref,ref.generation)){
        memset(&certificate,0,sizeof(certificate));certificate.owner=ref;
        certificate.serial=ref.generation;certificate.current=TrainingRigReceiverCurrent;
        *out=certificate;return 1;
    }
    if(ScriptThreadTurretCurrent(&ref,ref.generation)) {
        GscEntityRef stale=ref;
        stale.generation^=1;
        if(ScriptThreadTurretCurrent(&stale,stale.generation))return 0;
        stale=ref; stale.epoch^=1;
        if(ScriptThreadTurretCurrent(&stale,ref.generation) ||
           ScriptThreadTurretCurrent(&ref,(uint64_t)ref.generation+1))return 0;
        memset(&certificate,0,sizeof(certificate));
        certificate.owner=ref;
        certificate.serial=ref.generation;
        certificate.current=ScriptThreadTurretCurrent;
        *out=certificate;
        G_Printf("[Foley private] current turret thread slot=%u generation=%u stale rejected\n",ref.slot,ref.generation);
        return 1;
    }
    if (G_Cod1BombWallThreadCurrent(&ref, ref.generation)) {
        memset(&certificate, 0, sizeof(certificate));
        certificate.owner = ref;
        certificate.serial = ref.generation;
        certificate.current = G_Cod1BombWallThreadCurrent;
        *out = certificate;
        return 1;
    }
    if (G_Cod1GrenadeTargetThreadCurrent(&ref,ref.generation)) {
        memset(&certificate,0,sizeof(certificate));
        certificate.owner=ref;
        certificate.serial=ref.generation;
        certificate.current=G_Cod1GrenadeTargetThreadCurrent;
        *out=certificate;
        return 1;
    }
    if(G_Cod1TouchThreadCurrent(&ref,ref.generation)){
        GscEntityRef stale=ref;
        stale.epoch^=1;
        if(G_Cod1TouchThreadCurrent(&stale,ref.generation))return 0;
        stale=ref;stale.generation^=1;
        if(G_Cod1TouchThreadCurrent(&stale,stale.generation) ||
           G_Cod1TouchThreadCurrent(&ref,(uint64_t)ref.generation+1))return 0;
        memset(&certificate,0,sizeof(certificate));
        certificate.owner=ref;certificate.serial=ref.generation;
        certificate.current=G_Cod1TouchThreadCurrent;*out=certificate;
        G_Printf("[Foley private] current trigger_multiple thread slot=%u generation=%u stale rejected\n",ref.slot,ref.generation);
        return 1;
    }
    if(G_Cod1SignThreadCurrent(&ref,ref.generation)){
        GscEntityRef stale=ref;
        stale.epoch^=1;
        if(G_Cod1SignThreadCurrent(&stale,ref.generation))return 0;
        stale=ref;stale.generation^=1;
        if(G_Cod1SignThreadCurrent(&stale,stale.generation) ||
           G_Cod1SignThreadCurrent(&ref,(uint64_t)ref.generation+1))return 0;
        memset(&certificate,0,sizeof(certificate));certificate.owner=ref;
        certificate.serial=ref.generation;certificate.current=G_Cod1SignThreadCurrent;
        *out=certificate;
        G_Printf("[Foley private] current trigger_lookat thread slot=%u generation=%u stale rejected\n",ref.slot,ref.generation);
        return 1;
    }
    if(G_Cod1VehicleThreadRead(&ref,&certificate)){*out=certificate;return 1;}
    if(G_Cod1TurretFactoryThreadRead(&ref,&certificate)){*out=certificate;return 1;}
    if (ScriptModelThreadCurrent(&ref,ref.generation)) {
        memset(&certificate,0,sizeof(certificate));certificate.owner=ref;
        certificate.serial=ref.generation;certificate.current=ScriptModelThreadCurrent;
        *out=certificate;return 1;
    }
    if (ScriptBrushThreadCurrent(&ref,ref.generation)) {
        memset(&certificate,0,sizeof(certificate));
        certificate.owner=ref;
        certificate.serial=ref.generation;
        certificate.current=ScriptBrushThreadCurrent;
        *out=certificate;
        return 1;
    }
    return ActorThreadResolve(NULL,&ref,out,error,size);
}
const GscThreadEntityAuthorityHost *G_Cod1ScriptThreadAuthorityHost(void) {
    static const GscThreadEntityAuthorityHost host={NULL,ScriptThreadResolve};
    return &host;
}
