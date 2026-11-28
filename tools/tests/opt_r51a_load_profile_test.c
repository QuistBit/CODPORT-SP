#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean;
typedef struct {int integer;} cvar_t;
#define CVAR_TEMP 1
static unsigned tick, clocks, cvar_reads, lines;
static char value[64], logbuf[8192];
static cvar_t *Cvar_Get(const char *name,const char *fallback,int flags){
    static cvar_t c;assert(!strcmp(name,"com_cod1LoadProfile")&&!strcmp(fallback,"0")&&flags==CVAR_TEMP);
    ++cvar_reads;c.integer=atoi(value);return &c;
}
static int Sys_Milliseconds(void){++clocks;return (int)tick;}
static void record(const char *fmt,va_list args){size_t n=strlen(logbuf);int written=vsnprintf(logbuf+n,sizeof(logbuf)-n,fmt,args);assert(written>=0&&(size_t)written<sizeof(logbuf)-n);++lines;}
static void Com_Printf(const char *fmt,...){va_list a;va_start(a,fmt);record(fmt,a);va_end(a);}
static void CG_Printf(const char *fmt,...){va_list a;va_start(a,fmt);record(fmt,a);va_end(a);}
static int trap_Milliseconds(void){++clocks;return (int)tick;}
static void trap_Cvar_VariableStringBuffer(const char *name,char *out,int size){assert(!strcmp(name,"com_cod1LoadProfile"));++cvar_reads;snprintf(out,(size_t)size,"%s",value);}
#define cod1LoadProfile_t OldProfile
#define Cod1LoadProfileBegin OldBegin
#define Cod1LoadProfileMark OldMark
#define Cod1LoadProfileReport OldReport
#include "old.h"
#undef COD1_LOAD_PROFILE_H
#undef cod1LoadProfile_t
#undef Cod1LoadProfileBegin
#undef Cod1LoadProfileMark
#undef Cod1LoadProfileReport
#define cod1LoadProfile_t EngineProfile
#define Cod1LoadProfileBegin EngineBegin
#define Cod1LoadProfileMark EngineMark
#define Cod1LoadProfileReport EngineReport
#include "src/qcommon/cod1_load_profile.h"
#undef COD1_LOAD_PROFILE_H
#undef cod1LoadProfile_t
#undef Cod1LoadProfileBegin
#undef Cod1LoadProfileMark
#undef Cod1LoadProfileReport
#include "src/cgame/cg_cod1_load_profile51.h"
static void reset(unsigned t,const char *v){tick=t;clocks=cvar_reads=lines=0;logbuf[0]=0;snprintf(value,sizeof(value),"%s",v);}
#define RUN(T,B,M,R) do{T p={0},q={0};B(&p);tick+=7;M(&p,"first");B(&q);for(unsigned n=0;n<20;n++){tick+=n+1;M(&q,"nested");}R(&q,"graphics","training");M(&p,"last");R(&p,"init","training");}while(0)
int main(void){
  const char *modes[]={"0","1","-1","2","","no"," 1","0.5"};
  char reference[8192];unsigned cases=0,base_clocks,base_lines,base_reads;
  for(unsigned i=0;i<sizeof(modes)/sizeof(modes[0]);i++)for(unsigned j=0;j<3;j++){
    unsigned start=j==0?0:j==1?UINT32_MAX-10:123456;
    reset(start,modes[i]);RUN(OldProfile,OldBegin,OldMark,OldReport);
    strcpy(reference,logbuf);base_clocks=clocks;base_lines=lines;base_reads=cvar_reads;
    reset(start,modes[i]);RUN(EngineProfile,EngineBegin,EngineMark,EngineReport);assert(!strcmp(reference,logbuf)&&clocks==base_clocks&&lines==base_lines&&cvar_reads==base_reads);
    reset(start,modes[i]);RUN(cod1LoadProfile_t,Cod1LoadProfileBegin,Cod1LoadProfileMark,Cod1LoadProfileReport);assert(!strcmp(reference,logbuf)&&clocks==base_clocks&&lines==base_lines&&cvar_reads==base_reads);++cases;
  }
  printf("%u cases: old engine, current engine and CGAME logs/clocks identical\n",cases);return 0;
}
