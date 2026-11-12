/* Sole owner g_main.c. RTCW script communication allocation, not a native
 * CoD entity. Only an exact unused, unlinked startup helper is accounted
 * for by the normal point trace cohort. No graph/actor/camera capability. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_actor.h"
typedef struct {
 uint64_t attemptedEpoch;GscEntityRef ref;gentity_t initial;int phase;
} C1ScriptHelper;
static C1ScriptHelper cod1ScriptHelper;
static void Cod1ScriptHelperBegin(void){
 uint64_t epoch=G_Cod1EntityMapEpoch();
 if(!epoch||!G_Cod1MapEntities()||level.spawning||
    cod1ScriptHelper.attemptedEpoch==epoch){cod1ScriptHelper.phase=-1;return;}
 memset(&cod1ScriptHelper,0,sizeof(cod1ScriptHelper));
 cod1ScriptHelper.attemptedEpoch=epoch;cod1ScriptHelper.phase=1;
}
static void Cod1ScriptHelperBorn(const GscEntityRef*ref){
 gentity_t*e;
 if(cod1ScriptHelper.phase!=1||!G_Cod1EntitySameAllocation(ref)||
    cod1ScriptHelper.attemptedEpoch!=ref->epoch)return;
 cod1ScriptHelper.phase=-1;e=g_entities+ref->slot;
 if(g_camEnt!=e||level.spawning||level.scriptEntity||!G_Cod1MapEntities()||
    e->client||e->r.linked||e->r.contents||e->r.bmodel||e->s.eType!=ET_CAMERA||
    e->r.svFlags!=SVF_NOCLIENT||!e->classname||strcmp(e->classname,"noclass")||
    !e->scriptName||strcmp(e->scriptName,"scriptcamera")||e->scriptEvents||e->numScriptEvents||
    e->scriptStatus.scriptEventIndex!=-1||e->scriptStatus.scriptFlags||
    e->think||e->touch||e->use||e->die||e->nextthink||e->takedamage||
    e->aiCharacter||e->physicsObject||e->s.modelindex||e->s.modelindex2)return;
 cod1ScriptHelper.ref=*ref;cod1ScriptHelper.initial=*e;cod1ScriptHelper.phase=2;
}
int G_Cod1ScriptHelperCurrent(const GscEntityRef*ref){
 gentity_t expected;uint64_t actor;char error[128];
 if(!ref||cod1ScriptHelper.phase!=2||!G_Cod1MapEntities()||level.spawning||level.scriptEntity||
    !G_Cod1EntitySameAllocation(ref)||ref->epoch!=cod1ScriptHelper.ref.epoch||
    ref->slot!=cod1ScriptHelper.ref.slot||ref->generation!=cod1ScriptHelper.ref.generation||
    g_camEnt!=g_entities+ref->slot)return 0;
 expected=cod1ScriptHelper.initial;
 /* Exact inactive status copy made by the ordinary G_RunThink scheduler. */
 if(!memcmp(&g_camEnt->scriptStatusCurrent,&expected.scriptStatus,sizeof(expected.scriptStatus)))
  expected.scriptStatusCurrent=expected.scriptStatus;
 if(memcmp(g_camEnt,&expected,sizeof(expected)))return 0;
 return G_Cod1ActorPresenceRead(ref,&actor,error,sizeof(error))&&!actor;
}
#endif
