#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;
typedef struct { int integer; } cvar_t;
#define CVAR_TEMP 256
static cvar_t cvar;
static unsigned tick, clock_calls, print_calls;
static char logs[4096];
static cvar_t *Cvar_Get(const char *n,const char *v,int f) {
 assert(!strcmp(n,"com_cod1LoadProfile")); assert(!strcmp(v,"0")); assert(f==CVAR_TEMP); return &cvar;
}
static int Sys_Milliseconds(void){ ++clock_calls; return (int)tick; }
static void Com_Printf(const char *format,...) {
 va_list args; size_t offset=strlen(logs); ++print_calls;
 va_start(args,format); vsnprintf(logs+offset,sizeof(logs)-offset,format,args); va_end(args);
}
#include "cod1_load_profile.h"
int main(void) {
 cod1LoadProfile_t p, q; unsigned i;
 memset(&p,0xa5,sizeof p);
 Cod1LoadProfileBegin(&p); Cod1LoadProfileMark(&p,"disabled"); Cod1LoadProfileReport(&p,"test","map");
 assert(clock_calls==0 && print_calls==0 && p.count==0);
 cvar.integer=1; tick=100; Cod1LoadProfileBegin(&p); tick=130; Cod1LoadProfileMark(&p,"one");
 tick=140; Cod1LoadProfileBegin(&q); tick=155; Cod1LoadProfileMark(&q,"nested"); Cod1LoadProfileReport(&q,"child","map");
 tick=160; Cod1LoadProfileMark(&p,"two"); Cod1LoadProfileReport(&p,"parent","map");
 assert(!strcmp(logs,"[OPT_LOAD] scope=child map=map stage=nested ms=15\n[OPT_LOAD] scope=child map=map total_ms=15\n[OPT_LOAD] scope=parent map=map stage=one ms=30\n[OPT_LOAD] scope=parent map=map stage=two ms=30\n[OPT_LOAD] scope=parent map=map total_ms=60\n"));
 tick=UINT_MAX-5; Cod1LoadProfileBegin(&p); tick=4; Cod1LoadProfileMark(&p,"wrap"); assert(p.elapsed[0]==10);
 tick=0; Cod1LoadProfileBegin(&p); for(i=0;i<20;i++){++tick;Cod1LoadProfileMark(&p,"cap");} assert(p.count==16&&p.last==16);
 cvar.integer=0; Cod1LoadProfileBegin(&p); assert(!p.enabled && !p.count);
 puts("PASS disabled, exact log, nested, wraparound, capacity, reset"); return 0;
}
