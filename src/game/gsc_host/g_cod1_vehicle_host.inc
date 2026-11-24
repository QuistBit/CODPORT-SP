/* Original constructor, selected C47/no turret/no clip-model branch.
 * Pending owner records are independent of actor/sentient/script_model pools.
 * VM selection remains off until the complete GAME path is qualified. */
#include "../g_cod1_vehicle.h"
#include <limits.h>
#include "../bg_cod1_vehicle_resources14.h"
#include "../bg_cod1_script_entity.h"
#include "../g_cod1_vehicle_nodes.h"
#include "../bg_cod1_vehicle_methods14.h"
extern int G_Cod1ScriptModelResolve(const char*,const char**);
typedef struct {
 int present,committed,broken,freeing,dobj,modelIndex,deleteAttempted;uint64_t serial,program;
 unsigned mode;cod1VehiclePathLease_t pathLease;int firstThink,nextThink;float currentSpeed;
 float childOrigin[2][3],childAngles[2][3];int childLinked[2];
 GscEntityRef owner,children[2];C1V14Record record;cod1DObjSummary_t topology;
 char model[128],targetname[128];
} GscVehicleRow;
static GscVehicleRow gscVehicles[32];
static C1V14PathNode gscVehicleGraph[GSC_VEHICLE_NODE_CAPACITY];
static C1VR14Bank gscVehicleBank,gscVehicleScratch;
static uint64_t gscVehicleBankEpoch,gscVehicleSerial;
static int gscVehicleBusy,gscVehicleCancelled;
static int gscVehicleResourceListCount,gscVehicleResourceLength,gscVehicleResourceRead;
static char gscVehicleResourceFile[80];
static int GscVehicleDeleteStoragePrivate(const void*,size_t);
static int GscVehicleDeleteInProgress(void);
static int GscVehicleDeleteBeforeFree(const GscEntityRef*);
static int GscVehiclePrivate(const void*p,size_t n){return GscVehicleDeleteStoragePrivate(p,n)|| C1V14_Overlap(p,n,gscVehicleGraph,sizeof(gscVehicleGraph))||C1V14_Overlap(p,n,gscVehicles,sizeof(gscVehicles))||C1V14_Overlap(p,n,&gscVehicleBank,sizeof(gscVehicleBank))||C1V14_Overlap(p,n,&gscVehicleScratch,sizeof(gscVehicleScratch))||C1V14_Overlap(p,n,gscSoundIndices,sizeof(gscSoundIndices))||C1V14_Overlap(p,n,g_entities,sizeof(g_entities))||C1V14_Overlap(p,n,&level,sizeof(level));}
typedef struct {uint64_t epoch,soundEpoch;int time;char registry[256][128];} GscVehicleResourceLease;
static int GscVehicleResourceCurrent(void*u){GscVehicleResourceLease*l=u;
 return !gscVehicleCancelled&&G_Cod1EntityMapEpoch()==l->epoch&&l->epoch&&level.time==l->time&&!level.spawning&&!level.restarted&&
  G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&gscSoundEpoch==l->soundEpoch&&!memcmp(gscSoundIndices,l->registry,sizeof(l->registry));}
