/* Sole owner g_spawn.c via mapspawn. No saved/network struct changes. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_path_entity_host.h"
#include "g_cod1_actor.h"
#include "g_cod1_spawner.h"
typedef struct {
 GscEntityRef entity;uint32_t flags,nativeContents;int present,birth,kind,projected,zeroAngles;uint64_t spawnerIntake;
 entityShared_t shape;int model,type,spawnflags,takedamage;
 entityState_t modelState;unsigned modelEpoch;int modelPortFlags;char modelName[MAX_QPATH];
 void (*touch)(gentity_t*,gentity_t*,trace_t*);
 void (*use)(gentity_t*,gentity_t*,gentity_t*);
} C1PathEntityRecord;
static C1PathEntityRecord cod1PathEntities[MAX_GENTITIES];
static int Cod1PathProjectionActive(const GscEntityRef*,uint32_t);
static void Cod1PathNodeWorldReset(void);
static void Cod1LateDisconnectReset(void);
static void Cod1PathEntitiesReset(void){Cod1PathNodeWorldReset();Cod1LateDisconnectReset();memset(cod1PathEntities,0,sizeof(cod1PathEntities));}
/* Native map init1E2B6 clears the entity pool; free464CF clears each recycled
 * record. Ordinary G_Spawn45FA0 slots therefore enter field parsing with
 * flags134=0. The original field table8A200 does not write flags134.
 * Capture that provenance ONLY at our actual BSP allocation call, before
 * parsing. A classname query, runtime spawn or bare constructor is no proof.
 * birth: 1=allocation,2=typed dispatch,3=completed,4=invalid. */
static void Cod1PathEntityMapBirth(gentity_t*ent){
 GscEntityRef ref;C1PathEntityRecord*r;
 if(!g_cod1MapEntities||!level.spawning||!G_Cod1EntityCapture(ent,&ref)||
    ent->client||ent->r.linked||!ent->classname||strcmp(ent->classname,"noclass"))return;
 r=&cod1PathEntities[ref.slot];
 if((r->birth||r->present)&&r->entity.epoch==ref.epoch&&r->entity.generation==ref.generation){
  r->present=0;r->birth=4;return;
 }
 memset(r,0,sizeof(*r));r->entity=ref;r->birth=1;
}
static void Cod1PathEntityDispatch(gentity_t*ent){
 GscEntityRef ref;C1PathEntityRecord*r;
 if(!G_Cod1EntityCapture(ent,&ref))return;r=&cod1PathEntities[ref.slot];
 if(!r->birth||r->entity.epoch!=ref.epoch||r->entity.generation!=ref.generation)return;
 r->present=0;
 if(!g_cod1MapEntities||!level.spawning||r->birth!=1||!ent->classname){r->birth=4;return;}
 r->kind=!strcmp(ent->classname,"script_origin")?1:!strcmp(ent->classname,"trigger_multiple")?2:
  !strcmp(ent->classname,"trigger_use")?3:!strcmp(ent->classname,"trigger_lookat")?4:
  !strcmp(ent->classname,"trigger_damage")?5:!strcmp(ent->classname,"info_player_start")?6:!strcmp(ent->classname,"script_model")?7:
  (!strcmp(ent->classname,"item_health")||!strcmp(ent->classname,"item_health_small")||!strcmp(ent->classname,"item_health_large"))?8:
  (!strcmp(ent->classname,"weapon_m1carbine")||!strcmp(ent->classname,"weapon_thompson")||!strcmp(ent->classname,"weapon_springfield")||!strcmp(ent->classname,"weapon_fraggrenade"))?9:!strcmp(ent->classname,"misc_mg42")?10:(!strncmp(ent->classname,"actor_",6)&&(ent->spawnflags&1))?11:0;
 /* InitTrigger/G_SetMovedir can erase the parsed angle. A later zero is
  * not proof of the admitted native zero-angle brush contract. */
 r->zeroAngles=ent->s.angles[0]==0&&ent->s.angles[1]==0&&ent->s.angles[2]==0;
 r->birth=r->kind?2:4;
}
static C1PathEntityRecord*Cod1PathEntityPending(gentity_t*ent,const GscEntityRef*ref,int kind){
 C1PathEntityRecord*r;
 if(!g_cod1MapEntities||!level.spawning||!G_Cod1EntitySameAllocation(ref)||
    ent!=&g_entities[ref->slot]||ent->client||!ent->r.linked)return NULL;
 r=&cod1PathEntities[ref->slot];
 if(r->birth!=2||r->kind!=kind||r->entity.epoch!=ref->epoch||
    r->entity.generation!=ref->generation||r->present)return NULL;
 return r;
}
static void Cod1PathEntityOriginBorn(gentity_t*ent,const GscEntityRef*ref){
 C1PathEntityRecord*r=Cod1PathEntityPending(ent,ref,1);
 if(!r||!ent->classname||strcmp(ent->classname,"script_origin")||ent->r.bmodel||
    ent->s.eType!=ET_GENERAL||ent->s.modelindex||ent->r.contents)return;
 /* Native39640 OR8000 does not set either dynamic-path bit. */
 r->flags=0;r->present=1;r->birth=3;
}
/* These real typed constructors preserve allocation flags134 but their
 * native contents differ from the port's accepted interaction lane. Keep
 * physical contents unchanged until the scoped world transaction owns them.
 * No model/player/item/default-zero producer is admitted here. */
