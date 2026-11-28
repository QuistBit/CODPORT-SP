#include "../g_cod1_native_thread_frame_policy.h"
/* Same Session selected EMPTY before link; never patch an active runtime.
 * Native4695B0 drains current cursor;46ACA0 drains [old,target), preserving
 * target for the next real frame. Actor/turret work remains once per GAME
 * frame after these phases; full combatAI/phase6 owner is separate work. */
static int GscTestNativeFrameError(char*error,size_t cap,const char*text){
 if(error&&cap)snprintf(error,cap,"%s",text);return 0;
}
int TrainingCourseClocksBeforeVm(const GscEntityRef*,int,uint64_t,int(*)(void*),void*);
int TrainingCourseClocksBeforeVmSlice(const GscEntityRef*,int,uint64_t,int(*)(void*),void*,int,int(*)(void));
static int FoleyPrivateBeforeVm(int now,uint64_t generation,int resume,int (*expired)(void)){
 GscR237FrameView view;
 GscSessionSnapshot session;
 const GscEntityRef *actor=&gscTestTurret.instructor;
 if(!gscTestTurret.programSelected || gscTestTurret.instructorStopped)return !resume;
 if(!resume && !GscTestResolveInstructor())return 0;
 if(!actor->epoch)return !resume;
 if(!gsc_session_snapshot(gscTestSession,&session) || session.session_id!=gscTestId || session.busy ||
    session.error[0] || session.last_host_ms!=now || session.origin_host_ms<0 ||
    !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view) || !view.frame_open ||
    view.frame_generation!=generation || view.host_now_ms!=session.logical_ms ||
    now-session.origin_host_ms!=session.logical_ms || actor->epoch!=gscTestTurret.epoch)return 0;
 return TrainingCourseClocksBeforeVmSlice(actor,now,generation,GscTestTurretCurrent,NULL,resume,expired);
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
typedef struct {GscSession*session;uint64_t id,epoch,generation;int now;} GscVehicleSessionFrame;
static int GscVehicleSessionCurrent(void*u){GscVehicleSessionFrame*c=u;GscSessionSnapshot s;GscR237FrameView v;
 return gscVehicleFramesSelected&&!gscTestStopRequested&&!gscTestCompleted&&c->session==gscTestSession&&c->id==gscTestId&&c->epoch==G_Cod1EntityMapEpoch()&&level.time==c->now&&!level.restarted&&
  gsc_session_snapshot(c->session,&s)&&s.session_id==c->id&&!s.busy&&!s.error[0]&&s.state==GSC_SESSION_RUNNING&&s.origin_host_ms>=0&&s.last_host_ms==c->now&&
  gsc_session_r237_frame_view(c->session,c->id,&v)&&v.frame_open&&v.frame_generation==c->generation&&v.host_now_ms==s.logical_ms&&c->now-s.origin_host_ms==s.logical_ms;
}
static int GscVehicleSessionNote(void*u,const GscEntityRef*owner,const char*event){GscVehicleSessionFrame*c=u;int woken=-1,ok;char error[GSC_SESSION_ERROR_CAP];
 if(!GscVehicleSessionCurrent(c))return 0;
 ok=gsc_session_notify_entity(c->session,c->id,owner,event,&woken,error,sizeof(error));
 G_Printf("[vehicle path] tick=%d owner=%u event=%s woken=%d result=%d\n",c->now,owner->slot,event,woken,ok);
 return ok&&woken>=0&&GscVehicleSessionCurrent(c);
}
static int GscVehicleSessionAdvance(int now,int*advanced,char*error,size_t cap){GscSessionSnapshot s;GscR237FrameView view;GscVehicleSessionFrame c;int decision;
 if(!gsc_session_r237_frame_buckets_selected(gscTestSession)||!gscVehicleBankEpoch||gscVehicleBankEpoch!=G_Cod1EntityMapEpoch()||
  !gsc_session_snapshot(gscTestSession,&s)||s.session_id!=gscTestId||s.busy||s.error[0]||s.origin_host_ms<0||
  !gsc_session_r237_frame_view(gscTestSession,gscTestId,&view))return GscTestNativeFrameError(error,cap,"vehicle Session frame identity unavailable");
 decision=C1NativeThreadFrameDecision(s.origin_host_ms,s.last_host_ms,s.logical_ms,view.host_now_ms,view.frame_open,now);
 if(decision==C1_NATIVE_FRAME_REFUSE)return GscTestNativeFrameError(error,cap,"vehicle physical/logical frame clocks differ");
 if(decision==C1_NATIVE_FRAME_SAME){*advanced=0;return 1;}
 memset(&c,0,sizeof(c));c.session=gscTestSession;c.id=gscTestId;c.epoch=gscVehicleBankEpoch;c.now=now;
 if(!gsc_session_r237_frame_begin(c.session,c.id,now,&c.generation,error,cap)||!GscVehicleSessionCurrent(&c)||!GscVehicleFrames(now,GscVehicleSessionCurrent,&c,GscVehicleSessionNote,&c)||!GscTurretFactoryFrames(now,GscVehicleSessionCurrent,&c))return GscTestNativeFrameError(error,cap,"vehicle firstthink/frame/world boundary refused");
 return GscProfilePhase53(c.session,c.id,c.generation,0,NULL,error,cap,GSC53_CURRENT)&&GscProfilePhase53(c.session,c.id,c.generation,1,NULL,error,cap,GSC53_EXCLUSIVE);
}
enum { NATIVE_BEGIN,NATIVE_GOALS,NATIVE_RIG,NATIVE_JUMPER_BEGIN,NATIVE_MOODY_BEGIN,
       NATIVE_COURSE,NATIVE_MOODY_END,NATIVE_JUMPER_END,NATIVE_CURRENT,NATIVE_EXCLUSIVE };
