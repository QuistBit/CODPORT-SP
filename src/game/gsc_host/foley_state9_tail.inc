/* Present normal record, GAME 200576C3..20057745. The private Training
 * caller admits only the empty contact branches on the mounted map.
 * The map-side certificate contains every authored trigger brush. */
#include "foley_state9_brushes.h"
#include "bg_cod1_actor_immediate14.h"
#include "../state9_result.h"
/* The selected state-9 actor tests its native team mask against the
 * constructor's original trigger_damage contents. The port body keeps
 * CONTENTS_TRIGGER for bullet handling. No link/unlink is needed here. */
static int TrainingState9Brush(C1ControllerOperation *o,const gentity_t *e,unsigned team,
                               GscEntityRef *brush,int *narrow){
    const cod1ActorRecord_t *a=o->stage;
    vec3_t mins,maxs;
    GscEntityRef ref;
    int box,capsule;
    if(team!=2 || !o->entity->targetname || strcmp(o->entity->targetname,"moody") ||
       !e->targetname || strcmp(e->targetname,"moody_damage") ||
       !G_Cod1EntityCapture(e,&ref) || !G_Cod1TowerDamageBrushCurrent(&ref) ||
       a->nativeData.entity.cell01b4!=1 || a->turretMount.currentTop!=1 ||
       a->turretMount.currentStack[1]!=9 || a->defaults.lifeWords[(0x35c-0x26c)/4]==ref.slot)return 0;
    VectorAdd(o->entity->r.currentOrigin,a->nativeData.entity.mins,mins);
    VectorAdd(o->entity->r.currentOrigin,a->nativeData.entity.maxs,maxs);
    box=trap_EntityContact(mins,maxs,e);
    capsule=trap_EntityContactCapsule(mins,maxs,e);
    if(box!=capsule || !ActorControllerCurrent(o) || !G_Cod1TowerDamageBrushCurrent(&ref))return 0;
    *narrow=box;*brush=ref;
    return 1;
}
int G_Cod1TakeTowerTouch(const GscEntityRef *owner,uint64_t serial,uint64_t frame,int now,GscEntityRef *brush){
    cod1ActorLease_t lease;
    cod1ActorRecord_t *a;
    if(now!=level.time || !serial || !frame || !ActorGoalBuffers(owner,sizeof(*owner),brush,sizeof(*brush),NULL,0) ||
       !ActorGoalLease(owner,&lease) || lease.serial!=serial || cod1ControllerOperation.busy)return 0;
    a=cod1Actors+lease.actorIndex;
    if(a->towerTouch.frame!=frame || a->towerTouch.tick!=now ||
       !G_Cod1TowerDamageBrushCurrent(&a->towerTouch.brush))return 0;
    *brush=a->towerTouch.brush;
    memset(&a->towerTouch,0,sizeof(a->towerTouch));
    return 1;
}

