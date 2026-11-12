#include "../g_cod1_actor_scripted_resources.h"
#define COD1_TOWER_GREETING
#include "../../qcommon/cod1_actor_scripted_bank14.h"
typedef struct {
 uint64_t epoch,revision,busy;
 const void *stage;
 size_t stageBytes;
 c1asb14Bank_t bank;
 TurretClipExtent clipExtent[C1ASB14_COUNT];
} C1ScriptedResources;
static C1ScriptedResources cod1ScriptedResources;
#include "g_cod1_actor_jumper_resources.inc"
static int ActorScriptedDataPrivate(const void *p,size_t n,const C1ScriptedResources *r){
 unsigned i;
 uintptr_t address=(uintptr_t)p;
 if(!p || !n)return 0;
 for(i=0;i<C1ASB14_COUNT;i++){
  /* Only the committed bank has immutable clip allocations. The staging
   * owner remains visible to callbacks while individual clips are loaded. */
  if(r==&cod1ScriptedResources && r->epoch){
   const TurretClipExtent *extent=&r->clipExtent[i];
   if(address>extent->hi || (address<extent->lo && n<=extent->lo-address))continue;
  }
  if(TurretBodyPrivate(p,n,r->bank.body[i]) ||
     ActorOverlap(p,n,r->bank.root[i],r->bank.root[i]?sizeof(*r->bank.root[i]):0))return 1;
 }
 return 0;
}
static int ActorScriptedPrivate(const void*p,size_t n){C1ScriptedResources*r=&cod1ScriptedResources;
 if(ActorJumperPrivate(p,n))return 1;
 if(ActorOverlap(p,n,r,sizeof(*r))||ActorOverlap(p,n,r->stage,r->stageBytes))return 1;
 if(!r->epoch&&!r->stage)return 0;if(ActorScriptedDataPrivate(p,n,r))return 1;
 return r->stage&&r->stageBytes==sizeof(*r)&&ActorScriptedDataPrivate(p,n,(const C1ScriptedResources*)r->stage);
}
static void ActorScriptedReset(void){ActorJumperReset();C1ScriptedResources*r=&cod1ScriptedResources;if(r->revision!=UINT64_MAX)r->revision++;C1ASB14_Free(&r->bank);r->epoch=0;/* Callback owner retains its busy/stage lease until unwind. */}
static int ActorScriptedReady(uint64_t epoch){C1ScriptedResources*r=&cod1ScriptedResources;return epoch&&r->epoch==epoch&&!r->busy&&ActorFaceReady(epoch)&&C1ASB14_Ready(&r->bank);}
static int ActorScriptedIndex(unsigned ordinal){unsigned i;if(!ordinal||ordinal>=C1GA_COUNT||cod1GenericDefinitions[ordinal].children)return -1;for(i=0;i<C1ASB14_COUNT;i++)if(ordinal==c1asb14Additional[2+i].ordinal&&!strcmp(cod1GenericNames[ordinal].name,c1asb14Names[i]))return (int)i;return -1;}
static const c1ar14Root_t*ActorScriptedRoot(unsigned ordinal){if(ordinal==892&&ActorJumperReady(cod1EntityEpoch))return cod1JumperResources.root;int i=ActorScriptedIndex(ordinal);return i>=0&&ActorScriptedReady(cod1EntityEpoch)?cod1ScriptedResources.bank.root[i]:NULL;}
static const c1ab14Clip_t*ActorScriptedBody(unsigned ordinal){if(ordinal==892&&ActorJumperReady(cod1EntityEpoch))return cod1JumperResources.body;int i=ActorScriptedIndex(ordinal);return i>=0&&ActorScriptedReady(cod1EntityEpoch)?cod1ScriptedResources.bank.body[i]:NULL;}
static int ActorScriptedSamplerBank(const c1ad14Resource_t*resource,c1ast14Bank_t*out){
 c1afu14Bank_t borrowed;unsigned i;if(!ActorScriptedReady(cod1EntityEpoch))return 0;
 memset(&borrowed,0,sizeof(borrowed));borrowed.base=cod1GenericUnmountedBank;
 for(i=0;i<5;i++){borrowed.faceRoot[i]=cod1FaceResources.root[i];borrowed.faceBody[i]=cod1FaceResources.body[i];}
 return C1ASB14_Bank(resource,&borrowed,&cod1ScriptedResources.bank,out);
}
int G_Cod1ActorScriptedResourcesLoad(void){C1ScriptedResources stage,*r=&cod1ScriptedResources;uint64_t epoch=cod1EntityEpoch,revision=r->revision;unsigned i;int ok=0;char path[128];
 if(ActorScriptedReady(epoch))return 1;
 if(!epoch||!level.spawning||!cod1EntityActive||r->busy||revision==UINT64_MAX||r->epoch||cod1GenericOperation.busy||!ActorFaceReady(epoch))return 0;
 memset(&stage,0,sizeof(stage));r->busy=epoch;r->stage=&stage;r->stageBytes=sizeof(stage);
 for(i=0;i<C1ASB14_COUNT;i++){
  snprintf(path,sizeof(path),"xanim/%s",c1asb14Names[i]);
  if(!TurretBankRead(path,stage.bank.root+i,stage.bank.body+i,epoch)||r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||!ActorFaceReady(epoch)||
   !C1ASB14_Valid(i,stage.bank.root[i],stage.bank.body[i])||!C1ANM14_Build(stage.bank.root[i],stage.bank.metadata+i))goto done;
 }
 if(r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||r->busy!=epoch||r->epoch||!ActorFaceReady(epoch)||!C1ASB14_Ready(&stage.bank))goto done;
 for(i=0;i<C1ASB14_COUNT;i++){
  TurretClipExtent *extent=&r->clipExtent[i];
  extent->lo=UINTPTR_MAX;extent->hi=0;
  TurretBodyRanges(NULL,0,stage.bank.body[i],extent);
  TurretClipRange(NULL,0,stage.bank.root[i],sizeof(*stage.bank.root[i]),extent);
 }
 r->bank=stage.bank;memset(&stage.bank,0,sizeof(stage.bank));r->epoch=epoch;ok=1;
 done:C1ASB14_Free(&stage.bank);r->stage=NULL;r->stageBytes=0;if(r->busy==epoch)r->busy=0;return ok;
}
