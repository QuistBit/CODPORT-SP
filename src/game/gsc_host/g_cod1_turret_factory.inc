/* GAME31150/233D0. Native-created turrets initially have no model: the GSC
 * assigns it later. This owner admits that genuine unlinked, unoccupied phase.
 * Model assignment, linked/target/occupied frames and nonempty weapon scripts
 * remain explicit boundaries; no stand-in model or manufactured tag is used. */
#include "../g_cod1_turret_factory.h"
#include "../bg_cod1_turret_spawn_resources14.h"
#include "../bg_cod1_turret_initial_pose14.h"
#include "../bg_cod1_turret_unoccupied14.h"
typedef struct {
 int present,committed,broken,definition,handler,due;
 uint64_t serial,program,sessionId,itemEpoch,itemIncarnation;GscSession*session;GscEntityRef owner;
 unsigned registry;const cwDef_t*resource;cwDef_t definitionBytes;
 C1TS14Record record;C1TS14EntityProjection spawn;float controls[3];
} GscTurretFactoryRow;
static GscTurretFactoryRow gscFactoryTurrets[C1TS14_CAPACITY];
static GscTurretFactoryRow gscFactoryTurretBefore[C1TS14_CAPACITY];
static gentity_t gscFactoryTurretBodies[C1TS14_CAPACITY];
static char gscFactoryTurretSounds[256][128];
static uint64_t gscFactoryTurretSerial;
static int gscFactoryTurretBusy,gscFactoryTurretCancelled;
int G_Cod1TurretFactoryPrivate(const void*p,size_t n){return C1TS14_Overlap(p,n,gscFactoryTurrets,sizeof(gscFactoryTurrets))||C1TS14_Overlap(p,n,gscFactoryTurretBefore,sizeof(gscFactoryTurretBefore))||C1TS14_Overlap(p,n,gscFactoryTurretBodies,sizeof(gscFactoryTurretBodies))||C1TS14_Overlap(p,n,gscFactoryTurretSounds,sizeof(gscFactoryTurretSounds))||C1TS14_Overlap(p,n,&gscFactoryTurretSerial,sizeof(gscFactoryTurretSerial))||C1TS14_Overlap(p,n,&gscFactoryTurretBusy,sizeof(gscFactoryTurretBusy))||C1TS14_Overlap(p,n,&gscFactoryTurretCancelled,sizeof(gscFactoryTurretCancelled));}
static int GscTurretFactoryPrivate(const void*p,size_t n){return G_Cod1TurretFactoryPrivate(p,n)||GscTurretPrecachePrivate(p,n);}
static GscTurretFactoryRow*GscTurretFactoryFind(const GscEntityRef*ref){unsigned i;if(!ref)return NULL;for(i=0;i<C1TS14_CAPACITY;i++)if(gscFactoryTurrets[i].present&&GscVehicleRef(ref,&gscFactoryTurrets[i].owner))return gscFactoryTurrets+i;return NULL;}
static int GscTurretFactoryBody(const GscTurretFactoryRow*r){const gentity_t*e;
 if(!r||!r->present||r->broken||!r->registry||r->registry!=CW_RegistryRevision()||CW_Get(r->definition)!=r->resource||memcmp(r->resource,&r->definitionBytes,sizeof(*r->resource))||!G_Cod1EntitySameAllocation(&r->owner)||r->itemEpoch!=gscTestItem.epoch||r->itemIncarnation!=gscTestItem.incarnation||r->session!=gscTestItem.session||r->sessionId!=gscTestItem.id||gscTestItem.failed||!gscTestItem.registered[r->definition+1]||!gscTestItem.complete[r->definition+1])return 0;
 e=g_entities+r->owner.slot;
 return e->classname&&!strcmp(e->classname,"misc_turret")&&!e->client&&!e->aiCharacter&&!e->item&&!e->model&&!e->s.modelindex&&!e->r.bmodel&&!e->neverFree&&!e->touch&&!e->use&&!e->die&&!e->think&&!e->nextthink&&
  e->s.eType==ET_GENERAL&&e->r.linked&&e->r.contents==r->spawn.contents&&(e->r.svFlags&SVF_USE_CURRENT_ORIGIN)&&!(e->r.svFlags&SVF_NOCLIENT)&&e->takedamage&&e->health==r->spawn.health&&e->damage==r->spawn.damage&&
  e->s.pos.trType==TR_STATIONARY&&e->s.apos.trType==TR_INTERPOLATE&&!e->s.loopSound&&
  !memcmp(e->r.mins,r->spawn.mins,12)&&!memcmp(e->r.maxs,r->spawn.maxs,12)&&!memcmp(e->r.currentOrigin,r->spawn.origin,12)&&!memcmp(e->r.currentAngles,r->spawn.angles,12);
}
static int GscTurretFactoryThreadCurrent(const GscEntityRef*ref,uint64_t serial){GscTurretFactoryRow*r=GscTurretFactoryFind(ref);return r&&r->committed&&r->serial==serial&&r->session==gscTestSession&&r->sessionId==gscTestId&&GscTurretFactoryBody(r);}
int G_Cod1TurretFactoryThreadRead(const GscEntityRef*ref,GscEntityExecutionCertificate*out){GscTurretFactoryRow*r;GscEntityExecutionCertificate c;
 if(!ref||!out||GscTurretFactoryPrivate(ref,sizeof(*ref))||GscTurretFactoryPrivate(out,sizeof(*out))||C1TS14_Overlap(ref,sizeof(*ref),out,sizeof(*out)))return 0;
 r=GscTurretFactoryFind(ref);if(!r||!GscTurretFactoryThreadCurrent(ref,r->serial))return 0;memset(&c,0,sizeof(c));c.owner=*ref;c.serial=r->serial;c.current=GscTurretFactoryThreadCurrent;*out=c;return 1;
}
int G_Cod1TurretFactoryBeforeFree(const GscEntityRef*ref){GscTurretFactoryRow*r=GscTurretFactoryFind(ref);if(r)memset(r,0,sizeof(*r));return 1;}
static int GscTurretFactoryDestroy(GscTurretFactoryRow*r){GscEntityRef ref;if(!r||!r->present)return 1;ref=r->owner;memset(r,0,sizeof(*r));if(G_Cod1EntitySameAllocation(&ref))G_FreeEntity(g_entities+ref.slot);return !G_Cod1EntitySameAllocation(&ref);}
int G_Cod1TurretFactoryMapCleanup(void){unsigned i;int ok=1;if(gscFactoryTurretBusy){gscFactoryTurretCancelled=1;return 0;}for(i=0;i<C1TS14_CAPACITY;i++)if(!GscTurretFactoryDestroy(gscFactoryTurrets+i))ok=0;return ok;}
typedef struct {
 GscTurretSpawnRequest q;const GscTurretSpawnRequest*borrowed;GscTestItemScene items;
 GscTurretFactoryRow*row;GscEntityRef owner;uint64_t serial;int time,initialization;unsigned revision;
 const cwDef_t*definition;cwDef_t expectedDefinition;
 char sounds[256][128];int(*current)(void*);void*context;
} GscTurretFactoryLease;
static int GscTurretFactoryDirect(const GscTurretFactoryLease*l){return gscFactoryTurretBusy&&!gscFactoryTurretCancelled&&!gscTestStopRequested&&!gscTestCompleted&&!level.spawning&&!level.restarted&&level.time==l->time&&
 l->initialization==gscTestItemInitialization&&l->revision&&l->revision==CW_RegistryRevision()&&(!l->definition||!memcmp(l->definition,&l->expectedDefinition,sizeof(l->expectedDefinition)))&&!memcmp(l->borrowed,&l->q,sizeof(l->q))&&GscTestItemCurrent(&l->items)&&gscSoundEpoch==l->items.map_epoch&&!memcmp(l->sounds,gscSoundIndices,sizeof(l->sounds))&&
 (!l->row||(l->row->present&&l->row->serial==l->serial&&GscVehicleRef(&l->row->owner,&l->owner)&&G_Cod1EntitySameAllocation(&l->owner)));}
