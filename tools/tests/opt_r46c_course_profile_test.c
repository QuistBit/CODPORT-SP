/* Exercise the actual profiling wrappers and kernel seam with controlled
 * providers. No native QPC, VM or real actor/resource calculations. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define C1AP31_OWNERS 1024u
enum {C1AP46_COURSE_BEGIN=11,C1AP46_COURSE_STEP,C1AP46_COURSE_DRAIN,C1AP46_COURSE_SAMPLE,C1AP46_COURSE_BUCKET,C1AP46_COURSE_NOTE,C1AP46_CLOCK_KERNEL};
typedef struct {uint64_t epoch;unsigned slot,generation;} GscEntityRef;
typedef struct {GscEntityRef entity;} cod1ActorLease_t;
typedef struct {GscEntityRef entity;int produced_ms;} cod1ActorFaceNote_t;
typedef int(*cod1ActorFaceNoteCallback)(void*,const cod1ActorFaceNote_t*);
static uint64_t cod1EntityEpoch=5;
static struct {int time,framenum;} level={100,2};
static unsigned enabled,starts,finishes,calls,callbacks,deliveries,trace[40],nt;
static int result=1,change;
static GscEntityRef ref={5,7,9};static const uint64_t gen=17;
static cod1ActorLease_t lease;
static cod1ActorFaceNote_t note;
static int resume,retired;
static int context;
static unsigned recordedStage,recordedOwner;static int recordedNow,recordedOK;
static uint64_t recordedEpoch,recordedStart;
static float weights[98],phases[2][21],facial[2][25];
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
static uint64_t ActorProfileStart31(void){++starts;return enabled?123:0;}
static void ActorProfileFinish31(unsigned stage,uint64_t start,uint64_t epoch,int now,unsigned owner,int ok,unsigned iterations,unsigned nodes,unsigned leaves,unsigned ladder){
 ++finishes;trace[nt++]=100+stage;recordedStage=stage;recordedStart=start;recordedEpoch=epoch;recordedNow=now;recordedOwner=owner;recordedOK=ok;assert(!iterations&&!nodes&&!leaves&&!ladder);
}
#endif
static int current(void*p){assert(p==&context);++callbacks;trace[nt++]=2;return 1;}
static int deliver(void*p,const cod1ActorFaceNote_t*n){assert(p==&context&&n==&note);++deliveries;trace[nt++]=3;return 1;}
static void mutation(void){switch(change){case 1:++cod1EntityEpoch;break;case 2:++level.time;break;case 3:++level.framenum;break;case 4:++ref.slot;break;}}
static void check(const GscEntityRef*r,int now,uint64_t generation,int(*cb)(void*),void*p){assert(r==&ref&&now==100&&generation==gen&&cb==current&&p==&context);++calls;trace[nt++]=1;assert(cb(p)==1);mutation();}
static int G_Cod1ActorFaceFrameBegin(const GscEntityRef*r,int now,uint64_t generation,int(*cb)(void*),void*p){check(r,now,generation,cb,p);return result;}
static int G_Cod1ActorFaceFrameStep(const GscEntityRef*r,int now,uint64_t generation,int*out,int(*cb)(void*),void*p){assert(out==&resume);check(r,now,generation,cb,p);if(result)*out=27;return result;}
static int G_Cod1ActorFaceFrameDrain(const GscEntityRef*r,int now,uint64_t generation,int(*cb)(void*),cod1ActorFaceNoteCallback d,void*p){check(r,now,generation,cb,p);assert(d==deliver);if(result)assert(d(p,&note));return result;}
static int G_Cod1ActorScriptedSampleFull(const GscEntityRef*r,int now,float*w,float a[2][21],float b[2][25],int(*cb)(void*),void*p){assert(w==weights&&a==phases&&b==facial);check(r,now,gen,cb,p);if(result)w[97]=.25f;return result;}
static int FoleyRunNoteBucket(const GscEntityRef*r,int now,uint64_t generation,const cod1ActorLease_t*l,int*out){assert(r==&ref&&now==100&&generation==gen&&l==&lease&&out==&retired);++calls;trace[nt++]=1;mutation();if(result)*out=1;return result;}
static int FoleySessionQueueFrameNote(const cod1ActorFaceNote_t*n){assert(n==&note);++calls;trace[nt++]=1;mutation();return result;}
#include "g_cod1_course_profile46.h"
/* Match actual kernel wrapper parameter forwarding without running the math. */
#include "g_cod1_phase_profile46.h"
typedef struct {int flags[2];} Notify;
typedef struct {cod1ActorLease_t lease;int*nodes;Notify*notify;} C1GARecord;
typedef struct {unsigned count;} c1agc14Step_t;
static int cod1GenericDefinitions[2];
static struct {int topology,metadata[2];float frequency[2];int metadataRanges;} cod1LocoResources;
static Notify notify;static int nodes[2];static C1GARecord r;static c1agc14Step_t step;
static int C1AGC14_AdvanceDisposablePrepared(const int*d,int*n,int*f,const int*t,const int*m,const float*frequency,float dt,int owner,int send,c1agc14Step_t*s,const int*index){
 assert(d==cod1GenericDefinitions&&n==nodes&&f==notify.flags&&t==&cod1LocoResources.topology&&m==cod1LocoResources.metadata&&frequency==cod1LocoResources.frequency&&dt==.05f&&owner==1&&send==1&&s==&step&&index==&cod1LocoResources.metadataRanges);++calls;trace[nt++]=1;mutation();if(result)s->count=3;return result;
}
#include "kernel_profile.inc"
static void reset(void){starts=finishes=calls=callbacks=deliveries=nt=0;recordedStage=recordedOwner=0;recordedNow=recordedOK=0;recordedEpoch=recordedStart=0;cod1EntityEpoch=5;level.time=100;level.framenum=2;ref.slot=7;lease.entity=ref;note.entity=ref;note.produced_ms=100;resume=retired=-1;weights[97]=-1;step.count=0;r.lease=lease;r.nodes=nodes;r.notify=&notify;}
static int invoke(unsigned stage){switch(stage){case 0:return CourseClockBegin46(&ref,100,gen,current,&context);case 1:return CourseClockStep46(&ref,100,gen,&resume,current,&context);case 2:return CourseClockDrain46(&ref,100,gen,current,deliver,&context);case 3:return CourseClockSample46(&ref,100,weights,phases,facial,current,&context);case 4:return CourseClockBucket46(&ref,100,gen,&lease,&retired);case 5:return CourseClockQueue46(&note);default:return ActorLocomotionKernel46(&r,.05f,&step);}}
int main(void){unsigned s,e,k,checks=0;
 for(s=0;s<7;s++)for(e=0;e<2;e++)for(k=0;k<2;k++){
  reset();enabled=e;result=(int)k;change=0;assert(invoke(s)==result&&calls==1&&callbacks==(s<4)&&deliveries==(s==2&&k));assert(trace[0]==1);if(s<4)assert(trace[1]==2);if(s==2&&k)assert(trace[2]==3);
  assert(resume==((s==1&&k)?27:-1)&&retired==((s==4&&k)?1:-1)&&weights[97]==((s==3&&k)?.25f:-1)&&step.count==((s==6&&k)?3u:0u));
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
  assert(starts==1&&finishes==e);if(e)assert(recordedStage==C1AP46_COURSE_BEGIN+s&&recordedEpoch==5&&recordedNow==100&&recordedOwner==7&&recordedOK==result&&recordedStart==123&&trace[nt-1]==100+recordedStage);
#else
  assert(!starts&&!finishes);
#endif
  ++checks;
 }
 for(s=0;s<7;s++)for(k=1;k<=4;k++){
  reset();enabled=1;result=1;change=(int)k;assert(invoke(s)==1&&calls==1);
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
  assert(finishes==(k==4));if(k==4)assert(recordedOwner==7);
#else
  assert(!finishes);
#endif
  ++checks;
 }
 printf("R46C profile forwarding: %u cases PASS\n",checks);return 0;
}
