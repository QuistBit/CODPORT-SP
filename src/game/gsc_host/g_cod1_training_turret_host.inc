#include "../g_cod1_navigation_probe.h"
int G_Cod1TrainingJumperPreparePath(const GscEntityRef*,int,int(*)(void*),void*);
int TrainingRigFrame(int,int(*)(const GscEntityRef*,const char*,const char*));
static int TrainingRigNote(const GscEntityRef*,const char*,const char*);
int TrainingJumperExitNode(const GscEntityRef *,uint64_t,const GscNodeRef *);
#include "../cod1_gsc/stance-profile.h"
#include "../g_cod1_actor_scripted_record.h"
#include "../g_cod1_actor_scripted_controller.h"
#include "../g_cod1_actor_scripted_frame.h"
#include "../g_cod1_actor_scripted_prefix.h"
#include "../g_cod1_entity_host.h"
#include "../g_cod1_path_entity_host.h"
#include "../g_cod1_actor_script_state.h"
#include "../g_cod1_actor_thread_authority.h"
#include "../cod1_gsc/animtime-profile.h"
/* R232 bounded original Avery route / MG42 service. Admission is separate
 * from availability: this file does not declare original training main ready. */
#include "../g_cod1_turret.h"
#include "../g_cod1_turret_mount.h"
#include "../g_cod1_actor_look.h"
#include "../g_cod1_actor_immediate.h"
#include "../g_cod1_actor_animation_motion.h"
#include "../g_cod1_weapon_model_cw.h"
extern const GscModelPartHost *G_Cod1ModelPartHost(void);
#include "../g_cod1_dropped_item.h"
#include "../g_cod1_actor_bootstrap.h"
typedef struct {
 GscEntityRef owner;
 cod1TrainingRoute_t route;
 int resolved,published,stopped;
 GscNodeRef diagnosticLastNode;uint64_t diagnosticAssignment;
} TrainingRunner;
static const struct {
 const char *name,*placedName,*first,*specialBody;
} trainingRunnerProfiles[]={
 {"Elder","Pvt. Elder","auto509","character_Elder"},
 {"Mantarro","Pvt. Mantarro","auto457",NULL},
 {"Davis","Pvt. Davis","auto504",NULL},
 {"Ambrose","Pvt. Ambrose","auto504",NULL},
 {"Lawrence","Pvt. Lawrence","auto501",NULL},
 {"Cline","Pvt. Cline","auto501",NULL},
 {"Murphy","Pvt. Murphy","auto457",NULL}
};
#define TRAINING_RUNNER_COUNT (sizeof(trainingRunnerProfiles)/sizeof(trainingRunnerProfiles[0]))

static struct {
 int programSelected,selected,developer,motion,dropSelected,observed,observedState,nativeThreads;
 uint64_t epoch,id,developerReceipt;
 GscEntityRef actor,gun;
 GscEntityRef instructor;
 int instructorResolved,instructorStopped;
 TrainingRunner runners[TRAINING_RUNNER_COUNT];
 GscActorControllerPrefixRun heldThink,moodyThink;
 cod1ActorLease_t deleteLease;uint64_t deleteReceipt;uint32_t deleteRow;int actorRetired;
 GscTurretHost realTurret;GscTurretAnimationHost realAnimation;GscActorAnimationHost realActor;GscActorLookHost realLook;GscSpatialNativeHost realSpatial;
} gscTestTurret;
static uint64_t gscTestTurretCertificateSerial;
/* Copy only the selected host; script roots load code, never execute/fake init. */
static const GscSessionActorBootstrapHost *GscTestTurretActorHost(const GscSessionActorBootstrapHost *input){
 const GscSessionActorBootstrapHost *base;char module[128];uint32_t i;uint64_t epoch=G_Cod1EntityMapEpoch();
 if(!gscTestTurret.selected||!epoch||gscTestTurret.epoch!=epoch||!input||
    !input->map_certificate.current||!input->map_certificate.receipt||
    input->map_certificate.current(input->map_certificate.receipt)!=1||
    !G_Cod1TurretScriptModule(module,sizeof(module))||
    epoch!=G_Cod1EntityMapEpoch()||input->map_certificate.current(input->map_certificate.receipt)!=1)return NULL;
 base=GscTestCarbineActorHost(input);if(!base||!base->module_count||base->module_count>GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY)return NULL;
 gscTestCarbineActors.methods=*G_Cod1ActorRuntimeMethodHost();
 for(i=0;i<base->module_count;i++){
  if(!memchr(base->module_roots[i],0,sizeof(base->module_roots[i])))return NULL;
  if(!strcmp(base->module_roots[i],module))return base;
 }
 if(base->module_count==GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY)return NULL;
 strcpy(gscTestCarbineActors.module_roots[gscTestCarbineActors.module_count++],module);
 return &gscTestCarbineActors;
}
static GscActorScriptedControllerHost foleyPrivateController;
static int FoleyPrivateInitReturn(void *user, const GscActorInitReturnRequest *q, char *error, size_t cap, int (*current)(void *), void *ctx) {
    const GscActorScriptedControllerHost *host = G_Cod1ActorScriptedControllerHost();
    int accepted;
    if(g_debugMove.integer)G_Printf("[Foley private] ordinary init GAME entry slot=%u gen=%u request=%d level=%d task=%llu\n", q->entity.slot, q->entity.generation, q->now_ms, level.time, (unsigned long long)q->task_serial);
    accepted = host->init_return(user, q, error, cap, current, ctx);
    if(!accepted||g_debugMove.integer)G_Printf("[Foley private] ordinary init GAME returned=%d\n", accepted);
    return accepted;
}
static const GscActorScriptedControllerHost *FoleyPrivateController(void) {
    foleyPrivateController = *G_Cod1ActorScriptedControllerHost();
    foleyPrivateController.init_return = FoleyPrivateInitReturn;
    return &foleyPrivateController;
}
static int GscTestTurretStorageProtected(const void*p,size_t n){
 return CAR_Overlap(p,n,&cod1CWWeaponModelStage,sizeof(cod1CWWeaponModelStage))||CAR_Overlap(p,n,&gscTestTurret,sizeof(gscTestTurret))||
  CAR_Overlap(p,n,&gscTestTurretCertificateSerial,sizeof(gscTestTurretCertificateSerial));
}
static void GscTestTurretReset(void){memset(&gscTestTurret,0,sizeof(gscTestTurret));}
static int GscTestTurretDeveloperCurrent(uint64_t receipt){
 return receipt&&receipt==gscTestTurret.developerReceipt&&gscTestTurret.selected&&
  gscTestTurret.epoch==G_Cod1EntityMapEpoch()&&!level.spawning&&!level.restarted&&
  trap_Cvar_VariableIntegerValue("g_cod1GscR232Turret")==1&&
  trap_Cvar_VariableIntegerValue("developer_script")==gscTestTurret.developer&&
  GscTestModuleReadCurrent();
}
/* VM clocks stay logical. Only a verified selected profile translates a copied request. */
static int GscTestTurretCurrent(void*);
typedef struct {const void*input;const void*copy;size_t bytes;int logical,actual;uint64_t epoch,id;int(*current)(void*);void*context;} GscTestTurretClock;
static int GscTestTurretBuffers(const void*input,size_t bytes,void*out,size_t outBytes,char*error,size_t cap){return input&&bytes&&!WeaponRegistrationPrivate(input,bytes)&&(!outBytes||(out&&!WeaponRegistrationPrivate(out,outBytes)))&&(!cap||(error&&!WeaponRegistrationPrivate(error,cap)))&&!CAR_Overlap(input,bytes,out,outBytes)&&!CAR_Overlap(input,bytes,error,cap)&&!CAR_Overlap(out,outBytes,error,cap);}
static int GscTestTurretClockValid(const GscTestTurretClock*t){return t&&t->current&&t->input&&t->copy&&t->logical>=0&&t->epoch==G_Cod1EntityMapEpoch()&&t->id==gscTestId&&t->actual==level.time&&!memcmp(t->input,t->copy,t->bytes)&&GscTestTurretCurrent(NULL)&&gsc_session_running_at(gscTestSession,t->id,t->logical,t->actual);}
static int GscTestTurretClockCurrent(void*opaque){GscTestTurretClock*t=opaque;return GscTestTurretClockValid(t)&&t->current(t->context)==1&&GscTestTurretClockValid(t);}
static void GscTestTurretClockSet(GscTestTurretClock*t,const void*input,const void*copy,size_t bytes,int logical,int(*current)(void*),void*ctx){memset(t,0,sizeof(*t));t->input=input;t->copy=copy;t->bytes=bytes;t->logical=logical;t->actual=level.time;t->epoch=G_Cod1EntityMapEpoch();t->id=gscTestId;t->current=current;t->context=ctx;}
#include "g_cod1_model_part_clock.inc"
static int GscTestTurretInvoke(void*u,const GscTurretRequest*input,GscTurretReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){GscTurretRequest source,q;GscTestTurretClock t;GscTurretHost host=gscTestTurret.realTurret;(void)u;if(!host.invoke||!GscTestTurretBuffers(input,sizeof(*input),out,sizeof(*out),error,cap))return 0;source=q=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockValid(&t))return 0;q.now_ms=t.actual;return host.invoke(host.user,&q,out,error,cap,GscTestTurretClockCurrent,&t);}
static int GscTestTurretDropInvoke(void*u,const GscActorDropRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){GscActorDropRequest source;GscTestTurretClock t;GscEntityRef result;float speed;(void)u;if(!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockCurrent(&t))return 0;
 if(!gscTestTurret.dropSelected){if(!G_Cod1DroppedItemsSelect(gscTestId,GscTestTurretCurrent,NULL))return 0;gscTestTurret.dropSelected=1;}
 if(!GscTestTurretClockCurrent(&t))return 0;memcpy(&speed,&source.speed_bits,4);memset(&result,0,sizeof(result));return G_Cod1ActorDropWeaponChecked(&source.entity,source.name,source.hand,speed,&result,error,cap,GscTestTurretClockCurrent,&t)&&GscTestTurretClockCurrent(&t);}
