/* A selected direct-CM backend, not native nav-search or combat AI. Include
 * after actor_motion.inc. Every published distance comes from the owned
 * one-waypoint geometry record and original63010 summation, not a query hack. */
#include "../g_cod1_actor_direct_path.h"
typedef struct {entityShared_t shared;GscEntityRef ref;int inuse,model,solid;} C1DPSceneEntry;
typedef struct {cod1ActorDirectPathView_t view;cod1ActorLease_t lease;vec3_t position,angles,mins,maxs;int clipmask,contents,entityCount;C1DPSceneEntry*scene;} C1DPState;
static C1DPState cod1DirectPaths[COD1_ACTOR_CAPACITY];
static uint64_t cod1DirectPathBusy,cod1DirectPathSerial;
typedef struct {const GscEntityRef*borrowed;GscEntityRef actor;cod1ActorLease_t lease;cod1ActorRecord_t record;cod1SentientRecord_t sentient;gentity_t body;C1DPState before;C1DPSceneEntry*scene;uint64_t epoch,serial;int now,count;int(*current)(void*);void*ctx;} C1DPCall;
static C1DPCall*cod1DirectPathCall;
static int ActorDirectPathPrivate(const void*p,size_t n){unsigned i;if(ActorOverlap(p,n,cod1DirectPaths,sizeof(cod1DirectPaths))||ActorOverlap(p,n,&cod1DirectPathBusy,sizeof(cod1DirectPathBusy))||ActorOverlap(p,n,&cod1DirectPathSerial,sizeof(cod1DirectPathSerial))||ActorOverlap(p,n,&cod1DirectPathCall,sizeof(cod1DirectPathCall)))return 1;
 if(cod1DirectPathCall&&(ActorOverlap(p,n,cod1DirectPathCall,sizeof(*cod1DirectPathCall))||ActorOverlap(p,n,cod1DirectPathCall->scene,MAX_GENTITIES*sizeof(C1DPSceneEntry))))return 1;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1DirectPaths[i].scene&&ActorOverlap(p,n,cod1DirectPaths[i].scene,MAX_GENTITIES*sizeof(C1DPSceneEntry)))return 1;return 0;}
static void ActorDirectPathReset(void){unsigned i;for(i=0;i<COD1_ACTOR_CAPACITY;i++){free(cod1DirectPaths[i].scene);memset(cod1DirectPaths+i,0,sizeof(C1DPState));}if(cod1DirectPathSerial!=UINT64_MAX)cod1DirectPathSerial++;/* Busy callback retains its private call until unwind. */}
static void ActorDirectPathRetire(const GscEntityRef*actor){unsigned i;if(!actor)return;for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1DirectPaths[i].view.initialized&&ActorRefEqual(actor,&cod1DirectPaths[i].view.actor)){free(cod1DirectPaths[i].scene);memset(cod1DirectPaths+i,0,sizeof(C1DPState));}}
static int DirectPathSceneCapture(C1DPSceneEntry*out,int count){int i;if(!out||count<0||count>MAX_GENTITIES)return 0;memset(out,0,MAX_GENTITIES*sizeof(*out));for(i=0;i<count;i++){out[i].inuse=g_entities[i].inuse;out[i].shared=g_entities[i].r;out[i].model=g_entities[i].s.modelindex;out[i].solid=g_entities[i].s.solid;if(g_entities[i].inuse&&!G_Cod1EntityCapture(g_entities+i,&out[i].ref))return 0;}return 1;}
static int DirectPathSceneCurrent(const C1DPSceneEntry*s,int count){int i;GscEntityRef ref;if(!s||count!=level.num_entities)return 0;for(i=0;i<count;i++){
 if(s[i].inuse!=g_entities[i].inuse||memcmp(&s[i].shared,&g_entities[i].r,sizeof(s[i].shared))||s[i].model!=g_entities[i].s.modelindex||s[i].solid!=g_entities[i].s.solid)return 0;
 if(s[i].inuse&&(!G_Cod1EntityCapture(g_entities+i,&ref)||!ActorRefEqual(&ref,&s[i].ref)))return 0;}return 1;}