static int TrainingContactRefused(C1ControllerOperation *o,int line,int slot){
 if(g_debugMove.integer)
 G_Printf("[Training actor contact] refused line=%d tick=%d candidate=%d class=%s origin=%.1f %.1f %.1f\n",line,o->now,slot,
    slot>=0&&slot<level.num_entities&&g_entities[slot].classname?g_entities[slot].classname:"-",
    o->entity->r.currentOrigin[0],o->entity->r.currentOrigin[1],o->entity->r.currentOrigin[2]);
 return 0;
}
static int ActorState9Contacts(C1ControllerOperation *o,unsigned team,GscEntityRef *hit){
 cod1SpatialAreaQuery_t q={0};
 int slots[MAX_GENTITIES],count=-1,result,i,self=0,player=0,narrow=0;
 GscEntityRef brush={0};
 cod1PlayerLifetime_t life;int candidate=-1;
 static const float extent[3]={40.f,40.f,52.f};
 if(team<1 || team>3 || o->entity->client || !ActorControllerCurrent(o))return TrainingContactRefused(o,__LINE__,candidate);
 q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);
 for(i=0;i<3;i++){
  q.mins[i]=o->entity->r.currentOrigin[i]-extent[i];
  q.maxs[i]=o->entity->r.currentOrigin[i]+extent[i];
 }

 /* All linked bodies including zero-port-contents entities must be
  * accounted for independently of the native mask projection. */
 count=trap_EntitiesInBox(q.mins,q.maxs,slots,MAX_GENTITIES);
 if(!ActorControllerCurrent(o)||count<1||count>MAX_GENTITIES)return TrainingContactRefused(o,__LINE__,candidate);
 for(i=0;i<count;i++){
  gentity_t *p;GscEntityRef ref;int slot=slots[i];candidate=slot;
  if(slot==(int)o->lease.entity.slot){if(self++)return TrainingContactRefused(o,__LINE__,candidate);continue;}
  if(slot<0 || slot>=level.num_entities)return TrainingContactRefused(o,__LINE__,candidate);
  if(slot>=level.maxclients){
   if(G_Cod1EntityCapture(&g_entities[slot],&ref) && G_Cod1TeleportEffectCurrent(&ref))continue;
   if(G_Cod1EntityCapture(&g_entities[slot],&ref)&&TrainingRigReceiverCurrent(&ref,ref.generation)&&
      !g_entities[slot].r.contents&&!g_entities[slot].r.bmodel)continue;
   if(brush.epoch || !TrainingState9Brush(o,&g_entities[slot],team,&brush,&narrow))return TrainingContactRefused(o,__LINE__,candidate);
   continue;
  }
  if(player++)return TrainingContactRefused(o,__LINE__,candidate);
  p=&g_entities[slot];
  /* GAME 200149B3 initializes the live player body to 02000000. Verify
   * the linked current body, not only the slot or its spawn history. */
  if(!p->inuse || p->client!=&level.clients[slot] || p->s.number!=slot ||
     p->client->ps.clientNum!=slot || p->client->pers.connected!=CON_CONNECTED ||
     !p->r.linked || p->r.bmodel || p->r.contents!=CONTENTS_BODY ||
     p->health<=0 || !G_Cod1EntityCapture(p,&ref) ||
     !G_Cod1PlayerLifetimeRead(&ref,&life) || !G_Cod1PlayerLifetimeSameCompleted(&life))return TrainingContactRefused(o,__LINE__,candidate);
 }
 if(self!=1 || !TrainingBrushesCovered(q.mins,q.maxs,brush.epoch?g_entities[brush.slot].s.modelindex:0))return TrainingContactRefused(o,__LINE__,candidate);
 q.contentsMask=0x40000u<<(team-1);
 result=trap_Cod1SpatialArea(&q,slots,MAX_GENTITIES,&count);
 if(!ActorControllerCurrent(o)||result!=COD1_SPATIAL_OK||count){
  if(g_debugMove.integer)G_Printf("[Training actor area] owner=%u mask=%x status=%d count=%d first=%d\n",
    o->lease.entity.slot,q.contentsMask,result,count,result==COD1_SPATIAL_OK&&count?slots[0]:-1);
  return TrainingContactRefused(o,__LINE__,candidate);
 }
 q.contentsMask=0x800000u;
 result=trap_Cod1SpatialArea(&q,slots,MAX_GENTITIES,&count);
 if(!ActorControllerCurrent(o)||result!=COD1_SPATIAL_OK||count){
  if(g_debugMove.integer)G_Printf("[Training actor area] owner=%u mask=%x status=%d count=%d first=%d\n",
    o->lease.entity.slot,q.contentsMask,result,count,result==COD1_SPATIAL_OK&&count?slots[0]:-1);
  return TrainingContactRefused(o,__LINE__,candidate);
 }
 if(player && (!G_Cod1PlayerLifetimeSameCompleted(&life) ||
    g_entities[life.entity.slot].r.contents!=CONTENTS_BODY))return TrainingContactRefused(o,__LINE__,candidate);
 if(brush.epoch){
  if(!G_Cod1TowerDamageBrushCurrent(&brush))return TrainingContactRefused(o,__LINE__,candidate);
  if(narrow)*hit=brush;
 }
 if(player && !narrow && g_debugMove.integer)G_Printf("[Foley contact] tick=%d owner=%u player=%u native_masks_empty=1\n",o->now,o->lease.entity.slot,life.entity.slot);
 return 1;
}
/* The record writes the real actor origin before the state9 area query.
 * Relink the same assembly under its existing native spatial allocation. */