static int GscTestTurretAnimationInvoke(void*u,const GscTurretAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){GscTurretAnimationRequest source,q;GscTestTurretClock t;GscTurretAnimationHost host=gscTestTurret.realAnimation;(void)u;if(!host.invoke||!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=q=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockValid(&t))return 0;q.now_ms=t.actual;return host.invoke(host.user,&q,error,cap,GscTestTurretClockCurrent,&t);}
static int GscTestTurretActorInvoke(void*u,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){GscActorAnimationRequest source,q;GscTestTurretClock t;GscActorAnimationHost host=gscTestTurret.realActor;(void)u;if(!host.invoke||!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=q=*input;
 if(gscTestTurret.nativeThreads && (q.method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART ||
    (q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART &&
     (q.animation[0].ordinal==107||q.animation[0].ordinal==108||q.animation[0].ordinal==109||
      q.animation[0].ordinal==117||q.animation[0].ordinal==118)) ||
    (q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART && q.animation[0].ordinal==889 &&
     gscTestCarbine.moodyCharacter && !memcmp(&q.entity,&gscTestCarbine.moodyRef,sizeof(q.entity)))))
    host=*G_Cod1ActorScriptedAnimationHost();
 GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockValid(&t)){
   GscSessionSnapshot snap;
   if(gsc_session_snapshot(gscTestSession,&snap))
     G_Printf("[Foley private] GscTestTurretActorInvoke clock refused request=%d actual=%d logical=%d origin=%lld last=%lld state=%d\n",t.logical,t.actual,snap.logical_ms,(long long)snap.origin_host_ms,(long long)snap.last_host_ms,(int)snap.state);
   return 0;
 }q.now_ms=t.actual;{int ok=host.invoke(host.user,&q,error,cap,GscTestTurretClockCurrent,&t);if(!ok)G_Printf("[Foley private] GscTestTurretActorInvoke typed host refused request=%d actual=%d method=%u node=%u reason=%s\n",t.logical,t.actual,q.method,q.animation[0].ordinal,error?error:"");return ok;}}
static int GscTestTurretLookInvoke(void*u,const GscActorLookRequest*input,GscActorLookReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){GscActorLookRequest source,q;GscTestTurretClock t;GscActorLookHost host=gscTestTurret.realLook;(void)u;if(!host.invoke||!GscTestTurretBuffers(input,sizeof(*input),out,sizeof(*out),error,cap))return 0;source=q=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockValid(&t))return 0;q.now_ms=t.actual;return host.invoke(host.user,&q,out,error,cap,GscTestTurretClockCurrent,&t);}
static int GscTestTurretAnimationValidate(void*u,const GscEntityRef*input,uint64_t runtime,int now,char*error,size_t cap,int(*current)(void*),void*ctx){GscEntityRef source;GscTestTurretClock t;GscTurretAnimationHost host=gscTestTurret.realAnimation;(void)u;if(!host.validate||!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),now,current,ctx);if(!GscTestTurretClockValid(&t))return 0;return host.validate(host.user,input,runtime,t.actual,error,cap,GscTestTurretClockCurrent,&t);}
static int GscTestTurretActorValidate(void*u,const GscEntityRef*input,uint64_t runtime,int now,char*error,size_t cap,int(*current)(void*),void*ctx){GscEntityRef source;GscTestTurretClock t;GscActorAnimationHost host=gscTestTurret.realActor;(void)u;if(!host.validate||!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),now,current,ctx);if(!GscTestTurretClockValid(&t)){
   GscSessionSnapshot snap;
   if(gsc_session_snapshot(gscTestSession,&snap))
     G_Printf("[Foley private] GscTestTurretActorValidate clock refused request=%d actual=%d logical=%d origin=%lld last=%lld state=%d\n",t.logical,t.actual,snap.logical_ms,(long long)snap.origin_host_ms,(long long)snap.last_host_ms,(int)snap.state);
   return 0;
 }{int ok=host.validate(host.user,input,runtime,t.actual,error,cap,GscTestTurretClockCurrent,&t);if(!ok)G_Printf("[Foley private] GscTestTurretActorValidate typed host refused request=%d actual=%d\n",t.logical,t.actual);return ok;}}
static int GscTestTurretLookValidate(void*u,const GscEntityRef*input,uint64_t runtime,int now,char*error,size_t cap,int(*current)(void*),void*ctx){GscEntityRef source;GscTestTurretClock t;GscActorLookHost host=gscTestTurret.realLook;(void)u;if(!host.validate||!GscTestTurretBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;source=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),now,current,ctx);if(!GscTestTurretClockValid(&t))return 0;return host.validate(host.user,input,runtime,t.actual,error,cap,GscTestTurretClockCurrent,&t);}
static int GscTestTurretSpatialInvoke(void*u,const GscSpatialNativeRequest*input,GscSpatialNativeResult*out,char*error,size_t cap,int(*current)(void*),void*ctx){GscSpatialNativeRequest source,q;GscTestTurretClock t;GscSpatialNativeHost host=gscTestTurret.realSpatial;(void)u;if(!host.invoke||!GscTestTurretBuffers(input,sizeof(*input),out,sizeof(*out),error,cap))return 0;source=q=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);if(!GscTestTurretClockValid(&t))return 0;q.now_ms=t.actual;return host.invoke(host.user,&q,out,error,cap,GscTestTurretClockCurrent,&t);}
static const GscSpatialNativeHost*GscTestTurretSpatialHost(void){static const GscSpatialNativeHost host={NULL,GscTestTurretSpatialInvoke};gscTestTurret.realSpatial=*G_Cod1ActorSpatialNativeHost();return &host;}
/* Actual Avery aitype declares m1garand. Bind the complete current parsed CW
 * definition table, and require its real nonempty model rather than a fallback. */
static int GscTestTurretWeaponModels(char*error,size_t cap){const cwDef_t*definition=NULL;const char*model;char expected[128],actual[128];unsigned revision=CW_RegistryRevision();int i,n=CW_Count();size_t bytes;if(!revision||n<=0||n>CW_MAX_DEFS)return 0;for(i=0;i<n;i++){const cwDef_t*d=CW_Get(i);if(d&&!Q_stricmp(d->name,"m1garand")){if(definition)return 0;definition=d;}}if(!definition||!definition->hash||(model=CW_String(definition,"worldModel"))==NULL||(bytes=strlen(model))<8||bytes>=sizeof(expected)||strncmp(model,"xmodel/",7))return 0;memset(expected,0,sizeof(expected));memcpy(expected,model,bytes+1);if(G_Cod1QueryWeaponModel("m1garand",actual,error,cap)){if(revision!=CW_RegistryRevision()||strcmp(actual,expected))return 0;if(error&&cap)error[0]=0;return 1;}if(!G_Cod1SelectCWWeaponModels(error,cap)||!G_Cod1QueryWeaponModel("m1garand",actual,error,cap)||revision!=CW_RegistryRevision()||strcmp(actual,expected))return 0;if(error&&cap)error[0]=0;return 1;}
#include "../cod1_gsc/actor-float-profile.h"
#include "../cod1_gsc/move-delta-profile.h"
static int GscTestTurretSelect(const GscActorAnimationHost*actorAnimations,
 const GscActorLookHost*look,char*error,size_t size){
 GscSessionTurretAnimationSelection animations;GscDeveloperScriptSelection developer;GscSessionTurretProgramPlan plan={0};
 GscTurretHost turret={NULL,GscTestTurretInvoke};GscActorAnimationHost actor={NULL,GscTestTurretActorValidate,GscTestTurretActorInvoke};GscActorLookHost lookFacade={NULL,GscTestTurretLookValidate,GscTestTurretLookInvoke};
 const void*bytes[2];size_t sizes[2];int i,value;
 if(!gscTestTurret.selected||!gscTestSession||!actorAnimations||!look||
    gscTestTurretCertificateSerial==UINT64_MAX)return 0;
 value=trap_Cvar_VariableIntegerValue("developer_script");if(value!=0&&value!=1)return 0;
 memset(&animations,0,sizeof(animations));memset(&developer,0,sizeof(developer));
 gscTestTurret.programSelected=trap_Cvar_VariableIntegerValue("g_cod1GscActorPrefix")==1;
  gscTestTurret.nativeThreads=trap_Cvar_VariableIntegerValue("g_cod1GscNativeThreads");
  if((gscTestTurret.nativeThreads!=0&&gscTestTurret.nativeThreads!=1)||
     (gscTestTurret.nativeThreads&&!gscTestTurret.programSelected))return 0;
  {int placedState=trap_Cvar_VariableIntegerValue("g_cod1GscPlacedCurrentState");
   if((placedState!=0&&placedState!=1)||(placedState&&(!gscTestTurret.nativeThreads||trap_Cvar_VariableIntegerValue("g_cod1GscPlacedInit")!=1)))return 0;}
 {int cohort=trap_Cvar_VariableIntegerValue("g_cod1GscPlacedCohort");
  if((cohort!=0&&cohort!=1)||(cohort&&trap_Cvar_VariableIntegerValue("g_cod1GscPlacedCurrentState")!=1))return 0;}
 if(gscTestTurret.programSelected){
  if(!G_Cod1EarlyTurretProgramPlan(&plan)||plan.developer_enabled!=value)return 0;
 }else{
  if(!GscTestTurretWeaponModels(error,size)||!G_Cod1TurretAnimationCatalogs(bytes,sizes,&animations.map_certificate))return 0;
  for(i=0;i<2;i++){animations.catalogs[i].atr_bytes=bytes[i];animations.catalogs[i].atr_size=sizes[i];}
 }
 gscTestTurret.realTurret=*G_Cod1TurretControlHost();gscTestTurret.realAnimation=*G_Cod1TurretAnimationHost();gscTestTurret.realActor=*actorAnimations;gscTestTurret.realLook=*look;
 animations.host.user=NULL;animations.host.validate=GscTestTurretAnimationValidate;animations.host.invoke=GscTestTurretAnimationInvoke;
 gscTestTurret.developer=value;gscTestTurret.developerReceipt=++gscTestTurretCertificateSerial;
 developer.enabled=value;developer.certificate.receipt=gscTestTurret.developerReceipt;
 developer.certificate.current=GscTestTurretDeveloperCurrent;
 return (gscTestTurret.programSelected?plan.current(plan.user,plan.epoch,plan.receipt):GscTestTurretDeveloperCurrent(developer.certificate.receipt))&&
  gsc_session_select_r232_turrets(gscTestSession,&turret,error,size)&&
  (gscTestTurret.programSelected?gsc_session_select_turret_program(gscTestSession,&plan,error,size):gsc_session_select_r232_turret_animations(gscTestSession,&animations,error,size))&&
  (!gscTestTurret.programSelected || trap_Cvar_VariableIntegerValue("g_cod1GscTrainingRigs")!=1 ||
     (GscTestTrainingRigDeclarations(&plan,error,size)&&GscTestNamespaceStartOriginSelect(error,size)))&&
  gsc_session_select_r243_stance(gscTestSession,R243GameStanceHost(),error,size)&&
  gsc_session_select_r243_animtime(gscTestSession,R243GameAnimTimeHost(),error,size)&&
  (!gscTestTurret.programSelected || gsc_session_select_r239_mixed_scalar_equality(gscTestSession,error,size))&&
  gsc_session_select_r232_quoted_strings(gscTestSession,error,size)&&
  gsc_session_select_sparse_numeric_arrays(gscTestSession,error,size)&&
  gsc_session_select_r233_owned_numeric_delete(gscTestSession,error,size)&&
  gsc_session_select_r232_actor_animations(gscTestSession,&actor,error,size)&&
  (!gscTestTurret.nativeThreads||gsc_session_select_r400_source_animations(gscTestSession,G_Cod1ActorSourceAnimationHost(),error,size))&&
  gsc_session_select_r233_actor_animation_end(gscTestSession,error,size)&&
  gsc_session_select_r232_actor_look(gscTestSession,&lookFacade,error,size)&&
  (!gscTestTurret.nativeThreads||gsc_session_select_r402_source_look(gscTestSession,G_Cod1ActorSourceLookHost(),error,size))&&
  gsc_session_select_r233_actor_drop(gscTestSession,&(GscActorDropHost){NULL,GscTestTurretDropInvoke},error,size)&&
  gsc_session_select_r233_actor_presence(gscTestSession,G_Cod1ActorPresenceHost(),error,size)&&
  gsc_session_select_r232_weapon_model(gscTestSession,G_Cod1WeaponModelHost(),error,size)&&
  gsc_session_select_r232_randomfloat_range(gscTestSession,error,size)&&
  gsc_session_select_r232_source_float_arithmetic(gscTestSession,error,size)&&
  gsc_session_select_r232_function_admission(gscTestSession,error,size)&&
  gsc_session_select_r233_native_animation_roots(gscTestSession,error,size)&&
  gsc_session_select_r233_flagged_all_restart(gscTestSession,error,size)&&
  gsc_session_select_r243_flagged_all(gscTestSession,&gscTestTurret.realActor,error,size)&&
  gsc_session_select_r243_critical(gscTestSession,G_Cod1ActorCriticalSessionHost(),error,size)&&
  gsc_session_select_r243_script_state(gscTestSession,G_Cod1ActorScriptStateHost(),error,size)&&
  gsc_session_select_r233_namespace_animation_length(gscTestSession,G_Cod1LoadedAnimationLengthHost(),error,size)&&
  (!gscTestTurret.nativeThreads||
    (gsc_session_select_r238_array_value_cow(gscTestSession,error,size)&&
     gsc_session_select_r237_frame_buckets(gscTestSession,error,size)&&
     gsc_session_select_r243_thread_start(gscTestSession,error,size)&&
     (trap_Cvar_VariableIntegerValue("g_cod1GscPlacedCohort")!=1||
      gsc_session_select_configured_precache_interval(gscTestSession,error,size))&&
     gsc_session_select_r243_same_bucket_wait(gscTestSession,error,size)&&
     gsc_session_select_r243_thread_entities(gscTestSession,G_Cod1ScriptThreadAuthorityHost(),error,size)&&
     gsc_session_select_model_part(gscTestSession,&(GscModelPartHost){NULL,GscTestTurretModelPart},error,size)&&
     gsc_session_select_weapon_classname(gscTestSession,G_Cod1WeaponClassnameHost(),error,size)&&
     gsc_session_select_entity_links(gscTestSession,G_Cod1EntityLinkHost(),error,size)&&
     gsc_session_select_r243_scripted_init_dependency(gscTestSession,error,size)&&
     gsc_session_select_r243_scripted_prefix(gscTestSession,G_Cod1ActorScriptedPrefixHost(),error,size)&&
     gsc_session_select_r243_scripted_start(gscTestSession,G_Cod1ActorScriptedStartHost(),error,size)&&
     gsc_session_select_r243_controller_with_cleanup(gscTestSession,FoleyPrivateController(),G_Cod1ActorSelectWithCleanup,error,size)))&&
  (gscTestTurret.programSelected||gsc_session_select_r232_developer_script(gscTestSession,&developer,error,size));
}
/* Early preparation can bind the identities, but constructors and teardown
 * always use the individual, fully guarded validation path. */
