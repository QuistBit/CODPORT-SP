/* GAME380F0 +1CBC0. Owns a real gravity trajectory; no script events.
 * Qualified host domain: unlinked nonsolid script_model with zero trace hull.
 * Original RunItem precedes mover/script-motion dispatch while physics is set. */
#include "../bg_cod1_drop_frame.h"
typedef struct {GscEntityRef owner;int broken;} ScriptLaunchOwner;
static ScriptLaunchOwner scriptLaunch[MAX_GENTITIES];
static struct {int active,cancelled,removed,now,previous,count;uint64_t epoch;GscEntityRef owner;gentity_t before;} scriptLaunchCall;
static int ScriptLaunchBody(const GscEntityRef *owner){
    gentity_t *e;
    if(!YawMap()||!ActorOwnerAllowed(owner,0)||ScriptMotionParent(owner))return 0;
    e=g_entities+owner->slot;
    return e->classname&&!strcmp(e->classname,"script_model")&&!e->client&&!e->aiCharacter&&
        !e->r.bmodel&&!e->r.contents&&!e->neverFree&&e->s.eType==ET_GENERAL&&
        e->s.density==COD1_SCRIPT_MODEL_NET_MARKER&&!e->touch&&!e->use&&
        (!e->think||e->think==G_FreeEntity)&&!memcmp(e->r.mins,vec3_origin,12)&&!memcmp(e->r.maxs,vec3_origin,12)&&
        YawVector(e->r.currentOrigin,16777216.0f)&&YawVector(e->r.currentAngles,10000000.0f)&&
        YawVector(e->s.pos.trBase,16777216.0f)&&YawFinite(e->physicsBounce)&&
        (e->r.ownerNum==ENTITYNUM_NONE||(e->r.ownerNum>=0&&e->r.ownerNum<ENTITYNUM_WORLD));
}
static int ScriptLaunchInvoke(const GscYawMotionRequest *input,int(*current)(void*),void*context){
    GscYawMotionRequest q=*input;gentity_t before,*e;ScriptLaunchOwner old;uint64_t epoch=cod1EntityEpoch;
    int now=level.time,count=level.num_entities;
    if(scriptLaunchCall.active){scriptLaunchCall.cancelled=1;return 0;}
    if(!q.operandsValid||q.now_ms<0||!YawVector(q.velocity,10000000.0f)||!ScriptLaunchBody(&q.entity))return 0;
    e=g_entities+q.entity.slot;before=*e;old=scriptLaunch[q.entity.slot];
    if(current(context)!=1||!cod1YawGuard.active||cod1YawGuard.cancelled||cod1EntityEpoch!=epoch||
       level.time!=now||level.num_entities!=count||memcmp(input,&q,sizeof(q))||!ScriptLaunchBody(&q.entity)||
       memcmp(e,&before,sizeof(before))||memcmp(&old,scriptLaunch+q.entity.slot,sizeof(old)))return 0;
    /* Native launch does not rebase position, reset duration, link, or touch angular state. */
    e->s.pos.trType=TR_GRAVITY;e->s.pos.trTime=now;VectorCopy(q.velocity,e->s.pos.trDelta);e->physicsObject=qtrue;
    scriptLaunch[q.entity.slot].owner=q.entity;scriptLaunch[q.entity.slot].broken=0;
    return 1;
}
static int ScriptLaunchCurrent(void *unused){
    (void)unused;
    return scriptLaunchCall.active&&!scriptLaunchCall.cancelled&&scriptLaunchCall.epoch==cod1EntityEpoch&&
        level.time==scriptLaunchCall.now&&level.previousTime==scriptLaunchCall.previous&&level.num_entities==scriptLaunchCall.count&&
        (scriptLaunchCall.removed?!G_Cod1EntitySameAllocation(&scriptLaunchCall.owner):
         (ScriptLaunchBody(&scriptLaunchCall.owner)&&
          !memcmp(&scriptLaunchCall.before,g_entities+scriptLaunchCall.owner.slot,sizeof(gentity_t))));
}
static int ScriptLaunchNativeId(int v){return v==ENTITYNUM_NONE?2047:v==ENTITYNUM_WORLD?2046:v>=0&&v<ENTITYNUM_WORLD?v:-1;}
static int ScriptLaunchPortId(int v){return v==2047?ENTITYNUM_NONE:v==2046?ENTITYNUM_WORLD:v>=0&&v<ENTITYNUM_WORLD?v:-1;}
static int ScriptLaunchKind(int v){return v==TR_STATIONARY?0:v==TR_LINEAR?2:v==TR_LINEAR_STOP?3:v==TR_GRAVITY?5:v==TR_GRAVITY_PAUSED?8:v==TR_ACCELERATE?9:v==TR_DECCELERATE?10:-1;}
static void ScriptLaunchApply(const c1dfState *s){
    gentity_t *e=g_entities+scriptLaunchCall.owner.slot;
    e->s.pos.trType=s->kind==0?TR_STATIONARY:s->kind==5?TR_GRAVITY:s->kind==8?TR_GRAVITY_PAUSED:TR_LINEAR;
    e->s.pos.trTime=s->time;if(s->kind==0)e->s.pos.trDuration=0;VectorCopy(s->base,e->s.pos.trBase);VectorCopy(s->delta,e->s.pos.trDelta);
    VectorCopy(s->origin,e->r.currentOrigin);VectorCopy(s->origin,e->s.origin);e->s.groundEntityNum=ScriptLaunchPortId(s->ground);
    /* The flight branch preserves every angular trajectory byte. Landing alone sets stationary. */
    if(s->angularKind==0&&s->kind==0){e->s.apos.trType=TR_STATIONARY;e->s.apos.trTime=s->angularTime;e->s.apos.trDuration=0;
        VectorCopy(s->angularBase,e->s.apos.trBase);VectorCopy(s->angularDelta,e->s.apos.trDelta);
        VectorCopy(s->angles,e->r.currentAngles);VectorCopy(s->angles,e->s.angles);}
    scriptLaunchCall.before=*e;
}
static int ScriptLaunchTrace(void *u,int capsule,const float start[3],const float mins[3],const float maxs[3],const float end[3],int pass,unsigned mask,c1dfTrace*out){
    cod1SpatialTraceQuery_t q;trace_t tr;int status,id=ScriptLaunchPortId(pass);(void)u;
    if(capsule||id<0||!ScriptLaunchCurrent(NULL)||memcmp(mins,vec3_origin,12)||memcmp(maxs,vec3_origin,12))return 0;
    memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);VectorCopy(start,q.start);VectorCopy(end,q.end);
    q.passEntity[0]=id;q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;
    status=trap_Cod1SpatialPointTrace(&q,&tr);
    if(status!=COD1_SPATIAL_OK||!ScriptLaunchCurrent(NULL)||ScriptLaunchNativeId(tr.entityNum)<0)return 0;
    out->fraction=tr.fraction;VectorCopy(tr.endpos,out->end);VectorCopy(tr.plane.normal,out->normal);
    out->ground=ScriptLaunchNativeId(tr.entityNum);out->startsolid=tr.startsolid?1:0;return 1;
}
static int ScriptLaunchLink(void*u,const c1dfState*s){
    gentity_t after,*e;(void)u;if(!ScriptLaunchCurrent(NULL))return 0;ScriptLaunchApply(s);e=g_entities+scriptLaunchCall.owner.slot;
    trap_LinkEntity(e);if(!G_Cod1EntitySameAllocation(&scriptLaunchCall.owner))return 0;after=*e;
    after.r.linked=scriptLaunchCall.before.r.linked;after.r.linkcount=scriptLaunchCall.before.r.linkcount;
    VectorCopy(scriptLaunchCall.before.r.absmin,after.r.absmin);VectorCopy(scriptLaunchCall.before.r.absmax,after.r.absmax);
    if(memcmp(&after,&scriptLaunchCall.before,sizeof(after)))return 0;scriptLaunchCall.before=*e;
    return ScriptLaunchCurrent(NULL)&&e->r.linked;
}
static int ScriptLaunchRemove(void*u,c1dfState*s){
    GscEntityRef owner=scriptLaunchCall.owner;(void)u;if(!ScriptLaunchCurrent(NULL))return 0;
    G_FreeEntity(g_entities+owner.slot);if(G_Cod1EntitySameAllocation(&owner))return 0;
    scriptLaunchCall.removed=1;s->alive=0;return !scriptLaunchCall.cancelled&&cod1EntityEpoch==scriptLaunchCall.epoch&&level.time==scriptLaunchCall.now;
}
static int ScriptLaunchThink(void*u,c1dfState*s){
    gentity_t *e;(void)u;if(!ScriptLaunchCurrent(NULL))return 0;e=g_entities+scriptLaunchCall.owner.slot;
    if(e->nextthink>0&&e->nextthink<=level.time)return e->think==G_FreeEntity&&ScriptLaunchRemove(u,s);return 1;
}
static int ScriptLaunchContents(void*u,const float p[3],int pass,unsigned mask,int*out){
    int bits;(void)u;if(pass!=-1||mask!=0x80000000u||!ScriptLaunchCurrent(NULL))return 0;
    bits=trap_PointContents(p,-1);if(!ScriptLaunchCurrent(NULL))return 0;*out=(uint32_t)bits&mask;return 1;
}
static int ScriptLaunchRandom(void*u,int*out){(void)u;if(!ScriptLaunchCurrent(NULL))return 0;*out=rand();return *out>=0&&*out<=32767&&ScriptLaunchCurrent(NULL);}
int G_Cod1ScriptLaunchFrame(gentity_t *e){
    GscEntityRef owner;c1dfState s={0};c1dfServices h={0};int ok;
    if(!e||!G_Cod1EntityCapture(e,&owner)||!ActorRefEqual(&owner,&scriptLaunch[owner.slot].owner))return 0;
    if(scriptLaunchCall.active){scriptLaunchCall.cancelled=1;return 1;}
    if(scriptLaunch[owner.slot].broken)return 1;
    memset(&scriptLaunchCall,0,sizeof(scriptLaunchCall));scriptLaunchCall.active=1;scriptLaunchCall.owner=owner;
    scriptLaunchCall.epoch=cod1EntityEpoch;scriptLaunchCall.now=level.time;scriptLaunchCall.previous=level.previousTime;
    scriptLaunchCall.count=level.num_entities;scriptLaunchCall.before=*e;
    s.alive=1;s.kind=ScriptLaunchKind(e->s.pos.trType);s.time=e->s.pos.trTime;s.ground=ScriptLaunchNativeId(e->s.groundEntityNum);
    s.angularKind=ScriptLaunchKind(e->s.apos.trType);s.angularTime=e->s.apos.trTime;s.owner=ScriptLaunchNativeId(e->r.ownerNum);s.number=owner.slot;
    s.mask=e->clipmask;s.bounce=e->physicsBounce;
    VectorCopy(e->s.pos.trBase,s.base);VectorCopy(e->s.pos.trDelta,s.delta);VectorCopy(e->r.currentOrigin,s.origin);
    VectorCopy(e->s.apos.trBase,s.angularBase);VectorCopy(e->s.apos.trDelta,s.angularDelta);VectorCopy(e->r.currentAngles,s.angles);
    h.current=ScriptLaunchCurrent;h.trace=ScriptLaunchTrace;h.link=ScriptLaunchLink;h.think=ScriptLaunchThink;
    h.contents=ScriptLaunchContents;h.remove=ScriptLaunchRemove;h.draw_random=ScriptLaunchRandom;
    ok=e->physicsObject&&s.kind>=0&&s.angularKind>=0&&s.ground>=0&&C1DF_Run(&s,level.time,level.previousTime,&h);
    if(ok&&s.alive){ok=ScriptLaunchCurrent(NULL);if(ok)ScriptLaunchApply(&s);}
    if(!ok){
        if(G_Cod1EntitySameAllocation(&owner))scriptLaunch[owner.slot].broken=1;G_Printf("[389 launch] frame refused owner=%u/%u tick=%d\n",owner.slot,owner.generation,level.time);
    }
    memset(&scriptLaunchCall,0,sizeof(scriptLaunchCall));return 1;
}
