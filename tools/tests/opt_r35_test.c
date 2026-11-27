#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "r31_include/windows.h"
static uint64_t epoch=1;
static int enabled,polls,counters,frequency,prints,badClock;
static int64_t clockValue=100;
static char line[1024];
static uint64_t G_Cod1EntityMapEpoch(void){return epoch;}
static int trap_Cvar_VariableIntegerValue(const char *name){assert(!strcmp(name,"g_cod1FrameProfile"));polls++;return enabled;}
static int QueryPerformanceCounter(LARGE_INTEGER *q){counters++;q->QuadPart=clockValue;return !badClock;}
static int QueryPerformanceFrequency(LARGE_INTEGER *q){frequency++;q->QuadPart=1000;return 1;}
static void G_Printf(const char *fmt,...){va_list ap;va_start(ap,fmt);vsnprintf(line,sizeof(line),fmt,ap);va_end(ap);prints++;}
#define COD1_GSC_TEST_WIP 1
#define _WIN32 1
#define _MSC_VER 1930
#define _M_IX86 600
#include "../../src/game/gsc_host/g_cod1_frame_profile35.h"
typedef struct {uint64_t words[333];} cod1ActorRecord_t;
typedef struct {uint64_t words[511];} gentity_t;
typedef struct {int owner,busy;void *other;cod1ActorRecord_t *actor;gentity_t *entity;} C1FaceFrameCall;
static unsigned allocations,frees;static int refuse;
static void *testMalloc(size_t n){void *p;if(refuse)return NULL;p=malloc(n);if(p){allocations++;memset(p,0xab,n);}return p;}
static void testFree(void *p){if(p)frees++;free(p);}
#define malloc testMalloc
#define free testFree
#include "../../src/game/gsc_host/g_cod1_face_frame_storage35.h"
#undef malloc
#undef free
int main(void){
 c1fp35Token a,b;C1FaceFrameCall *c,*d;
 assert(!C1FP35_Begin(100).stamp&&!C1FP35_Begin(100).stamp&&polls==1&&!counters&&!frequency);
 enabled=1;a=C1FP35_Begin(150);assert(a.stamp&&polls==2&&frequency==1);
 clockValue+=10;C1FP35_End(C1FP35_GAME,a,2);
 clockValue+=1000; /* wait between slices is excluded */
 a=C1FP35_Begin(150);b=C1FP35_Begin(150);clockValue+=20;C1FP35_End(C1FP35_GSC,b,1);C1FP35_End(C1FP35_GAME,a,1);
 assert(c1fp35.stage[C1FP35_GAME].sum==30&&c1fp35.stage[C1FP35_GAME].maxTick==30&&c1fp35.stage[C1FP35_GAME].paused==1&&polls==2);
 a=C1FP35_Begin(200);clockValue+=25;C1FP35_End(C1FP35_GAME,a,0);
 assert(c1fp35.stage[C1FP35_GAME].sum==55&&c1fp35.stage[C1FP35_GAME].maxTick==30&&c1fp35.stage[C1FP35_GAME].failed==1);
 a=C1FP35_Begin(400);assert(prints==2&&strstr(line,"stage=gsc")&&strstr(line,"sum_ticks=20")&&c1fp35.ticks==1);
 epoch++;C1FP35_End(C1FP35_GAME,a,1);assert(!c1fp35.stage[C1FP35_GAME].calls);
 a=C1FP35_Begin(400);badClock=1;assert(!C1FP35_Begin(400).stamp);C1FP35_End(C1FP35_GAME,a,1);assert(!c1fp35.stage[C1FP35_GAME].calls);badClock=0;
 a=C1FP35_Begin(390);assert(c1fp35.first==390&&c1fp35.epoch==2);clockValue--;C1FP35_End(C1FP35_GAME,a,1);assert(!c1fp35.stage[C1FP35_GAME].calls);
 enabled=0;assert(!C1FP35_Begin(450).stamp&&!c1fp35.ticks);
 c=ActorFaceFrameAllocate();d=ActorFaceFrameAllocate();assert(c&&d&&c!=d&&allocations==2);
 assert(!c->owner&&!c->busy&&!c->other&&c->actor&&c->entity);
 assert(c->actor==&((C1FaceFrameAllocation*)c)->actor&&c->entity==&((C1FaceFrameAllocation*)c)->entity);
 assert(c->actor->words[0]==UINT64_C(0xabababababababab));
 c->actor->words[0]=42;c->entity->words[0]=17;assert(d->actor->words[0]!=42&&d->entity->words[0]!=17);
 ActorFaceFrameFree(c);ActorFaceFrameFree(d);assert(frees==2);
 refuse=1;assert(!ActorFaceFrameAllocate()&&allocations==2);ActorFaceFrameFree(NULL);assert(frees==2);
 puts("R35 observer and allocation: PASS");return 0;
}
