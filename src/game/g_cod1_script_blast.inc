/* R113 nonactor / R200 connected-player script blast. Generic RTCW explosions, rockets and
 * grenades keep their existing helpers. The selected domain uses native point
 * traces with an opaque original mask, never a MASK_SOLID approximation.
 * All targets/LOS are preflighted before damage; committed damage is retained
 * if a subsequent allocation, geometry or session boundary changes. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include <math.h>
#include <limits.h>
#define COD1_SCRIPT_RADIUS_LOS_MASK UINT32_C(0x0280e091)

typedef struct {
    GscEntityRef entity;
    vec3_t origin,absmin,absmax,dir;
    int bmodel,linked,takedamage,svFlags,eType,eligible,visible,damage;
    gclient_t *client;
    const playerState_t *playerState;
    vec3_t eye;
    int health;
} cod1ScriptBlastTarget_t;

/* A real local player has an existing stance/lean eye provider. This does not
 * classify an actor reservation or a model as a human client. Slot zero is the
 * selected SP player, with the same concrete client ownership as player hosts. */
static int Cod1ScriptBlastPlayer(const gentity_t *entity) {
    return entity==&g_entities[0] && entity->s.number==0 &&
        level.clients && level.maxclients>0 && level.maxclients<=MAX_CLIENTS &&
        entity->client==&level.clients[0] && entity->s.eType==ET_PLAYER &&
        !(entity->r.svFlags&(SVF_CASTAI|SVF_BOT)) && !entity->aiCharacter &&
        !entity->aiAttributes && !entity->AIScript_AlertEntity && !entity->aiInactive &&
        entity->client->pers.connected==CON_CONNECTED && !entity->client->ps.aiChar &&
        (!entity->aiName || !strcmp(entity->aiName,"player"));
}