void G_Cod1PathTriggerProjectedBorn(gentity_t*ent,const GscEntityRef*ref,int kind){
 C1PathEntityRecord*r;int i;
 if(kind<3||kind>5)return;r=Cod1PathEntityPending(ent,ref,kind);
 if(!r||!r->zeroAngles||!ent->classname||strcmp(ent->classname,kind==3?"trigger_use":kind==4?"trigger_lookat":"trigger_damage")||
    !ent->r.bmodel||ent->s.modelindex<=0||ent->s.eType!=ET_INVISIBLE||
    ent->r.contents!=CONTENTS_TRIGGER||!(ent->r.svFlags&SVF_NOCLIENT)||ent->touch||
    (kind==3?!ent->use:!!ent->use)||
    (kind==5?(ent->spawnflags!=0&&ent->spawnflags!=3):ent->spawnflags!=0))return;
 for(i=0;i<3;i++)if(ent->r.currentAngles[i]!=0||!isfinite(ent->r.currentOrigin[i])||
    !isfinite(ent->r.mins[i])||!isfinite(ent->r.maxs[i])||ent->r.mins[i]>ent->r.maxs[i])return;
 r->flags=0;r->nativeContents=kind==3?0x00200000u:kind==4?0x20000000u:0x405c0008u;
 r->shape=ent->r;r->model=ent->s.modelindex;r->type=ent->s.eType;r->spawnflags=ent->spawnflags;
 r->takedamage=ent->takedamage;r->touch=ent->touch;r->use=ent->use;
 r->projected=1;r->present=1;r->birth=3;
}
/* The wall's original bomb_think moves its USE trigger while the timer runs.
 * Only the completed BSP constructor and its current allocation own the thread. */