static int DirectPathCurrent(C1DPCall*c){cod1ActorLease_t lease;return c&&cod1DirectPathCall==c&&cod1DirectPathBusy==c->epoch&&cod1DirectPathSerial==c->serial&&c->epoch==cod1EntityEpoch&&c->now==level.time&&cod1EntityReady&&!level.spawning&&!level.restarted&&
 ActorGoalBuffers(c->borrowed,sizeof(*c->borrowed),NULL,0,NULL,0)&&!ActorDirectPathPrivate(c->borrowed,sizeof(*c->borrowed))&&!memcmp(c->borrowed,&c->actor,sizeof(c->actor))&&ActorGoalLease(&c->actor,&lease)&&!memcmp(&lease,&c->lease,sizeof(lease))&&
 !memcmp(cod1Actors+c->lease.actorIndex,&c->record,sizeof(c->record))&&!memcmp(cod1Sentients+c->lease.sentientIndex,&c->sentient,sizeof(c->sentient))&&
 !memcmp(g_entities+c->actor.slot,&c->body,sizeof(c->body))&&!memcmp(cod1DirectPaths+c->lease.actorIndex,&c->before,sizeof(c->before))&&DirectPathSceneCurrent(c->scene,c->count);}
static int DirectPathFence(C1DPCall*c){return DirectPathCurrent(c)&&c->current(c->ctx)==1&&DirectPathCurrent(c);}
static C1DPCall*DirectPathBegin(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){C1DPCall*c;
 if(!input||!current||now!=level.time||!cod1EntityEpoch||cod1DirectPathBusy||cod1DirectPathSerial==UINT64_MAX||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorDirectPathPrivate(input,sizeof(*input)))return NULL;
 c=calloc(1,sizeof(*c));if(!c)return NULL;c->scene=calloc(MAX_GENTITIES,sizeof(*c->scene));if(!c->scene){free(c);return NULL;}c->borrowed=input;c->actor=*input;c->now=now;c->epoch=cod1EntityEpoch;c->serial=++cod1DirectPathSerial;c->current=current;c->ctx=ctx;c->count=level.num_entities;cod1DirectPathCall=c;cod1DirectPathBusy=c->epoch;
 if(!ActorGoalLease(input,&c->lease))return c;c->record=cod1Actors[c->lease.actorIndex];c->sentient=cod1Sentients[c->lease.sentientIndex];c->body=g_entities[input->slot];c->before=cod1DirectPaths[c->lease.actorIndex];if(!DirectPathSceneCapture(c->scene,c->count))c->count=-1;return c;}
