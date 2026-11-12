/* R103 private, checked CoD query lane. Sole owner: sv_world.c.
 * No DObj backend is substituted with a bbox. The supported no-DObj branch
 * is the native constructor's initial ordinary hull/link subset. */
#include "../qcommon/cod1_point_capsule.h"
typedef struct {
    int active;
    cod1SpatialActorKey_t key;
    sharedEntity_t *entity;
    uint32_t contents, flags;
    vec3_t origin, angles, mins, maxs;
    vec3_t areaMins, areaMaxs;
    uint64_t topologySerial;
    int hasDObj;
} cod1SpatialActorState_t;

static int CS_ModelLaneCurrent(int slot);
static int CS_LinkModelArea(int slot,cod1SpatialActorState_t *state);

static cod1SpatialActorState_t cod1SpatialActors[MAX_GENTITIES];
static uint64_t cod1SpatialEpoch;
static int cod1SpatialActive;

static void SV_Cod1SpatialClearScene( void ) {
    memset(cod1SpatialActors,0,sizeof(cod1SpatialActors));
    cod1SpatialEpoch=0;
    cod1SpatialActive=0;
}

static void SV_Cod1SpatialDropSlot( int slot ) {
    if (slot<0 || slot>=MAX_GENTITIES) return;
    if (cod1SpatialActors[slot].active) {
        memset(&cod1SpatialActors[slot],0,sizeof(cod1SpatialActors[slot]));
        --cod1SpatialActive;
        if (!cod1SpatialActive) cod1SpatialEpoch=0;
    }
}

static int CS_TableReady( void ) {
    return sv.gentities && sv.gentitySize>=(int)sizeof(sharedEntity_t) &&
        sv.gentitySize<=INT_MAX/MAX_GENTITIES &&
        sv.num_entities>0 && sv.num_entities<=MAX_GENTITIES;
}

static int CS_Finite( const float *v ) {
    int i;
    if (!v) return 0;
    for (i=0;i<3;++i)
        if (!(v[i]>=-FLT_MAX && v[i]<=FLT_MAX)) return 0;
    return 1;
}

static int CS_Bounds( const float *mins, const float *maxs ) {
    int i;
    if (!CS_Finite(mins) || !CS_Finite(maxs)) return 0;
    for (i=0;i<3;++i) if (mins[i]>maxs[i]) return 0;
    return 1;
}

static int CS_Passes( const int32_t *passes ) {
    int i;
    for (i=0;i<2;++i) {
        int slot=passes[i];
        if (slot==ENTITYNUM_NONE) continue;
        if (slot<0 || slot>=sv.num_entities || slot>=ENTITYNUM_WORLD ||
            SV_GentityNum(slot)->s.number!=slot) return 0;
    }
    return 1;
}

static int CS_KeyEqual( const cod1SpatialActorKey_t *a, const cod1SpatialActorKey_t *b ) {
    return a->epoch==b->epoch && a->lease==b->lease &&
        a->slot==b->slot && a->generation==b->generation;
}

static int CS_KeyValid( const cod1SpatialActorKey_t *key ) {
    return key->epoch && key->lease && key->generation &&
        key->slot<ENTITYNUM_WORLD;
}

static int CS_StateValid( int slot, const sharedEntity_t *ent ) {
    const cod1SpatialActorState_t *state=&cod1SpatialActors[slot];
    return state->active && state->entity==ent && state->key.epoch==cod1SpatialEpoch &&
        ent->s.number==slot && ent->r.linked && !ent->r.bmodel &&
        (ent->r.svFlags & SVF_NOCLIENT) && !(ent->r.svFlags & (SVF_BOT|SVF_CASTAI)) &&
        VectorCompare(state->origin,ent->r.currentOrigin) &&
        VectorCompare(state->angles,ent->r.currentAngles) &&
        VectorCompare(state->mins,ent->r.mins) && VectorCompare(state->maxs,ent->r.maxs);
}

/* Cleanup needs the exact registered association, not query eligibility.
 * Same-key callbacks may alter pose or flags and still require unlink. */