int G_Cod1BombWallThreadCurrent(const GscEntityRef *ref, uint64_t serial) {
    const C1PathEntityRecord *record;
    const gentity_t *trigger;
    if (!ref || !serial || serial != ref->generation || !g_cod1MapEntities ||
        level.spawning || level.restarted || !G_Cod1EntitySameAllocation(ref)) return 0;
    record = &cod1PathEntities[ref->slot];
    trigger = &g_entities[ref->slot];
    return record->birth == 3 && record->present && record->kind == 3 &&
        record->projected && record->entity.epoch == ref->epoch &&
        record->entity.slot == ref->slot && record->entity.generation == ref->generation &&
        record->model == trigger->s.modelindex && record->use == trigger->use &&
        !trigger->client && G_Cod1TriggerUseEligible(trigger) &&
        trigger->targetname && !strcmp(trigger->targetname, "bomb_wall_trigger");
}
static C1PathEntityRecord*Cod1PathProjectionRecord(const GscEntityRef*ref){
 C1PathEntityRecord*r;gentity_t*e;entityShared_t shape;
 if(!G_Cod1EntitySameAllocation(ref))return NULL;r=&cod1PathEntities[ref->slot];e=&g_entities[ref->slot];
 if(!r->present||r->birth!=3||!r->projected||r->entity.epoch!=ref->epoch||
    r->entity.generation!=ref->generation||r->entity.slot!=ref->slot||e->client)return NULL;
 shape=e->r;shape.contents=r->shape.contents;
 if(memcmp(&shape,&r->shape,sizeof(shape))||e->s.modelindex!=r->model||e->s.eType!=r->type||
    e->spawnflags!=r->spawnflags||e->takedamage!=r->takedamage||e->touch!=r->touch||e->use!=r->use)return NULL;
 return r;
}
int G_Cod1TowerDamageBrushCurrent(const GscEntityRef *ref){
 C1PathEntityRecord *r=Cod1PathProjectionRecord(ref);
 const gentity_t *e;
 if(!r || r->kind!=5 || r->model!=2 || r->nativeContents!=0x405c0008u || r->shape.contents!=CONTENTS_TRIGGER)return 0;
 e=&g_entities[ref->slot];
 return e->r.linked && e->r.contents==r->shape.contents && e->s.modelindex==r->model &&
   e->classname && !strcmp(e->classname,"trigger_damage") && e->targetname && !strcmp(e->targetname,"moody_damage");
}
void G_Cod1PathTriggerMultipleBorn(gentity_t*ent,const GscEntityRef*ref){
 C1PathEntityRecord*r=Cod1PathEntityPending(ent,ref,2);
 if(!r||!ent->classname||strcmp(ent->classname,"trigger_multiple")||ent->spawnflags||
    !ent->r.bmodel||ent->s.modelindex<=0||ent->s.eType!=ET_INVISIBLE||
    ent->r.contents!=CONTENTS_TRIGGER)return;
 /* Native43A60/43890/43910 keep allocation flags134, set contents40000000
  * for this exact flags-zero branch. Trigger-use/lookat are not admitted. */
 r->flags=0;r->present=1;r->birth=3;
}
/* Original23A20/233D0 initial BSP turret: native C4=00200004,
 * flags134|8000 preserves zero birth path bits. Only normal point filtering;
 * no graph geometry or mounted/commanded lifetime authority is issued. */
static void Cod1PathEntityTurretBorn(gentity_t*e){
 GscEntityRef ref;C1PathEntityRecord*r;
 if(!G_Cod1EntityCapture(e,&ref))return;r=Cod1PathEntityPending(e,&ref,10);
 if(!r)return;r->flags=0;r->nativeContents=0x00200004u;r->present=1;r->birth=3;
}
static int Cod1PathEntityTurretCurrent(const GscEntityRef*ref){
 C1PathEntityRecord*r;
 if(!G_Cod1EntitySameAllocation(ref))return 0;r=cod1PathEntities+ref->slot;
 return r->kind==10&&r->present&&r->birth==3&&
  !memcmp(&r->entity,ref,sizeof(*ref))&&r->flags==0&&r->nativeContents==0x00200004u&&
  G_Cod1TurretInitialMaskCurrent(ref);
}
/* Unlike brush/trigger births this native point remains unlinked. Its
 * constructor AND delayed think preserve the allocation's flags134/C4.
 * The original position/ground transition is owned by g_client.c. */
int G_Cod1PathSpawnPointBorn(gentity_t*e,const GscEntityRef*ref){
 C1PathEntityRecord*r;
 if(!ref||!g_cod1MapEntities||!level.spawning||!G_Cod1EntitySameAllocation(ref)||
    e!=g_entities+ref->slot||!G_Cod1SpawnPointCurrent(ref))return 0;
 r=cod1PathEntities+ref->slot;
 if(r->birth!=2||r->kind!=6||r->entity.epoch!=ref->epoch||
    r->entity.generation!=ref->generation||r->present)return 0;
 r->flags=0;r->present=1;r->birth=3;return 1;
}
/* Actual original397F0 ->448F0/39640: native C4=2080 and flags|28000.
 * This owner proves the native filter input, not XModel collision geometry.
 * Only unchanged unit-scale BSP births enter; runtime edits retire eligibility. */