static void DirectPathEnd(C1DPCall*c){if(!c)return;if(cod1DirectPathCall==c){cod1DirectPathCall=NULL;if(cod1DirectPathBusy==c->epoch)cod1DirectPathBusy=0;}free(c->scene);free(c);}
int G_Cod1ActorDirectPathFrame(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){C1DPCall*c=DirectPathBegin(input,now,current,ctx);C1DPState next;trace_t trace;vec3_t end,at,floor;float distance;int i,steps,ground,ok=0;
 if(!c)return 0;if(!DirectPathFence(c)||!c->record.motion.active||!c->record.goal.assigned||!c->record.goal.assignment||c->record.turretMount.mounted||!c->body.r.linked||!c->record.spatial.linked)goto done;
 memset(&next,0,sizeof(next));next.view.initialized=1;next.view.actor=c->actor;next.view.assignment=c->record.goal.assignment;next.view.tick=now;next.lease=c->lease;next.entityCount=c->count;
 VectorCopy(c->body.r.currentOrigin,next.position);VectorCopy(c->body.r.currentAngles,next.angles);VectorCopy(c->body.r.mins,next.mins);VectorCopy(c->body.r.maxs,next.maxs);next.clipmask=c->body.clipmask;next.contents=c->body.r.contents;
 if(c->record.motion.arrived&&c->record.motion.notified&&c->record.motion.assignment==c->record.goal.assignment){next.view.consumed=1;goto commit;}
 ground=MotionFloor(&c->lease,c->record.goal.assignment,now,c->record.goal.position,end);if(!DirectPathFence(c))goto done;if(ground!=1){if(ground<0){next.view.blocked=1;goto commit;}goto done;}
 if(!C1DP_Direct(next.position,end,&next.view.record)||!C1DP_Distance(&next.view.record,&distance))goto done;
 if(distance>512){next.view.blocked=1;goto commit;}steps=(int)ceilf(distance/8);if(steps<1)steps=1;
 for(i=0;i<=steps;i++){float f=(float)i/(float)steps;int k;for(k=0;k<3;k++)at[k]=next.position[k]+(end[k]-next.position[k])*f;
  ground=MotionFloor(&c->lease,c->record.goal.assignment,now,at,floor);if(!DirectPathFence(c))goto done;if(ground!=1||fabsf(floor[2]-at[2])>.5f){if(ground==0)goto done;next.view.blocked=1;goto commit;}}
 trap_TraceCapsule(&trace,next.position,next.mins,next.maxs,end,c->actor.slot,next.clipmask);if(!DirectPathFence(c))goto done;
 if(trace.startsolid||trace.allsolid||!isfinite(trace.fraction)||trace.fraction<1)next.view.blocked=1;
commit:
 if(!DirectPathFence(c))goto done;next.scene=c->scene;c->scene=NULL;cod1DirectPaths[c->lease.actorIndex]=next;free(c->before.scene);ok=1;
done:DirectPathEnd(c);return ok;
}
static int DirectPathStoredCurrent(C1DPCall*c){C1DPState*s=&c->before;return DirectPathCurrent(c)&&s->view.initialized&&!s->view.blocked&&s->view.tick==c->now&&s->view.assignment==c->record.goal.assignment&&ActorRefEqual(&s->view.actor,&c->actor)&&!memcmp(&s->lease,&c->lease,sizeof(c->lease))&&!memcmp(s->position,c->body.r.currentOrigin,12)&&!memcmp(s->angles,c->body.r.currentAngles,12)&&!memcmp(s->mins,c->body.r.mins,12)&&!memcmp(s->maxs,c->body.r.maxs,12)&&s->clipmask==c->body.clipmask&&s->contents==c->body.r.contents&&DirectPathSceneCurrent(s->scene,s->entityCount);}
int G_Cod1ActorDirectPathRead(const GscEntityRef*input,int now,float*out,int(*current)(void*),void*ctx){C1DPCall*c;float value;int ok=0;if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),NULL,0)||ActorDirectPathPrivate(out,sizeof(*out)))return 0;c=DirectPathBegin(input,now,current,ctx);if(!c)return 0;
 if(DirectPathStoredCurrent(c)&&DirectPathFence(c)&&DirectPathStoredCurrent(c)&&C1DP_Distance(&c->before.view.record,&value)&&DirectPathStoredCurrent(c)&&ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),NULL,0)&&!ActorDirectPathPrivate(out,sizeof(*out))){*out=value;ok=1;}DirectPathEnd(c);return ok;}
int G_Cod1ActorDirectPathInspect(const GscEntityRef*input,cod1ActorDirectPathView_t*out){cod1ActorLease_t lease;if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),NULL,0)||ActorDirectPathPrivate(input,sizeof(*input))||ActorDirectPathPrivate(out,sizeof(*out))||!ActorGoalLease(input,&lease))return 0;if(!cod1DirectPaths[lease.actorIndex].view.initialized||!ActorRefEqual(input,&cod1DirectPaths[lease.actorIndex].view.actor))return 0;*out=cod1DirectPaths[lease.actorIndex].view;return 1;}
