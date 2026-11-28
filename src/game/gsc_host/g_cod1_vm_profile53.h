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
 static int known,frame,enabled,shadow,dirty,indexed,source_detail,source_breakdown,last[GSC53_LANES];
 static struct {uint64_t ticks,checks,matches,mismatches,skipped,rows;} queueTotals[GSC53_LANES];
 static struct {uint64_t ticks,checks,matches,mismatches,skipped,feeds,rows,reseeds,lookups,probes,map_rows,misses,indexed_choices,indexed_fallbacks;} dirtyTotals[GSC53_LANES];
 static const char *names[GSC53_LANES]={"note","current","exclusive"};
 uint64_t e=G_Cod1EntityMapEpoch(),start,end,elapsed;
 int now=level.time,ok;
 GscR237Profile53 p={0};LARGE_INTEGER q;
 if(!known||epoch!=e){
  unsigned i;for(i=0;i<GSC53_LANES;i++)last[i]=-1;
  memset(queueTotals,0,sizeof(queueTotals));memset(dirtyTotals,0,sizeof(dirtyTotals));epoch=e;known=0;
 }
 if(!known||frame!=level.framenum){
  enabled=trap_Cvar_VariableIntegerValue("g_cod1VmProfile")==1;
  shadow=enabled&&trap_Cvar_VariableIntegerValue("g_cod1QueueShadow")==1;
  dirty=enabled&&trap_Cvar_VariableIntegerValue("g_cod1QueueDirty")==1;
  indexed=dirty&&trap_Cvar_VariableIntegerValue("g_cod1QueueVerified")==1;
  source_breakdown=enabled&&trap_Cvar_VariableIntegerValue("g_cod1VmBreakdown")==1;
  source_detail=enabled&&(source_breakdown||trap_Cvar_VariableIntegerValue("g_cod1VmSource")==1);
  frame=level.framenum;known=1;
 }
 if(enabled&&!hz&&QueryPerformanceFrequency(&q)&&q.QuadPart>0)hz=(uint64_t)q.QuadPart;
 if(!enabled||!hz||lane>=GSC53_LANES)return gsc_session_r237_frame_phase_slice(s,id,generation,exclusive,expired,error,cap);
 p.clock=GscProfileClock53;p.hz=hz;p.queue_shadow=shadow;p.queue_dirty=dirty;p.queue_indexed=indexed;p.source_detail=source_detail;p.source_breakdown=source_breakdown;start=GscProfileClock53();
 ok=gsc_session_r237_frame_phase_profile53(s,id,generation,exclusive,expired,error,cap,&p);
 end=GscProfileClock53();
 if(!start||!end||end<start||e!=G_Cod1EntityMapEpoch()||now!=level.time)return ok;
 elapsed=end-start;
 if(shadow){
  queueTotals[lane].ticks+=p.ticks[GSC53_QUEUE_SHADOW];
  queueTotals[lane].checks+=p.shadow_checks;queueTotals[lane].matches+=p.shadow_matches;
  queueTotals[lane].mismatches+=p.shadow_mismatches;queueTotals[lane].skipped+=p.shadow_skipped;
  queueTotals[lane].rows+=p.shadow_rows;
 }
 /* One line per slow phase (>=8ms), or a periodic sample per lane. Never
  * print per task. Log outside the measured Session call. */
 if(dirty){
  dirtyTotals[lane].ticks+=p.ticks[GSC53_QUEUE_DIRTY];dirtyTotals[lane].checks+=p.dirty_checks;
  dirtyTotals[lane].matches+=p.dirty_matches;dirtyTotals[lane].mismatches+=p.dirty_mismatches;
  dirtyTotals[lane].skipped+=p.dirty_skipped;dirtyTotals[lane].feeds+=p.dirty_feeds;
  dirtyTotals[lane].rows+=p.dirty_audit_rows;dirtyTotals[lane].reseeds+=p.dirty_reseeds;
  dirtyTotals[lane].lookups+=p.dirty_mark_lookups;dirtyTotals[lane].probes+=p.dirty_mark_probes;
  dirtyTotals[lane].map_rows+=p.dirty_map_rows;dirtyTotals[lane].misses+=p.dirty_mark_misses;
  dirtyTotals[lane].indexed_choices+=p.indexed_choices;dirtyTotals[lane].indexed_fallbacks+=p.indexed_fallbacks;
 }
 if(elapsed>=hz/125||!ok||p.clock_failures||p.shadow_mismatches||p.dirty_mismatches||p.dirty_mark_misses||p.indexed_fallbacks||last[lane]<0||now<last[lane]||(int64_t)now-last[lane]>=1000){
  last[lane]=now;
  G_Printf("[OPT_VM53] lane=%s epoch=%llu now=%d gen=%llu hz=%llu result=%d total=%llu scan=%llu dispatch=%llu source=%llu endon=%llu array=%llu scans=%llu examined=%llu tasks=%u dispatched=%llu budget_spent=%llu source_calls=%llu endon_calls=%llu array_calls=%llu clock_failures=%llu session_admit=%llu session_pre=%llu session_post=%llu session_finish=%llu task_guard=%llu task_guard_calls=%llu\n",
   names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,ok,
   (unsigned long long)elapsed,(unsigned long long)p.ticks[GSC53_SCAN],(unsigned long long)p.ticks[GSC53_DISPATCH],
   (unsigned long long)p.ticks[GSC53_SOURCE],(unsigned long long)p.ticks[GSC53_ENDON],(unsigned long long)p.ticks[GSC53_ARRAY],
   (unsigned long long)p.scans,(unsigned long long)p.examined,p.max_tasks,(unsigned long long)p.dispatches,
   (unsigned long long)p.budget_spent,(unsigned long long)p.calls[GSC53_SOURCE],(unsigned long long)p.calls[GSC53_ENDON],
   (unsigned long long)p.calls[GSC53_ARRAY],(unsigned long long)p.clock_failures,
   (unsigned long long)p.ticks[GSC53_SESSION_ADMIT],(unsigned long long)p.ticks[GSC53_SESSION_PRE],
   (unsigned long long)p.ticks[GSC53_SESSION_POST],(unsigned long long)p.ticks[GSC53_SESSION_FINISH],
   (unsigned long long)p.ticks[GSC53_TASK_GUARD],(unsigned long long)p.calls[GSC53_TASK_GUARD]);
  if(source_detail&&p.source_worst.ticks){
   const GscSourceSample55 *w=&p.source_worst;
   G_Printf("[OPT_SOURCE55] lane=%s epoch=%llu now=%d gen=%llu hz=%llu ticks=%llu serial=%llu depth=%d known=%d module=%s function=%s fn=%d pc=%d line=%d budget_spent=%llu\n",
    names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,
    (unsigned long long)w->ticks,(unsigned long long)w->serial,w->depth,w->known,w->known?w->module:"?",w->known?w->function:"?",w->fn,w->pc,w->line,(unsigned long long)w->budget_spent);
  }
  if(source_breakdown&& (p.breakdown.sources||p.breakdown.legacy_sources)){
   const GscBreakdown55 *b=&p.breakdown;
   G_Printf("[OPT_BREAK55] lane=%s epoch=%llu now=%d gen=%llu hz=%llu sources=%llu legacy=%llu failures=%llu guard=%llu guard_calls=%llu guard_outer=%llu eval=%llu eval_calls=%llu eval_outer=%llu native_expr=%llu native_expr_calls=%llu native_expr_outer=%llu native_stmt=%llu native_stmt_calls=%llu native_stmt_outer=%llu sound=%llu sound_calls=%llu sound_outer=%llu\n",
    names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,
    (unsigned long long)b->sources,(unsigned long long)b->legacy_sources,(unsigned long long)b->failures,
    (unsigned long long)b->ticks[R55B_GUARD],(unsigned long long)b->calls[R55B_GUARD],(unsigned long long)b->outer[R55B_GUARD],
    (unsigned long long)b->ticks[R55B_EVAL],(unsigned long long)b->calls[R55B_EVAL],(unsigned long long)b->outer[R55B_EVAL],
    (unsigned long long)b->ticks[R55B_NATIVE_EXPR],(unsigned long long)b->calls[R55B_NATIVE_EXPR],(unsigned long long)b->outer[R55B_NATIVE_EXPR],
    (unsigned long long)b->ticks[R55B_NATIVE_STMT],(unsigned long long)b->calls[R55B_NATIVE_STMT],(unsigned long long)b->outer[R55B_NATIVE_STMT],
    (unsigned long long)b->ticks[R56A_SOUND],(unsigned long long)b->calls[R56A_SOUND],(unsigned long long)b->outer[R56A_SOUND]);
  }
  if(shadow)G_Printf("[OPT_QUEUE54] scope=epoch lane=%s epoch=%llu now=%d gen=%llu hz=%llu ticks=%llu checks=%llu matches=%llu mismatches=%llu skipped=%llu rows=%llu\n",
   names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,
   (unsigned long long)queueTotals[lane].ticks,(unsigned long long)queueTotals[lane].checks,(unsigned long long)queueTotals[lane].matches,
   (unsigned long long)queueTotals[lane].mismatches,(unsigned long long)queueTotals[lane].skipped,(unsigned long long)queueTotals[lane].rows);
  if(dirty)G_Printf("[OPT_DIRTY54] scope=epoch lane=%s epoch=%llu now=%d gen=%llu hz=%llu ticks=%llu checks=%llu matches=%llu mismatches=%llu skipped=%llu feeds=%llu audit_rows=%llu reseeds=%llu mark_lookups=%llu mark_probes=%llu map_rows=%llu mark_misses=%llu indexed_choices=%llu indexed_fallbacks=%llu\n",
   names[lane],(unsigned long long)e,now,(unsigned long long)generation,(unsigned long long)hz,
   (unsigned long long)dirtyTotals[lane].ticks,(unsigned long long)dirtyTotals[lane].checks,
   (unsigned long long)dirtyTotals[lane].matches,(unsigned long long)dirtyTotals[lane].mismatches,
   (unsigned long long)dirtyTotals[lane].skipped,(unsigned long long)dirtyTotals[lane].feeds,
   (unsigned long long)dirtyTotals[lane].rows,(unsigned long long)dirtyTotals[lane].reseeds,
   (unsigned long long)dirtyTotals[lane].lookups,(unsigned long long)dirtyTotals[lane].probes,
   (unsigned long long)dirtyTotals[lane].map_rows,(unsigned long long)dirtyTotals[lane].misses,
   (unsigned long long)dirtyTotals[lane].indexed_choices,(unsigned long long)dirtyTotals[lane].indexed_fallbacks);
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