static void Cod1PathEntityModelBorn(gentity_t*e,const GscEntityRef*ref){
 C1PathEntityRecord*r=Cod1PathEntityPending(e,ref,7);int i;
 if(!r||!e->classname||strcmp(e->classname,"script_model")||!e->model||
    strlen(e->model)>=MAX_QPATH||e->s.modelindex<=0||e->s.modelindex>=MAX_MODELS||
    g_cod1ScriptModelCacheRetired||strcmp(g_cod1ScriptModelPaths[e->s.modelindex],e->model)||
    e->s.eType!=ET_GENERAL||e->s.density!=COD1_SCRIPT_MODEL_NET_MARKER||
    e->r.bmodel||e->r.contents||e->r.svFlags!=SVF_USE_CURRENT_ORIGIN||e->spawnflags||
    e->flags||e->takedamage||e->touch||e->use||e->think||e->tagParent)return;
 for(i=0;i<3;i++)if(!isfinite(e->r.currentOrigin[i])||!isfinite(e->r.currentAngles[i])||
    e->s.angles2[i]!=1)return;
 r->nativeContents=0x2080u;r->flags=0;r->model=e->s.modelindex;r->shape=e->r;r->modelState=e->s;
 r->modelEpoch=g_cod1ScriptModelCacheEpoch;r->modelPortFlags=e->flags;
 strcpy(r->modelName,e->model);r->present=1;r->birth=3;
}
static int Cod1PathEntityModelCurrent(const GscEntityRef*ref){
 C1PathEntityRecord*r;gentity_t*e;
 if(!G_Cod1EntitySameAllocation(ref))return 0;r=cod1PathEntities+ref->slot;e=g_entities+ref->slot;
 if(r->kind!=7||!r->present||r->birth!=3||r->entity.epoch!=ref->epoch||
    r->entity.generation!=ref->generation||r->model<=0||r->model>=MAX_MODELS||
    g_cod1ScriptModelCacheRetired||r->modelEpoch!=g_cod1ScriptModelCacheEpoch||
    strcmp(g_cod1ScriptModelPaths[r->model],r->modelName)||!e->model||strcmp(e->model,r->modelName)||
    !e->classname||strcmp(e->classname,"script_model")||e->client||e->tagParent||
    e->flags!=r->modelPortFlags||e->spawnflags||e->takedamage||e->touch||e->use||e->think||
    memcmp(&e->s,&r->modelState,sizeof(e->s))||memcmp(&e->r,&r->shape,sizeof(e->r)))return 0;
 return r->nativeContents==0x2080u;
}
#include "g_cod1_health_mask.inc"
static int Cod1PathEntityOverlap(const void*p,size_t n){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)cod1PathEntities;size_t z=sizeof(cod1PathEntities);
 return p&&n&&(a<=b?b-a<n:a-b<z);
}
int G_Cod1PathEntityRead(const GscEntityRef*input,uint32_t*out){
 GscEntityRef ref;const C1PathEntityRecord*r;cod1ActorPathView_t actor;
 if(!input||!out||Cod1PathEntityOverlap(out,sizeof(*out)))return 0;ref=*input;
 if(!G_Cod1EntitySameAllocation(&ref))return 0;r=&cod1PathEntities[ref.slot];
 if(!r->present||r->entity.epoch!=ref.epoch||r->entity.generation!=ref.generation||r->entity.slot!=ref.slot){
  if(!G_Cod1ActorPathRead(&ref,&actor))return 0;
  *out=actor.flags&0x3000u;return 1;
 }
 if(r->kind==10||r->kind==11)return 0; /* Initial turret proof is mask-specific only. */
 /* Two simultaneous native owners cannot be resolved by lookup priority. */
 if(G_Cod1ActorPathRead(&ref,&actor))return 0;
 if(r->kind==6&&!G_Cod1SpawnPointCurrent(&ref))return 0;
 if(r->kind==7&&!Cod1PathEntityModelCurrent(&ref))return 0;
 if((r->kind==8||r->kind==9)&&!Cod1HealthCurrent(&ref))return 0;
 if(r->projected&&(!Cod1PathProjectionRecord(&ref)||
    (g_entities[ref.slot].r.contents!=r->shape.contents&&
     !Cod1PathProjectionActive(&ref,(uint32_t)g_entities[ref.slot].r.contents))))return 0;
 *out=r->flags;return 1;
}
/* Original6B180 is a non-linked type12 spawner, native C4=0. Its
 * genuine BSP dispatch and current intake identity are independent of actor
 * leases. No classname/physical contents alone can authorize this lane. */