static int GscVehicleFilesList(void*u,char*out,size_t cap){(void)u;gscVehicleResourceListCount=trap_FS_GetFileList("vehicles","",out,(int)cap);return gscVehicleResourceListCount;}
static int GscVehicleFilesRead(void*u,const char*path,char**out,size_t*bytes){
 fileHandle_t file=0;int length,got;char*p;(void)u;
 Q_strncpyz(gscVehicleResourceFile,path,sizeof(gscVehicleResourceFile));gscVehicleResourceRead=-1;
 length=trap_FS_FOpenFile(path,&file,FS_READ);gscVehicleResourceLength=length;
 if(length<12||length>=8203||!file){if(file)trap_FS_FCloseFile(file);return 0;}
 p=malloc((size_t)length);if(!p){trap_FS_FCloseFile(file);return 0;}
 got=trap_FS_ReadCount(p,length,file);gscVehicleResourceRead=got;trap_FS_FCloseFile(file);
 if(got!=length){free(p);return 0;}*out=p;*bytes=(size_t)length;return 1;
}
static int GscVehicleResourcesBegin(void){GscVehicleResourceLease lease;C1VR14Files files;int ok;
 if(gscVehicleBusy){gscVehicleCancelled=1;return 0;}
 if(gscVehicleBankEpoch)return gscVehicleBankEpoch==G_Cod1EntityMapEpoch();
 memset(&lease,0,sizeof(lease));lease.epoch=G_Cod1EntityMapEpoch();lease.soundEpoch=gscSoundEpoch;lease.time=level.time;memcpy(lease.registry,gscSoundIndices,sizeof(lease.registry));
 gscVehicleBusy=1;gscVehicleCancelled=0;gscVehicleResourceListCount=-1;gscVehicleResourceFile[0]=0;gscVehicleResourceLength=gscVehicleResourceRead=0;
 files.user=&lease;files.current=GscVehicleResourceCurrent;files.list=GscVehicleFilesList;files.read=GscVehicleFilesRead;files.release=GscSoundRelease;
 ok=C1VR14_Load(&files,lease.registry,&gscVehicleScratch,&gscVehicleBank)&&GscVehicleResourceCurrent(&lease);
 if(ok){memcpy(gscSoundIndices,gscVehicleBank.aliases,sizeof(gscSoundIndices));gscSoundEpoch=lease.epoch;gscVehicleBankEpoch=lease.epoch;}
 else {G_Printf("[vehicle resources] refused count=%d file=%s length=%d read=%d current=%d epoch=%llu sound=%llu time=%d/%d spawning=%d restart=%d map=%d gt=%d cancelled=%d\n",gscVehicleResourceListCount,gscVehicleResourceFile,gscVehicleResourceLength,gscVehicleResourceRead,GscVehicleResourceCurrent(&lease),(unsigned long long)lease.epoch,(unsigned long long)gscSoundEpoch,lease.time,level.time,level.spawning,level.restarted,G_Cod1MapEntities(),g_gametype.integer,gscVehicleCancelled);memset(&gscVehicleBank,0,sizeof(gscVehicleBank));}
 gscVehicleBusy=0;return ok;
}
static int GscVehicleRef(const GscEntityRef*a,const GscEntityRef*b){return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;}
static cod1SpatialActorKey_t GscVehicleKey(const GscVehicleRow*r){cod1SpatialActorKey_t k;memset(&k,0,sizeof(k));k.epoch=r->owner.epoch;k.slot=r->owner.slot;k.generation=r->owner.generation;k.lease=r->serial;return k;}
static GscVehicleRow*GscVehicleFind(const GscEntityRef*ref){int i;if(!ref)return NULL;for(i=0;i<32;i++)if(gscVehicles[i].present&&GscVehicleRef(&gscVehicles[i].owner,ref))return gscVehicles+i;return NULL;}
static int GscVehicleBody(GscVehicleRow*r){const gentity_t*e;
 if(!r||!r->present||r->broken||!G_Cod1EntitySameAllocation(&r->owner))return 0;e=g_entities+r->owner.slot;
 return e->classname&&!strcmp(e->classname,"script_vehicle")&&!e->client&&!e->aiCharacter&&!e->item&&!e->r.bmodel&&!e->neverFree&&
  e->s.eType==ET_GENERAL&&e->s.density==COD1_VEHICLE_NET_MARKER&&e->s.modelindex==r->modelIndex&&e->model&&!strcmp(e->model,r->model)&&
  e->targetname&&!strcmp(e->targetname,r->targetname)&&!e->touch&&!e->use;
}
static int GscVehiclePrepared(GscVehicleRow*r){gentity_t*e;int i;
 if(!GscVehicleBody(r)||!r->dobj||!r->topology.topologySerial)return 0;e=g_entities+r->owner.slot;
 if(!e->r.linked||(e->r.svFlags&SVF_NOCLIENT)||e->s.pos.trType!=TR_INTERPOLATE||e->s.apos.trType!=TR_INTERPOLATE||e->r.contents!=0x2080||e->clipmask!=0x810211||!e->takedamage||e->health||
  memcmp(e->r.currentOrigin,r->record.origin,12)||memcmp(e->r.currentAngles,r->record.angles,12)||
  memcmp(e->r.mins,r->topology.firstModelBounds.mins,12)||memcmp(e->r.maxs,r->topology.firstModelBounds.maxs,12))return 0;
 for(i=0;i<2;i++)if(r->record.soundEntities[i]!=2047){
  const gentity_t*c;if(!G_Cod1EntitySameAllocation(r->children+i)||r->record.soundEntities[i]!=(int)r->children[i].slot)return 0;
  c=g_entities+r->children[i].slot;if(!c->classname||strcmp(c->classname,"sound_blend")||c->client||c->aiCharacter||c->r.contents||c->r.bmodel||c->neverFree||c->touch||c->use||c->model||c->s.modelindex||c->health||c->takedamage||c->s.eType!=ET_GENERAL||!(c->r.svFlags&SVF_NOCLIENT)||
    c->r.linked!=r->childLinked[i]||memcmp(c->r.currentOrigin,r->childOrigin[i],12)||memcmp(c->r.currentAngles,r->childAngles[i],12))return 0;
 }
 return 1;
}
int G_Cod1VehicleThreadCurrent(const GscEntityRef*ref,uint64_t serial){GscVehicleRow*r=GscVehicleFind(ref);
 if(!r||!r->committed||r->serial!=serial||r->freeing)return 0;
 if(!GscVehicleBody(r)){r->broken=1;return 0;}return 1;
}
int G_Cod1VehicleThreadRead(const GscEntityRef*ref,GscEntityExecutionCertificate*out){GscVehicleRow*r;GscEntityExecutionCertificate c;
 if(!ref||!out||GscVehiclePrivate(ref,sizeof(*ref))||GscVehiclePrivate(out,sizeof(*out))||C1V14_Overlap(ref,sizeof(*ref),out,sizeof(*out)))return 0;
 r=GscVehicleFind(ref);if(!r||!G_Cod1VehicleThreadCurrent(ref,r->serial))return 0;
 memset(&c,0,sizeof(c));c.owner=*ref;c.serial=r->serial;c.current=G_Cod1VehicleThreadCurrent;*out=c;return 1;
}
static int GscVehicleSameRow(const GscVehicleRow*r,const GscVehicleRow*copy){return r->present&&r->serial==copy->serial&&GscVehicleRef(&r->owner,&copy->owner);}
static int GscVehicleDestroy(GscVehicleRow*r,int releaseOwner){GscVehicleRow copy;cod1DObjFree_t freeRequest;int i,status;
 if(!r||!r->present)return 1;if(r->freeing)return 0;copy=*r;r->freeing=1;r->broken=1;
 if(copy.dobj){memset(&freeRequest,0,sizeof(freeRequest));freeRequest.version=COD1_DOBJ_VERSION;freeRequest.bytes=sizeof(freeRequest);freeRequest.key=GscVehicleKey(&copy);
  status=trap_Cod1DObjFree(&freeRequest);if(!GscVehicleSameRow(r,&copy))return status==COD1_DOBJ_OK;
  if(status!=COD1_DOBJ_OK){r->freeing=0;return 0;}r->dobj=0;}
 for(i=0;i<2;i++)if(copy.children[i].epoch&&G_Cod1EntitySameAllocation(copy.children+i)){
  G_FreeEntity(g_entities+copy.children[i].slot);if(!GscVehicleSameRow(r,&copy))return 0;
  if(G_Cod1EntitySameAllocation(copy.children+i)){r->freeing=0;return 0;}
 }
 if(releaseOwner&&G_Cod1EntitySameAllocation(&copy.owner)){
  /* Remove the exact row before ordinary free calls back into this owner. */
  memset(r,0,sizeof(*r));G_FreeEntity(g_entities+copy.owner.slot);
  return !G_Cod1EntitySameAllocation(&copy.owner);
 }
 memset(r,0,sizeof(*r));return 1;
}
int G_Cod1VehicleBeforeFree(const GscEntityRef*ref){GscVehicleRow*r;
 if(GscVehicleDeleteInProgress())return GscVehicleDeleteBeforeFree(ref);
 r=GscVehicleFind(ref);return !r||GscVehicleDestroy(r,0);}