static int GscTestNativeFrameAdvance(int now,int*advanced,int (*expired)(void),char*error,size_t cap){
 GscSessionSnapshot s;GscR237FrameView view;int decision,result;
 if(!advanced)return 0;*advanced=1;
 if(gscFrameCursor.nativeStage==NATIVE_BEGIN){
  if(gscVehicleFramesSelected)return GscVehicleSessionAdvance(now,advanced,error,cap);
  if(!gsc_session_r237_frame_buckets_selected(gscTestSession)){
   if(gscTestTurret.nativeThreads)return GscTestNativeFrameError(error,cap,"native thread selection lost frame capability");
   return gsc_session_advance(gscTestSession,gscTestId,(int64_t)now,error,cap);
  }
  if(!gscTestTurret.nativeThreads||G_Cod1GscFeature47(C1CP47_F_NATIVE_THREADS)!=1||
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
  if(!gsc_session_r237_frame_begin(gscTestSession,gscTestId,now,&gscFrameCursor.generation,error,cap))return 0;
  gscFrameCursor.nativeStage=NATIVE_GOALS;
 }
 for(;;){
  uint64_t generation=gscFrameCursor.generation;
  const int stageAtStart=gscFrameCursor.nativeStage;
  c1fp35Token profilePart=C1FP35_Begin(now);
  switch(gscFrameCursor.nativeStage){
   case NATIVE_GOALS: result=!!G_Cod1TrainingStationaryGoals(now,GscTestTurretCurrent,GscTestTurretGoal,NULL);break;
   case NATIVE_RIG: result=!!TrainingRigFrame(now,TrainingRigNote);break;
   case NATIVE_JUMPER_BEGIN: result=!!TrainingJumperClock(now,generation,1,GscTestTurretCurrent,NULL);break;
   case NATIVE_MOODY_BEGIN: result=!!TrainingMoodyBeforeVm(now,generation,1);break;
   case NATIVE_COURSE:
    result=FoleyPrivateBeforeVm(now,generation,gscFrameCursor.coursePaused,expired);
    gscFrameCursor.coursePaused=result==2;
    break;
   case NATIVE_MOODY_END: result=!!TrainingMoodyBeforeVm(now,generation,0);break;
   case NATIVE_JUMPER_END: result=!!TrainingJumperClock(now,generation,0,GscTestTurretCurrent,NULL);break;
   case NATIVE_CURRENT:
   case NATIVE_EXCLUSIVE:
    result=GscProfilePhase53(gscTestSession,gscTestId,generation,
              gscFrameCursor.nativeStage==NATIVE_EXCLUSIVE,expired,error,cap,
              gscFrameCursor.nativeStage==NATIVE_EXCLUSIVE?GSC53_EXCLUSIVE:GSC53_CURRENT);
    break;
   default:return GscTestNativeFrameError(error,cap,"invalid native frame phase");
  }
  C1FP35_End(stageAtStart==NATIVE_COURSE?C1FP35_COURSE:stageAtStart==NATIVE_CURRENT?C1FP35_VM_CURRENT:
             stageAtStart==NATIVE_EXCLUSIVE?C1FP35_VM_EXCLUSIVE:C1FP35_NATIVE_OTHER,profilePart,result);
  if(!result){G_Printf("[native frame] tick=%d failed phase=%d\n",now,gscFrameCursor.nativeStage);return 0;}
  if(result==2)return 2;
  if(gscFrameCursor.nativeStage==NATIVE_EXCLUSIVE)return 1;
  ++gscFrameCursor.nativeStage;
  if(expired && expired())return 2;
 }
}