static void Cod1PathEntitySpawnerBorn(gentity_t*ent){
 GscEntityRef ref;C1PathEntityRecord*r;uint64_t intake;
 if(!g_cod1MapEntities||!level.spawning||!G_Cod1EntityCapture(ent,&ref)||
    !(intake=G_Cod1SpawnerIntakeIdentity(&ref)))return;
 r=&cod1PathEntities[ref.slot];
 if(r->birth!=2||r->kind!=11||r->present||r->entity.epoch!=ref.epoch||
    r->entity.generation!=ref.generation||r->entity.slot!=ref.slot)return;
 r->spawnerIntake=intake;r->flags=0;r->nativeContents=0;r->present=1;r->birth=3;
}
static int Cod1PathEntitySpawnerCurrent(const GscEntityRef*ref){
 const C1PathEntityRecord*r;const gentity_t*e;uint64_t actor=0;
 if(!ref||!G_Cod1EntitySameAllocation(ref)||level.spawning||level.restarted)return 0;
 r=&cod1PathEntities[ref->slot];e=&g_entities[ref->slot];
 return r->kind==11&&r->present&&r->birth==3&&r->spawnerIntake&&
  r->entity.epoch==ref->epoch&&r->entity.generation==ref->generation&&r->entity.slot==ref->slot&&
  G_Cod1SpawnerIntakeIdentity(ref)==r->spawnerIntake&&
  G_Cod1ActorPresenceRead(ref,&actor,NULL,0)&&!actor&&
  !e->think&&!e->touch&&!e->use&&!e->die&&!e->nextthink&&!e->scriptEvents&&!e->numScriptEvents;
}
static int Cod1PathEntityBrushBorn(gentity_t*ent,const GscEntityRef*ref,int32_t flags){
 C1PathEntityRecord*r;
 if(!g_cod1MapEntities||!G_Cod1EntitySameAllocation(ref)||ent!=&g_entities[ref->slot]||
    !ent->r.bmodel||!ent->r.linked||ent->s.eType!=ET_MOVER||ent->r.contents!=CONTENTS_SOLID)return 0;
 r=&cod1PathEntities[ref->slot];
 /* A repeated spawn on one allocation cannot silently change native bits. */
 if(r->present&&r->entity.epoch==ref->epoch&&r->entity.generation==ref->generation)return 0;
 memset(r,0,sizeof(*r));r->entity=*ref;r->flags=(flags&1)?0x3000u:0;r->present=1;return 1;
}
#include "g_cod1_path_world.inc"
#include "g_cod1_path_sight.inc"
#endif

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
/* Conservative normal-world gate, not the path bracket: do not silently
 * use accepted gameplay trigger masks where original native masks differ.
 * The missing full cohort remains an explicit refusal, including during a
 * graph's temporary zero/projection. No diagnostic/ready capability. */
static int Cod1PathQueryWorldReady(uint32_t mask,int noDynamic){
 int i;GscEntityRef ref;uint32_t flags;cod1ActorPathView_t a;
 if(!G_Cod1EntityMapEpoch()||!g_cod1MapEntities||cod1PathWorldPhase||
    level.num_entities<0||level.num_entities>MAX_GENTITIES)return 0;
 for(i=0;i<level.num_entities;i++)if(g_entities[i].inuse){
  C1PathEntityRecord*r=cod1PathEntities+i;gentity_t*e=g_entities+i;
  if(!G_Cod1EntityCapture(e,&ref))return 0;
  if(G_Cod1BodyReserveCurrent(&ref)||G_Cod1ScriptHelperCurrent(&ref)||Cod1PathEntityTurretCurrent(&ref)||Cod1PathEntitySpawnerCurrent(&ref))continue;
  if(!G_Cod1PathEntityRead(&ref,&flags))return 0;
  if(noDynamic&&(flags&0x1000u))return 0;
  if(G_Cod1ActorPathRead(&ref,&a)){if(!!a.linked!=!!e->r.linked)return 0;continue;}
  if(r->kind==8||r->kind==9){if((r->nativeContents|(uint32_t)e->r.contents)&mask)return 0;continue;}
  if(r->kind==6)continue; /* Current() proved the native unlinked point. */
  if(!e->r.linked)return 0;
  if(r->kind==7){
   /* Original global capsule callback44DFF0 tests C4&mask BEFORE geometry.
    * Native2080 and the accepted rendering-only physical0 both miss this
    * EXACT mask. Never use this as a general collision-body certificate. */
   if((r->nativeContents&mask)||e->r.contents)return 0;continue;
  }
  if(r->projected){
   if(!Cod1PathProjectionRecord(&ref))return 0;
   if((uint32_t)e->r.contents!=r->nativeContents&&
      (((uint32_t)e->r.contents|r->nativeContents)&mask))return 0;
  }
  else if(r->kind==1){if(e->r.contents)return 0;}
  else if(r->kind==2){if(e->r.contents!=CONTENTS_TRIGGER||!e->r.bmodel||e->spawnflags)return 0;}
  else if(e->r.contents!=CONTENTS_SOLID||!e->r.bmodel||e->s.eType!=ET_MOVER)return 0;
 }
 return 1;
}
int G_Cod1PathSpawnWorldReady(void){return Cod1PathQueryWorldReady(0x0281c011u,0);}
#endif

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
/* A synchronous normal trace has no contents mutations to roll back, but
 * its result must not cross a new allocation, lease, pose or map. Keep exact
 * bytes/identities, not a hash or a second boolean readiness observation. */