static int CS_StateAssociated( int slot, const sharedEntity_t *ent ) {
    const cod1SpatialActorState_t *state=&cod1SpatialActors[slot];
    return state->active && state->entity==ent && state->key.epoch==cod1SpatialEpoch &&
        slot>=0 && slot<sv.num_entities && SV_GentityNum(slot)==ent && ent->s.number==slot;
}

static int CS_Contents( int slot, const sharedEntity_t *ent, uint32_t *contents ) {
    if (slot<0 || slot>=sv.num_entities || slot>=ENTITYNUM_WORLD || ent->s.number!=slot)
        return COD1_SPATIAL_INVALID;
    if (cod1SpatialActors[slot].active) {
        if (!CS_StateValid(slot,ent) || !CS_ModelLaneCurrent(slot)) return COD1_SPATIAL_STALE;
        *contents=cod1SpatialActors[slot].contents;
    } else *contents=(uint32_t)ent->r.contents;
    return COD1_SPATIAL_OK;
}

typedef struct {
    const float *mins, *maxs;
    uint32_t mask;
    int slots[MAX_GENTITIES], count;
} cod1SpatialArea_t;

static int CS_Area_r( worldSector_t *node, cod1SpatialArea_t *area ) {
    svEntity_t *entry;
    for (entry=node->entities;entry;entry=entry->nextEntityInWorldSector) {
        int slot=(int)(entry-sv.svEntities), status;
        uint32_t contents;
        sharedEntity_t *ent;
        if (slot<0 || slot>=MAX_GENTITIES) return COD1_SPATIAL_INVALID;
        ent=SV_GEntityForSvEntity(entry);
        status=CS_Contents(slot,ent,&contents);
        if (status!=COD1_SPATIAL_OK) return status;
        if (!(contents & area->mask)) continue;
        if (!CS_Bounds(ent->r.absmin,ent->r.absmax)) return COD1_SPATIAL_INVALID;
        {
            const float *low=ent->r.absmin, *high=ent->r.absmax;
            if (cod1SpatialActors[slot].active) {
                low=cod1SpatialActors[slot].areaMins;
                high=cod1SpatialActors[slot].areaMaxs;
            }
            if (low[0]>area->maxs[0] || low[1]>area->maxs[1] ||
                low[2]>area->maxs[2] || high[0]<area->mins[0] ||
                high[1]<area->mins[1] || high[2]<area->mins[2]) continue;
        }
        if (area->count>=MAX_GENTITIES) return COD1_SPATIAL_OVERFLOW;
        area->slots[area->count++]=slot;
    }
    if (node->axis!=-1) {
        int status;
        if (area->maxs[node->axis]>node->dist) {
            status=CS_Area_r(node->children[0],area);
            if (status!=COD1_SPATIAL_OK) return status;
        }
        if (area->mins[node->axis]<node->dist) {
            status=CS_Area_r(node->children[1],area);
            if (status!=COD1_SPATIAL_OK) return status;
        }
    }
    return COD1_SPATIAL_OK;
}

int SV_Cod1SpatialArea( const cod1SpatialAreaQuery_t *query, int *slots, int capacity, int *count ) {
    cod1SpatialAreaQuery_t input;
    cod1SpatialArea_t area;
    int status;
    if (!query || !count || capacity<0 || capacity>MAX_GENTITIES || (capacity && !slots))
        return COD1_SPATIAL_INVALID;
    input=*query;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) ||
        !CS_Bounds(input.mins,input.maxs)) return COD1_SPATIAL_INVALID;
    if (!CS_TableReady() || !CM_HasV59() || !sv_numworldSectors) return COD1_SPATIAL_UNAVAILABLE;
    memset(&area,0,sizeof(area));
    area.mins=input.mins; area.maxs=input.maxs; area.mask=input.contentsMask;
    status=CS_Area_r(sv_worldSectors,&area);
    if (status!=COD1_SPATIAL_OK) return status;
    *count=area.count;
    if (area.count>capacity) return COD1_SPATIAL_OVERFLOW;
    if (area.count) memcpy(slots,area.slots,(size_t)area.count*sizeof(*slots));
    return COD1_SPATIAL_OK;
}

/* Exact native two-pass policy. Only the pass owner's NONE sentinel becomes
 * -1. A candidate's owner remains its raw value; this is not recursive. */