int G_Cod1VehicleMapCleanup(void){int i,ok=1;
 if(gscVehicleBusy){gscVehicleCancelled=1;return 0;}
 for(i=0;i<32;i++)if(gscVehicles[i].present&&!GscVehicleDestroy(gscVehicles+i,1))ok=0;
 if(ok){memset(&gscVehicleBank,0,sizeof(gscVehicleBank));memset(&gscVehicleScratch,0,sizeof(gscVehicleScratch));gscVehicleBankEpoch=0;}return ok;
}
typedef struct {GscVehicleSpawnRequest request;const GscVehicleSpawnRequest*borrowed;GscVehicleRow*row;GscEntityRef owner;uint64_t serial,epoch;int time;int(*current)(void*);void*context;} GscVehicleLease;
static int GscVehicleCurrent(GscVehicleLease*l){return !gscVehicleCancelled&&gscVehicleBusy&&level.time==l->time&&l->epoch==G_Cod1EntityMapEpoch()&&l->epoch==gscVehicleBankEpoch&&
 !memcmp(l->borrowed,&l->request,sizeof(l->request))&&!level.restarted&&!level.spawning&&l->current(l->context)==1&&
 !gscVehicleCancelled&&!memcmp(l->borrowed,&l->request,sizeof(l->request))&&level.time==l->time&&l->epoch==G_Cod1EntityMapEpoch()&&(!l->row||(l->row->present&&l->row->serial==l->serial&&GscVehicleRef(&l->row->owner,&l->owner)&&G_Cod1EntitySameAllocation(&l->owner)));}