typedef struct {
 GscEntityRef ref;entityShared_t shared;entityState_t state;
 cod1ActorPathView_t actor;uint32_t flags;int active,isActor,isReserve,isHelper,isTurret,isSpawner;
} C1SpawnTraceRow;
static C1SpawnTraceRow cod1SpawnTraceRows[MAX_GENTITIES];
static int cod1SpawnTraceBusy;
static int Cod1PathQueryTrace(const cod1SpatialTraceQuery_t*input,trace_t*out,uint32_t mask,int noDynamic){
 cod1SpatialTraceQuery_t q,before;trace_t result;uint64_t epoch;
 int i,count,ok=0,status;uint32_t flags;cod1ActorPathView_t actor;
 if(!input||!out||input->contentsMask!=mask||cod1SpawnTraceBusy||!Cod1PathQueryWorldReady(mask,noDynamic))return 0;
 cod1SpawnTraceBusy=1;epoch=G_Cod1EntityMapEpoch();count=level.num_entities;
 memset(cod1SpawnTraceRows,0,sizeof(cod1SpawnTraceRows));
 for(i=0;i<count;i++){
  C1SpawnTraceRow*r=cod1SpawnTraceRows+i;gentity_t*e=g_entities+i;
  if(!(r->active=!!e->inuse))continue;
  if(!G_Cod1EntityCapture(e,&r->ref))goto done;
  r->isReserve=G_Cod1BodyReserveCurrent(&r->ref);
  r->isHelper=G_Cod1ScriptHelperCurrent(&r->ref);
  r->isTurret=Cod1PathEntityTurretCurrent(&r->ref);r->isSpawner=Cod1PathEntitySpawnerCurrent(&r->ref);
  if(!r->isReserve&&!r->isHelper&&!r->isTurret&&!r->isSpawner&&!G_Cod1PathEntityRead(&r->ref,&r->flags))goto done;
  r->shared=e->r;r->state=e->s;r->isActor=G_Cod1ActorPathRead(&r->ref,&r->actor);
 }
 q=*input;before=q;memset(&result,0,sizeof(result));status=trap_Cod1SpatialCapsuleTrace(&q,&result);
 if(status!=COD1_SPATIAL_OK||memcmp(&q,&before,sizeof(q))||epoch!=G_Cod1EntityMapEpoch()||
    count!=level.num_entities||!Cod1PathQueryWorldReady(mask,noDynamic))goto done;
 for(i=0;i<count;i++){
  C1SpawnTraceRow*r=cod1SpawnTraceRows+i;gentity_t*e=g_entities+i;
  if(r->active!=!!e->inuse)goto done;if(!r->active)continue;
  if(!G_Cod1EntitySameAllocation(&r->ref)||
     r->isReserve!=G_Cod1BodyReserveCurrent(&r->ref)||
     r->isHelper!=G_Cod1ScriptHelperCurrent(&r->ref)||
     r->isTurret!=Cod1PathEntityTurretCurrent(&r->ref)||
     r->isSpawner!=Cod1PathEntitySpawnerCurrent(&r->ref)||
     (!r->isReserve&&!r->isHelper&&!r->isTurret&&!r->isSpawner&&(!G_Cod1PathEntityRead(&r->ref,&flags)||flags!=r->flags))||
     memcmp(&r->shared,&e->r,sizeof(e->r))||memcmp(&r->state,&e->s,sizeof(e->s))||
     r->isActor!=G_Cod1ActorPathRead(&r->ref,&actor))goto done;
  if(r->isActor&&memcmp(&r->actor,&actor,sizeof(actor)))goto done;
 }
 *out=result;ok=1;
done:
 memset(cod1SpawnTraceRows,0,sizeof(cod1SpawnTraceRows));cod1SpawnTraceBusy=0;return ok;
}
int G_Cod1PathSpawnTrace(const cod1SpatialTraceQuery_t*input,trace_t*out){
 return Cod1PathQueryTrace(input,out,0x0281c011u,0);
}
/* Original6D9F0 uses trace33/00820011 with these two capsule hulls.
 * Only the no-DYNAMICPATH cohort is admitted: original6DBB0 changes no
 * contents there. Native mask exclusions are proven independently for this
 * mask. This synchronous query neither opens a world bracket nor publishes
 * a graph. A future graph owner must fence the entire construction too. */