static int CS_Ignored( int slot, const sharedEntity_t *ent, const int32_t *passes ) {
    int i;
    for (i=0;i<2;++i) {
        int pass=passes[i], owner;
        if (pass==ENTITYNUM_NONE) continue;
        if (slot==pass || ent->r.ownerNum==pass) return 1;
        owner=SV_GentityNum(pass)->r.ownerNum;
        if (owner==ENTITYNUM_NONE) owner=-1;
        if (ent->r.ownerNum==owner) return 1;
    }
    return 0;
}

static int CS_TraceValid( const trace_t *trace ) {
    return trace->fraction>=0.0f && trace->fraction<=1.0f &&
        CS_Finite(trace->endpos) && CS_Finite(trace->plane.normal) &&
        trace->plane.dist>=-FLT_MAX && trace->plane.dist<=FLT_MAX;
}

/* CS_Area_r first tests the swept AABB. For point rays the same actor
 * bounds can reject diagonal misses before requesting a DObj narrow phase.
 * Include the native area query's unit margin and all boundary contacts. */
static int CS_PointMissesArea(const float *start,const float *end,const float *lo,const float *hi){
    double enter=0.0,leave=1.0;
    int i;
    if(!CS_Bounds(lo,hi))return 0;
    for(i=0;i<3;i++){
        double a=(double)lo[i]-1.0,b=(double)hi[i]+1.0,d=(double)end[i]-start[i];
        if(d==0){
            if(start[i]<a || start[i]>b)return 1;
        }else{
            double t0=(a-start[i])/d,t1=(b-start[i])/d;
            if(t0>t1){double swap=t0;t0=t1;t1=swap;}
            if(t0>enter)enter=t0;
            if(t1<leave)leave=t1;
            if(enter>leave)return 1;
        }
    }
    return 0;
}

static int CS_TargetTrace( trace_t *trace, int slot, const sharedEntity_t *ent,
    const float *start, const float *end, const float *mins, const float *maxs,
    uint32_t mask, int capsule, uint32_t contents ) {
    clipHandle_t model;
    const float *angles=vec3_origin;
    if (cod1SpatialActors[slot].active && cod1SpatialActors[slot].hasDObj &&
        (cod1SpatialActors[slot].flags & COD1_SPATIAL_NATIVE_DOBJ_FLAG))
        return COD1_SPATIAL_UNAVAILABLE;
    if (!CS_Finite(ent->r.currentOrigin) || !CS_Finite(ent->r.currentAngles) ||
        !CS_Bounds(ent->r.mins,ent->r.maxs)) return COD1_SPATIAL_INVALID;
    {
        int i;
        for (i=0;i<3;++i) {
            double offset=((double)mins[i]+maxs[i])*0.5;
            double a=(double)start[i]+offset-ent->r.currentOrigin[i];
            double b=(double)end[i]+offset-ent->r.currentOrigin[i];
            if (a< -FLT_MAX || a>FLT_MAX || b< -FLT_MAX || b>FLT_MAX ||
                (double)ent->r.mins[i]+ent->r.maxs[i]< -FLT_MAX ||
                (double)ent->r.mins[i]+ent->r.maxs[i]>FLT_MAX)
                return COD1_SPATIAL_INVALID;
        }
    }
    if (ent->r.bmodel) {
        if (ent->s.modelindex<=0 || ent->s.modelindex>=CM_NumInlineModels())
            return COD1_SPATIAL_INVALID;
        /* The current V59 kernel supports yaw-only capsule transforms. */
        if (capsule && (ent->r.currentAngles[0] || ent->r.currentAngles[2]))
            return COD1_SPATIAL_UNAVAILABLE;
        model=CM_InlineModel(ent->s.modelindex);
        angles=ent->r.currentAngles;
    } else {
        int targetCapsule;
        if (cod1SpatialActors[slot].active)
            targetCapsule=(cod1SpatialActors[slot].flags & COD1_SPATIAL_NATIVE_CAPSULE_FLAG)!=0;
        else targetCapsule=(ent->r.svFlags & SVF_CAPSULE)!=0;
        if (targetCapsule && !capsule) {
            int i;
            /* Original 41C8D0 -> 41B8F0 is a six-plane capsule/point
             * exchange, including its native endcap behavior. Admit only
             * the moving zero-size branch compared to original x86 code.
             * A swept box or position test retains the explicit refusal. */
            for(i=0;i<3;i++)if(mins[i]!=0 || maxs[i]!=0)
                return COD1_SPATIAL_UNAVAILABLE;
            if(VectorCompare(start,end))return COD1_SPATIAL_UNAVAILABLE;
            C1PC_MovingTrace(trace,start,end,ent->r.mins,ent->r.maxs,
                ent->r.currentOrigin,(int)contents);
            return CS_TraceValid(trace)?COD1_SPATIAL_OK:COD1_SPATIAL_UNAVAILABLE;
        }
        model=CM_TempBoxModelContents(ent->r.mins,ent->r.maxs,targetCapsule,(int)contents);
    }
    CM_Cod1NativeTransformedBoxTrace(trace,start,end,mins,maxs,model,(int)mask,
        ent->r.currentOrigin,angles,capsule);
    if (!CS_TraceValid(trace)) return COD1_SPATIAL_UNAVAILABLE;
    return COD1_SPATIAL_OK;
}

