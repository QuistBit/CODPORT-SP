#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "r31_include/windows.h"
static uint64_t cgfa4Revision=1;
static struct {int clientFrame,time;} cg;
static int enabled,queries,counters,frequencies,prints,badClock,treeOK=1,buildOK=1;
static int64_t clockValue=100;
static char logText[8192];
static int QueryPerformanceCounter(LARGE_INTEGER *q){counters++;q->QuadPart=clockValue;return !badClock;}
static int QueryPerformanceFrequency(LARGE_INTEGER *q){frequencies++;q->QuadPart=1000;return 1;}
static void trap_Cvar_VariableStringBuffer(const char *name,char *out,int n){assert(!strcmp(name,"cg_cod1AnimProfile"));queries++;snprintf(out,n,"%d",enabled);}
static void CG_Printf(const char *fmt,...){va_list ap;size_t used=strlen(logText);va_start(ap,fmt);vsnprintf(logText+used,sizeof(logText)-used,fmt,ap);va_end(ap);prints++;}
typedef struct {unsigned ordinal;} c1ast14Node_t;
typedef struct {unsigned count,leaves;c1ast14Node_t node[60];} c1asp14Decoded_t;
typedef int c1ast14Bank_t;typedef int c1ab14Accumulator_t;typedef int c1ad14Resource_t;typedef int c1aw14Pose_t;
static int C1AST14_Sample(const c1ast14Bank_t *b,const c1ast14Node_t *n,unsigned count,c1ab14Accumulator_t *out,unsigned capacity){(void)b;(void)n;(void)count;(void)out;(void)capacity;clockValue+=7;return treeOK;}
static int C1AD14_Build(const c1ad14Resource_t *r,const c1ab14Accumulator_t *a,unsigned n,c1aw14Pose_t *out){(void)r;(void)a;(void)n;(void)out;clockValue+=5;return buildOK;}
#define _WIN32 1
#define _MSC_VER 1930
#define _M_IX86 600
#include "../../src/cgame/cg_cod1_anim_profile32.inc"
int main(void){
 uint64_t start;c1asp14Decoded_t decoded={2,1,{{318},{321}}};c1ap32Window w={0},out={0};
 CGAP32_Frame();CGAP32_Frame();assert(queries==1&&!CGAP32_Start()&&!counters&&!frequencies);
 assert(CGAP32_Tree(NULL,NULL,0,NULL,0)==1&&counters==0);
 enabled=1;cg.clientFrame=1;cg.time=100;CGAP32_Frame();assert(queries==2&&frequencies==1);
 start=CGAP32_Start();clockValue+=10;CGAP32_End(C1AP32_ACTOR,start,1,7);
 start=CGAP32_Start();clockValue+=20;CGAP32_End(C1AP32_ACTOR,start,0,8);
 CGAP32_Decoded(7,&decoded);CGAP32_Decoded(7,&decoded);
 assert(CGAP32_Tree(NULL,NULL,0,NULL,0)==1);treeOK=0;assert(CGAP32_Tree(NULL,NULL,0,NULL,0)==0);
 assert(CGAP32_Skeleton(NULL,NULL,0,NULL)==1);buildOK=0;assert(CGAP32_Skeleton(NULL,NULL,0,NULL)==0);
 cg.clientFrame++;CGAP32_Frame();assert(cgAnim32.frames==2); /* same cg.time, different frame */
 start=CGAP32_Start();clockValue+=25;CGAP32_End(C1AP32_ACTOR,start,1,7);
 assert(cgAnim32.stage[C1AP32_ACTOR].maxFrame==30&&cgAnim32.stage[C1AP32_ACTOR].sum==55&&cgAnim32.actors==2);
 assert(cgAnim32.decodedLadderActors==1&&cgAnim32.ladderMask==9&&cgAnim32.maxNodes==2);
 cg.clientFrame++;cg.time=350;CGAP32_Frame();assert(prints==4&&strstr(logText,"frames=2 actors=2 decoded_ladder_actors=1")&&strstr(logText,"stage=actor")&&strstr(logText,"sum_ticks=55 max_frame_ticks=30")&&strstr(logText,"stage=tree_sample")&&strstr(logText,"stage=skeleton"));
 enabled=0;cg.clientFrame++;CGAP32_Frame();assert(!CGAP32_Start()&&!cgAnim32.frames);
 enabled=1;cg.clientFrame++;CGAP32_Frame();start=CGAP32_Start();cgfa4Revision++;CGAP32_End(C1AP32_ACTOR,start,1,7);assert(!cgAnim32.stage[C1AP32_ACTOR].calls);CGAP32_Frame();assert(cgAnim32.revision==2);
 badClock=1;assert(!CGAP32_Start());badClock=0;
 start=CGAP32_Start();clockValue--;CGAP32_End(C1AP32_ACTOR,start,1,7);assert(!cgAnim32.stage[C1AP32_ACTOR].calls);
 cg.time=-1;cg.clientFrame++;CGAP32_Frame();assert(!CGAP32_Start());CGAP32_Decoded(7,&decoded);assert(!cgAnim32.decodedLadderActors);
 assert(!C1AP32_Frame(&w,&out,1,0,0));C1AP32_Record(&w,C1AP32_ACTOR,1,1,C1AP32_OWNERS);assert(w.actors==0);
 assert(!C1AP32_Frame(&w,&out,1,1,-1)&&w.frames==1);assert(!C1AP32_Frame(&w,&out,2,1,0)&&w.revision==2);
 assert(!C1AP32_Frame(&w,&out,2,0,1)&&w.first==0);assert(C1AP32_Ladder(317)==0&&C1AP32_Ladder(319)==2&&C1AP32_Ladder(320)==4&&C1AP32_Ladder(322)==0);
 assert(!C1AP32_Frame(&w,&out,2,1,2));
 C1AP32_Record(&w,C1AP32_COURSE,10,1,7);
 C1AP32_Record(&w,C1AP32_COURSE_LADDER,20,1,8);
 C1AP32_Record(&w,C1AP32_COURSE_LOAD,4,0,8);
 C1AP32_Record(&w,C1AP32_COURSE_MAP,2,1,8);
 assert(w.stage[C1AP32_COURSE].sum==10&&w.stage[C1AP32_COURSE_LADDER].sum==20);
 assert(w.stage[C1AP32_COURSE_LOAD].failed==1&&w.stage[C1AP32_COURSE_MAP].calls==1);
 CGAP32_Print(&w);
 assert(strstr(logText,"stage=course ")&&strstr(logText,"stage=course_ladder")&&strstr(logText,"stage=course_load")&&strstr(logText,"stage=course_map"));
 puts("R32/R33 observer: PASS");return 0;
}
