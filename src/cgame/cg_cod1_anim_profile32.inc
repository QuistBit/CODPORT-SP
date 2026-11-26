/* Native Windows CGame observer; no renderer-thread access, no heap. */
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../qcommon/cod1_anim_profile32.h"
static c1ap32Window cgAnim32;
static uint64_t cgAnim32Hz,cgAnim32Revision;
static int cgAnim32Frame,cgAnim32Known,cgAnim32Enabled;
static void CGAP32_Print(const c1ap32Window *w){
 static const char *names[C1AP32_STAGES]={"actor","resource","pose_hit","pose_miss","source","clip_load","bone_map","tree_sample","skeleton","blend","submit"};
 unsigned i;
 CG_Printf("[OPT_ANIM_CG] revision=%llu first=%d last=%d hz=%llu frames=%u actors=%u decoded_ladder_actors=%u max_nodes=%u max_leaves=%u ladder_mask=%u\n",
  (unsigned long long)w->revision,w->first,w->last,(unsigned long long)cgAnim32Hz,w->frames,w->actors,w->decodedLadderActors,w->maxNodes,w->maxLeaves,w->ladderMask);
 for(i=0;i<C1AP32_STAGES;i++)if(w->stage[i].calls){const c1ap32Stage *s=w->stage+i;
  CG_Printf("[OPT_ANIM_CG] stage=%s revision=%llu first=%d last=%d calls=%u failed=%u sum_ticks=%llu max_frame_ticks=%llu max_call_ticks=%llu\n",
   names[i],(unsigned long long)w->revision,w->first,w->last,s->calls,s->failed,(unsigned long long)s->sum,(unsigned long long)s->maxFrame,(unsigned long long)s->maxCall);
 }
}
static void CGAP32_Frame(void){
 LARGE_INTEGER q;c1ap32Window completed;char value[16]={0};int enabled;
 if(cgAnim32Known&&cgAnim32Revision==cgfa4Revision&&cgAnim32Frame==cg.clientFrame)return;
 trap_Cvar_VariableStringBuffer("cg_cod1AnimProfile",value,sizeof(value));
 enabled=!strcmp(value,"1");
 if(cgAnim32Revision!=cgfa4Revision||enabled!=cgAnim32Enabled)C1AP32_Reset(&cgAnim32);
 cgAnim32Known=1;cgAnim32Revision=cgfa4Revision;cgAnim32Frame=cg.clientFrame;cgAnim32Enabled=enabled;
 if(!enabled)return;
 if(!cgAnim32Hz&&QueryPerformanceFrequency(&q)&&q.QuadPart>0)cgAnim32Hz=(uint64_t)q.QuadPart;
 if(cgAnim32Hz&&C1AP32_Frame(&cgAnim32,&completed,cgfa4Revision,cg.time,cg.clientFrame))CGAP32_Print(&completed);
}
static uint64_t CGAP32_Start(void){LARGE_INTEGER q;
 if(!cgAnim32Enabled||!cgAnim32Hz||cgAnim32Revision!=cgfa4Revision||!cgAnim32.frames||cgAnim32.frame!=cg.clientFrame||!QueryPerformanceCounter(&q)||q.QuadPart<=0)return 0;
 return (uint64_t)q.QuadPart;
}
static void CGAP32_End(unsigned stage,uint64_t start,int ok,unsigned owner){LARGE_INTEGER q;
 if(!start||!cgAnim32Enabled||cgAnim32Revision!=cgfa4Revision||cgAnim32Frame!=cg.clientFrame||cgAnim32.frame!=cg.clientFrame||!QueryPerformanceCounter(&q)||q.QuadPart<=0||(uint64_t)q.QuadPart<start)return;
 C1AP32_Record(&cgAnim32,stage,(uint64_t)q.QuadPart-start,ok,owner);
}
static void CGAP32_Decoded(unsigned owner,const c1asp14Decoded_t *d){unsigned i,ladder=0;
 if(!cgAnim32Enabled||!cgAnim32Hz||cgAnim32Revision!=cgfa4Revision||cgAnim32Frame!=cg.clientFrame||!cgAnim32.frames||cgAnim32.frame!=cg.clientFrame)return;
 for(i=0;i<d->count;i++)ladder|=C1AP32_Ladder(d->node[i].ordinal);
 C1AP32_Decoded(&cgAnim32,owner,d->count,d->leaves,ladder);
}
#else
#define CGAP32_Frame() ((void)0)
#define CGAP32_Start() ((uint64_t)0)
#define CGAP32_End(stage,start,ok,owner) ((void)0)
#define CGAP32_Decoded(owner,d) ((void)0)
#endif
/* Preserve the original sampler and skeleton builder, including their failures. */
static int CGAP32_Tree(const c1ast14Bank_t *b,const c1ast14Node_t *n,unsigned count,c1ab14Accumulator_t *out,unsigned capacity){
 uint64_t start=CGAP32_Start();int ok=C1AST14_Sample(b,n,count,out,capacity);
 if(start)CGAP32_End(C1AP32_TREE,start,ok,0);
 return ok;
}
static int CGAP32_Skeleton(const c1ad14Resource_t *r,const c1ab14Accumulator_t *a,unsigned n,c1aw14Pose_t *out){
 uint64_t start=CGAP32_Start();int ok=C1AD14_Build(r,a,n,out);
 if(start)CGAP32_End(C1AP32_SKELETON,start,ok,0);
 return ok;
}
