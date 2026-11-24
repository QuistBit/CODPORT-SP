#include "../bg_cod1_vehicle_frame14.h"
/* Vehicle-only native branch. A real overlapping trigger is an explicit
 * boundary until its original touch/action callbacks are admitted. The
 * broadphase and contact test always use actual world/brush geometry. */
static int GscVehicleNoTouch(GscVehicleMethodLease*l){
 vec3_t queryMin,queryMax,mins,maxs;float expectedBounds[12];static const float half[3]={40,40,52};
 int entities[MAX_GENTITIES],count,i,j;GscEntityRef ref;gentity_t before;const gentity_t*e;
 if(!GscVehicleMethodCurrent(l))return 0;
 for(i=MAX_CLIENTS;i<level.num_entities;i++){
  e=g_entities+i;if(!e->inuse||!e->classname||strncmp(e->classname,"trigger_",8)||!(e->spawnflags&16))continue;
  if(strcmp(e->classname,"trigger_multiple")||e->spawnflags!=24||!e->r.linked||!e->r.bmodel||e->r.contents!=8||e->touch||e->use||e->s.modelindex<=0)return 0;
 }
 for(i=0;i<3;i++){
  queryMin[i]=l->row->record.origin[i]-half[i];queryMax[i]=l->row->record.origin[i]+half[i];
  mins[i]=l->row->record.origin[i]+l->row->topology.firstModelBounds.mins[i];maxs[i]=l->row->record.origin[i]+l->row->topology.firstModelBounds.maxs[i];
 }
 memcpy(expectedBounds,queryMin,12);memcpy(expectedBounds+3,queryMax,12);memcpy(expectedBounds+6,mins,12);memcpy(expectedBounds+9,maxs,12);
 count=trap_EntitiesInBox(queryMin,queryMax,entities,MAX_GENTITIES);
 if(!GscVehicleMethodCurrent(l)||memcmp(expectedBounds,queryMin,12)||memcmp(expectedBounds+3,queryMax,12)||count<0||count>=MAX_GENTITIES)return 0;
 for(i=0;i<count;i++){
  if(entities[i]<0||entities[i]>=level.num_entities)return 0;for(j=0;j<i;j++)if(entities[j]==entities[i])return 0;
  e=g_entities+entities[i];if(!e->inuse||!(e->r.contents&8))continue;
  if(!G_Cod1EntityCapture(e,&ref)||!e->classname||strcmp(e->classname,"trigger_multiple")||e->spawnflags!=24||!e->r.linked||!e->r.bmodel||e->touch||e->use)return 0;
  before=*e;j=trap_EntityContact(mins,maxs,e);
  if(!GscVehicleMethodCurrent(l)||memcmp(expectedBounds+6,mins,12)||memcmp(expectedBounds+9,maxs,12)||!G_Cod1EntitySameAllocation(&ref)||memcmp(e,&before,sizeof(before)))return 0;
  if(j){G_Printf("[vehicle] native trigger touch boundary owner=%u trigger=%u\n",l->q.owner.slot,ref.slot);return 0;}
 }
 return 1;
}
static int GscVehicleRootZero(GscVehicleMethodLease*l){
 cod1DObjQuery_t q,expected;cod1DObjQueryResult_t result;int status;
 if(!GscVehicleNoWheels(l))return 0;
 memset(&q,0,sizeof(q));memset(&result,0,sizeof(result));q.version=COD1_DOBJ_QUERY_VERSION;q.bytes=sizeof(q);q.key=GscVehicleKey(l->row);q.topologySerial=l->expected.topology.topologySerial;q.phase=COD1_DOBJ_QUERY_BONE;strcpy(q.tag,"tag_origin");expected=q;
 status=trap_Cod1DObjQuery(&q,&result);
 return GscVehicleMethodCurrent(l)&&!memcmp(&q,&expected,sizeof(q))&&status==COD1_DOBJ_OK&&result.version==COD1_DOBJ_QUERY_VERSION&&result.bytes==sizeof(result)&&result.topologySerial==q.topologySerial&&result.modelCount==1&&result.boneCount==l->expected.topology.boneCount&&memchr(result.model,0,sizeof(result.model))&&!strcmp(result.model,l->expected.model)&&result.boneIndex==0;
}
static int GscVehicleFrameOne(GscVehicleRow*r,int now,int(*current)(void*),void*context,
 int(*notify)(void*,const GscEntityRef*,const char*),void*notifyContext){
 GscVehicleMethodRequest request;GscVehicleMethodLease l;C1V14Record next;C1VC14Result result;unsigned i;int ok=0;
 static const char*events[]={"","reached_wait_node","reached_end_node","reached_wait_speed"};
 if(!r->present)return 1;if(!r->committed||r->freeing||r->broken||r->nextThink<=0||now<0||level.time!=now||now>INT_MAX-50||gscVehicleBusy)return 0;
 if(now<r->nextThink)return GscVehiclePrepared(r);
 memset(&l,0,sizeof(l));memset(&request,0,sizeof(request));request.owner=r->owner;request.program=r->program;
 l.input=&request;l.q=request;l.row=r;l.expected=*r;l.time=now;l.epoch=r->owner.epoch;l.current=current;l.context=context;l.graph=r->pathLease;
 gscVehicleBusy=1;gscVehicleCancelled=0;
 if(!GscVehicleMethodCurrent(&l)||(r->firstThink&&!GscVehicleRootZero(&l)))goto done;
 if(!C1VF14_Frame(gscVehicleGraph,l.graph.count,gscVehicleBank.definitions+r->record.definition,&r->record,r->mode,r->firstThink,r->currentSpeed,&next,&result))goto done;
 if(!GscVehicleMethodPublish(&l,&next)||!GscVehicleNoTouch(&l))goto done;
 if(!r->firstThink)for(i=0;i<2;i++)if(r->children[i].epoch){
  gentity_t*child=g_entities+r->children[i].slot;r->childLinked[i]=1;l.expected=*r;
  trap_LinkEntity(child);if(!GscVehicleMethodCurrent(&l))goto done;
 }
 r->firstThink=0;r->nextThink=now+50;r->currentSpeed=result.speed;l.expected=*r;
 for(i=0;i<result.count;i++){
  GscEntityRef owner=r->owner;if(result.events[i]<1||result.events[i]>3||!GscVehicleMethodCurrent(&l))goto done;
  if(!notify(notifyContext,&owner,events[result.events[i]])||!GscVehicleMethodCurrent(&l)||!GscVehicleRef(&owner,&r->owner))goto done;
 }
 ok=GscVehicleMethodCurrent(&l);
done:if(!ok&&GscVehicleSameRow(r,&l.expected))r->broken=1;gscVehicleBusy=0;return ok;
}
static int GscVehicleFrames(int now,int(*current)(void*),void*context,int(*notify)(void*,const GscEntityRef*,const char*),void*notifyContext){
 unsigned i;if(!current||!notify)return 0;
 for(i=0;i<32;i++)if(gscVehicles[i].present&&(!GscVehiclePrepared(gscVehicles+i)||gscVehicles[i].mode>1))return 0;
 for(i=0;i<32;i++)if(!GscVehicleFrameOne(gscVehicles+i,now,current,context,notify,notifyContext))return 0;
 return 1;
}