int G_Cod1PathNodePlaceTrace(const cod1SpatialTraceQuery_t*input,trace_t*out){
 if(!input||input->passEntity[0]!=ENTITYNUM_NONE||input->passEntity[1]!=ENTITYNUM_NONE||
    input->mins[0]!=-15.0f||input->mins[1]!=-15.0f||input->mins[2]!=0.0f||
    input->maxs[0]!=15.0f||input->maxs[1]!=15.0f||
    (input->maxs[2]!=30.0f&&input->maxs[2]!=72.0f))return 0;
 return Cod1PathQueryTrace(input,out,0x00820011u,1);
}
/* An entire synchronous graph preparation must see ONE world, not a series
 * of separately valid queries. This receipt owns no contents mutation and
 * grants no dynamic-path/native graph authority. Any observed change retires
 * it permanently; restoring old bytes cannot revive it. */
static C1SpawnTraceRow cod1NodeWorldRows[MAX_GENTITIES];
static uint64_t cod1NodeWorldSerial,cod1NodeWorldReceipt,cod1NodeWorldEpoch;
static int cod1NodeWorldCount,cod1NodeWorldTime;
static void Cod1PathNodeWorldReset(void){
 cod1NodeWorldReceipt=0;cod1NodeWorldEpoch=0;cod1NodeWorldCount=0;
 memset(cod1NodeWorldRows,0,sizeof(cod1NodeWorldRows));
}
void G_Cod1PathNodeWorldEnd(uint64_t receipt){
 if(receipt&&receipt==cod1NodeWorldReceipt)Cod1PathNodeWorldReset();
}
int G_Cod1PathNodeWorldCurrent(uint64_t receipt){
 int i;uint32_t flags;cod1ActorPathView_t actor;
 if(!receipt||receipt!=cod1NodeWorldReceipt)return 0;
 if(cod1NodeWorldEpoch!=G_Cod1EntityMapEpoch()||cod1NodeWorldCount!=level.num_entities||
    cod1NodeWorldTime!=level.time||!Cod1PathQueryWorldReady(0x00820011u,1))goto stale;
 for(i=0;i<cod1NodeWorldCount;i++){
  C1SpawnTraceRow*r=cod1NodeWorldRows+i;gentity_t*e=g_entities+i;
  if(r->active!=!!e->inuse)goto stale;if(!r->active)continue;
  if(!G_Cod1EntitySameAllocation(&r->ref)||
     r->isReserve!=G_Cod1BodyReserveCurrent(&r->ref)||r->isHelper!=G_Cod1ScriptHelperCurrent(&r->ref)||
     r->isTurret!=Cod1PathEntityTurretCurrent(&r->ref)||r->isSpawner!=Cod1PathEntitySpawnerCurrent(&r->ref)||
     (!r->isReserve&&!r->isHelper&&!r->isTurret&&!r->isSpawner&&(!G_Cod1PathEntityRead(&r->ref,&flags)||flags!=r->flags))||
     memcmp(&r->shared,&e->r,sizeof(e->r))||memcmp(&r->state,&e->s,sizeof(e->s))||
     r->isActor!=G_Cod1ActorPathRead(&r->ref,&actor))goto stale;
  if(r->isActor&&memcmp(&r->actor,&actor,sizeof(actor)))goto stale;
 }
 return 1;
stale:Cod1PathNodeWorldReset();return 0;
}
int G_Cod1PathNodeWorldBegin(uint64_t*out){
 int i;
 if(!out||cod1NodeWorldReceipt||cod1SpawnTraceBusy||cod1NodeWorldSerial==UINT64_MAX||
    !Cod1PathQueryWorldReady(0x00820011u,1))return 0;
 cod1NodeWorldEpoch=G_Cod1EntityMapEpoch();cod1NodeWorldCount=level.num_entities;cod1NodeWorldTime=level.time;
 memset(cod1NodeWorldRows,0,sizeof(cod1NodeWorldRows));
 for(i=0;i<cod1NodeWorldCount;i++){
  C1SpawnTraceRow*r=cod1NodeWorldRows+i;gentity_t*e=g_entities+i;
  if(!(r->active=!!e->inuse))continue;
  if(!G_Cod1EntityCapture(e,&r->ref))goto fail;
  r->isReserve=G_Cod1BodyReserveCurrent(&r->ref);r->isHelper=G_Cod1ScriptHelperCurrent(&r->ref);
  r->isTurret=Cod1PathEntityTurretCurrent(&r->ref);r->isSpawner=Cod1PathEntitySpawnerCurrent(&r->ref);
  if(!r->isReserve&&!r->isHelper&&!r->isTurret&&!r->isSpawner&&!G_Cod1PathEntityRead(&r->ref,&r->flags))goto fail;
  r->shared=e->r;r->state=e->s;r->isActor=G_Cod1ActorPathRead(&r->ref,&r->actor);
 }
 cod1NodeWorldReceipt=++cod1NodeWorldSerial;
 if(!G_Cod1PathNodeWorldCurrent(cod1NodeWorldReceipt))return 0;
 *out=cod1NodeWorldReceipt;return 1;
fail:Cod1PathNodeWorldReset();return 0;
}
/* Separate typed path-flag proof, intentionally not G_Cod1PathEntityRead:
 * widening that general API would also widen geometry/bracket callers.
 * Original23A20/233D0 OR8000, deferred22630 and unmounted22960 preserve
 * path bits from proved map birth. Original6B180 preserves flags134 entirely.
 * Retained exact initial-current owners fence every admitted lifecycle. */
