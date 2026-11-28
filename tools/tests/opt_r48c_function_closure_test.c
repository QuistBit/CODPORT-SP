#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define MAX_FUNCS 8192
#define N 32
#define M (N*3)
enum {I_EMPTY,I_CALL,I_THREAD,I_METHOD,I_METHOD_THREAD,I_DYNAMIC_CALL,I_DYNAMIC_THREAD};
enum {E_NUMBER,E_FUNC_REF,E_SCOPED_CALL,E_NATIVE_CALL,E_ISDEFINED,E_THREAD_CALL,E_METHOD_CALL,E_DYNAMIC_CALL};
enum {GSC_RECEIVER_TARGET_LOCAL_SCRIPT=1,GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT=2,GSC_EXPR_TARGET_LOCAL_SCRIPT=1,GSC_THREAD_EXPR_NAMED=1};
typedef struct {int kind,target,receiver_binding_kind,receiver_script_target,expr;} Instr;
typedef struct {int kind,target,call_binding,op,b,a,receiver_binding_kind,receiver_script_target;} Expr;
typedef struct {int admission,module,start,end,expr_start,expr_end;char name[32],admission_error[64];} Func;
typedef struct {int function_admission_selected,func_count,expr_count;uint64_t function_admission_token;Func funcs[N];Instr code[M];Expr expr[M];char modules[2][32];unsigned char hidden_code[M],hidden_expr[M];} GscProgram;
typedef struct {const GscProgram *program;uint64_t function_admission_token;unsigned char function_admission[MAX_FUNCS],verified_function_closure[MAX_FUNCS];} GscRuntime;
static unsigned scans,logs;
static int developer_code_hidden(const GscProgram*g,int n){++scans;return g->hidden_code[n];}
static int developer_expr_hidden(const GscProgram*g,int n){++scans;return g->hidden_expr[n];}
static void G_Printf(const char *fmt,...){(void)fmt;++logs;}
#define admission_target_push old_push
#define function_admission_collect old_collect
#define function_admission_current_mask old_current_mask
#define function_admission_current old_current
#define function_admission_runtime_current old_runtime_current
#include "original.inc"
#undef admission_target_push
#undef function_admission_collect
#undef function_admission_current_mask
#undef function_admission_current
#undef function_admission_runtime_current
#include "candidate.inc"
static GscProgram g;
static GscRuntime r;
static unsigned seed=0x5e48c123;static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static unsigned cases,success,refused;
static void check(int first){
 unsigned char previous[MAX_FUNCS],reachable[MAX_FUNCS],expected[MAX_FUNCS];char a[384]={0},b[384]={0};int x,y;unsigned old_logs,old_scans;
 memcpy(previous,r.verified_function_closure,sizeof(previous));memcpy(expected,previous,sizeof(expected));memset(reachable,0,sizeof(reachable));
 scans=logs=0;x=old_runtime_current(&r,first,a,sizeof(a));old_logs=logs;old_scans=scans;
 scans=logs=0;y=function_admission_runtime_cache(&r,first,b,sizeof(b));
 assert(x==y&&logs==old_logs&&scans==old_scans&&!memcmp(a,b,sizeof(a)));
 if(x&&g.function_admission_selected){assert(old_collect(&g,first,r.function_admission,reachable,NULL,0));for(int i=0;i<g.func_count;i++)if(reachable[i])expected[i]=1;}
 assert(!memcmp(expected,r.verified_function_closure,sizeof(expected)));
 for(int i=0;i<g.func_count;i++)if(r.verified_function_closure[i])assert(old_runtime_current(&r,i,NULL,0));
 ++cases;if(x)++success;else ++refused;
}
static void init(int count){memset(&g,0,sizeof(g));memset(&r,0,sizeof(r));g.func_count=count;g.expr_count=3*count;g.function_admission_selected=1;g.function_admission_token=7;r.program=&g;r.function_admission_token=7;strcpy(g.modules[0],"maps/training");strcpy(g.modules[1],"animscripts/run");for(int i=0;i<count;i++){Func*f=g.funcs+i;f->admission=1;f->start=f->expr_start=3*i;f->end=f->expr_end=3*i+3;snprintf(f->name,sizeof(f->name),"f%d",i);r.function_admission[i]=1;}}
int main(void){
 init(N);for(int i=0;i<N-1;i++){g.code[3*i].kind=I_CALL;g.code[3*i].target=i+1;}g.code[3*(N-1)].kind=I_CALL;g.code[3*(N-1)].target=0;
 assert(old_current(&g,0,NULL,0)&&function_admission_current(&g,0,NULL,0));assert(function_admission_runtime_current(&r,0,NULL,0));
 check(0);for(int i=0;i<N;i++)assert(r.verified_function_closure[i]);
 /* An already proven cyclic graph: old root-only cache would walk N times. */
 scans=0;for(int i=0;i<N;i++)assert(old_runtime_current(&r,i,NULL,0));unsigned old=scans;scans=0;for(int i=0;i<N;i++)if(!r.verified_function_closure[i])assert(function_admission_runtime_cache(&r,i,NULL,0));assert(scans==0);printf("cyclic graph subsequent scans: %u -> %u\n",old,scans);
 for(int k=0;k<2000;k++){
  init(1+(int)(rnd()%N));
  for(int i=0;i<g.func_count;i++){Func*f=g.funcs+i;f->module=(int)(rnd()%2);if(k%3==0&&rnd()%8==0){f->admission=0;strcpy(f->admission_error,"blocked");}if(k%5==0&&rnd()%7==0)r.function_admission[i]=0;}
  for(int i=0;i<g.expr_count;i++){Instr *p=g.code+i;Expr*e=g.expr+i;p->kind=(int)(rnd()%7);p->target=(int)(rnd()%(g.func_count+2))-1;p->receiver_binding_kind=(int)(rnd()%3);p->receiver_script_target=p->target;p->expr=(int)(rnd()%g.expr_count);e->kind=(int)(rnd()%8);e->target=(int)(rnd()%(g.func_count+2))-1;e->op=(int)(rnd()%2);e->b=(int)(rnd()%2)-1;e->a=(int)(rnd()%g.expr_count);e->call_binding=(int)(rnd()%2);e->receiver_binding_kind=(int)(rnd()%3);e->receiver_script_target=e->target;g.hidden_code[i]=(rnd()%7==0);g.hidden_expr[i]=(rnd()%7==0);}
  for(int j=0;j<4;j++)check((int)(rnd()%(g.func_count+2))-1);
 }
 init(3);g.function_admission_token=8;check(0);init(3);g.function_admission_selected=0;check(0);
 printf("%u cases PASS (%u admitted, %u refused)\n",cases,success,refused);return 0;
}
