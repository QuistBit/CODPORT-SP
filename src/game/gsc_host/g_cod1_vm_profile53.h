#ifndef G_COD1_VM_PROFILE53_H
#define G_COD1_VM_PROFILE53_H
#include "../cod1_gsc/gsc_r237_profile53.h"
enum { GSC53_NOTE, GSC53_CURRENT, GSC53_EXCLUSIVE, GSC53_LANES };
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP && defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
/* windows.h is provided by g_cod1_frame_profile35.h. Same GAME owner thread. */
static uint64_t GscProfileClock53(void){
 LARGE_INTEGER q;return QueryPerformanceCounter(&q)&&q.QuadPart>0?(uint64_t)q.QuadPart:0;
}
static int GscProfilePhase53(GscSession *s,uint64_t id,uint64_t generation,int exclusive,
 int (*expired)(void),char *error,size_t cap,unsigned lane){
 static uint64_t epoch,hz;
 static int known,frame,enabled,last[GSC53_LANES];
 static const char *names[GSC53_LANES]={"note","current","exclusive"};
 uint64_t e=G_Cod1EntityMapEpoch(),start,end,elapsed;
 int now=level.time,ok;
 GscR237Profile53 p={0};LARGE_INTEGER q;
 if(!known||epoch!=e){
  unsigned i;for(i=0;i<GSC53_LANES;i++)last[i]=-1;
  epoch=e;known=0;
 }
 if(!known||frame!=level.framenum){
  enabled=trap_Cvar_VariableIntegerValue("g_cod1VmProfile")==1;frame=level.framenum;known=1;
 }
 if(enabled&&!hz&&QueryPerformanceFrequency(&q)&&q.QuadPart>0)hz=(uint64_t)q.QuadPart;
 if(!enabled||!hz||lane>=GSC53_LANES)return gsc_session_r237_frame_phase_slice(s,id,generation,exclusive,expired,error,cap);
 p.clock=GscProfileClock53;p.hz=hz;start=GscProfileClock53();
 ok=gsc_session_r237_frame_phase_profile53(s,id,generation,exclusive,expired,error,cap,&p);
 end=GscProfileClock53();
 if(!start||!end||end<start||e!=G_Cod1EntityMapEpoch()||now!=level.time)return ok;
 elapsed=end-start;
 /* One line per slow phase (>=8ms), or a periodic sample per lane. Never
  * print per task. Log outside the measured Session call. */
 if(elapsed>=hz/125||!ok||p.clock_failures||last[lane]<0||now<last[lane]||(int64_t)now-last[lane]>=1000){
  last[lane]=now;
  G_Printf("[OPT_VM53] lane=%s epoch=%llu now=%d gen=%llu hz=%llu result=%d total=%llu scan=%llu dispatch=%llu source=%llu endon=%llu array=%llu scans=%llu examined=%llu tasks=%u dispatched=%llu budget_spent=%llu source_calls=%llu endon_calls=%llu array_calls=%llu clock_failures=%llu\n",
   names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,ok,
   (unsigned long long)elapsed,(unsigned long long)p.ticks[GSC53_SCAN],(unsigned long long)p.ticks[GSC53_DISPATCH],
   (unsigned long long)p.ticks[GSC53_SOURCE],(unsigned long long)p.ticks[GSC53_ENDON],(unsigned long long)p.ticks[GSC53_ARRAY],
   (unsigned long long)p.scans,(unsigned long long)p.examined,p.max_tasks,(unsigned long long)p.dispatches,
   (unsigned long long)p.budget_spent,(unsigned long long)p.calls[GSC53_SOURCE],(unsigned long long)p.calls[GSC53_ENDON],
   (unsigned long long)p.calls[GSC53_ARRAY],(unsigned long long)p.clock_failures);
 }
 return ok;
}
#else
static int GscProfilePhase53(GscSession *s,uint64_t id,uint64_t generation,int exclusive,
 int (*expired)(void),char *error,size_t cap,unsigned lane){
 (void)lane;return gsc_session_r237_frame_phase_slice(s,id,generation,exclusive,expired,error,cap);
}
#endif
#endif
