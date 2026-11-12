/* R115 private reliable script FX consumer. Script resource indices are
 * never renderer/cache handles. Arrival cg.time and delivery are port policy. */
#include "cg_cod1_script_fx.h"

typedef struct {
    char name[COD1_SCRIPT_FX_NAME_CAPACITY];
    uint64_t ticket;
    int cache_id,registering;
} cod1ScriptFxClientResource_t;
typedef struct {
    uint64_t incarnation,epoch,sequence,generation,ticket;
    int index,time;
} cod1ScriptFxClientLease_t;
static struct {
    uint64_t epoch,floor,sequence,incarnation,ticket,generation;
    cod1ScriptFxClientResource_t resource[MAX_COD1_SCRIPT_FX];
    int last_time,have_time;
} cgCod1ScriptFx;
static void CFT_Reset(qboolean full);
static void CFL_Reset(qboolean full);
static void CFL_ResetEnd(void);

static void CFS_Advance(uint64_t *value) { if(!++*value)++*value; }

void CG_Cod1ScriptFxReset(qboolean full) {
    CFL_Reset(full);
    CFT_Reset(full);
    CG_Cod1EfxScriptClear();
    CFS_Advance(&cgCod1ScriptFx.incarnation);
    cgCod1ScriptFx.have_time=0;cgCod1ScriptFx.last_time=0;
    cgCod1ScriptFx.generation=CG_Cod1EfxGeneration();
    if(full) {
        cgCod1ScriptFx.epoch=0;cgCod1ScriptFx.floor=0;cgCod1ScriptFx.sequence=0;
        memset(cgCod1ScriptFx.resource,0,sizeof(cgCod1ScriptFx.resource));
    } else {
        int i;
        for(i=1;i<MAX_COD1_SCRIPT_FX;++i)cgCod1ScriptFx.resource[i].registering=0;
    }
    CFL_ResetEnd();
}

static int CFS_SynchronizeGeneration(void) {
    uint64_t generation=CG_Cod1EfxGeneration();
    int i;
    if(cgCod1ScriptFx.generation==generation)return 0;
    /* Full EFX shutdown destroys cache IDs. Keep scene/replay history while
     * invalidating every registration; next observer/command reacquires it. */
    CFS_Advance(&cgCod1ScriptFx.incarnation);
    cgCod1ScriptFx.generation=generation;
    for(i=1;i<MAX_COD1_SCRIPT_FX;++i) {
        cgCod1ScriptFx.resource[i].cache_id=0;
        cgCod1ScriptFx.resource[i].registering=0;
    }
    return 1;
}

static cod1ScriptFxClientLease_t CFS_Lease(int index) {
    cod1ScriptFxClientLease_t lease;
    lease.incarnation=cgCod1ScriptFx.incarnation;
    lease.epoch=cgCod1ScriptFx.epoch;lease.sequence=cgCod1ScriptFx.sequence;
    lease.generation=cgCod1ScriptFx.generation;lease.index=index;lease.time=cg.time;
    lease.ticket=index?cgCod1ScriptFx.resource[index].ticket:0;
    return lease;
}
static int CFS_Current(void *user) {
    const cod1ScriptFxClientLease_t *lease=(const cod1ScriptFxClientLease_t *)user;
    return lease && lease->time==cg.time && lease->incarnation==cgCod1ScriptFx.incarnation &&
        lease->epoch && lease->epoch==cgCod1ScriptFx.epoch &&
        lease->sequence==cgCod1ScriptFx.sequence &&
        lease->generation==cgCod1ScriptFx.generation &&
        lease->generation==CG_Cod1EfxGeneration() &&
        (!lease->index || (lease->index>0 && lease->index<MAX_COD1_SCRIPT_FX &&
          lease->ticket==cgCod1ScriptFx.resource[lease->index].ticket));
}
static int CFS_OwnsResource(const cod1ScriptFxClientLease_t *lease) {
    /* Cancellation on sequence or clock changes may release only the same
     * still-owned unpublished reservation; it never authorizes a commit. */
    return lease->index>0 && lease->index<MAX_COD1_SCRIPT_FX &&
        lease->incarnation==cgCod1ScriptFx.incarnation &&
        lease->epoch==cgCod1ScriptFx.epoch &&
        lease->generation==cgCod1ScriptFx.generation &&
        lease->generation==CG_Cod1EfxGeneration() &&
        lease->ticket==cgCod1ScriptFx.resource[lease->index].ticket;
}

static int CFS_ReadResource(int index,cod1ScriptFxResource_t *out,
    cod1ScriptFxClientLease_t *lease) {
    const char *text;
    cod1ScriptFxResource_t resource;
    if(index<1 || index>=MAX_COD1_SCRIPT_FX || !CFS_Current(lease))return 0;
    text=CG_ConfigString(CS_COD1_SCRIPT_FX_TABLE+index);
    if(!CFS_Current(lease) || !text || !CF_DecodeResource(text,&resource) ||
       resource.epoch!=lease->epoch || resource.index!=index)return 0;
    *out=resource;return 1;
}