#include "sv_cod1_path_sight.inc"

static int CS_QueryTrace( trace_t *output, const float *start, const float *end,
    const float *mins, const float *maxs, const int32_t *passes, uint32_t mask, int capsule, int sight ) {
    trace_t result;
    cod1SpatialArea_t area;
    int i, status;
    if (!CS_TableReady() || !CM_HasV59() || !sv_numworldSectors) return COD1_SPATIAL_UNAVAILABLE;
    if (!CS_Passes(passes)) return COD1_SPATIAL_INVALID;
    /* Validate arithmetic before entering the CM kernel. Finite endpoints
     * alone do not make subtraction, centering or swept bounds finite. */
    for (i=0;i<3;++i) {
        double sum=(double)mins[i]+maxs[i], offset=sum*0.5;
        double delta=(double)end[i]-start[i];
        double lo=(start[i]<end[i]?start[i]:end[i])+(double)mins[i]-1.0;
        double hi=(start[i]>end[i]?start[i]:end[i])+(double)maxs[i]+1.0;
        double a=(double)start[i]+offset, b=(double)end[i]+offset;
        if (sum< -FLT_MAX || sum>FLT_MAX || delta< -FLT_MAX || delta>FLT_MAX ||
            lo< -FLT_MAX || hi>FLT_MAX || a< -FLT_MAX || a>FLT_MAX ||
            b< -FLT_MAX || b>FLT_MAX) return COD1_SPATIAL_INVALID;
    }
    if (!CM_Cod1NativeWorldTrace(&result,start,end,mins,maxs,(int)mask,capsule) ||
        !CS_TraceValid(&result)) return COD1_SPATIAL_UNAVAILABLE;
    if (result.fraction==0.0f || (sight &&
        (result.fraction<1.0f || result.startsolid || result.allsolid))) {
        *output=result; return COD1_SPATIAL_OK;
    }
    memset(&area,0,sizeof(area));
    {
        static const float zero[3]={0,0,0};
        float low[3], high[3];
        for (i=0;i<3;++i) {
            double lo=(start[i]<end[i]?start[i]:end[i])+(double)mins[i]-1.0;
            double hi=(start[i]>end[i]?start[i]:end[i])+(double)maxs[i]+1.0;
            if (lo< -FLT_MAX || hi>FLT_MAX) return COD1_SPATIAL_INVALID;
            low[i]=(float)lo; high[i]=(float)hi;
        }
        area.mins=low; area.maxs=high; area.mask=mask;
        status=CS_Area_r(sv_worldSectors,&area);
        if (status!=COD1_SPATIAL_OK) return status;
        /* The collected slot list owns no pointers into these local bounds. */
        area.mins=zero; area.maxs=zero;
    }
    for (i=0;i<area.count;++i) {
        int slot=area.slots[i];
        sharedEntity_t *ent=SV_GentityNum(slot);
        trace_t candidate;
        uint32_t contents;
        if (result.allsolid) break;
        if (CS_Ignored(slot,ent,passes)) continue;
        status=CS_Contents(slot,ent,&contents);
        if (status!=COD1_SPATIAL_OK) return status;
        if(!capsule && !sight && mask==UINT32_C(0x0280e031) &&
           cod1SpatialActors[slot].active && cod1SpatialActors[slot].hasDObj &&
           (cod1SpatialActors[slot].flags & COD1_SPATIAL_NATIVE_DOBJ_FLAG) &&
           VectorCompare(mins,vec3_origin) && VectorCompare(maxs,vec3_origin) &&
           CS_PointMissesArea(start,end,cod1SpatialActors[slot].areaMins,cod1SpatialActors[slot].areaMaxs))
            continue;
        status=CS_TargetTrace(&candidate,slot,ent,start,end,mins,maxs,mask,capsule,contents);
        if (status!=COD1_SPATIAL_OK) {
            if(mask==UINT32_C(0x0280e031)) Com_Printf("[R234] shot target trace refusal status=%d slot=%d native=%d dobj=%d flags=%08x svFlags=%08x bmodel=%d contents=%08x origin=(%g %g %g) mins=(%g %g %g) maxs=(%g %g %g) start=(%g %g %g) end=(%g %g %g) worldFraction=%g\n",status,slot,cod1SpatialActors[slot].active,cod1SpatialActors[slot].hasDObj,cod1SpatialActors[slot].flags,ent->r.svFlags,ent->r.bmodel,contents,ent->r.currentOrigin[0],ent->r.currentOrigin[1],ent->r.currentOrigin[2],ent->r.mins[0],ent->r.mins[1],ent->r.mins[2],ent->r.maxs[0],ent->r.maxs[1],ent->r.maxs[2],start[0],start[1],start[2],end[0],end[1],end[2],result.fraction);
            return status;
        }
        if (candidate.fraction<result.fraction) {
            qboolean oldStart=result.startsolid, oldAll=result.allsolid;
            candidate.entityNum=slot;
            /* Generic native clip merge preserves backend contents. DObj
             * point trace's separate restitution path is not implemented. */
            result=candidate;
            result.startsolid|=oldStart; result.allsolid|=oldAll;
        } else {
            result.startsolid|=candidate.startsolid;
            result.allsolid|=candidate.allsolid;
        }
        /* Native sight is a boolean first-blocker query, not a nearest-hit
         * search. Later targets cannot invalidate an already proved block. */
        if (sight && (result.fraction<1.0f || result.startsolid || result.allsolid)) {
            *output=result; return COD1_SPATIAL_OK;
        }
    }
    *output=result;
    return COD1_SPATIAL_OK;
}

