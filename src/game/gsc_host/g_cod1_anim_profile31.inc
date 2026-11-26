/* GAME-only opt-in observer. Called on the existing server owner thread. */
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../../qcommon/cod1_anim_profile31.h"
static c1ap31Window actorProfile31[2];
static uint64_t actorProfileEpoch31,actorProfileHz31;
static int actorProfileFrame31,actorProfileEnabled31,actorProfileKnown31;
static uint64_t ActorProfileStart31(void){
 LARGE_INTEGER q;
 if(!actorProfileKnown31||actorProfileEpoch31!=cod1EntityEpoch||actorProfileFrame31!=level.framenum){
  int enabled=trap_Cvar_VariableIntegerValue("g_cod1AnimProfile")==1;
  if(actorProfileEpoch31!=cod1EntityEpoch||enabled!=actorProfileEnabled31)
   memset(actorProfile31,0,sizeof(actorProfile31));
  actorProfileKnown31=1;actorProfileEpoch31=cod1EntityEpoch;actorProfileFrame31=level.framenum;
  actorProfileEnabled31=enabled;
  if(enabled&&!actorProfileHz31&&QueryPerformanceFrequency(&q)&&q.QuadPart>0)
   actorProfileHz31=(uint64_t)q.QuadPart;
 }
 if(!actorProfileEnabled31||!actorProfileHz31||!QueryPerformanceCounter(&q)||q.QuadPart<=0)return 0;
 return (uint64_t)q.QuadPart;
}
static void ActorProfileFinish31(unsigned stage,uint64_t start,uint64_t epoch,int now,unsigned owner,
 int ok,unsigned iterations,unsigned nodes,unsigned leaves,unsigned ladder){
 LARGE_INTEGER q;c1ap31Window complete;
 if(!start||stage>=2||epoch!=cod1EntityEpoch||!QueryPerformanceCounter(&q)||q.QuadPart<=0||(uint64_t)q.QuadPart<start)return;
 if(C1AP31_Record(actorProfile31+stage,&complete,epoch,now,owner,(uint64_t)q.QuadPart-start,ok,iterations,nodes,leaves,ladder))
  G_Printf("[OPT_ANIM] stage=%s epoch=%llu first=%d last=%d hz=%llu calls=%u failed=%u actors=%u ladder_actors=%u sum_ticks=%llu max_tick_ticks=%llu max_call_ticks=%llu iterations=%llu max_iter=%u max_nodes=%u max_leaves=%u ladder_mask=%u\n",
   stage?"pose":"controller",(unsigned long long)complete.epoch,complete.first,complete.last,(unsigned long long)actorProfileHz31,
   complete.calls,complete.failures,complete.actors,complete.ladderActors,(unsigned long long)complete.sum,
   (unsigned long long)complete.maxTick,(unsigned long long)complete.maxCall,(unsigned long long)complete.iterations,
   complete.maxIterations,complete.maxNodes,complete.maxLeaves,complete.ladderMask);
}
#else
#define ActorProfileStart31() ((uint64_t)0)
#define ActorProfileFinish31(stage,start,epoch,now,owner,ok,iterations,nodes,leaves,ladder) ((void)0)
#define C1AP31_Ladder(ordinal) 0u
#endif