static int GscTestReadOnlyFrameActive(void){
 return !gscTestPreparingEarly&&!gscTestStopRequested&&!level.spawning&&!level.restarted&&
    gscTestTurret.selected&&gscTestTurret.id==gscTestId&&
    gscTestTurret.epoch==G_Cod1EntityMapEpoch()&&gsc_session_is_running(gscTestSession,gscTestId);
}
static int GscTestTurretAdmit(char*error,size_t size,GscReadOnlyContext *context){
 GscSessionTurretProgramAdmission a={0};const void*bytes[2];size_t sizes[2];int i;
 if(!gscTestTurret.programSelected)return 1;
 if(!GscTestTurretWeaponModels(error,size)||!G_Cod1TurretAnimationCatalogs(bytes,sizes,&a.animations.map_certificate))return 0;
 a.epoch=gscTestTurret.epoch;for(i=0;i<2;i++){a.animations.catalogs[i].atr_bytes=bytes[i];a.animations.catalogs[i].atr_size=sizes[i];}
 a.animations.host.validate=GscTestTurretAnimationValidate;a.animations.host.invoke=GscTestTurretAnimationInvoke;
 a.developer.enabled=gscTestTurret.developer;a.developer.certificate.receipt=gscTestTurret.developerReceipt;a.developer.certificate.current=GscTestTurretDeveloperCurrent;
 if (!gsc_session_admit_turret_program(gscTestSession,gscTestId,&a,error,size)) return 0;
 if(trap_Cvar_VariableIntegerValue("g_cod1GscTrainingRigs")==1 && !GscTestAnimationNamespaceResources(error,size,1))return 0;
 context->animation=a.animations.map_certificate;
 context->developer=a.developer.certificate;
 return 1;
}
static int GscTestTurretCurrent(void*unused){(void)unused;
 return gscTestTurret.selected&&gscTestTurret.id==gscTestId&&
  GscTestTurretDeveloperCurrent(gscTestTurret.developerReceipt)&&GscTestCarbineCurrent(NULL);
}
static int GscTestTurretNotify(void*unused,const GscEntityRef*entity,const char*event){
 int awakened=0;char error[GSC_SESSION_ERROR_CAP];(void)unused;
 return GscTestTurretCurrent(NULL)&&
  gsc_session_notify_entity(gscTestSession,gscTestId,entity,event,&awakened,error,sizeof(error));
}
static int GscTestTurretAnimationEnd(void*unused,const cod1ActorAnimationNotify_t*input){
 GscActorAnimationEnd packet;int awakened=0;char error[GSC_SESSION_ERROR_CAP];(void)unused;
 if(!input||!GscTestTurretCurrent(NULL))return 0;memset(&packet,0,sizeof(packet));
 packet.entity=input->entity;packet.runtime=input->runtime;packet.catalog=input->catalog;packet.owner_identity=input->owner_identity;packet.generation=input->generation;packet.ordinal=input->ordinal;packet.produced_ms=input->produced_ms;memcpy(packet.event,input->event,sizeof(packet.event));memcpy(packet.payload,input->payload,sizeof(packet.payload));
 return gsc_session_notify_actor_animation_end(gscTestSession,gscTestId,&packet,&awakened,error,sizeof(error));
}
static int GscTestTurretGoal(void*unused,const GscEntityRef*entity){
 return GscTestTurretNotify(unused,entity,"goal");
}
static int GscTestTurretStand(void*unused,const GscEntityRef*actor,const GscEntityRef*gun,
 const char*module,const char*function){
 GscEntityScriptEntry request;GscEntityScriptRun result;cod1ActorTurretMount_t mount;GscEntityRef actorValue,gunValue;uint64_t mountSerial;int now;
 char error[GSC_SESSION_ERROR_CAP];(void)unused;
 if(!GscTestTurretCurrent(NULL)||!actor||!gun||!module||!function||
    strcmp(module,"animscripts/mg42/stand")||strcmp(function,"main")||
    !G_Cod1TurretMountRead(actor,&mount)||!mount.mounted||
    memcmp(&mount.occupiedGun,gun,sizeof(*gun)))return 0;
 actorValue=*actor;gunValue=*gun;mountSerial=mount.serial;now=level.time;
 memset(&request,0,sizeof(request));request.entity=actorValue;
 strcpy(request.module,module);strcpy(request.function,function);
 if(!G_Cod1ActorAnimationMotionScriptBegin(&actorValue,now,0,GscTestTurretCurrent,NULL)||
    now!=level.time||!GscTestTurretCurrent(NULL)||memcmp(actor,&actorValue,sizeof(actorValue))||memcmp(gun,&gunValue,sizeof(gunValue))||
    !G_Cod1TurretMountRead(&actorValue,&mount)||!mount.mounted||mount.serial!=mountSerial||
    mount.currentTop!=0||mount.currentStack[0]!=3||memcmp(&mount.occupiedGun,&gunValue,sizeof(gunValue)))return 0;
 return gsc_session_execute_r232_turret_entry(gscTestSession,gscTestId,&request,&result,error,sizeof(error));
}
/* Only an authoritative original-prefix tombstone closes actor pumping.
 * Keep the old key and profile; never scan/adopt a replacement allocation. */