int SV_Cod1SpatialCapsuleTrace( const cod1SpatialTraceQuery_t *query, trace_t *output ) {
    cod1SpatialTraceQuery_t input;
    trace_t result;
    int status;
    double widthX, widthY, height;
    if (!query || !output) return COD1_SPATIAL_INVALID;
    input=*query;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) ||
        !CS_Finite(input.start) || !CS_Finite(input.end) || !CS_Bounds(input.mins,input.maxs))
        return COD1_SPATIAL_INVALID;
    widthX=(double)input.maxs[0]-input.mins[0];
    widthY=(double)input.maxs[1]-input.mins[1];
    height=(double)input.maxs[2]-input.mins[2];
    if (!(widthX>0 && widthX==widthY && widthX<=FLT_MAX && height>0 && height<=FLT_MAX))
        return COD1_SPATIAL_INVALID;
    status=CS_QueryTrace(&result,input.start,input.end,input.mins,input.maxs,
        input.passEntity,input.contentsMask,qtrue,qfalse);
    if (status==COD1_SPATIAL_OK) *output=result;
    return status;
}

/* R113 reuses the versioned typed trace shape with an exact zero-size hull.
 * Preserve raw contents and both exclusions through the existing native-only
 * nearest-hit lane. Unsupported capsule/DObj geometry remains unavailable;
 * a failure never publishes an incomplete world or dynamic result. */