static int GscTurretFactoryCurrent(GscTurretFactoryLease*l){GscTurretFactoryRow saved;gentity_t body;int row;
 if(!GscTurretFactoryDirect(l))return 0;row=l->row!=NULL;if(row){saved=*l->row;body=g_entities[l->owner.slot];}
 if(l->current(l->context)!=1||!GscTurretFactoryDirect(l))return 0;
 return !row||(!memcmp(&saved,l->row,sizeof(saved))&&!memcmp(&body,g_entities+l->owner.slot,sizeof(body)));
}
static int GscTurretFactorySoundGet(void*u,unsigned index,char*out,size_t cap){GscTurretFactoryLease*l=u;if(index<1||index>255||cap<128||!GscTurretFactoryCurrent(l))return 0;memcpy(out,gscSoundIndices[index],128);return 1;}
static int GscTurretFactorySoundSet(void*u,unsigned index,const char*name){GscTurretFactoryLease*l=u;if(index<1||index>255||!Cod1ScriptSoundText(name)||!GscTurretFactoryCurrent(l))return 0;strcpy(gscSoundIndices[index],name);strcpy(l->sounds[index],name);return GscTurretFactoryCurrent(l);}
static int GscTurretFactoryPrepare(void*u,const GscTurretSpawnRequest*input,GscTurretSpawnTicket*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscTurretFactoryLease l;GscTurretFactoryRow*r=NULL,saved;GscEntityRef ref;gentity_t*e,savedBody;const cwDef_t*d;C1TS14Weapon weapon;C1TS14EntityInput initial;Cod1ScriptSoundOps sounds;GscPrecacheItemEvent registration;unsigned loop,stop;int index,i,ok=0;(void)u;
 if(gscFactoryTurretBusy){gscFactoryTurretCancelled=1;return 0;}
 if(!input||!out||!current||(cap&&!error)||GscTurretFactoryPrivate(input,sizeof(*input))||GscTurretFactoryPrivate(out,sizeof(*out))||GscTurretFactoryPrivate(error,cap)||C1TS14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1TS14_Overlap(input,sizeof(*input),error,cap)||C1TS14_Overlap(out,sizeof(*out),error,cap)||!input->program||!input->precache_operation||input->precache_operation==UINT64_MAX||!memchr(input->classname,0,128)||!memchr(input->weapon,0,128)||strcmp(input->classname,"misc_turret")||gscFactoryTurretSerial==UINT64_MAX)return 0;
 memset(&l,0,sizeof(l));l.q=*input;l.borrowed=input;l.items=gscTestItem;l.time=level.time;l.initialization=gscTestItemInitialization;l.revision=CW_RegistryRevision();l.current=current;l.context=context;memcpy(l.sounds,gscSoundIndices,sizeof(l.sounds));
 l.definition=CW_Get(CW_Find(l.q.weapon));if(l.definition)l.expectedDefinition=*l.definition;
 gscFactoryTurretBusy=1;gscFactoryTurretCancelled=0;
 if(!GscTurretFactoryCurrent(&l))goto done;
 /* Native reserves a first-free turret record before validating the weapon.
  * The VM cancellation ticket owns this exact allocation on later refusal. */
 for(i=0;i<C1TS14_CAPACITY;i++)if(!gscFactoryTurrets[i].present){r=gscFactoryTurrets+i;break;}if(!r||!GscVehicleCreateEntity(&ref))goto done;
 if(ref.epoch!=l.items.map_epoch||!G_Cod1EntitySameAllocation(&ref))goto done;
 memset(r,0,sizeof(*r));r->present=1;r->owner=ref;r->serial=++gscFactoryTurretSerial;r->program=l.q.program;r->session=gscTestSession;r->sessionId=gscTestId;l.row=r;l.owner=ref;l.serial=r->serial;
 memset(out,0,sizeof(*out));out->operation=r->serial;out->program=r->program;out->entity=ref;
 if(!GscTurretFactoryCurrent(&l))goto done;
 index=CW_Find(l.q.weapon);d=CW_Get(index);if(!d||d!=l.definition||!*l.q.weapon||*CW_String(d,"script"))goto done;
 if(!gscTestItemInitialization&&(!gscTestItem.registered[index+1]||!gscTestItem.complete[index+1]))goto done;
 if(gscTestItemInitialization){saved=*r;savedBody=g_entities[ref.slot];memset(&registration,0,sizeof(registration));strcpy(registration.name,l.q.weapon);registration.operation_id=l.q.precache_operation;if(!GscTestItemPublish(NULL,gscTestId,l.q.now_ms,&registration,error,cap)||memcmp(&saved,r,sizeof(saved))||memcmp(&savedBody,g_entities+ref.slot,sizeof(savedBody))||!GscTurretFactoryCurrent(&l))goto done;}
 if(!gscTestItem.registered[index+1]||!gscTestItem.complete[index+1])goto done;
 memset(&sounds,0,sizeof(sounds));sounds.user=&l;sounds.get_index=GscTurretFactorySoundGet;sounds.set_index=GscTurretFactorySoundSet;
 if(Cod1ScriptSoundIndex(&sounds,CW_String(d,"loopFireSound"),&loop)!=COD1_SS_OK||Cod1ScriptSoundIndex(&sounds,CW_String(d,"stopFireSound"),&stop)!=COD1_SS_OK||!GscTurretFactoryCurrent(&l)||!C1TSR14_Defaults(d,(unsigned)index+1,loop,stop,&weapon))goto done;
 memset(&initial,0,sizeof(initial));initial.time=level.time;memcpy(initial.origin,l.q.origin,12);
 if(!C1TS14_Construct(&weapon,&initial,&r->record,&r->spawn)||!GscTurretFactoryCurrent(&l))goto done;
 r->definition=index;r->resource=d;r->definitionBytes=*d;r->registry=l.revision;r->itemEpoch=gscTestItem.epoch;r->itemIncarnation=gscTestItem.incarnation;r->handler=r->spawn.handler;r->due=r->spawn.nextThink;
 e=g_entities+ref.slot;e->classname=G_NewString(l.q.classname);if(!GscTurretFactoryCurrent(&l))goto done;
 e->s.eType=ET_GENERAL;e->s.groundEntityNum=ENTITYNUM_NONE;e->takedamage=qtrue;e->health=r->spawn.health;e->damage=r->spawn.damage;
 e->r.contents=r->spawn.contents;e->r.svFlags|=SVF_USE_CURRENT_ORIGIN;e->r.svFlags&=~SVF_NOCLIENT;VectorCopy(r->spawn.mins,e->r.mins);VectorCopy(r->spawn.maxs,e->r.maxs);
 G_SetOrigin(e,r->spawn.origin);G_SetAngle(e,r->spawn.angles);e->s.pos.trType=TR_STATIONARY;e->s.apos.trType=TR_INTERPOLATE;
 saved=*r;trap_LinkEntity(e);if(memcmp(&saved,r,sizeof(saved))||!GscTurretFactoryCurrent(&l)||!GscTurretFactoryBody(r))goto done;ok=1;
done:gscFactoryTurretBusy=0;if(!ok)GscTestError(error,cap,"turret constructor allocation/precache/script/resource/owner boundary refused");return ok;
}
/* A currentness callback cannot silently alter another row of this pool.
 * Scratch belongs to the same nonreentrant scope and never to a stack frame. */