static int GscTestTurretActorRetired(void){
 cod1ActorDeleteSnapshot_t d;cod1ActorBootstrapSnapshot_t p;uint32_t row=gscTestTurret.deleteRow;
 if(!gscTestTurret.deleteReceipt||!gscTestCarbineActors.map_certificate.current||gscTestCarbineActors.map_certificate.receipt!=gscTestTurret.deleteReceipt||gscTestCarbineActors.map_certificate.current(gscTestTurret.deleteReceipt)!=1||!G_Cod1ActorBootstrapDeleteRead(&d,NULL,0)||!G_Cod1ActorBootstrapRead(&p,NULL,0)||
  !p.enabled||!p.declarationsReady||!p.sealed||!p.selected||!p.runComplete||p.closed||p.invalid||d.count>COD1_ACTOR_CAPACITY||p.placedRows>COD1_ACTOR_CAPACITY||
  d.epoch!=gscTestTurret.epoch||p.epoch!=d.epoch||d.receipt!=gscTestTurret.deleteReceipt||p.receipt!=d.receipt||
  row>=d.count||row>=p.placedRows||d.rows[row].state!=COD1_ACTOR_DELETE_RETIRED||!d.rows[row].transaction||!p.rows[row].retired||
  memcmp(&d.rows[row].entity,&gscTestTurret.actor,sizeof(GscEntityRef))||memcmp(&p.rows[row].entity,&gscTestTurret.actor,sizeof(GscEntityRef))||
  memcmp(&d.rows[row].lease,&gscTestTurret.deleteLease,sizeof(cod1ActorLease_t))||memcmp(&p.rows[row].lease,&gscTestTurret.deleteLease,sizeof(cod1ActorLease_t)))return 0;
 return 1;
}
static int GscTestTurretDeleteBind(void){
 cod1ActorBootstrapSnapshot_t p;cod1ActorLease_t lease;uint32_t i;
 if(!gscTestTurret.deleteReceipt){
  if(!G_Cod1ActorBootstrapRead(&p,NULL,0)||p.epoch!=gscTestTurret.epoch||!G_Cod1ActorBootstrapDeleteCandidate(&gscTestTurret.actor,&lease))return 0;
  for(i=0;i<p.placedRows&&i<COD1_ACTOR_CAPACITY;i++)if(!memcmp(&p.rows[i].entity,&gscTestTurret.actor,sizeof(GscEntityRef)))break;
  if(i>=p.placedRows||i>=COD1_ACTOR_CAPACITY||memcmp(&p.rows[i].lease,&lease,sizeof(lease))||!G_Cod1EntitySelectRuntimeActorDeleteOwner(p.epoch,p.receipt,&lease))return 0;
  gscTestTurret.deleteReceipt=p.receipt;gscTestTurret.deleteLease=lease;gscTestTurret.deleteRow=i;return 1;
 }
 /* Immutable once-only selection; full current authority is checked at delete. */
 return 1;
}
/* The script obtains the placed instructor by targetname. Keep that exact
 * allocation for this map, including after retirement or slot reuse. */
int FoleyPrivateDispatch(const GscEntityRef*,int,uint64_t,int(*)(void*),void*);
static int GscTestCourseActorFrame(const GscEntityRef *ref,int now,int(*current)(void*),void *ctx){
 GscR237FrameView view={0};GscSessionTiming snap={0};
 int v,m;
 if(!gscTestTurret.nativeThreads)return G_Cod1ActorGenericFrame(ref,now,current,ctx);
 v=gsc_session_r237_frame_view(gscTestSession,gscTestId,&view);m=gsc_session_timing(gscTestSession,&snap);
 if(g_debugMove.integer&&now<=250)G_Printf("[Foley private] frame clocks slot=%u level=%d origin=%lld last=%lld logical=%d bucket=%d open=%d gen=%llu snapshot=%d view=%d\n",ref->slot,now,(long long)snap.origin_host_ms,(long long)snap.last_host_ms,snap.logical_ms,view.host_now_ms,view.frame_open,(unsigned long long)view.frame_generation,m,v);
 if(!v||!m||snap.session_id!=gscTestId||snap.busy||snap.has_error||view.frame_open||!view.frame_generation||
    snap.origin_host_ms<0||snap.last_host_ms!=now||now<snap.origin_host_ms||
    now-snap.origin_host_ms!=view.host_now_ms||snap.logical_ms!=view.host_now_ms)return 0;
 return FoleyPrivateDispatch(ref,now,view.frame_generation,current,ctx);
}
#include "g_cod1_actor_face_frame.h"
int FoleyFrameNoteCurrent(const cod1ActorFaceNote_t *);
static int TrainingRunnerIndex(const GscEntityRef *actor){
 unsigned i;
 if(!actor || !actor->epoch || actor->epoch!=gscTestTurret.epoch)return -1;
 for(i=0;i<TRAINING_RUNNER_COUNT;i++){
    const TrainingRunner *r=&gscTestTurret.runners[i];
    if(r->resolved && r->published && !r->stopped && !memcmp(actor,&r->owner,sizeof(*actor)) &&
       G_Cod1EntitySameAllocation(actor)){
       const gentity_t *e=&g_entities[actor->slot];
       if(e->target && !strcmp(e->target,trainingRunnerProfiles[i].first))return (int)i;
    }
 }
 return -1;
}
const cod1TrainingRoute_t *TrainingCurrentRoute(const GscEntityRef *actor){
 int i=TrainingRunnerIndex(actor);
 return i<0?NULL:&gscTestTurret.runners[i].route;
}
/* The BSP identifies shared goals. Each route belongs to a current actor
 * allocation and was validated against the same immutable node catalog. */
int G_Cod1RunnerSharesNode(const GscEntityRef *actor,unsigned assignment){
 const cod1TrainingRoute_t *route=TrainingCurrentRoute(actor);
 unsigned i;
 if(!route || !assignment || assignment>COD1_TRAINING_ROUTE_LENGTH)return 0;
 for(i=0;i<TRAINING_RUNNER_COUNT;i++){
    const TrainingRunner *runner=&gscTestTurret.runners[i];
    if(&runner->route!=route && runner->route.epoch==route->epoch &&
       runner->route.index[assignment-1]==route->index[assignment-1] &&
       TrainingRunnerIndex(&runner->owner)==(int)i)return 1;
 }
 return 0;
}
const char *G_Cod1RunnerName(const GscEntityRef *actor){
 int i=TrainingRunnerIndex(actor);
 return i<0?"unbound":trainingRunnerProfiles[i].name;
}
/* Published stationary runners do not enter CONTENTS_BODY until their first
 * movement assignment. Respect their actual linked hull during that window. */
int G_Cod1WaitingRunnerContact(const GscEntityRef *actor,const float at[3]){
 unsigned i;
 vec3_t mins,maxs;
 int moving=TrainingRunnerIndex(actor);
 if(moving<0)return -1;
 VectorAdd(at,g_entities[actor->slot].r.mins,mins);
 VectorAdd(at,g_entities[actor->slot].r.maxs,maxs);
 for(i=0;i<TRAINING_RUNNER_COUNT;i++){
    const TrainingRunner *peer=&gscTestTurret.runners[i];
    const gentity_t *body;
    if(i==(unsigned)moving || !peer->published || peer->stopped ||
       TrainingRunnerIndex(&peer->owner)!=(int)i)continue;
    body=&g_entities[peer->owner.slot];
    if(!body->r.linked || body->r.contents)continue;
    if(trap_EntityContactCapsule(mins,maxs,body))return (int)peer->owner.slot;
 }
 return -1;
}
int TrainingElderCurrent(const GscEntityRef *actor){
 /* Only Elder owns the greeting voice/face notetrack. */
 return TrainingRunnerIndex(actor)==0 && GscTestTurretCurrent(NULL);
}
int TrainingJumperRead(GscEntityRef *);
static int TrainingIsJumper(const GscEntityRef *a){
 GscEntityRef j;return TrainingJumperRead(&j)&&!memcmp(&j,a,sizeof(j));
}
static int TrainingNoteOwner(const GscEntityRef *actor){
 return (gscTestCarbine.moodyCharacter && actor && actor->epoch==gscTestCarbine.epoch &&
    !memcmp(actor,&gscTestCarbine.moodyRef,sizeof(*actor)) && G_Cod1EntitySameAllocation(actor)) ||
    TrainingIsJumper(actor) || TrainingRunnerIndex(actor)>=0 || (gscTestTurret.instructorResolved &&
    !gscTestTurret.instructorStopped && !memcmp(actor,&gscTestTurret.instructor,sizeof(*actor)));
}
int FoleySessionQueueFrameNote(const cod1ActorFaceNote_t *note){
 GscEventValue value={0};GscR237FrameView view;GscSessionSnapshot snap;
 int awakened=-1,ok;char error[GSC_SESSION_ERROR_CAP]={0};
 if(!FoleyFrameNoteCurrent(note)||!TrainingNoteOwner(&note->entity)||!GscTestTurretCurrent(NULL)||
    !gsc_session_snapshot(gscTestSession,&snap)||snap.session_id!=gscTestId||snap.busy||snap.error[0]||
    snap.last_host_ms!=note->produced_ms||!gsc_session_r237_frame_view(gscTestSession,gscTestId,&view)||
    !view.frame_open||view.frame_generation!=note->frame_generation||!note->event[0]||
    !memchr(note->event,0,sizeof(note->event))||!memchr(note->payload,0,sizeof(note->payload)))return 0;
 value.kind=GSC_EVENT_STRING;memcpy(value.string,note->payload,sizeof(note->payload));
 ok=gsc_session_notify_entity_with_payload(gscTestSession,gscTestId,&note->entity,note->event,&value,1,&awakened,error,sizeof(error));
 if(!ok||awakened<0||error[0]||g_debugMove.integer)G_Printf("[Foley note] owner=%u/%u tick=%d ordinal=%u event=%s payload=%s accepted=%d woken=%d error=%s\n",
    note->entity.slot,note->entity.generation,note->produced_ms,note->ordinal,note->event,note->payload,ok,awakened,error);
 return ok&&awakened>=0&&FoleyFrameNoteCurrent(note)&&GscTestTurretCurrent(NULL);
}
/* close_door can delete this instructor inside the resumed GSC bucket.
 * Only the committed original row can discharge the pre-VM actor update. */
