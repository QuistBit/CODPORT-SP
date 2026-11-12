#include "../g_cod1_native_thread_frame_policy.h"
/* Same Session selected EMPTY before link; never patch an active runtime.
 * Native4695B0 drains current cursor;46ACA0 drains [old,target), preserving
 * target for the next real frame. Actor/turret work remains once per GAME
 * frame after these phases; full combatAI/phase6 owner is separate work. */
static int GscTestNativeFrameError(char*error,size_t cap,const char*text){
 if(error&&cap)snprintf(error,cap,"%s",text);return 0;
}
int TrainingCourseClocksBeforeVm(const GscEntityRef*,int,uint64_t,int(*)(void*),void*);
static int FoleyPrivateBeforeVm(int now,uint64_t generation){
 GscR237FrameView view;
 GscSessionSnapshot session;
 const GscEntityRef *actor=&gscTestTurret.instructor;
 if(!gscTestTurret.instructorResolved || !actor->epoch || gscTestTurret.instructorStopped)return 1;
 if(!gsc_session_snapshot(gscTestSession,&session) || session.session_id!=gscTestId || session.busy ||
    session.error[0] || session.last_host_ms!=now || session.origin_host_ms<0 ||
    !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view) || !view.frame_open ||
    view.frame_generation!=generation || view.host_now_ms!=session.logical_ms ||
    now-session.origin_host_ms!=session.logical_ms || actor->epoch!=gscTestTurret.epoch ||
    !G_Cod1EntitySameAllocation(actor))return 0;
 return TrainingCourseClocksBeforeVm(actor,now,generation,GscTestTurretCurrent,NULL);
}
int TrainingMoodyFrame(const GscEntityRef*,int,uint64_t,int,int(*)(void*),void*);
int TrainingJumperClock(int,uint64_t,int,int(*)(void*),void*);
static int TrainingMoodyBeforeVm(int now,uint64_t generation,int prepare){
 const GscEntityRef *actor=&gscTestCarbine.moodyRef;
 if(!gscTestCarbine.moodyCharacter||!actor->epoch)return 1;
 return actor->epoch==gscTestCarbine.epoch && GscTestCarbineCurrent(NULL) &&
    G_Cod1EntitySameAllocation(actor) &&
    TrainingMoodyFrame(actor,now,generation,prepare,GscTestTurretCurrent,NULL);
}
int TrainingRigFrame(int,int(*)(const GscEntityRef*,const char*,const char*));
static int TrainingRigNote(const GscEntityRef *ref,const char *event,const char *payload){
 GscEventValue v={0};int woken=-1,ok;char error[GSC_SESSION_ERROR_CAP]={0};
 if(!G_Cod1EntitySameAllocation(ref)||!GscTestTurretCurrent(NULL)||strlen(payload)>=sizeof(v.string))return 0;
 v.kind=GSC_EVENT_STRING;strcpy(v.string,payload);
 ok=gsc_session_notify_entity_with_payload(gscTestSession,gscTestId,ref,event,&v,1,&woken,error,sizeof(error));
 G_Printf("[Training tower] note owner=%u/%u host=%d event=%s payload=%s woken=%d result=%d %s\n",ref->slot,ref->generation,level.time,event,payload,woken,ok,error);
 return ok&&woken>=0;
}
static int GscTestNativeFrameAdvance(int now,int*advanced,char*error,size_t cap){
 GscSessionSnapshot s;GscR237FrameView view;uint64_t generation;int decision;
 if(!advanced)return 0;*advanced=1;
 if(!gsc_session_r237_frame_buckets_selected(gscTestSession)){
  if(gscTestTurret.nativeThreads)return GscTestNativeFrameError(error,cap,"native thread selection lost frame capability");
  return gsc_session_advance(gscTestSession,gscTestId,(int64_t)now,error,cap);
 }
 if(!gscTestTurret.nativeThreads||trap_Cvar_VariableIntegerValue("g_cod1GscNativeThreads")!=1||
    !GscTestEarlyCurrent()||gscTestStopRequested||level.restarted||
    !gsc_session_snapshot(gscTestSession,&s)||s.session_id!=gscTestId||s.busy||s.error[0]||
    !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view)||view.frame_open||s.origin_host_ms<0||
    s.last_host_ms<s.origin_host_ms||now<s.last_host_ms)
  return GscTestNativeFrameError(error,cap,"native thread/frame owner or monotonic host clock retired");
 decision=C1NativeThreadFrameDecision(s.origin_host_ms,s.last_host_ms,s.logical_ms,view.host_now_ms,view.frame_open,now);
 if(decision==C1_NATIVE_FRAME_REFUSE)
  return GscTestNativeFrameError(error,cap,"native thread/frame physical and logical clocks differ");
 /* This helper skips duplicate VM phases and subsequent Actor/turret work.
  * Earlier committed event producers retain the caller delivery order. */
 if(decision==C1_NATIVE_FRAME_SAME){*advanced=0;return 1;}
 return gsc_session_r237_frame_begin(gscTestSession,gscTestId,now,&generation,error,cap)&&
  G_Cod1TrainingStationaryGoals(now,GscTestTurretCurrent,GscTestTurretGoal,NULL)&&
  TrainingRigFrame(now,TrainingRigNote)&&TrainingJumperClock(now,generation,1,GscTestTurretCurrent,NULL)&&
  /* Foley's notification can resume Moody's real stop initializer in the
   * same VM bucket. Prepare Moody's tree before that cross-actor callback. */
  TrainingMoodyBeforeVm(now,generation,1)&&FoleyPrivateBeforeVm(now,generation)&&
  TrainingMoodyBeforeVm(now,generation,0)&&
  TrainingJumperClock(now,generation,0,GscTestTurretCurrent,NULL)&&
  gsc_session_r237_run_current_bucket(gscTestSession,gscTestId,generation,error,cap)&&
  gsc_session_r237_advance_exclusive(gscTestSession,gscTestId,generation,error,cap);
}