static int TrainingRecordLink(C1ControllerOperation *o,int changed){
 gentity_t *e=g_entities+o->lease.entity.slot;
 /* The scripted record may change a walking instructor's transform even on
  * its first sample. Spatial queries require the registration to match
  * the current transform. Keep the same actor lease and native contents. */
 if(!TrainingJumperAllocation(&o->lease.entity) &&
    !(changed && InstructorCourseGoal(o,o->stage)))return 1;
 if(!ActorControllerCurrent(o)||!MotionLink(&o->lease)||!e->r.linked||
    e->r.linkcount!=o->entity->r.linkcount+1)return 0;
 o->entity->r.linkcount=e->r.linkcount;o->entity->s.solid=e->s.solid;
 VectorCopy(e->r.absmin,o->entity->r.absmin);VectorCopy(e->r.absmax,o->entity->r.absmax);
 return !memcmp(e,o->entity,sizeof(*e))&&ActorControllerCurrent(o);
}
static int ActorState9Tail(C1ControllerOperation *o,int changed,char *error,size_t cap){
 cod1ActorRecord_t *a=o->stage;unsigned team,i;
 uint32_t angles[2],quantized[2][5];
 const GscEntityRef empty={0};
 GscEntityRef hit={0};
 /* Original4C4FC clears the entire actor allocation. The selected actor
  * classifies favoriteenemy as native and refuses its unsupported setter;
  * general imported/nonempty sentient references require 4FD10. */
 if(!ActorControllerCurrent(o)||(!ActorControllerFreshDomain(a)&&!TrainingJumperLanding(o,a)&&!FoleyArrivedBody(o,a))||
    memcmp(&a->favoriteEnemy10f8,&empty,sizeof(empty))||
    !ActorPerceptionTailTeam(&o->lease,&team))return TrainingContactRefused(o,__LINE__,-1);
 if(!ActorSpawnNative6AA70(&o->lease,error,cap)||!ActorControllerCurrent(o)||
    !ActorSpawnNative6BA60(&o->lease,error,cap)||!ActorControllerCurrent(o))return TrainingContactRefused(o,__LINE__,-1);
 memcpy(angles,a->nativeData.entity.angles,sizeof(angles));
 if(memcmp(angles,o->entity->r.currentAngles,sizeof(angles)))return TrainingContactRefused(o,__LINE__,-1);
 for(i=0;i<2;i++)if(!C1AI14_OrientAngle(angles[i],quantized[i]))return TrainingContactRefused(o,__LINE__,-1);
 /* Neither of the admitted area queries reads these staged actor cells. */
 if(!TrainingRecordLink(o,changed)||a->towerTouch.frame || !ActorState9Contacts(o,team,&hit))return TrainingContactRefused(o,__LINE__,-1);
 if(hit.epoch){a->towerTouch.brush=hit;a->towerTouch.tick=o->now;a->towerTouch.frame=o->q.step.frame_generation;}
 memcpy(&a->nativeData.orientation.pitch,&quantized[0][2],4);
 memcpy(&a->nativeData.orientation.yaw,&quantized[1][2],4);
 a->nativeData.orientation.yawCopy=a->nativeData.orientation.yaw;
 memset(a->defaults.lifeWords+(0x278-0x26c)/4,0,3*sizeof(uint32_t));
 memset(a->defaults.lifeWords+(0x298-0x26c)/4,0,3*sizeof(uint32_t));
 ++a->scriptedController.revision;
 if(!ActorControllerCurrent(o))return TrainingContactRefused(o,__LINE__,-1);
 cod1Actors[o->lease.actorIndex]=*a;
 return hit.epoch?C1_STATE9_TOWER_CONTACT_RETURNED:C1_STATE9_EMPTY_CONTACT_RETURNED;
}
