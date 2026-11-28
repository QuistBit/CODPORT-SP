#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gsc_source_profile55.h"
#define MAX_FRAMES 16
#define GSC53_SOURCE 0
typedef struct {int fn,pc;} Frame;
typedef struct {char name[64];int module,start,end;} Func;
typedef struct {int line;} Instr;
typedef struct {Func*funcs;char (*modules)[128];Instr*code;int func_count,module_count,code_count;} GscProgram;
typedef struct {uint64_t serial;int depth;Frame frames[MAX_FRAMES];} Task;
typedef struct {const GscProgram*program;int results_enabled,action,calls;} GscRuntime;
typedef struct {uint64_t(*clock)(void);uint64_t ticks[1],calls[1],clock_failures;GscSourceSample55 source_worst;} GscR237Profile53;
static uint64_t clock_now,clock_step;static unsigned fail_clock;
static uint64_t clock_mock(void){clock_now+=clock_step;return fail_clock?0:clock_now;}
static uint64_t r237_profile_begin53(GscR237Profile53*p){return p&&p->clock?p->clock():0;}
static void r237_profile_end53(GscR237Profile53*p,unsigned stage,uint64_t start){uint64_t end;if(!p)return;++p->calls[stage];end=p->clock?p->clock():0;if(!start||!end||end<start){++p->clock_failures;return;}p->ticks[stage]+=end-start;}
static int run_task(GscRuntime*r,Task*t,int*b){++r->calls;*b-=r->action%7;t->depth=0;t->serial+=1000;return r->action%3!=0;}
static int run_task_results(GscRuntime*r,Task*t,int*b){return run_task(r,t,b);}
#include "gsc_vm_r237_source_profile55.inc"
int main(void){
 Func funcs[2]={{"run",0,0,2},{"waiter",1,2,4}};char modules[2][128]={"maps/training","animscripts/stop"};Instr code[4]={{11},{12},{21},{22}};
 GscProgram g={funcs,modules,code,2,2,4};unsigned i,known=0,unknown=0,failed=0,accepted=0;
 for(i=0;i<16384;i++){
  GscRuntime a={&g,(int)(i&1),(int)(i%31),0},b;Task x={0},y;GscR237Profile53 pa={0},pb={0};int ba=100,bb=100,ra,rb;uint64_t start;int valid;
  x.serial=i+1;x.depth=(int)((i>>1)%19)-1;x.frames[0].fn=(int)((i>>5)%4)-1;x.frames[0].pc=(int)((i>>7)%6)-1;
  if(x.depth>0&&x.depth<=MAX_FRAMES)x.frames[x.depth-1]=x.frames[0];
  if(i%11==0)a.program=NULL;
  b=a;y=x;pa.clock=pb.clock=clock_mock;pa.source_worst.ticks=pb.source_worst.ticks=i%5;
  clock_step=i%17+1;fail_clock=i%13==0;clock_now=100;start=r237_profile_begin53(&pa);
  ra=a.results_enabled?run_task_results(&a,&x,&ba):run_task(&a,&x,&ba);r237_profile_end53(&pa,GSC53_SOURCE,start);
  clock_now=100;rb=r55_run_source(&b,&y,&bb,&pb);
  assert(ra==rb&&ba==bb&&!memcmp(&a,&b,sizeof(a))&&!memcmp(&x,&y,sizeof(x)));
  assert(pa.ticks[0]==pb.ticks[0]&&pa.calls[0]==pb.calls[0]&&pa.clock_failures==pb.clock_failures);
  if(fail_clock||clock_step<=i%5){assert(pb.source_worst.ticks==i%5);failed++;continue;}
  ++accepted;assert(pb.source_worst.ticks==clock_step&&pb.source_worst.serial==i+1&&pb.source_worst.budget_spent==(unsigned)(100-bb));
  valid=b.program&&pb.source_worst.depth>0&&pb.source_worst.depth<=MAX_FRAMES&&pb.source_worst.fn>=0&&pb.source_worst.fn<2;
  assert(pb.source_worst.known==valid);
  if(valid){int fn=pb.source_worst.fn,pc=pb.source_worst.pc;known++;assert(!strcmp(pb.source_worst.module,modules[fn])&&!strcmp(pb.source_worst.function,funcs[fn].name));assert(pb.source_worst.line==((pc>=funcs[fn].start&&pc<funcs[fn].end)?code[pc].line:0));}
  else unknown++;
 }
 assert(known&&unknown&&failed&&accepted);printf("R55A PASS: 16384 cases, %u accepted samples, %u known, %u unknown, %u retained prior sample\n",accepted,known,unknown,failed);return 0;
}