int SV_Cod1SpatialPointTrace( const cod1SpatialTraceQuery_t *query, trace_t *output ) {
    cod1SpatialTraceQuery_t input;
    trace_t result;
    int i, status;
    if (!query || !output) return COD1_SPATIAL_INVALID;
    input=*query;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) ||
        !CS_Finite(input.start) || !CS_Finite(input.end)) return COD1_SPATIAL_INVALID;
    for (i=0;i<3;++i)
        if (input.mins[i]!=0.0f || input.maxs[i]!=0.0f) return COD1_SPATIAL_INVALID;
    status=CS_QueryTrace(&result,input.start,input.end,input.mins,input.maxs,
        input.passEntity,input.contentsMask,qfalse,qfalse);
    if (status==COD1_SPATIAL_OK) *output=result;
    return status;
}

int SV_Cod1SpatialSight( const cod1SpatialSightQuery_t *query, int *blocked ) {
    cod1SpatialSightQuery_t input;
    trace_t result;
    int status;
    if (!query || !blocked) return COD1_SPATIAL_INVALID;
    input=*query;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) ||
        !CS_Finite(input.start) || !CS_Finite(input.end)) return COD1_SPATIAL_INVALID;
    status=CS_QueryTrace(&result,input.start,input.end,vec3_origin,vec3_origin,
        input.passEntity,input.contentsMask,qfalse,qtrue);
    if (status==COD1_SPATIAL_OK)
        *blocked=result.fraction<1.0f || result.startsolid || result.allsolid;
    return status;
}

int SV_Cod1SpatialActorLink( const cod1SpatialActorLink_t *request ) {
    cod1SpatialActorLink_t input;
    cod1SpatialActorState_t state;
    sharedEntity_t *ent;
    int i;
    if (!request) return COD1_SPATIAL_INVALID;
    input=*request;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input)) return COD1_SPATIAL_INVALID;
    if (!CS_TableReady() || !CM_HasV59() || !sv_numworldSectors) return COD1_SPATIAL_UNAVAILABLE;
    if (!CS_KeyValid(&input.key) || input.key.slot>=(uint32_t)sv.num_entities ||
        input.nativeContents!=COD1_SPATIAL_ACTOR_CONTENTS)
        return COD1_SPATIAL_INVALID;
    if (input.hasDObj && (!cod1SpatialActors[input.key.slot].active ||
        !cod1SpatialActors[input.key.slot].hasDObj || !CS_ModelLaneCurrent((int)input.key.slot))) return COD1_SPATIAL_UNAVAILABLE;
    if (!input.hasDObj && cod1SpatialActors[input.key.slot].active && cod1SpatialActors[input.key.slot].hasDObj) return COD1_SPATIAL_STALE;
    if (cod1SpatialEpoch && input.key.epoch!=cod1SpatialEpoch) return COD1_SPATIAL_STALE;
    if (cod1SpatialActors[input.key.slot].active &&
        !CS_KeyEqual(&cod1SpatialActors[input.key.slot].key,&input.key)) return COD1_SPATIAL_STALE;
    ent=SV_GentityNum((int)input.key.slot);
    if (ent->s.number!=(int)input.key.slot || ent->r.bmodel ||
        !(ent->r.svFlags & SVF_NOCLIENT) || (ent->r.svFlags & (SVF_BOT|SVF_CASTAI)) ||
        !CS_Finite(ent->r.currentOrigin) || !CS_Finite(ent->r.currentAngles) ||
        !CS_Bounds(ent->r.mins,ent->r.maxs) ||
        (ent->r.linked && !cod1SpatialActors[input.key.slot].active)) return COD1_SPATIAL_INVALID;
    for (i=0;i<3;++i) {
        double low=(double)ent->r.currentOrigin[i]+ent->r.mins[i]-1.0;
        double high=(double)ent->r.currentOrigin[i]+ent->r.maxs[i]+1.0;
        if (low< -FLT_MAX || high>FLT_MAX) return COD1_SPATIAL_INVALID;
    }
    memset(&state,0,sizeof(state));
    state.active=1; state.key=input.key; state.entity=ent;
    state.contents=input.nativeContents; state.flags=input.nativeServerFlags;
    VectorCopy(ent->r.currentOrigin,state.origin); VectorCopy(ent->r.currentAngles,state.angles);
    VectorCopy(ent->r.mins,state.mins); VectorCopy(ent->r.maxs,state.maxs);
    state.hasDObj=input.hasDObj!=0;
    if(state.hasDObj){state.topologySerial=cod1SpatialActors[input.key.slot].topologySerial;if(!CS_LinkModelArea((int)input.key.slot,&state))return COD1_SPATIAL_STALE;}
    SV_LinkEntityArea(ent,state.hasDObj?state.areaMins:NULL,state.hasDObj?state.areaMaxs:NULL);
    if (!ent->r.linked) return COD1_SPATIAL_UNAVAILABLE;
    if(!state.hasDObj){VectorCopy(ent->r.absmin,state.areaMins); VectorCopy(ent->r.absmax,state.areaMaxs);}
    if (!cod1SpatialActors[input.key.slot].active) ++cod1SpatialActive;
    cod1SpatialActors[input.key.slot]=state;
    cod1SpatialEpoch=input.key.epoch;
    return COD1_SPATIAL_OK;
}