static int CFS_Acquire(const cod1ScriptFxResource_t *resource) {
    cod1ScriptFxClientResource_t *entry;
    cod1ScriptFxClientLease_t lease;
    char copied[COD1_SCRIPT_FX_NAME_CAPACITY];
    int id,index=resource->index;
    if(resource->epoch!=cgCod1ScriptFx.epoch || index<1 || index>=MAX_COD1_SCRIPT_FX)return 0;
    entry=&cgCod1ScriptFx.resource[index];
    /* Within a namespace a published index is immutable. A different raw
     * name cannot rebind an already received reliable event to another FX. */
    if(entry->name[0] && strcmp(entry->name,resource->name))return 0;
    if(entry->registering)return 0;
    if(!entry->name[0]) {
        strcpy(entry->name,resource->name);
        CFS_Advance(&cgCod1ScriptFx.ticket);entry->ticket=cgCod1ScriptFx.ticket;
    }
    if(entry->cache_id)return entry->cache_id>0?entry->cache_id:0;
    strcpy(copied,entry->name);entry->registering=1;
    lease=CFS_Lease(index);
    id=CG_Cod1EfxScriptRegister(copied,CFS_Current,&lease);
    /* Registration may have replaced this namespace or reset the cache.
     * The old entry pointer must not be used after the external calls. */
    if(!CFS_Current(&lease)) {
        /* A different admitted command can advance sequence without retiring
         * this resource owner. Release only our own busy reservation; never
         * touch the replacement namespace/ticket/generation. */
        if(CFS_OwnsResource(&lease))cgCod1ScriptFx.resource[index].registering=0;
        return 0;
    }
    cgCod1ScriptFx.resource[index].registering=0;
    cgCod1ScriptFx.resource[index].cache_id=id>0?id:-1;
    return id>0?id:0;
}

void CG_Cod1ScriptFxConfigChanged(int number) {
    cod1ScriptFxClientLease_t lease;
    cod1ScriptFxResource_t resource;
    uint64_t epoch,incarnation,floor;
    const char *text;
    int i,synchronized=CFS_SynchronizeGeneration();
    if(number==CS_COD1_SCRIPT_FX_METADATA) {
        incarnation=cgCod1ScriptFx.incarnation;
        text=CG_ConfigString(number);
        if(incarnation!=cgCod1ScriptFx.incarnation || !text)return;
        if(!text[0]) {
            floor=cgCod1ScriptFx.floor;
            CG_Cod1ScriptFxReset(qtrue);cgCod1ScriptFx.floor=floor;
            return;
        }
        if(!CF_DecodeMetadata(text,&epoch))return;
        if(epoch!=cgCod1ScriptFx.epoch) {
            if(epoch<=cgCod1ScriptFx.floor)return;
            CG_Cod1ScriptFxReset(qtrue);
            cgCod1ScriptFx.epoch=epoch;cgCod1ScriptFx.floor=epoch;
        } else if(!synchronized)return;
        for(i=1;i<MAX_COD1_SCRIPT_FX;++i) {
            lease=CFS_Lease(0);
            if(CFS_ReadResource(i,&resource,&lease)) {
                (void)CFS_Acquire(&resource);
                if(lease.incarnation!=cgCod1ScriptFx.incarnation || lease.epoch!=cgCod1ScriptFx.epoch)return;
            } else if(!CFS_Current(&lease))return;
        }
    } else if(number>CS_COD1_SCRIPT_FX_TABLE &&
              number<CS_COD1_SCRIPT_FX_TABLE+MAX_COD1_SCRIPT_FX && cgCod1ScriptFx.epoch) {
        lease=CFS_Lease(0);
        if(CFS_ReadResource(number-CS_COD1_SCRIPT_FX_TABLE,&resource,&lease))
            (void)CFS_Acquire(&resource);
    }
}

void CG_Cod1ScriptFxCommand(void) {
    char copied[COD1_SCRIPT_FX_ARGC][24];
    const char *arguments[COD1_SCRIPT_FX_ARGC];
    cod1ScriptFx_t event;
    cod1ScriptFxResource_t resource;
    cod1ScriptFxClientLease_t lease;
    vec3_t direction;
    int i,count,id;
    CFS_SynchronizeGeneration();
    if(!cgCod1ScriptFx.epoch)return;
    lease=CFS_Lease(0);
    count=trap_Argc();
    if(!CFS_Current(&lease) || count!=COD1_SCRIPT_FX_ARGC)return;
    for(i=0;i<count;++i) {
        memset(copied[i],0,sizeof(copied[i]));
        trap_Argv(i,copied[i],sizeof(copied[i]));
        if(!CFS_Current(&lease))return;
        arguments[i]=copied[i];
    }
    /* No cache registration or cosmetic random consumption before this
     * replay test. Bad/current-old commands cannot prime resource state. */
    if(!CF_DecodeCommand(count,arguments,&event) ||
       !CF_CommandFollows(&event,lease.epoch,lease.sequence))return;
    if(cg.time<0 || (cgCod1ScriptFx.have_time && cg.time<cgCod1ScriptFx.last_time)) {
        CG_Cod1ScriptFxReset(qfalse);return;
    }
    if(cgCod1ScriptFx.resource[event.index].registering)return;
    if(!CFS_ReadResource(event.index,&resource,&lease))return;
    if(cgCod1ScriptFx.resource[event.index].name[0] &&
       strcmp(cgCod1ScriptFx.resource[event.index].name,resource.name))return;
    /* Consume an admitted event before acquisition and external callbacks.
     * Unsupported/missing/disabled visuals are not queued for later replay. */
    cgCod1ScriptFx.sequence=event.sequence;
    cgCod1ScriptFx.last_time=cg.time;cgCod1ScriptFx.have_time=1;
    lease=CFS_Lease(0);
    id=CFS_Acquire(&resource);
    if(!CFS_Current(&lease))return;
    lease=CFS_Lease(event.index);
    if(!id || lease.epoch!=event.epoch || lease.sequence!=event.sequence || !CFS_Current(&lease))return;
    if(event.directed) {
        ByteToDir((int)event.direction_code,direction);
        for(i=0;i<3;++i)direction[i]*=event.direction_length;
    } else VectorClear(direction);
    (void)CG_Cod1EfxScriptPlay(id,event.origin,event.directed,direction,CFS_Current,&lease);
}

#include "cg_cod1_tag_fx.inc"
#include "cg_cod1_loop_fx.inc"
