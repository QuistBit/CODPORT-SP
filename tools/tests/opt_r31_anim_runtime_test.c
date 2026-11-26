/* POSIX test of the actual observer with deterministic Win32 clock stubs. */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "r31_include/windows.h"
static uint64_t cod1EntityEpoch=1;
static struct {int framenum;} level;
static int enabled,queries,counters,frequencies,prints,badClock;
static int64_t clockValue=100;
static char line[1024];
static int trap_Cvar_VariableIntegerValue(const char *name){assert(!strcmp(name,"g_cod1AnimProfile"));queries++;return enabled;}
static int QueryPerformanceCounter(LARGE_INTEGER *q){counters++;q->QuadPart=clockValue;return !badClock;}
static int QueryPerformanceFrequency(LARGE_INTEGER *q){frequencies++;q->QuadPart=1000;return 1;}
static void G_Printf(const char *fmt,...){va_list ap;va_start(ap,fmt);vsnprintf(line,sizeof(line),fmt,ap);va_end(ap);prints++;}
#define _WIN32 1
#define _MSC_VER 1930
#define _M_IX86 600
#include "../../src/game/gsc_host/g_cod1_anim_profile31.inc"
int main(void){
 uint64_t start;
 assert(!ActorProfileStart31()&&!ActorProfileStart31()&&queries==1&&counters==0&&frequencies==0);
 level.framenum++;enabled=1;start=ActorProfileStart31();assert(start==100&&queries==2&&frequencies==1);
 clockValue=120;ActorProfileFinish31(0,start,1,100,7,1,2,0,0,0);
 start=ActorProfileStart31();clockValue=150;ActorProfileFinish31(0,start,1,100,8,1,1,0,0,0);
 assert(actorProfile31[0].sum==50&&actorProfile31[0].actors==2&&queries==2&&prints==0);
 start=ActorProfileStart31();clockValue=160;ActorProfileFinish31(1,start,1,100,7,1,0,12,4,C1AP31_Ladder(318));
 assert(actorProfile31[1].ladderActors==1&&actorProfile31[1].sum==10);
 level.framenum++;start=ActorProfileStart31();clockValue=190;ActorProfileFinish31(0,start,1,350,7,1,1,0,0,0);
 assert(prints==1&&strstr(line,"stage=controller")&&strstr(line,"sum_ticks=50")&&strstr(line,"max_tick_ticks=50"));
 enabled=0;level.framenum++;assert(!ActorProfileStart31()&&actorProfile31[0].calls==0&&actorProfile31[1].calls==0);
 enabled=1;level.framenum++;start=ActorProfileStart31();cod1EntityEpoch++;ActorProfileFinish31(0,start,1,400,7,1,0,0,0,0);assert(actorProfile31[0].calls==0);
 badClock=1;assert(!ActorProfileStart31());
 badClock=0;level.framenum++;start=ActorProfileStart31();clockValue+=10;
 ActorProfileFinish31(C1AP33_PUBLISH,start,2,500,7,1,0,0,0,1);
 start=ActorProfileStart31();clockValue+=20;
 ActorProfileFinish31(C1AP33_PUBLISH,start,2,500,8,1,0,0,0,2);
 start=ActorProfileStart31();clockValue+=5;
 ActorProfileFinish31(C1AP33_DISCARD,start,2,500,7,1,0,0,0,0);
 assert(actorProfile31[C1AP33_PUBLISH].ladderActors==2&&actorProfile31[C1AP33_PUBLISH].ladderMask==3&&actorProfile31[C1AP33_PUBLISH].maxTick==30);
 assert(actorProfile31[C1AP33_DISCARD].sum==5&&actorProfile31[C1AP33_PRIOR].calls==0);
 level.framenum++;start=ActorProfileStart31();clockValue+=3;
 ActorProfileFinish31(C1AP33_PUBLISH,start,2,750,7,1,0,0,0,8);
 assert(strstr(line,"stage=publish")&&strstr(line,"ladder_actors=2")&&strstr(line,"ladder_mask=3"));
 start=ActorProfileStart31();clockValue+=2;
 ActorProfileFinish31(C1AP33_STAGES,start,2,750,7,1,0,0,0,0);
 puts("R31/R33 runtime: PASS");return 0;
}