int FoleySameBucketRetirement(const GscEntityRef *actor,const cod1ActorLease_t *lease){
 cod1ActorDeleteSnapshot_t d;
 cod1ActorBootstrapSnapshot_t p;
 uint32_t row;
 if(!lease||memcmp(actor,&lease->entity,sizeof(*actor))||
    !G_Cod1EntityDeleteRetiredIdentity(actor,lease))return 0;
 if(G_Cod1TrainingJumperDeleteCommitted(actor,lease))return 1;
 if(!G_Cod1ActorBootstrapDeleteRead(&d,NULL,0)||!G_Cod1ActorBootstrapRead(&p,NULL,0)||
    !p.enabled||!p.declarationsReady||!p.sealed||!p.selected||!p.runComplete||p.closed||p.invalid||
    p.epoch!=actor->epoch||d.epoch!=p.epoch||!p.receipt||p.receipt!=d.receipt||
    d.count!=p.placedRows||d.count>COD1_ACTOR_CAPACITY||
    !gscTestCarbineActors.map_certificate.current||
    gscTestCarbineActors.map_certificate.receipt!=p.receipt||
    gscTestCarbineActors.map_certificate.current(p.receipt)!=1)return 0;
 for(row=0;row<d.count;row++){
    if(memcmp(&p.rows[row].entity,actor,sizeof(*actor)))continue;
    return p.rows[row].retired&&d.rows[row].state==COD1_ACTOR_DELETE_RETIRED&&
       d.rows[row].transaction&&!memcmp(&d.rows[row].entity,actor,sizeof(*actor))&&
       !memcmp(&p.rows[row].lease,lease,sizeof(*lease))&&!memcmp(&d.rows[row].lease,lease,sizeof(*lease));
 }
 return 0;
}
int FoleyRunNoteBucket(const GscEntityRef *actor,int now,uint64_t generation,
                       const cod1ActorLease_t *retirementLease,int *retired){
 GscR237FrameView view;char error[GSC_SESSION_ERROR_CAP]={0};int ok;
 if(!retired)return 0;
 *retired=0;
 if(!actor||now!=level.time||!GscTestTurretCurrent(NULL)||
    !TrainingNoteOwner(actor)||!G_Cod1EntitySameAllocation(actor)||
    !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view)||!view.frame_open||view.frame_generation!=generation)return 0;
 ok=gsc_session_r237_run_current_bucket(gscTestSession,gscTestId,generation,error,sizeof(error));
 if(!ok||error[0]||g_debugMove.integer)G_Printf("[Foley note bucket] owner=%u/%u tick=%d result=%d error=%s\n",actor->slot,actor->generation,now,ok,error);
 if(!ok||now!=level.time||!GscTestTurretCurrent(NULL)||
    !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view)||!view.frame_open||
    view.frame_generation!=generation)return 0;
 if(G_Cod1EntitySameAllocation(actor))return 1;
 if(!FoleySameBucketRetirement(actor,retirementLease))return 0;
 *retired=1;
 G_Printf("[Foley note bucket] committed script retirement owner=%u/%u tick=%d frame=%llu\n",
    actor->slot,actor->generation,now,(unsigned long long)generation);
 return 1;
}
int FoleyPrivateQueuedController(const GscEntityRef*,int,uint64_t,uint64_t*);
#include "../state9_result.h"
int FoleyReturned4PoseCurrent(const GscEntityRef*,uint64_t,uint64_t,int,uint64_t);
int FoleyReturned4RepeatQueued(const GscEntityRef*,uint64_t,uint64_t,int,uint64_t);
int FoleyReturnedBodyReleased(const GscEntityRef*,uint64_t,uint64_t);
/* Snapshot the genuine returned selection before asking for another optional
 * course gesture. The native call still performs its own full admission. */
static int FoleyReadReleasedBody(const GscEntityRef *ref,GscStringCastValue *out,char *error,size_t size){
 const GscActorControllerPrefixRun *held=&gscTestTurret.heldThink;
 GscStringCastValue value={0};
 if(!GscTestTurretBuffers(ref,sizeof(*ref),out,sizeof(*out),error,size)||
    !GscTestTurretCurrent(NULL)||!gscTestTurret.instructorResolved||gscTestTurret.instructorStopped||
    memcmp(ref,&gscTestTurret.instructor,sizeof(*ref))||!G_Cod1EntitySameAllocation(ref))return 0;
 value.type=5;
 value.integer=held->main_receipt && held->task_serial && !held->record_present &&
    held->frontier==GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD && held->main_status==1 &&
    !memcmp(ref,&held->entity,sizeof(*ref)) && FoleyReturnedBodyReleased(ref,held->actor_serial,held->selection_serial);
 *out=value;
 return 1;
}
static int TrainingSendTowerTouch(const GscEntityRef *actor,uint64_t serial,uint64_t frame,int now){
    GscEntityRef brush;
    GscEventValue payload={0};
    char error[GSC_SESSION_ERROR_CAP]={0};
    int brushWake=0,actorWake=0;
    if(!gscTestCarbine.moodyCharacter || memcmp(actor,&gscTestCarbine.moodyRef,sizeof(*actor)) ||
       !GscTestTurretCurrent(NULL) || !G_Cod1TakeTowerTouch(actor,serial,frame,now,&brush))return 0;
    payload.kind=GSC_EVENT_ENTITY;payload.entity=*actor;
    if(!gsc_session_notify_entity_with_payload(gscTestSession,gscTestId,&brush,"touch",&payload,1,&brushWake,error,sizeof(error)) ||
       !GscTestTurretCurrent(NULL) || !G_Cod1TowerDamageBrushCurrent(&brush) || !G_Cod1EntitySameAllocation(actor))return 0;
    payload.entity=brush;
    if(!gsc_session_notify_entity_with_payload(gscTestSession,gscTestId,actor,"touch",&payload,1,&actorWake,error,sizeof(error)) ||
       !GscTestTurretCurrent(NULL) || !G_Cod1TowerDamageBrushCurrent(&brush) || !G_Cod1EntitySameAllocation(actor))return 0;
    if(g_debugMove.integer)G_Printf("[Training tower touch] tick=%d frame=%llu brush=%u/%u actor=%u/%u wakes=%d,%d\n",
        now,(unsigned long long)frame,brush.slot,brush.generation,actor->slot,actor->generation,brushWake,actorWake);
    return 1;
}

static int TrainingControllerAtClosedFrame(const GscEntityRef *ref,int now,GscActorControllerPrefixRun *receipt) {
 GscR237FrameView frame;
 GscActorControllerPrefixRun run={0};
 uint64_t serial=0;
 char error[GSC_SESSION_ERROR_CAP]={0};
 int queued,ok;
 if(!gsc_session_r237_frame_view(gscTestSession,gscTestId,&frame) || frame.frame_open || !frame.frame_generation)return 0;
 if(receipt->main_receipt){
    const GscActorControllerPrefixRun *held=receipt;
    if(memcmp(&held->entity,ref,sizeof(*ref))||!GscTestTurretCurrent(NULL))return 0;
    if(FoleyReturned4PoseCurrent(ref,held->actor_serial,held->selection_serial,now,frame.frame_generation))return 1;
    if(!FoleyReturned4RepeatQueued(ref,held->actor_serial,held->selection_serial,now,frame.frame_generation))return 0;
    serial=held->actor_serial;
    G_Printf("[repeat entry] actual queued request frame=%llu selection=%llu main=%llu task=%llu now=%d\n",
      (unsigned long long)frame.frame_generation,(unsigned long long)held->selection_serial,
      (unsigned long long)held->main_receipt,(unsigned long long)held->task_serial,now);
 }else{
   queued=FoleyPrivateQueuedController(ref,now,frame.frame_generation,&serial);
   if(queued<=0)return queued==0;
 }
 ok=gsc_session_r243_controller_prefix(gscTestSession,gscTestId,ref,serial,frame.frame_generation,&run,error,sizeof(error));
 if(!ok)G_Printf("[400 controller refusal] owner=%u tick=%d error=%s\n",ref->slot,now,error);
 if(TrainingIsJumper(ref)&&(!ok||(g_debugMove.integer&&(now<1400||now%500==0))))
   G_Printf("[Training jumper controller] tick=%d ok=%d frontier=%d present=%d status=%d selection=%llu error=%s\n",
    now,ok,run.frontier,run.record_present,run.main_status,run.selection_serial,error);

 if(receipt->main_receipt){
  const GscActorControllerPrefixRun *old=receipt;
  if(!ok||!run.source_changed||run.actor_serial!=old->actor_serial||
     run.selection_serial!=old->selection_serial+1||!run.main_receipt||run.main_receipt==old->main_receipt||
     !run.task_serial||run.task_serial==old->task_serial||memcmp(&run.entity,&old->entity,sizeof(run.entity)))return 0;
  G_Printf("[repeat switch] new=%llu receipt=%llu selection=%llu now=%d\n",(unsigned long long)run.task_serial,
    (unsigned long long)run.main_receipt,(unsigned long long)run.selection_serial,now);
  memset(receipt,0,sizeof(*receipt));
 }
 if(ok && run.frontier==C1_STATE9_TOWER_CONTACT_RETURNED && run.record_present &&
    run.actor_serial==serial && run.main_receipt && run.task_serial && !memcmp(&run.entity,ref,sizeof(*ref)))
    return TrainingSendTowerTouch(ref,serial,frame.frame_generation,now);
 if(ok && run.frontier==C1_STATE9_EMPTY_CONTACT_RETURNED && run.record_present &&
    run.actor_serial==serial && run.main_receipt && run.task_serial && !memcmp(&run.entity,ref,sizeof(*ref)))return 1;
 if(ok && run.frontier==GSC_ACTOR_CONTROLLER_STATE9_TAIL_HELD && run.record_present &&
    run.actor_serial==serial && run.main_receipt && run.task_serial && !memcmp(&run.entity,ref,sizeof(*ref))){
     if(G_Cod1ActorUnmountedPublish(ref,now,GscTestTurretCurrent,NULL)){
         const entityState_t *pose=&g_entities[ref->slot].s;
         G_Printf("[Foley wire] committed owner=%u/%u tick=%d version=%d body=%.9g face=%.9g\n",
             ref->slot,ref->generation,now,pose->cod1TurretActorVersion,pose->cod1TurretActorGenericCurrent[58],pose->cod1TurretActorGenericCurrent[59]);
     }
 }
 if(ok&&run.frontier==GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD&&!run.record_present&&
    run.main_status==1&&run.main_receipt&&run.task_serial&&run.actor_serial==serial&&
    !memcmp(&run.entity,ref,sizeof(*ref))&&GscTestTurretCurrent(NULL)&&
    FoleyReturned4PoseCurrent(ref,serial,run.selection_serial,now,frame.frame_generation)){
    *receipt=run;
    G_Printf("[Foley course] row4 Think held; presentation continues owner=%u/%u tick=%d\n",ref->slot,ref->generation,now);
    return 1;
 }
 /* This observer ends at the controller's published frontier. */
 return 0;
}
int TrainingJumperRunRate(const GscEntityRef *,int,float,int(*)(void*),void*);
static int TrainingJumperClosed(int now){
 static GscEntityRef last;
 static GscActorControllerPrefixRun receipt;
 GscEntityRef ref;
 if(!TrainingJumperRead(&ref))return 1;
 if(memcmp(&ref,&last,sizeof(ref))){last=ref;memset(&receipt,0,sizeof(receipt));}
 if(!TrainingControllerAtClosedFrame(&ref,now,&receipt))return 0;
 if(receipt.main_receipt && receipt.frontier==GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD){
  cod1ActorGoalSnapshot_t goal;cod1ActorMotionSnapshot_t motion;
  if(!G_Cod1ActorReadGoal(&ref,&goal,NULL,0)||!G_Cod1ActorMotionRead(&ref,&motion))return 0;
  if(goal.assigned&&TrainingJumperExitNode(&ref,goal.assignment,&goal.node)){
   GscTrainingAppearance fields={0};
   if(!motion.active){
    const GscThreadEntityAuthorityHost *authority=G_Cod1ActorThreadAuthorityHost();
    GscEntityExecutionCertificate cert={0};char error[GSC_SESSION_ERROR_CAP]={0};
    if(!G_Cod1TrainingJumperPreparePath(&ref,now,GscTestTurretCurrent,NULL)||
       !authority->resolve(authority->user,&ref,&cert,error,sizeof(error))||
       !gsc_session_course_anim_entry(gscTestSession,gscTestId,&cert,1,error,sizeof(error))){
      G_Printf("[384 jumper] run entry failed: %s\n",error);return 0;
    }
    G_Printf("[384 jumper] original run main started owner=%u/%u tick=%d\n",ref.slot,ref.generation,now);
    if(!gsc_session_training_appearance(gscTestSession,gscTestId,&ref,1,&fields)||
       !TrainingJumperRunRate(&ref,now,fields.rate,GscTestTurretCurrent,NULL)||
       !G_Cod1ActorMotionBeginUnpublished(&ref,now))return 0;
    if(g_debugMove.integer)G_Printf("[tower run rate] owner=%u/%u tick=%d rate=%.7f\n",ref.slot,ref.generation,now,fields.rate);
   }
   if(
      !G_Cod1ActorMotionFrame(&ref,now,GscTestTurretCurrent,GscTestTurretGoal,NULL)||
      !G_Cod1ActorMotionRead(&ref,&motion))return 0;
   if(g_debugMove.integer&&(now%500==0||motion.notified))
    G_Printf("[Training jump exit] tick=%d owner=%u/%u moving=%d blocked=%d notified=%llu xyz=%.3f %.3f %.3f\n",
      now,ref.slot,ref.generation,motion.moving,motion.blocked,motion.notified,motion.position[0],motion.position[1],motion.position[2]);
  }
 }
 {int ok=G_Cod1ActorUnmountedPublish(&ref,now,GscTestTurretCurrent,NULL);
 if(g_debugMove.integer&&(now<1350||now%500==0)){
 const gentity_t *e=g_entities+ref.slot;
 G_Printf("[Training jumper server] tick=%d owner=%u/%u publish=%d xyz=%.4f %.4f %.4f\n",
   now,ref.slot,ref.generation,ok,e->r.currentOrigin[0],e->r.currentOrigin[1],e->r.currentOrigin[2]);}}
 return 1;
}
/* The first native VM frame can already consume an instructor pose
 * installed by map startup. Resolve that same placed actor before the frame. */
