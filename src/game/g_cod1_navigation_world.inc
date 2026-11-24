#include "g_cod1_navigation_world.h"
#include "g_cod1_player_lifetime.h"
#include "g_cod1_r200_collision.h"
int G_Cod1TurretNavigationMask(const GscEntityRef*,unsigned*);
typedef struct {int active,kind;GscEntityRef ref;unsigned flags,nativeContents;cod1PlayerLifetime_t player;cod1ActorPathView_t actor;entityShared_t shape;entityState_t state;} C1NWRow;
static struct {uint64_t serial,receipt,epoch;int now,count,busy,cancelled;unsigned mask;C1NWRow rows[MAX_GENTITIES];} cod1NavigationWorld;
/* Completed original397F0/39640 BSP birth owns C4=2080. Movement/render
 * changes do not grant collision geometry: only masks excluding2080 enter.
 * Any physical solid/model-role change or ambiguous actor owner refuses. */
static int Cod1NavigationModelMask(const GscEntityRef*ref,unsigned mask,unsigned*out){
 C1PathEntityRecord*r=cod1PathEntities+ref->slot;gentity_t*e=g_entities+ref->slot;cod1ActorPathView_t a;
 if(r->birth!=3||!r->present||r->kind!=7||memcmp(&r->entity,ref,sizeof(*ref))||r->nativeContents!=0x2080||
  (mask&r->nativeContents)||e->client||e->aiCharacter||!e->classname||strcmp(e->classname,"script_model")||
  e->r.bmodel||e->r.contents||e->s.eType!=ET_GENERAL||e->s.density!=COD1_SCRIPT_MODEL_NET_MARKER||
  e->touch||e->use||e->takedamage||e->spawnflags||G_Cod1ActorPathRead(ref,&a))return 0;
 *out=r->nativeContents;return 1;
}
/* Native origin setter and normal relinks preserve original trigger C4.
 * Keep the construction projection untouched: this grants excluded-mask
 * filtering only and still pins the complete current shape in the ticket. */