static int GscVehicleCreateEntity(GscEntityRef*out){gentity_t*e;
 if(level.num_entities<MAX_CLIENTS||level.num_entities>ENTITYNUM_MAX_NORMAL||(level.num_entities==ENTITYNUM_MAX_NORMAL&&!G_EntitiesFree()))return 0;
 e=G_Spawn();return G_Cod1EntityCapture(e,out);
}
static int GscVehiclePrepare(void*u,const GscVehicleSpawnRequest*input,GscVehicleSpawnTicket*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscVehicleLease lease;GscVehicleRow*r=NULL;GscEntityRef mainRef;gentity_t*e;const C1V14Definition*d;const char*modelPath=NULL;
 cod1DObjBuild_t build;cod1DObjSummary_t topology;int i,index=-1,modelIndex,children[2]={2047,2047},ok=0;(void)u;
 if(gscVehicleBusy){gscVehicleCancelled=1;return 0;}
 if(!input||!out||!current||(cap&&!error)||GscVehiclePrivate(input,sizeof(*input))||GscVehiclePrivate(out,sizeof(*out))||GscVehiclePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),error,cap)||C1V14_Overlap(out,sizeof(*out),error,cap)||!input->program||!memchr(input->model,0,128)||!memchr(input->targetname,0,128)||!memchr(input->definition,0,128))return 0;
 if(!gscVehicleBankEpoch||gscVehicleBankEpoch!=G_Cod1EntityMapEpoch()||gscVehicleSerial==UINT64_MAX)return 0;
 /* Only the independently qualified definition branch. Other vehicles keep
  * an explicit boundary until their wheel/turret/clip contracts are ready. */
 for(i=0;i<(int)gscVehicleBank.count;i++)if(C1V14_Equal(input->definition,gscVehicleBank.definitions[i].name)){index=i;break;}
 if(index<0)return 0;d=gscVehicleBank.definitions+index;
 if(d->type!=2||!C1V14_Equal(d->name,"C47")||d->turretWeapon[0]||strcmp(input->model,"xmodel/c47"))return 0;
 for(i=0;i<3;i++)if(!isfinite(input->origin[i])||!isfinite(input->angles[i]))return 0;
 for(i=0;i<32;i++)if(!gscVehicles[i].present){r=gscVehicles+i;break;}if(!r)return 0;
 memset(&lease,0,sizeof(lease));lease.request=*input;lease.borrowed=input;lease.epoch=G_Cod1EntityMapEpoch();lease.time=level.time;lease.current=current;lease.context=context;
 gscVehicleBusy=1;gscVehicleCancelled=0;
 if(!GscVehicleCurrent(&lease)||!GscVehicleCreateEntity(&mainRef))goto done;
 /* Keep a cancellation receipt even if the caller expired in G_Spawn. The
  * receipt is for the captured allocation only, never a new map's slot. */
 if(mainRef.epoch!=lease.epoch||!G_Cod1EntitySameAllocation(&mainRef))goto done;
 memset(r,0,sizeof(*r));r->present=1;r->owner=mainRef;r->serial=++gscVehicleSerial;r->program=lease.request.program;lease.row=r;lease.serial=r->serial;lease.owner=mainRef;
 memset(out,0,sizeof(*out));out->operation=r->serial;out->program=r->program;out->entity=mainRef;
 if(!GscVehicleCurrent(&lease))goto done;e=g_entities+mainRef.slot;
 strcpy(r->model,lease.request.model);strcpy(r->targetname,lease.request.targetname);
 e->classname=G_NewString("script_vehicle");e->targetname=G_NewString(r->targetname);
 if(!GscVehicleCurrent(&lease))goto done;
 modelIndex=G_Cod1ScriptModelResolve(r->model,&modelPath);
 if(!GscVehicleCurrent(&lease)||modelIndex<=0||!modelPath)goto done;
 r->modelIndex=modelIndex;e->model=(char*)modelPath;e->s.modelindex=modelIndex;
 e->s.eType=ET_GENERAL;e->s.density=COD1_VEHICLE_NET_MARKER;VectorSet(e->s.angles2,1,1,1);
 /* Stage topology before publishing the ordinary native hull. No frame or
  * script may observe this hidden, nonsolid intermediate allocation. */
 e->r.contents=0;e->r.svFlags|=SVF_NOCLIENT;e->clipmask=0x810211;e->takedamage=qtrue;
 G_SetOrigin(e,lease.request.origin);G_SetAngle(e,lease.request.angles);e->s.pos.trType=e->s.apos.trType=TR_INTERPOLATE;
 memset(&build,0,sizeof(build));memset(&topology,0,sizeof(topology));build.version=COD1_DOBJ_VERSION;build.bytes=sizeof(build);build.key=GscVehicleKey(r);build.modelCount=1;strcpy(build.models[0].model,r->model);
 i=trap_Cod1DObjBuild(&build,&topology);
 if(!GscVehicleCurrent(&lease)){
  if(i==COD1_DOBJ_OK){cod1DObjFree_t old;memset(&old,0,sizeof(old));old.version=COD1_DOBJ_VERSION;old.bytes=sizeof(old);old.key=build.key;(void)trap_Cod1DObjFree(&old);}goto done;
 }
 if(i==COD1_DOBJ_OK)r->dobj=1;
 if(i!=COD1_DOBJ_OK||!topology.topologySerial||topology.modelCount!=1||!topology.boneCount||topology.boneCount>=128)goto done;
 r->topology=topology;VectorCopy(topology.firstModelBounds.mins,e->r.mins);VectorCopy(topology.firstModelBounds.maxs,e->r.maxs);
 e->r.contents=0x2080;e->r.svFlags&=~SVF_NOCLIENT;
 trap_LinkEntity(e);if(!GscVehicleCurrent(&lease)||!e->r.linked||!GscVehicleBody(r))goto done;
 for(i=0;i<2;i++)if(d->soundIndexes[i*2]||d->soundIndexes[i*2+1]){
  gentity_t*child;GscEntityRef childRef;if(!GscVehicleCreateEntity(&childRef))goto done;
  if(childRef.epoch!=lease.epoch)goto done;
  if(!r->present||r->serial!=lease.serial||!GscVehicleRef(&r->owner,&lease.owner)){
   if(G_Cod1EntitySameAllocation(&childRef))G_FreeEntity(g_entities+childRef.slot);goto done;
  }
  r->children[i]=childRef;if(!GscVehicleCurrent(&lease))goto done;
  child=g_entities+r->children[i].slot;child->classname=G_NewString("sound_blend");child->s.eType=ET_GENERAL;child->r.svFlags|=SVF_NOCLIENT;
  child->r.contents=0;children[i]=(int)r->children[i].slot;
 }
 if(!C1V14_RecordInit(mainRef.slot,(unsigned)index,lease.request.origin,lease.request.angles,children,&r->record)||!GscVehicleCurrent(&lease)||!GscVehiclePrepared(r))goto done;
 ok=1;
done:
 gscVehicleBusy=0;if(!ok)GscTestError(error,cap,"vehicle constructor resource/allocation/world boundary refused");return ok;
}
static int GscVehicleFinish(void*u,const GscVehicleSpawnTicket*input,int commit,char*error,size_t cap,int(*current)(void*),void*context){GscVehicleRow*r;(void)u;(void)error;(void)cap;
 if(!input||(commit!=0&&commit!=1)||!input->operation||gscVehicleBusy||GscVehiclePrivate(input,sizeof(*input))||GscVehiclePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),error,cap))return 0;r=GscVehicleFind(&input->entity);
 if(!r)return !commit;if(r->serial!=input->operation||r->program!=input->program)return 0;
 if(!commit)return GscVehicleDestroy(r,1);
 if(!current||!current(context)||!GscVehiclePrepared(r)||r->committed||level.time>INT_MAX-50)return 0;r->committed=1;r->firstThink=1;r->nextThink=level.time+50;return 1;
}
static const GscVehicleSpawnHost gscVehicleHost={NULL,GscVehiclePrepare,GscVehicleFinish};
#include "g_cod1_vehicle_methods.inc"
#include "g_cod1_vehicle_frame.inc"
#include "g_cod1_vehicle_delete.inc"
