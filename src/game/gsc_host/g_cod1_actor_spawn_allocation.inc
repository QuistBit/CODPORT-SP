#include "../g_cod1_spawner.h"
static int cod1ActorSpawnAllocationBusy;
int G_Cod1ActorAllocateSpawnPrefixSession(GscSession *session,uint64_t id,
    const cod1ActorSpawnPrefixRequest_t *input,cod1ActorSpawnPrefixResult_t *out,
    char *error,size_t size) {
    cod1ActorSpawnPrefixRequest_t copy;cod1ActorSpawnPrefixResult_t result;
    cod1ActorProjectedEntryRequest_t request;gentity_t *entity;
    uint64_t program,intake;int ok=0;
    if(!session||!input||!out||
       !gsc_session_spawn_entry_storage_separate(session,input,sizeof(*input))||
       !gsc_session_spawn_entry_storage_separate(session,out,sizeof(*out))||
       !gsc_session_spawn_entry_storage_separate(session,error,size)||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    copy=*input;memset(&result,0,sizeof(result));*out=result;
    if(cod1ActorSpawnAllocationBusy||cod1ActorInheritedEntryBusy||
       !NativeStaticTextValid(&copy.targetname)||
       !memchr(copy.module,0,sizeof(copy.module))||!copy.module[0]||
       !memchr(copy.function,0,sizeof(copy.function))||!copy.function[0]||
       !(program=gsc_session_spawn_entry_program(session,id,&copy.spawner))||
       !EntityValidate(NULL,&copy.spawner)||
       !(intake=G_Cod1SpawnerIntakeIdentity(&copy.spawner))||
       level.spawning||level.restarted||!G_Cod1RngOwnerReady()) {
        EntityError(error,size,"allocated actor prefix requires current factory and typed spawner");return 0;
    }
    /* Refuse bounded resource exhaustion before entering the fatal engine
     * allocator path. No native successful-null or ready result is invented. */
    if(level.num_entities>=ENTITYNUM_MAX_NORMAL&&!G_EntitiesFree()) {
        EntityError(error,size,"allocated actor prefix has no free entity slot");return 0;
    }
    cod1ActorSpawnAllocationBusy=1;
    entity=G_Spawn();
    if(!entity||!G_Cod1EntityCapture(entity,&result.allocation)) {
        EntityError(error,size,"fresh GAME allocation did not retain its identity");goto done;
    }
    result.allocated=1;
    if(gsc_session_spawn_entry_program(session,id,&copy.spawner)!=program||
       G_Cod1SpawnerIntakeIdentity(&copy.spawner)!=intake||
       !EntityValidate(NULL,&copy.spawner)||!G_Cod1EntitySameAllocation(&result.allocation)) {
        EntityError(error,size,"actor factory owner changed during allocation");goto failed;
    }
    /* Port staging flag only; native flags remain explicit separate inputs. */
    entity->r.svFlags|=SVF_NOCLIENT;
    memset(&request,0,sizeof(request));request.inherited.source=copy.spawner;
    request.inherited.destination=result.allocation;
    request.inherited.nativeServerFlags=copy.nativeServerFlags;
    request.inherited.nativeEntityFlags=copy.nativeEntityFlags;
    memcpy(request.inherited.module,copy.module,sizeof(copy.module));
    memcpy(request.inherited.function,copy.function,sizeof(copy.function));
    request.targetname=copy.targetname;
    ok=G_Cod1ActorRunProjectedSpawnEntrySession(session,id,&request,&result.prefix,error,size);
    if(ok&&(G_Cod1SpawnerIntakeIdentity(&copy.spawner)!=intake||
       !EntityValidate(NULL,&copy.spawner)||!G_Cod1EntitySameAllocation(&result.allocation))) {
        ok=0;EntityError(error,size,"actor prefix owner changed during script entry");
    }
    if(ok)goto done;
failed:
    /* Never free a replacement occupying the old slot. Actual G_FreeEntity
     * repeats this identity check after its unlink and resource callbacks. */
    if(G_Cod1EntitySameAllocation(&result.allocation))G_FreeEntity(&g_entities[result.allocation.slot]);
    result.cleanupPending=G_Cod1EntitySameAllocation(&result.allocation);
    result.retired=!result.cleanupPending;
    result.prefix.inherited.leaseRetained=result.prefix.inherited.lease.serial&&
        ActorLeaseValid(&result.prefix.inherited.lease);
done:
    cod1ActorSpawnAllocationBusy=0;*out=result;return ok;
}