static int Cod1NavigationTriggerMask(const GscEntityRef*ref,unsigned mask,unsigned*out){
 C1PathEntityRecord*r=cod1PathEntities+ref->slot;gentity_t*e=g_entities+ref->slot;entityShared_t shape=e->r;unsigned i;
 if(!r->present||r->birth!=3||!r->projected||(r->kind!=3&&r->kind!=4)||memcmp(&r->entity,ref,sizeof(*ref))||
  r->nativeContents!=(r->kind==3?0x200000u:0x20000000u)||((r->nativeContents|(unsigned)e->r.contents)&mask)||
  e->client||!e->classname||strcmp(e->classname,r->kind==3?"trigger_use":"trigger_lookat")||e->r.contents!=r->shape.contents)return 0;
 shape.linkcount=r->shape.linkcount;
 /* Original maps/_utility::triggerOff moves the same trigger by -10000 Z.
  * A mask that excludes its native and port contents does not consult its
  * translated bounds. No collision geometry is admitted by this capability. */
 for(i=0;i<3;i++)if(!isfinite(shape.currentOrigin[i])||fabsf(shape.currentOrigin[i])>1000000||!isfinite(shape.absmin[i])||!isfinite(shape.absmax[i]))return 0;
 memcpy(shape.currentOrigin,r->shape.currentOrigin,12);memcpy(shape.absmin,r->shape.absmin,12);memcpy(shape.absmax,r->shape.absmax,12);
 if(memcmp(&shape,&r->shape,sizeof(shape))||e->s.modelindex!=r->model||e->s.eType!=r->type||e->spawnflags!=r->spawnflags||e->takedamage!=r->takedamage||e->touch!=r->touch||e->use!=r->use)return 0;
 *out=r->nativeContents;return 1;
}
static int Cod1NavigationWorldRow(unsigned slot,unsigned mask,C1NWRow*out){
 C1NWRow v;gentity_t*e=g_entities+slot;C1PathEntityRecord*r=cod1PathEntities+slot;unsigned contents;
 memset(&v,0,sizeof(v));v.active=!!e->inuse;if(!v.active){*out=v;return 1;}
 if(!G_Cod1EntityCapture(e,&v.ref))return 0;
 if(G_Cod1BodyReserveCurrent(&v.ref))v.kind=1;
 else if(G_Cod1ScriptHelperCurrent(&v.ref))v.kind=2;
 else if(G_Cod1TeleportEffectCurrent(&v.ref)&&!e->r.contents&&!e->r.bmodel)v.kind=3;
 else if(G_Cod1ActorPathRead(&v.ref,&v.actor)){
  if(!!v.actor.linked!=!!e->r.linked)return 0;v.kind=4;v.nativeContents=v.actor.contents;v.flags=v.actor.flags;
 }else if(e->client){
  /* Original149B3 live body C4=02000000, matching this physical body. */
  if(slot>=MAX_CLIENTS||slot>=(unsigned)level.maxclients||level.clients!=g_clients||e->client!=level.clients+slot||
   e->s.number!=(int)slot||e->client->ps.clientNum!=(int)slot||e->client->pers.connected!=CON_CONNECTED||
   !e->r.linked||e->r.bmodel||e->r.contents!=CONTENTS_BODY||e->health<=0||
   !G_Cod1PlayerLifetimeRead(&v.ref,&v.player)||!G_Cod1PlayerLifetimeSameCompleted(&v.player))return 0;
  v.kind=5;v.nativeContents=0x2000000;
 }else if(Cod1PathEntityTurretCurrent(&v.ref)){
  if((r->nativeContents|(unsigned)e->r.contents)&mask)return 0;v.kind=6;v.nativeContents=r->nativeContents;
 }else if(Cod1PathEntitySpawnerCurrent(&v.ref))v.kind=7;
 else if(G_Cod1TurretNavigationMask(&v.ref,&contents)){
  if((contents|(unsigned)e->r.contents)&mask)return 0;v.kind=12;v.nativeContents=contents;
 }
 else if(Cod1NavigationTriggerMask(&v.ref,mask,&contents)){v.kind=13;v.nativeContents=contents;}
 else if(Cod1NavigationModelMask(&v.ref,mask,&contents)){v.kind=8;v.nativeContents=contents;}
 else if(G_Cod1ScriptBrushNonsolidOwned(&v.ref)){v.kind=11;v.nativeContents=0;}
 else if(G_Cod1RuntimeScriptMask(&v.ref,&contents)){
  if((contents|(unsigned)e->r.contents)&mask)return 0;v.kind=9;v.nativeContents=contents;
 }else{
  if(!G_Cod1PathEntityRead(&v.ref,&v.flags))return 0;
  v.kind=10;
  if(r->kind==8||r->kind==9){if((r->nativeContents|(unsigned)e->r.contents)&mask)return 0;v.nativeContents=r->nativeContents;}
  else if(r->kind==6){if(e->r.linked||e->r.contents)return 0;}
  else if(r->projected){
   if(!Cod1PathProjectionRecord(&v.ref)||((unsigned)e->r.contents!=r->nativeContents&&(((unsigned)e->r.contents|r->nativeContents)&mask)))return 0;v.nativeContents=r->nativeContents;
  }else if(r->kind==1){if(e->r.contents)return 0;}
  else if(r->kind==2){if(e->r.contents!=CONTENTS_TRIGGER||!e->r.bmodel||e->spawnflags)return 0;v.nativeContents=0x40000000;}
  else {if(e->r.contents!=CONTENTS_SOLID||!e->r.bmodel||e->s.eType!=ET_MOVER)return 0;v.nativeContents=1;}
 }
 v.shape=e->r;v.state=e->s;if(!G_Cod1EntitySameAllocation(&v.ref))return 0;*out=v;return 1;
}
void G_Cod1NavigationWorldEnd(uint64_t receipt){
 if(!receipt||receipt!=cod1NavigationWorld.receipt)return;
 if(cod1NavigationWorld.busy){cod1NavigationWorld.cancelled=1;return;}
 cod1NavigationWorld.receipt=0;cod1NavigationWorld.epoch=0;memset(cod1NavigationWorld.rows,0,sizeof(cod1NavigationWorld.rows));
}
int G_Cod1NavigationWorldCurrent(uint64_t receipt){
 C1NWRow v;int i,ok=0;
 if(!receipt||receipt!=cod1NavigationWorld.receipt)return 0;
 if(cod1NavigationWorld.busy){cod1NavigationWorld.cancelled=1;return 0;}cod1NavigationWorld.busy=1;
 if(cod1NavigationWorld.cancelled||!g_cod1MapEntities||cod1PathWorldPhase||
  cod1NavigationWorld.epoch!=G_Cod1EntityMapEpoch()||cod1NavigationWorld.count!=level.num_entities||cod1NavigationWorld.now!=level.time)goto done;
 for(i=0;i<cod1NavigationWorld.count;i++)if(!Cod1NavigationWorldRow(i,cod1NavigationWorld.mask,&v)||memcmp(&v,cod1NavigationWorld.rows+i,sizeof(v))||cod1NavigationWorld.cancelled)goto done;
 ok=cod1NavigationWorld.epoch==G_Cod1EntityMapEpoch()&&cod1NavigationWorld.count==level.num_entities&&cod1NavigationWorld.now==level.time&&!cod1NavigationWorld.cancelled;
done:cod1NavigationWorld.busy=0;if(!ok)G_Cod1NavigationWorldEnd(receipt);return ok;
}
int G_Cod1NavigationWorldBegin(unsigned mask,uint64_t*out){
 int i;uint64_t receipt;
 if(!out||(mask!=0x820011&&mask!=0x2820011)||G_Cod1EntityStorageProtected(out,sizeof(*out))||
  Cod1PathWorldOverlap(out,sizeof(*out),&cod1NavigationWorld,sizeof(cod1NavigationWorld))||
  cod1NavigationWorld.receipt||cod1NavigationWorld.busy||cod1NavigationWorld.serial==UINT64_MAX||
  !g_cod1MapEntities||cod1PathWorldPhase||!G_Cod1EntityMapEpoch()||level.num_entities<0||level.num_entities>MAX_GENTITIES)return 0;
 cod1NavigationWorld.busy=1;cod1NavigationWorld.cancelled=0;cod1NavigationWorld.epoch=G_Cod1EntityMapEpoch();cod1NavigationWorld.now=level.time;cod1NavigationWorld.count=level.num_entities;cod1NavigationWorld.mask=mask;
 for(i=0;i<cod1NavigationWorld.count;i++)if(!Cod1NavigationWorldRow(i,mask,cod1NavigationWorld.rows+i)||cod1NavigationWorld.cancelled){
  G_Printf("[549 navigation world] refused slot=%d class=%s tick=%d\n",i,g_entities[i].classname?g_entities[i].classname:"",level.time);cod1NavigationWorld.busy=0;return 0;
 }
 receipt=++cod1NavigationWorld.serial;cod1NavigationWorld.receipt=receipt;cod1NavigationWorld.busy=0;
 if(!G_Cod1NavigationWorldCurrent(receipt))return 0;*out=receipt;return 1;
}