static int GscTurretFactoryFrameFence(int now,int(*current)(void*),void*context){unsigned i;uint64_t epoch=G_Cod1EntityMapEpoch(),id=gscTestId;GscSession*session=gscTestSession;int initial=gscTestItemInitialization;
 if(!gscFactoryTurretBusy||gscFactoryTurretCancelled||!current||!epoch||now!=level.time||level.spawning||level.restarted||gscTestStopRequested||gscTestCompleted||gscSoundEpoch!=epoch)return 0;
 for(i=0;i<C1TS14_CAPACITY;i++)if(gscFactoryTurrets[i].present){if(!GscTurretFactoryBody(gscFactoryTurrets+i))return 0;gscFactoryTurretBodies[i]=g_entities[gscFactoryTurrets[i].owner.slot];}
 memcpy(gscFactoryTurretBefore,gscFactoryTurrets,sizeof(gscFactoryTurrets));memcpy(gscFactoryTurretSounds,gscSoundIndices,sizeof(gscSoundIndices));
 if(current(context)!=1||!gscFactoryTurretBusy||gscFactoryTurretCancelled||G_Cod1EntityMapEpoch()!=epoch||now!=level.time||level.spawning||level.restarted||gscTestStopRequested||gscTestCompleted||gscTestId!=id||gscTestSession!=session||gscTestItemInitialization!=initial||gscSoundEpoch!=epoch||memcmp(gscFactoryTurretBefore,gscFactoryTurrets,sizeof(gscFactoryTurrets))||memcmp(gscFactoryTurretSounds,gscSoundIndices,sizeof(gscSoundIndices)))return 0;
 for(i=0;i<C1TS14_CAPACITY;i++)if(gscFactoryTurrets[i].present&&(!GscTurretFactoryBody(gscFactoryTurrets+i)||memcmp(gscFactoryTurretBodies+i,g_entities+gscFactoryTurrets[i].owner.slot,sizeof(gentity_t))))return 0;
 return 1;
}
static int GscTurretFactoryFinish(void*u,const GscTurretSpawnTicket*input,int commit,char*error,size_t cap,int(*current)(void*),void*context){GscTurretFactoryRow*r;GscTurretSpawnTicket ticket;int ok;(void)u;(void)error;(void)cap;
 if(!input||(commit!=0&&commit!=1)||!input->operation||gscFactoryTurretBusy||GscTurretFactoryPrivate(input,sizeof(*input))||GscTurretFactoryPrivate(error,cap)||C1TS14_Overlap(input,sizeof(*input),error,cap))return 0;ticket=*input;r=GscTurretFactoryFind(&ticket.entity);
 if(!r)return !commit;if(r->serial!=ticket.operation||r->program!=ticket.program)return 0;
 if(!commit)return GscTurretFactoryDestroy(r);
 if(!current||r->committed||!GscTurretFactoryBody(r)||r->session!=gscTestSession||r->sessionId!=gscTestId)return 0;
 gscFactoryTurretBusy=1;gscFactoryTurretCancelled=0;ok=GscTurretFactoryFrameFence(level.time,current,context)&&!memcmp(input,&ticket,sizeof(ticket));gscFactoryTurretBusy=0;if(!ok)return 0;
 r->committed=1;G_Printf("[turret factory] committed slot=%u weapon=%s due=%d model=none\n",r->owner.slot,r->resource->name,r->due);return 1;
}
static const GscTurretSpawnHost gscTurretFactoryHost={NULL,GscTurretFactoryPrepare,GscTurretFactoryFinish};
static int GscTurretFactoryFrames(int now,int(*current)(void*),void*context){unsigned i;int next,handler,status,ok=0;C1TS14Record record;C1TU14Projection projection;
 if(gscFactoryTurretBusy){gscFactoryTurretCancelled=1;return 0;}if(!current)return 0;gscFactoryTurretBusy=1;gscFactoryTurretCancelled=0;
 if(!GscTurretFactoryFrameFence(now,current,context))goto done;
 for(i=0;i<C1TS14_CAPACITY;i++){GscTurretFactoryRow*r=gscFactoryTurrets+i;if(!r->present)continue;
  if(!r->committed||r->session!=gscTestSession||r->sessionId!=gscTestId||!GscTurretFactoryBody(r))goto done;
  status=C1TI14_Schedule(now,r->due,r->handler,&next,&handler);if(status<0)goto done;if(!status)continue;
  if(r->handler==13){/* No DObj/model: both native tag lookups are absent. */r->due=next;r->handler=handler;G_Printf("[turret factory] firstthink slot=%u tick=%d missing_model_tags=1 next=%d\n",r->owner.slot,now,next);continue;}
  if(!C1TU14_Update(&r->record,r->controls,now,&record,&projection)||projection.stopEventSound||projection.notifyStateChange||projection.loopSound)goto done;
  r->record=record;memcpy(r->controls,projection.controls,12);r->due=projection.nextThink;r->handler=handler;
 }
 ok=GscTurretFactoryFrameFence(now,current,context);
done:gscFactoryTurretBusy=0;return ok;
}
