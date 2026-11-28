#ifndef COD1_FRAME_PROFILE35_H
#define COD1_FRAME_PROFILE35_H
#include <stdint.h>
#include <string.h>
/* Independent observer in GAME main and GSC host translation units.
 * Measure active work only, never elapsed waiting time between slices. */
enum { C1FP35_GAME, C1FP35_GSC, C1FP35_ENTITIES, C1FP35_AI,
 C1FP35_GAME_POST, C1FP35_NATIVE, C1FP35_COURSE, C1FP35_VM_CURRENT,
 C1FP35_VM_EXCLUSIVE, C1FP35_NATIVE_OTHER, C1FP35_ACTORS,
 C1FP35_BUCKET_ADMIT, C1FP35_BUCKET_VM, C1FP35_BUCKET_POST, C1FP35_STAGES };
typedef struct {uint64_t stamp,epoch;int tick;} c1fp35Token;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP && defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
typedef struct {uint64_t sum,tickSum,maxTick,maxCall;unsigned calls,paused,failed;} c1fp35Stage;
static struct {
 uint64_t epoch,hz;
 int known,enabled,first,last;
 unsigned ticks;
 c1fp35Stage stage[C1FP35_STAGES];
} c1fp35;
static void C1FP35_Flush(void){
 static const char *names[C1FP35_STAGES]={"game","gsc","entities","ai","game_post","native","course","vm_current","vm_exclusive","native_other","actors","bucket_admit","bucket_vm","bucket_post"};
 unsigned i;
 for(i=0;i<C1FP35_STAGES;i++)if(c1fp35.stage[i].calls){
  const c1fp35Stage *s=c1fp35.stage+i;
  G_Printf("[OPT_FRAME] stage=%s epoch=%llu first=%d last=%d hz=%llu ticks=%u calls=%u paused=%u failed=%u sum_ticks=%llu max_tick_ticks=%llu max_call_ticks=%llu\n",
   names[i],(unsigned long long)c1fp35.epoch,c1fp35.first,c1fp35.last,(unsigned long long)c1fp35.hz,c1fp35.ticks,s->calls,s->paused,s->failed,
   (unsigned long long)s->sum,(unsigned long long)s->maxTick,(unsigned long long)s->maxCall);
 }
}
static c1fp35Token C1FP35_Begin(int now){
 c1fp35Token t={0};LARGE_INTEGER q;uint64_t epoch=G_Cod1EntityMapEpoch();unsigned i;
 if(!epoch||now<0)return t;
 if(!c1fp35.known||c1fp35.epoch!=epoch||c1fp35.last!=now){
  int enabled=trap_Cvar_VariableIntegerValue("g_cod1FrameProfile")==1;
  if(!c1fp35.known||c1fp35.epoch!=epoch||now<c1fp35.last||enabled!=c1fp35.enabled){
   memset(&c1fp35,0,sizeof(c1fp35));c1fp35.epoch=epoch;c1fp35.known=1;c1fp35.enabled=enabled;
  }
  if(enabled){
   if(c1fp35.ticks&&(int64_t)now-c1fp35.first>=250){C1FP35_Flush();memset(c1fp35.stage,0,sizeof(c1fp35.stage));c1fp35.ticks=0;}
   if(!c1fp35.ticks)c1fp35.first=now;
   c1fp35.ticks++;
   for(i=0;i<C1FP35_STAGES;i++)c1fp35.stage[i].tickSum=0;
  }
  c1fp35.last=now;
 }
 if(!c1fp35.enabled)return t;
 if(!c1fp35.hz&&QueryPerformanceFrequency(&q)&&q.QuadPart>0)c1fp35.hz=(uint64_t)q.QuadPart;
 if(!c1fp35.hz||!QueryPerformanceCounter(&q)||q.QuadPart<=0)return t;
 t.stamp=(uint64_t)q.QuadPart;t.epoch=epoch;t.tick=now;return t;
}
/* status: 0 refused, 1 completed, 2 paused (not a failure). */
static void C1FP35_End(unsigned stage,c1fp35Token t,int status){
 LARGE_INTEGER q;c1fp35Stage *s;uint64_t elapsed;
 if(!t.stamp||stage>=C1FP35_STAGES||!c1fp35.enabled||t.epoch!=c1fp35.epoch||t.tick!=c1fp35.last||t.epoch!=G_Cod1EntityMapEpoch()||!QueryPerformanceCounter(&q)||q.QuadPart<=0||(uint64_t)q.QuadPart<t.stamp)return;
 elapsed=(uint64_t)q.QuadPart-t.stamp;s=c1fp35.stage+stage;
 s->calls++;if(status==2)s->paused++;else if(!status)s->failed++;
 s->sum+=elapsed;s->tickSum+=elapsed;
 if(s->tickSum>s->maxTick)s->maxTick=s->tickSum;
 if(elapsed>s->maxCall)s->maxCall=elapsed;
}
#else
static c1fp35Token C1FP35_Begin(int now){c1fp35Token t={0};(void)now;return t;}
static void C1FP35_End(unsigned stage,c1fp35Token t,int status){(void)stage;(void)t;(void)status;}
#endif
#endif