static int GscTestResolveInstructor(void) {
    GscEntityRef *ref = &gscTestTurret.instructor;
    const GscEntityHost *entities;
    GscEntityRef candidate = {0};
    char map[MAX_QPATH], error[128];
    int found = 0;
    if (!gscTestTurret.instructorResolved) {
        trap_Cvar_VariableStringBuffer("mapname", map, sizeof(map));
        if (!Q_stricmp(map,"training")) {
            entities = G_Cod1EntityHost();
            if (!entities->lookup(entities->user,"foley","targetname",&candidate,&found,error,sizeof(error)) ||
                !found || candidate.epoch != gscTestTurret.epoch || !G_Cod1EntitySameAllocation(&candidate) ||
                !g_entities[candidate.slot].classname ||
                strcmp(g_entities[candidate.slot].classname,"actor_ally_airborne_officer_thompson")) return 0;
            *ref = candidate;
        }
        gscTestTurret.instructorResolved = 1;
    }
    return !ref->epoch || (ref->epoch == gscTestTurret.epoch && G_Cod1EntitySameAllocation(ref));
}
static void GscTestInstructorFrame(int now) {
    GscEntityRef *ref = &gscTestTurret.instructor;
    int present;
    const char *stage = "actor identity";
    if (gscTestTurret.instructorStopped || !gscTestTurret.programSelected) return;
    if (!GscTestResolveInstructor()) goto stopped;
    if (!ref->epoch) return;
    present = G_Cod1ActorGenericPresent(ref);
    if (!present) return;
    if (present < 0) goto stopped;
    stage = "animation frame";
    if (!GscTestCourseActorFrame(ref,now,GscTestTurretCurrent,NULL)) goto stopped;
    stage = "scripted controller boundary";
    if(!TrainingControllerAtClosedFrame(ref,now,&gscTestTurret.heldThink))goto stopped;
{
 cod1ActorGoalSnapshot_t goal;
 cod1ActorMotionSnapshot_t motion;
 int changed,arrivedBefore;
 if(!G_Cod1ActorReadGoal(ref,&goal,NULL,0))goto stopped;
 if(goal.assigned && G_Cod1FoleyCourseGoal(goal.assignment,&goal.node)){
  stage="first goal lease";
  if(!G_Cod1ActorMotionRead(ref,&motion))goto stopped;
  if(!motion.active){
   /* The stock go_gate task writes its first node immediately, including
    * during the gate-order gesture. The closed native controller above and
    * the mover both own the same live lease and original placed node. */
   if(goal.assignment!=1)goto stopped;
   stage="begin grounded movement";
   if(!G_Cod1ActorMotionBeginUnpublished(ref,now))goto stopped;
  }
  stage="first goal collision movement";
  arrivedBefore=motion.arrived;
  if(!G_Cod1ActorMotionFrame(ref,now,GscTestTurretCurrent,GscTestTurretGoal,NULL)||
     !G_Cod1ActorMotionRead(ref,&motion))goto stopped;
  changed=motion.arrived!=arrivedBefore;
  if(changed || (g_debugMove.integer && now%100==0))G_Printf("[node1 motion] owner=%u/%u assignment=%llu now=%d active=%d moving=%d blocked=%d arrived=%d notified=%llu travelled=%.3f pos=%.3f,%.3f,%.3f\n",ref->slot,ref->generation,(unsigned long long)motion.assignment,now,motion.active,motion.moving,motion.blocked,motion.arrived,(unsigned long long)motion.notified,motion.travelled,motion.position[0],motion.position[1],motion.position[2]);
 }
}

    stage = "look frame";
    if (!G_Cod1ActorLookFrame(ref,now,GscTestTurretCurrent,NULL)) goto stopped;
    stage = "unmounted pose";
    if (!G_Cod1ActorUnmountedPublish(ref,now,GscTestTurretCurrent,NULL)) goto stopped;
    return;
stopped:
    gscTestTurret.instructorStopped = 1;
    G_Printf("[C025] Instructor presentation stopped at %s, entity=%u generation=%u\n",
        stage,ref->slot,ref->generation);
}
/* Two map actors can share a target. Resolve the placed name once, then
 * keep only the captured allocation for the rest of the map. */