static int Cod1PathNodeOwnerFlags(const C1SpawnTraceRow*r,uint32_t*out){
 if(r->isTurret||r->isSpawner){
  const C1PathEntityRecord*p=cod1PathEntities+r->ref.slot;
  if(r->isTurret?r->isSpawner||!Cod1PathEntityTurretCurrent(&r->ref):!Cod1PathEntitySpawnerCurrent(&r->ref))return 0;
  if(p->flags||p->nativeContents!=(r->isTurret?0x00200004u:0u))return 0;
  *out=p->flags;return 1;
 }
 return G_Cod1PathEntityRead(&r->ref,out);
}
int G_Cod1PathNodeOwners(uint64_t receipt,cod1PathNodeOwners_t*out){
 cod1PathNodeOwners_t value;unsigned i;
 if(!out||Cod1PathEntityOverlap(out,sizeof(*out))||
    Cod1PathWorldOverlap(out,sizeof(*out),g_entities,sizeof(g_entities))||
    Cod1PathWorldOverlap(out,sizeof(*out),cod1NodeWorldRows,sizeof(cod1NodeWorldRows))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1NodeWorldReceipt,sizeof(cod1NodeWorldReceipt))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1NodeWorldSerial,sizeof(cod1NodeWorldSerial))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1NodeWorldEpoch,sizeof(cod1NodeWorldEpoch))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1NodeWorldCount,sizeof(cod1NodeWorldCount))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1NodeWorldTime,sizeof(cod1NodeWorldTime)))return 0;
 if(!G_Cod1PathNodeWorldCurrent(receipt))return 0;
 memset(&value,0,sizeof(value));value.count=(unsigned)cod1NodeWorldCount;value.time=cod1NodeWorldTime;
 for(i=0;i<value.count;i++){
  const C1SpawnTraceRow*r=cod1NodeWorldRows+i;cod1PathNodeOwner_t*p=value.rows+i;
  if(!r->active)continue;p->ref=r->ref;
  if(r->isReserve||r->isHelper){
   if(r->isReserve+r->isHelper!=1||r->isActor||r->isTurret||r->isSpawner)goto fail;
   p->adapter=1;continue;
  }
  if(!Cod1PathNodeOwnerFlags(r,&p->flags)||(p->flags&0x1000u))goto fail;
  p->active=1;
 }
 if(!G_Cod1PathNodeWorldCurrent(receipt))return 0;
 *out=value;return 1;
fail:Cod1PathNodeWorldReset();return 0;
}
#include "g_cod1_late_disconnect_world.inc"
#endif