static void Cod1ScriptBlastError(char *error,size_t capacity,const char *message) {
    if (error && capacity) {
        size_t n=strlen(message);
        if (n>=capacity) n=capacity-1;
        memcpy(error,message,n);error[n]=0;
    }
}
static int Cod1ScriptBlastTargetCurrent(const cod1ScriptBlastTarget_t *target) {
    const gentity_t *entity;
    if (!G_Cod1EntitySameAllocation(&target->entity)) return 0;
    entity=&g_entities[target->entity.slot];
    return entity->r.bmodel==target->bmodel && entity->r.linked==target->linked &&
        entity->takedamage==target->takedamage && entity->r.svFlags==target->svFlags &&
        entity->s.eType==target->eType &&
        VectorCompare(entity->r.currentOrigin,target->origin) &&
        VectorCompare(entity->r.absmin,target->absmin) &&
        VectorCompare(entity->r.absmax,target->absmax) &&
        (!target->eligible || (target->eligible==2 ?
            Cod1ScriptBlastPlayer(entity) && entity->client==target->client &&
            entity->health==target->health && target->playerState &&
            !memcmp(&entity->client->ps,target->playerState,sizeof(*target->playerState)) :
            G_Cod1ScriptBlastNonActor(entity)));
}
static int Cod1ScriptBlastCurrent(int (*current)(void *),void *user,uint64_t epoch,
    const cod1ScriptBlastTarget_t *targets,int count,int first) {
    int i;
    if (!current || !current(user) || !epoch || epoch!=G_Cod1EntityMapEpoch() ||
        level.restarted || !G_Cod1MapEntities() ||
        g_gametype.integer!=GT_SINGLE_PLAYER) return 0;
    for (i=first;i<count;++i) if (!Cod1ScriptBlastTargetCurrent(&targets[i])) return 0;
    return 1;
}
static float Cod1ScriptBlastDistance(const vec3_t vector) {
    volatile float x=vector[0]*vector[0],y=vector[1]*vector[1],z=vector[2]*vector[2];
    volatile float xy=x+y,sum=xy+z;
    return sqrtf(sum);
}
static int Cod1ScriptBlastPoints(float inner,float outer,float radius,float distance,
    int *damage) {
    /* Preserve the observed outer+(inner-outer)*(1-dist/radius) operation
     * order in this explicitly float32 port subset. */
    volatile float difference=inner-outer,ratio=distance/radius;
    volatile float falloff=1.0f-ratio,scaled=difference*falloff;
    volatile float points=outer+scaled;
    if (!isfinite(points) || (double)points<(double)INT_MIN ||
        (double)points>(double)INT_MAX) return 0;
    *damage=(int)points;return 1;
}
static int Cod1ScriptBlastVisible(const vec3_t origin,cod1ScriptBlastTarget_t *target,
    int (*current)(void *),void *user,uint64_t epoch,
    const cod1ScriptBlastTarget_t *targets,int count,int first) {
    static const float offsets[5][2]={{0,0},{15,15},{15,-15},{-15,15},{-15,-15}};
    cod1SpatialTraceQuery_t request;
    trace_t trace;
    vec3_t midpoint,ends[5],toward;
    int probe,axis,status;
    memset(&request,0,sizeof(request));
    request.version=COD1_SPATIAL_VERSION;request.bytes=sizeof(request);
    request.contentsMask=COD1_SCRIPT_RADIUS_LOS_MASK;
    request.passEntity[0]=(int32_t)target->entity.slot;
    request.passEntity[1]=ENTITYNUM_NONE;
    VectorCopy(origin,request.start);
    for (axis=0;axis<3;++axis) {
        volatile float sum=target->absmin[axis]+target->absmax[axis];
        midpoint[axis]=sum*0.5f;
        if (!isfinite(midpoint[axis])) return 0;
    }
    if(target->eligible==2) {
        float length,halfHeight;
        /* Retail CanDamage 20018CF0: client lateral radius8, normalized XY
         * direction toward the blast, center halfway between eye and origin,
         * four top/bottom side probes. The eye is the port's real unsnapped
         * activation eye including its current stance and lean. */
        toward[0]=origin[0]-target->origin[0];toward[1]=origin[1]-target->origin[1];toward[2]=0;
        length=Cod1ScriptBlastDistance(toward);
        if(!isfinite(length))return 0;
        if(length>0){toward[0]/=length;toward[1]/=length;}
        for(axis=0;axis<3;++axis){midpoint[axis]=(target->origin[axis]+target->eye[axis])*0.5f;if(!isfinite(midpoint[axis]))return 0;}
        halfHeight=(target->eye[2]-target->origin[2])*0.5f;
        if(!isfinite(halfHeight))return 0;
        for(probe=0;probe<5;++probe)VectorCopy(midpoint,ends[probe]);
        for(probe=1;probe<5;++probe){
            float side=probe<3?8.0f:-8.0f;
            ends[probe][0]-=toward[1]*side;ends[probe][1]+=toward[0]*side;
            ends[probe][2]+=(probe&1)?halfHeight:-halfHeight;
        }
    } else for(probe=0;probe<5;++probe){
        VectorCopy(midpoint,ends[probe]);ends[probe][0]+=offsets[probe][0];ends[probe][1]+=offsets[probe][1];
    }
    for (probe=0;probe<5;++probe) {
        if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,count,first)) return 0;
        VectorCopy(ends[probe],request.end);
        if (!isfinite(request.end[0]) || !isfinite(request.end[1]) || !isfinite(request.end[2])) return 0;
        memset(&trace,0,sizeof(trace));
        status=trap_Cod1SpatialPointTrace(&request,&trace);
        if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,count,first) ||
            status!=COD1_SPATIAL_OK || !isfinite(trace.fraction) ||
            trace.fraction<0.0f || trace.fraction>1.0f) return 0;
        /* Retail tests this exact fraction only. No hit-target shortcut, no
         * extra startsolid/allsolid rejection, and no through-wall fallback. */
        if (trace.fraction==1.0f) {target->visible=1;return 1;}
    }
    target->visible=0;return 1;
}
int G_Cod1ScriptRadiusDamage(const float inputOrigin[3],float radius,
    float innerDamage,float outerDamage,int (*current)(void *),void *user,
    char *error,size_t capacity) {
    cod1ScriptBlastTarget_t targets[MAX_GENTITIES];
    playerState_t playerSnapshot;
    int slots[MAX_GENTITIES],count,i,axis,used=0;
    unsigned char seen[MAX_GENTITIES];
    vec3_t origin,mins,maxs,vector;
    float boxradius,distance;
    uint64_t epoch=G_Cod1EntityMapEpoch();
    if (!inputOrigin) goto invalid;
    VectorCopy(inputOrigin,origin); /* Before any caller/engine callback. */
    if (!isfinite(radius) || !isfinite(innerDamage) || !isfinite(outerDamage)) goto invalid;
    if (radius<1.0f) radius=1.0f;
    boxradius=1.41421356f*radius;
    for (axis=0;axis<3;++axis) {
        if (!isfinite(origin[axis])) goto invalid;
        mins[axis]=origin[axis]-boxradius;maxs[axis]=origin[axis]+boxradius;
        if (!isfinite(mins[axis]) || !isfinite(maxs[axis])) goto invalid;
    }
    if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,0,0)) goto stale;
    count=trap_EntitiesInBox(mins,maxs,slots,MAX_GENTITIES);
    if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,0,0)) goto stale;
    if (count<0 || count>MAX_GENTITIES) goto invalid;
    memset(seen,0,sizeof(seen));memset(targets,0,sizeof(targets));
    for (i=0;i<count;++i) {
        gentity_t *entity;
        cod1ScriptBlastTarget_t *target;
        if (slots[i]<0 || slots[i]>=ENTITYNUM_WORLD || seen[slots[i]]) goto invalid;
        seen[slots[i]]=1;entity=&g_entities[slots[i]];
        if (!entity->inuse || !entity->takedamage) continue;
        target=&targets[used];
        if (!G_Cod1EntityCapture(entity,&target->entity) || !entity->r.linked) goto stale;
        target->bmodel=entity->r.bmodel;target->linked=entity->r.linked;
        target->takedamage=entity->takedamage;target->svFlags=entity->r.svFlags;
        target->eType=entity->s.eType;
        VectorCopy(entity->r.currentOrigin,target->origin);
        VectorCopy(entity->r.absmin,target->absmin);VectorCopy(entity->r.absmax,target->absmax);
        for (axis=0;axis<3;++axis) {
            if (!isfinite(target->origin[axis]) || !isfinite(target->absmin[axis]) ||
                !isfinite(target->absmax[axis]) || target->absmin[axis]>target->absmax[axis]) goto invalid;
            if (!target->bmodel) vector[axis]=target->origin[axis]-origin[axis];
            else if (origin[axis]<target->absmin[axis]) vector[axis]=target->absmin[axis]-origin[axis];
            else if (origin[axis]>target->absmax[axis]) vector[axis]=origin[axis]-target->absmax[axis];
            else vector[axis]=0.0f;
        }
        distance=Cod1ScriptBlastDistance(vector);
        if (!isfinite(distance)) goto invalid;
        ++used;
        if (distance>=radius) continue;
        if(Cod1ScriptBlastPlayer(entity)) {
            vec3_t zero={0,0,0};const playerState_t *ps=&entity->client->ps;
            for(axis=0;axis<3;++axis)if(!isfinite(ps->origin[axis]) || !isfinite(ps->viewangles[axis]) ||
                !isfinite(entity->s.pos.trBase[axis]))goto invalid;
            if(!isfinite(ps->mins[2]) || !isfinite(ps->cod1EyeHeight) || !isfinite(ps->leanf) ||
                ps->leanf < -1.0f || ps->leanf>1.0f)goto invalid;
            playerSnapshot=*ps;target->playerState=&playerSnapshot;
            target->client=entity->client;target->health=entity->health;target->eligible=2;
            CalcMuzzlePointForActivate(entity,zero,zero,zero,target->eye);
            for(axis=0;axis<3;++axis)if(!isfinite(target->eye[axis]))goto invalid;
        } else {
            if (!G_Cod1ScriptBlastNonActor(entity)) {
                Cod1ScriptBlastError(error,capacity,"script blast actor/model LOS unavailable before damage");return 0;
            }
            target->eligible=1;
        }
        if (!Cod1ScriptBlastPoints(innerDamage,outerDamage,radius,distance,&target->damage)) goto invalid;
        VectorSubtract(target->origin,origin,target->dir);target->dir[2]+=24.0f;
        for (axis=0;axis<3;++axis) if (!isfinite(target->dir[axis])) goto invalid;
    }
    if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,used,0)) goto stale;
    for (i=0;i<used;++i) if (targets[i].eligible &&
        !Cod1ScriptBlastVisible(origin,&targets[i],current,user,epoch,targets,used,0)) {
        Cod1ScriptBlastError(error,capacity,"script blast point LOS unavailable or scene changed before damage");return 0;
    }
    for (i=0;i<used;++i) {
        vec3_t direction,point;
        if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,used,i)) goto stale;
        if (!targets[i].eligible) continue;
        /* Native damage/die callbacks may move or remove a separate blocker.
         * Refresh the real LOS at each commit boundary. The initial full-scene
         * preflight prevents known unsupported geometry from partial damage;
         * a later unsupported boundary preserves already committed damage. */
        if (!Cod1ScriptBlastVisible(origin,&targets[i],current,user,epoch,targets,used,i)) {
            Cod1ScriptBlastError(error,capacity,"script blast point LOS unavailable after committed damage");return 0;
        }
        if (!targets[i].visible) continue;
        VectorCopy(targets[i].dir,direction);VectorCopy(origin,point);
        G_Damage(&g_entities[targets[i].entity.slot],NULL,&g_entities[ENTITYNUM_WORLD],
            direction,point,targets[i].damage,DAMAGE_RADIUS|DAMAGE_NO_KNOCKBACK,MOD_EXPLOSIVE);
        /* The consumed target may legitimately die/free. Validate the session
         * and every unconsumed lease instead of recapturing its replacement. */
        if (!Cod1ScriptBlastCurrent(current,user,epoch,targets,used,i+1)) goto stale;
    }
    return 1;
invalid:
    Cod1ScriptBlastError(error,capacity,"script blast parameters, damage conversion or candidate list invalid");return 0;
stale:
    Cod1ScriptBlastError(error,capacity,"script blast allocation, geometry or map/session changed; committed damage retained");return 0;
}
#endif