static int TrainingPlacedRunnerName(const GscEntityRef *ref,const char *name){
 cod1PlacedActorSnapshot_t placed;
 unsigned i;
 int matches=0;
 if(!G_Cod1PlacedActorRead(ref,&placed,NULL,0))return -1;
 for(i=0;i<placed.rawFieldCount;i++){
  char key[MAX_SPAWN_VARS_CHARS],value[MAX_SPAWN_VARS_CHARS];
  if(!G_Cod1PlacedActorRawField(ref,i,key,sizeof(key),value,sizeof(value),NULL,0))return -1;
  if(!Q_stricmp(key,"script_friendname")){
   if(matches || strcmp(value,name))return 0;
   matches=1;
  }
 }
 return matches;
}
static int TrainingReadRunnerAppearance(const GscEntityRef *owner,int kind,GscTrainingAppearance *out){
 return GscTestTurretCurrent(NULL) && gsc_session_training_appearance(gscTestSession,gscTestId,owner,kind,out);
}
static void GscTestRunnerFrame(unsigned index,int now){
 TrainingRunner *runner=&gscTestTurret.runners[index];
 GscEntityRef *owner=&runner->owner;
 const char *name=trainingRunnerProfiles[index].name,*placedName=trainingRunnerProfiles[index].placedName;
 const char *target=trainingRunnerProfiles[index].first,*special=trainingRunnerProfiles[index].specialBody;
 const char *stage="placed runner";
 int i,present;
 if(!gscTestTurret.programSelected || runner->stopped)return;
 if(!runner->resolved){
  char map[MAX_QPATH];
  runner->resolved=1;
  trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
  if(Q_stricmp(map,"training"))return;
  for(i=MAX_CLIENTS;i<level.num_entities;i++){
   gentity_t *e=&g_entities[i];
   GscEntityRef ref;
   int named;
   if(!e->inuse || !e->classname || strcmp(e->classname,"actor_ally_airborne_garand") ||
      !e->targetname || strcmp(e->targetname,"runner") || !e->target || strcmp(e->target,target))continue;
   if(!G_Cod1EntityCapture(e,&ref))goto stop;
   named=TrainingPlacedRunnerName(&ref,placedName);
   if(named<0 || (named && owner->epoch))goto stop;
   if(named)*owner=ref;
  }
 }
 if(!owner->epoch)return;
 stage="allocation";
 if(owner->epoch!=gscTestTurret.epoch || !G_Cod1EntitySameAllocation(owner))goto stop;
 present=G_Cod1ActorGenericPresent(owner);
 if(!present && !runner->published)return;
 if(present!=1)goto stop;
 if(!runner->published){
  GscStringCastValue model;
  stage="assigned model";
  if(!G_Cod1ActorReadModelField(owner,&model,NULL,0) || model.type!=1)goto stop;
  const char *body=model.string+7;
  if(Q_stricmpn(model.string,"xmodel",6) || (model.string[6]!='/' && model.string[6]!='\\'))goto stop;
  if(special){
   if(Q_stricmp(body,special))goto stop;
  }else if(Q_stricmp(body,"character_airborne") && Q_stricmp(body,"character_airborneslvup") &&
           Q_stricmp(body,"character_airborne_101") && Q_stricmp(body,"character_airborne_101slvup"))goto stop;
  stage="authored target chain";
  if(!G_Cod1BindTrainingRoute(g_entities[owner->slot].target,&runner->route))goto stop;
  G_Printf("[Training] %s placed owner=%u/%u model=%s origin=%.1f %.1f %.1f\n",
      name,owner->slot,owner->generation,model.string,g_entities[owner->slot].r.currentOrigin[0],
      g_entities[owner->slot].r.currentOrigin[1],g_entities[owner->slot].r.currentOrigin[2]);
 }
 stage="animation frame";
 if(!GscTestCourseActorFrame(owner,now,GscTestTurretCurrent,NULL))goto stop;
 {
  cod1ActorGoalSnapshot_t goal;
  cod1ActorMotionSnapshot_t motion;
  stage="runner goal";
  if(!G_Cod1ActorReadGoal(owner,&goal,NULL,0))goto stop;
  if(!G_Cod1ActorMotionRead(owner,&motion))goto stop;
  if(goal.assigned&&(goal.assignment!=runner->diagnosticAssignment||(motion.blocked&&now%1000==0))){
   /* Skip the optional experiment and its unavailable print when disabled. */
   int probeMode=trap_Cvar_VariableIntegerValue("g_cod1NavigationProbe");
   if(probeMode==1||probeMode==2){
    cod1NavigationProbe_t p;int valid=G_Cod1NavigationProbe(gscTestSession,gscTestId,owner,now,&p);
    if(valid)G_Printf("[542 nearest] owner=%u tick=%d assignment=%llu nodes=%d/%d candidates=%u/%u traces=%u/%u origin=%.9g,%.9g,%.9g target=%.9g,%.9g,%.9g start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g\n",owner->slot,now,(unsigned long long)p.assignment,p.start,p.goal,p.candidates[0],p.candidates[1],p.traces[0],p.traces[1],p.origin[0],p.origin[1],p.origin[2],p.target[0],p.target[1],p.target[2],p.startNode[0],p.startNode[1],p.startNode[2],p.goalNode[0],p.goalNode[1],p.goalNode[2]);
    if(valid){int count;unsigned flags;float direction[2];memcpy(&flags,p.path+0x3b8,4);count=(short)(p.path[0x380]|p.path[0x381]<<8);memcpy(direction,p.path+0x398,8);
     G_Printf("[544 route] owner=%u tick=%d assignment=%llu available=%d native=%d team=%d traces=%u count=%d flags=%x direction=%.9g,%.9g\n",owner->slot,now,(unsigned long long)p.assignment,p.routeAvailable,p.routeResult,p.team,p.routeTraces,count,flags,direction[0],direction[1]);
     G_Printf("[565 physics probe] owner=%u tick=%d assignment=%llu category=%d root=%d delta=%d physics=%d result=%d traces=%u rootXYZ=%.9g,%.9g,%.9g deltaXYZ=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g velocity=%.9g,%.9g,%.9g\n",owner->slot,now,(unsigned long long)p.assignment,p.category,p.rootAvailable,p.deltaAvailable,p.physicsAvailable,p.physicsResult,p.physicsTraces,p.root[0],p.root[1],p.root[2],p.delta[0],p.delta[1],p.delta[2],p.physicsEnd[0],p.physicsEnd[1],p.physicsEnd[2],p.physicsVelocity[0],p.physicsVelocity[1],p.physicsVelocity[2]);
     if(p.routeAvailable&&owner->slot==223&&(goal.assignment!=runner->diagnosticAssignment||now%5000==0)){
      G_Cod1NavigationDump(now,p.assignment,p.path);
     }
    }
    else G_Printf("[542 nearest unavailable] owner=%u tick=%d assignment=%llu\n",owner->slot,now,(unsigned long long)goal.assignment);
   }
  }
  if(goal.assigned&&goal.assignment!=runner->diagnosticAssignment){
   if(g_debugMove.integer){
    int kind=G_Cod1RunnerCourseGoal(owner,goal.assignment,&goal.node);
    if(kind==3||kind==4){
     cod1NegotiationPair_t pair;int valid=G_Cod1NodeGraphNegotiationPair(gscTestSession,gscTestId,&runner->diagnosticLastNode,&goal.node,&pair);
     G_Printf("[478 graph pair] owner=%u tick=%d assignment=%llu kind=%d valid=%d begin=%u end=%u\n",owner->slot,now,(unsigned long long)goal.assignment,kind,valid,runner->diagnosticLastNode.index,goal.node.index);
     if(valid){const float*origin=g_entities[owner->slot].r.currentOrigin;float dx=pair.start[0]-origin[0],dy=pair.start[1]-origin[1];
      G_Printf("[478 physical pair] owner=%u physical=%u/%u script=%llu/%u yaw=%.6f begin=%.6f,%.6f,%.6f end=%.6f,%.6f,%.6f origin=%.6f,%.6f,%.6f xySquared=%.9f current=%d\n",owner->slot,pair.beginPhysical,pair.endPhysical,(unsigned long long)pair.script.cache,(unsigned)pair.script.entry,pair.yaw,pair.start[0],pair.start[1],pair.start[2],pair.finish[0],pair.finish[1],pair.finish[2],origin[0],origin[1],origin[2],dx*dx+dy*dy,G_Cod1NodeGraphNegotiationCurrent(&pair));
     }
    }
   }
   /* Keep tracking assignments even with diagnostics disabled. */
   runner->diagnosticLastNode=goal.node;runner->diagnosticAssignment=goal.assignment;
  }
  /* This read-only root interval is not consumed by motion or publication. */
  if(g_debugMove.integer&&runner->published&&now%500==0){
   c1ari14Result_t root;int valid=G_Cod1ActorGeneralRootInterval(owner,now,0,&root,GscTestTurretCurrent,NULL);
   G_Printf("[479 general interval] owner=%u tick=%d valid=%d xyz=%.7f,%.7f,%.7f q=%.7f,%.7f\n",owner->slot,now,valid,valid?root.translation[0]:0,valid?root.translation[1]:0,valid?root.translation[2]:0,valid?root.q2[0]:0,valid?root.q2[1]:0);
  }
  /* Completed goals remain assigned for script inspection. An arrival belongs
   * only to the matching assignment; a newly assigned goal must select run.
   * Blocked movement is not arrival. The original MAIN owns the body tree. */
  if(runner->published){
   GscTrainingAppearance appearance;
   const GscThreadEntityAuthorityHost *authority=G_Cod1ActorThreadAuthorityHost();
   GscEntityExecutionCertificate cert={0};char error[GSC_SESSION_ERROR_CAP]={0};
   int source=(!goal.assigned || (motion.active && motion.assignment==goal.assignment &&
      motion.arrived && !motion.moving && !motion.blocked))?1:2;
   if(TrainingReadRunnerAppearance(owner,1,&appearance)){
    stage="owned locomotion MAIN";
    if(!authority->resolve(authority->user,owner,&cert,error,sizeof(error))||
       !G_Cod1ActorLocomotionSource(owner,cert.serial,now,source,&source,GscTestTurretCurrent,NULL)||
       !gsc_session_locomotion_main(gscTestSession,gscTestId,&cert,source,error,sizeof(error))){
     G_Printf("[Training] %s owned locomotion failed source=%d: %s\n",name,source,error);goto stop;
    }
    /* Entry can execute original script immediately. Re-read the actual goal
     * and mover before consuming either snapshot after that VM boundary. */
    if(!G_Cod1ActorReadGoal(owner,&goal,NULL,0)||!G_Cod1ActorMotionRead(owner,&motion))goto stop;
   }
  }
  if(goal.assigned){
   int wasArrived;
   int route=G_Cod1RunnerCourseGoal(owner,goal.assignment,&goal.node);
   if(!route || !G_Cod1ActorMotionRead(owner,&motion))goto stop;
   wasArrived=motion.arrived;
   stage="course appearance before motion";
   if(!G_Cod1TrainingRunnerAppearance(owner,0,TrainingReadRunnerAppearance))goto stop;
   stage="runner motion";
   if(!motion.active && !G_Cod1ActorMotionBeginUnpublished(owner,now))goto stop;
   if(!G_Cod1ActorMotionFrame(owner,now,GscTestTurretCurrent,GscTestTurretGoal,NULL) ||
      !G_Cod1ActorMotionRead(owner,&motion))goto stop;
   stage="course appearance after motion";
   if(!G_Cod1TrainingRunnerAppearance(owner,1,TrainingReadRunnerAppearance))goto stop;
   if((!wasArrived && motion.arrived)||(g_debugMove.integer && now%100==0))
    G_Printf("[%s route] owner=%u/%u tick=%d moving=%d blocked=%d arrived=%d notified=%llu distance=%.3f position=%.3f,%.3f,%.3f\n",
     name,owner->slot,owner->generation,now,motion.moving,motion.blocked,motion.arrived,(unsigned long long)motion.notified,
     motion.travelled,motion.position[0],motion.position[1],motion.position[2]);
  }
 }
 stage="look frame";
 if(!G_Cod1ActorLookFrame(owner,now,GscTestTurretCurrent,NULL))goto stop;
 stage="snapshot";
 if(!G_Cod1ActorUnmountedPublish(owner,now,GscTestTurretCurrent,NULL))goto stop;
 if(!runner->published){
  G_Printf("[Training] %s pose published owner=%u/%u\n",name,owner->slot,owner->generation);
  runner->published=1;
 }
 return;
stop:
 runner->stopped=1;
 G_Printf("[Training] %s presentation ended at %s owner=%u/%u\n",name,stage,owner->slot,owner->generation);
}
/* The real placed Moody keeps the same lease and grounded mover after his
 * authored model/attachment setup. This first step admits the base pose only. */