int SV_Cod1SpatialActorUnlink( const cod1SpatialActorUnlink_t *request ) {
    cod1SpatialActorUnlink_t input;
    cod1SpatialActorState_t *state;
    if (!request) return COD1_SPATIAL_INVALID;
    input=*request;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input)) return COD1_SPATIAL_INVALID;
    if (!CS_KeyValid(&input.key)) return COD1_SPATIAL_INVALID;
    state=&cod1SpatialActors[input.key.slot];
    /* Idempotent absence never unlinks the possibly replaced shared slot. */
    if (!state->active) return COD1_SPATIAL_OK;
    if (!CS_TableReady()) return COD1_SPATIAL_UNAVAILABLE;
    if (!CS_KeyEqual(&state->key,&input.key) || !CS_StateAssociated((int)input.key.slot,state->entity))
        return COD1_SPATIAL_STALE;
    SV_UnlinkEntity(state->entity);
    return COD1_SPATIAL_OK;
}

int SV_Cod1SpatialActorContents(const cod1SpatialActorContents_t *request) {
    cod1SpatialActorContents_t input;
    cod1SpatialActorState_t *state;
    if (!request) return COD1_SPATIAL_INVALID;
    input=*request;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) ||
        !CS_KeyValid(&input.key)) return COD1_SPATIAL_INVALID;
    if (!CS_TableReady()) return COD1_SPATIAL_UNAVAILABLE;
    if (input.key.slot>=(uint32_t)sv.num_entities) return COD1_SPATIAL_STALE;
    state=&cod1SpatialActors[input.key.slot];
    if (!CS_KeyEqual(&state->key,&input.key) ||
        !CS_StateAssociated((int)input.key.slot,SV_GentityNum((int)input.key.slot)) ||
        state->contents!=input.expectedContents) return COD1_SPATIAL_STALE;
    /* CS_Contents is the query consumer. GAME r.contents stays untouched. */
    state->contents=input.replacementContents;
    return COD1_SPATIAL_OK;
}

int SV_Cod1SpatialSceneReset( const cod1SpatialSceneReset_t *request ) {
    cod1SpatialSceneReset_t input;
    int i;
    if (!request) return COD1_SPATIAL_INVALID;
    input=*request;
    if (input.version!=COD1_SPATIAL_VERSION || input.bytes!=sizeof(input) || !input.epoch)
        return COD1_SPATIAL_INVALID;
    if (!cod1SpatialEpoch) return COD1_SPATIAL_OK;
    if (cod1SpatialEpoch!=input.epoch) return COD1_SPATIAL_STALE;
    if (!CS_TableReady()) return COD1_SPATIAL_UNAVAILABLE;
    for (i=0;i<MAX_GENTITIES;++i) if (cod1SpatialActors[i].active) {
        if (i>=sv.num_entities || !CS_StateAssociated(i,SV_GentityNum(i))) return COD1_SPATIAL_STALE;
    }
    for (i=0;i<MAX_GENTITIES;++i) if (cod1SpatialActors[i].active)
        SV_UnlinkEntity(cod1SpatialActors[i].entity);
    SV_Cod1SpatialClearScene();
    return COD1_SPATIAL_OK;
}