static int GscTestMoodyPoseFrame(int now){
 const GscEntityRef *owner=&gscTestCarbine.moodyRef;
 GscStringCastValue model;
 int present;
 if(!GscTestTurretCurrent(NULL)||owner->epoch!=gscTestTurret.epoch||
    !G_Cod1EntitySameAllocation(owner))return 0;
 present=G_Cod1ActorGenericPresent(owner);
 if(!present)return 1;
 if(present!=1||!G_Cod1ActorReadModelField(owner,&model,NULL,0)||model.type!=1||
    (Q_stricmp(model.string,"xmodel/character_Moody")&&Q_stricmp(model.string,"xmodel\\character_Moody")))return 0;
 cod1ActorGoalSnapshot_t goal;
 if(!GscTestCourseActorFrame(owner,now,GscTestTurretCurrent,NULL)||
    !TrainingControllerAtClosedFrame(owner,now,&gscTestTurret.moodyThink)||
    !G_Cod1ActorReadGoal(owner,&goal,NULL,0))return 0;
 /* The placed stationary actor enters the archive stop root after its
  * actual init fields and closed controller are ready. The script itself
  * selects standanim and owns its waits; no body leaf is installed here. */
 if(!goal.assigned&&!gscTestCarbine.moodyInitialStop){
  GscTrainingAppearance appearance;int needed=0;
  if(!G_Cod1MoodyIdleNeeded(owner,&needed))return 0;
  if(needed==1&&TrainingReadRunnerAppearance(owner,1,&appearance)){
   const GscThreadEntityAuthorityHost *authority=G_Cod1ActorThreadAuthorityHost();
   GscEntityExecutionCertificate cert={0};char error[GSC_SESSION_ERROR_CAP]={0};
   if(!authority->resolve(authority->user,owner,&cert,error,sizeof(error))||
      !gsc_session_initial_stop_main(gscTestSession,gscTestId,&cert,error,sizeof(error))){
    G_Printf("[382 Moody] stop entry failed: %s\n",error);return 0;
   }
   gscTestCarbine.moodyInitialStop=1;
   G_Printf("[382 Moody] original stop main started owner=%u/%u tick=%d\n",owner->slot,owner->generation,now);
  }
 }
 /* SetGoalNode is the first grounded route owner. Leave the actor in the
  * original stationary domain for the greeting before that assignment. */
 if(goal.assigned){
    cod1ActorMotionSnapshot_t motion;
    if(!G_Cod1MoodyCourseGoal(goal.assignment,&goal.node) ||
       !G_Cod1ActorMotionBeginUnpublished(owner,now) ||
       !G_Cod1ActorMotionFrame(owner,now,GscTestTurretCurrent,GscTestMoodyNotify,NULL) ||
       !G_Cod1ActorMotionRead(owner,&motion))return 0;
    if(g_debugMove.integer && now%100==0)
        G_Printf("[Moody route] owner=%u/%u tick=%d goal=%llu moving=%d blocked=%d arrived=%d notified=%llu xyz=%.4f,%.4f,%.4f\n",
          owner->slot,owner->generation,now,(unsigned long long)goal.assignment,motion.moving,motion.blocked,
          motion.arrived,(unsigned long long)motion.notified,motion.position[0],motion.position[1],motion.position[2]);
 }
 return
    G_Cod1ActorLookFrame(owner,now,GscTestTurretCurrent,NULL)&&
    G_Cod1ActorUnmountedPublish(owner,now,GscTestTurretCurrent,NULL);
}
static int GscTestTurretFrame(int now,int *nextRunner,int (*expired)(void),char*error,size_t size){
 int i;cod1ActorTurretMount_t mount;const char*stage="course frame admission";
 if(!*nextRunner){
  if(!GscTestTurretCurrent(NULL)||now!=level.time)return 0;
  if(!TrainingRigFrame(now,TrainingRigNote)||!TrainingJumperClosed(now)){snprintf(error,size,"jump actor closed controller at %d",now);return 0;}
  GscTestInstructorFrame(now);
  *nextRunner=1;
  if(expired && expired())return 2;
 }
 while(*nextRunner<=(int)TRAINING_RUNNER_COUNT){
  GscTestRunnerFrame((unsigned)(*nextRunner-1),now);
  ++*nextRunner;
  if(expired && expired())return 2;
 }
 if(!gscTestTurret.actor.epoch){
  for(i=MAX_CLIENTS;i<level.num_entities;i++)if(g_entities[i].inuse&&
   g_entities[i].classname&&!strcmp(g_entities[i].classname,"actor_ally_airborne_garand")&&
   g_entities[i].target&&!strcmp(g_entities[i].target,"auto433"))break;
  if(i==level.num_entities||!G_Cod1EntityCapture(&g_entities[i],&gscTestTurret.actor))goto refused;
 }
 if(!gscTestTurret.gun.epoch){
  for(i=MAX_CLIENTS;i<level.num_entities;i++)if(g_entities[i].inuse&&
   g_entities[i].classname&&!strcmp(g_entities[i].classname,"misc_mg42")&&
   g_entities[i].targetname&&!strcmp(g_entities[i].targetname,"mg42"))break;
  if(i==level.num_entities||!G_Cod1EntityCapture(&g_entities[i],&gscTestTurret.gun))goto refused;
 }
 if(gscTestTurret.actorRetired||!G_Cod1EntitySameAllocation(&gscTestTurret.actor)){
  /* Absence only triggers proof; it is never a completion certificate. */
  if(!GscTestTurretActorRetired())goto refused;
  if(!gscTestTurret.actorRetired)G_Printf("[R240] Avery exact original-prefix retirement committed; actor frame closed\n");gscTestTurret.actorRetired=1;
 }
 if(!GscTestTurretDeleteBind()||!G_Cod1EntityRuntimeActorDeleteFrameReady(gscTestTurret.epoch))goto refused;
 if(gscTestTurret.actorRetired)return G_Cod1TurretFrame(now,GscTestTurretCurrent,GscTestTurretNotify,NULL)&&GscTestTurretCurrent(NULL);
 if(!gscTestTurret.motion){
  if(!G_Cod1ActorMotionBeginUnpublished(&gscTestTurret.actor,now))goto refused;
  gscTestTurret.motion=1;
 }
 {int enabled=trap_Cvar_VariableIntegerValue("g_cod1GscProfile"), begin,ok;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1ActorMotionFrame";ok=G_Cod1ActorMotionFrame(&gscTestTurret.actor,now,GscTestTurretCurrent,GscTestTurretGoal,NULL);if(enabled)G_Printf("[R235 profile] time=%d motion ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1ActorGenericNotifyDrain";ok=G_Cod1ActorGenericNotifyDrain(&gscTestTurret.actor,now,GscTestTurretCurrent,GscTestTurretAnimationEnd,NULL);if(enabled)G_Printf("[R235 profile] time=%d drain1 ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1ActorGenericFrame";ok=GscTestCourseActorFrame(&gscTestTurret.actor,now,GscTestTurretCurrent,NULL);if(enabled)G_Printf("[R235 profile] time=%d generic ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1ActorGenericNotifyDrain";ok=G_Cod1ActorGenericNotifyDrain(&gscTestTurret.actor,now,GscTestTurretCurrent,GscTestTurretAnimationEnd,NULL);if(enabled)G_Printf("[R235 profile] time=%d drain2 ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1TurretMountFrame";ok=G_Cod1TurretMountFrame(now,GscTestTurretCurrent,GscTestTurretNotify,GscTestTurretStand,NULL);if(enabled)G_Printf("[R235 profile] time=%d mount ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1TurretFrame";ok=G_Cod1TurretFrame(now,GscTestTurretCurrent,GscTestTurretNotify,NULL);if(enabled)G_Printf("[R235 profile] time=%d gun ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1ActorLookFrame";ok=G_Cod1ActorLookFrame(&gscTestTurret.actor,now,GscTestTurretCurrent,NULL);if(enabled)G_Printf("[R235 profile] time=%d look ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  begin=enabled?trap_Milliseconds():0;stage="G_Cod1TurretMountRead";ok=G_Cod1TurretMountRead(&gscTestTurret.actor,&mount);if(enabled)G_Printf("[R235 profile] time=%d read ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;
  stage="G_Cod1ActorUnmountedPublish";if(!mount.mounted){int present=G_Cod1ActorGenericPresent(&gscTestTurret.actor);if(present<0)goto refused; if(present==1 && !G_Cod1ActorUnmountedPublish(&gscTestTurret.actor,now,GscTestTurretCurrent,NULL))goto refused;}
  if(mount.mounted&&g_entities[gscTestTurret.gun.slot].s.cod1TurretAnimated){begin=enabled?trap_Milliseconds():0;stage="G_Cod1TurretMountedFrame";ok=G_Cod1TurretMountedFrame(&gscTestTurret.gun,now,GscTestTurretCurrent,NULL);if(enabled)G_Printf("[R235 profile] time=%d mounted ms=%d\n",now,trap_Milliseconds()-begin);if(!ok)goto refused;}
 }
 {cod1ActorMotionSnapshot_t m;cod1ActorGoalSnapshot_t goal;int state;
  if(G_Cod1ActorMotionRead(&gscTestTurret.actor,&m)&&G_Cod1ActorReadGoal(&gscTestTurret.actor,&goal,NULL,0)){
   state=!!m.moving+2*!!m.blocked+4*!!m.arrived+8*!!goal.assigned+16*!!mount.mounted;
   if(!gscTestTurret.observed||state!=gscTestTurret.observedState){
    gscTestTurret.observed=1;gscTestTurret.observedState=state;
    G_Printf("[R234] Avery runtime time=%d moving=%d blocked=%d arrived=%d goal=%d mounted=%d position=(%.2f %.2f %.2f) target=(%.2f %.2f %.2f)\n",now,m.moving,m.blocked,m.arrived,goal.assigned,mount.mounted,m.position[0],m.position[1],m.position[2],goal.position[0],goal.position[1],goal.position[2]);
   }
  }
 }
 G_Cod1TurretPoseTrace(&gscTestTurret.gun,now);
 return GscTestTurretCurrent(NULL);
refused:snprintf(error,size,"R243 course frame failed stage=%s",stage);return 0;
}
