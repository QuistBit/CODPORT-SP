#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define V_UNDEFINED 0
#define V_ARRAY 6
#define E_VAR 1
#define E_FIELD 2
#define CAP 8
#define PATH 48
typedef struct {int kind; uint32_t array; uint64_t object_token; unsigned char other[240];} Value;
typedef struct {int kind;uint32_t array;uint64_t object_token;} ResultsAncestor;
typedef struct {char name[64];Value value;} Local;
typedef struct {int local_count;Local locals[CAP];} Frame;
typedef struct {Frame frames[2];} Task;
typedef struct {int kind;char name[64];} Expr;
typedef struct {int expr_count;Expr expr[2];} Program;
typedef struct {int owner,key;Value value;} Entry;
typedef struct {Value value;} Field;
typedef struct {Program *program;int game_array_enabled,arrays_enabled,array_count,array_entry_count,field_index;uint64_t array_token;Value game_array_root;Field state_fields[CAP];Entry array_entries[CAP];} GscRuntime;
typedef struct {int key_index;struct {int root,count;Value owner,base;int keys[PATH];} path;ResultsAncestor ancestors[PATH];} ResultsPending;
static unsigned trace[PATH*3+10],calls;
static void record(unsigned x){assert(calls<sizeof(trace)/sizeof(trace[0]));trace[calls++]=x;}
static int results_same_value(const Value*a,const Value*b){if(a->kind!=b->kind)return 0;return a->kind==V_UNDEFINED?1:a->kind==V_ARRAY?a->array==b->array:0;}
static int results_same_ancestor(const Value*a,const ResultsAncestor*b){return a->kind==b->kind&&(a->kind==V_UNDEFINED||(a->kind==V_ARRAY&&a->array==b->array));}
static int array_value_valid(const GscRuntime*r,uint32_t h){record(100+h);return r&&r->arrays_enabled&&h&&h<=(uint32_t)r->array_count;}
static int game_array_selected(const GscRuntime*r,const char*name){record(1);return r&&r->game_array_enabled&&name&&!strcmp(name,"game");}
static int game_array_value_valid(const GscRuntime*r,const Value*v){record(2);return r&&v&&v->kind==V_ARRAY&&r->array_token&&v->object_token==r->array_token&&array_value_valid(r,v->array);}
static int state_field_index(GscRuntime*r,const Value*v,const char*name){(void)v;(void)name;record(3);return r->field_index;}
static int array_entry_find(const GscRuntime*r,uint32_t h,const int*key){int i;record(1000+h*10+(unsigned)*key);for(i=0;i<r->array_entry_count;i++)if(r->array_entries[i].owner==(int)h&&r->array_entries[i].key==*key)return i;return -1;}
#define results_readonly_path old_path
#include "old.inc"
#undef results_readonly_path
#define results_readonly_path new_path
#include "new.inc"
#undef results_readonly_path
static uint64_t rng=UINT64_C(0xb2aebc43bd7e01c5);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static Value value(int kind,unsigned array){Value v={0};v.kind=kind;v.array=array;v.object_token=9;memset(v.other,0xa5,sizeof(v.other));return v;}
static Value random_value(void){unsigned k=rnd()%4;return value(k==0?V_UNDEFINED:k==1?3:V_ARRAY,rnd()%7);}
static ResultsAncestor ancestor(Value v){ResultsAncestor a={v.kind,v.array,v.object_token};return a;}
int main(void){unsigned i,yes=0,no=0,deep=0;for(i=0;i<200000;i++){
 Program g={2,{{E_VAR,"local"},{E_FIELD,"field"}}};GscRuntime r={0},before;Task task={0},tb;ResultsPending p={0},pb;unsigned expected[PATH*3+10],n,j;int a,b,frame=i%2;
 r.program=&g;r.arrays_enabled=1;r.array_count=5;r.array_token=9;r.field_index=(int)(rnd()%9)-1;r.game_array_enabled=i%3==0;r.game_array_root=random_value();
 task.frames[frame].local_count=CAP;for(j=0;j<CAP;j++){strcpy(task.frames[frame].locals[j].name,j%2?"local":"other");task.frames[frame].locals[j].value=random_value();r.state_fields[j].value=random_value();r.array_entries[j]=(Entry){(int)(rnd()%5)+1,(int)(rnd()%3),random_value()};}r.array_entry_count=CAP;
 if(i%5==0){strcpy(g.expr[0].name,"game");}
 p.path.root=(int)(rnd()%4)-1;p.path.count=(int)(rnd()%PATH)+1;p.key_index=(int)(rnd()%PATH);p.path.base=random_value();for(j=0;j<PATH;j++){p.path.keys[j]=(int)(rnd()%3);p.ancestors[j]=ancestor(random_value());}
 /* Construct valid long paths and perturb their root/ancestor/handle. */
 if(i%3==0){p.path.root=i%2; p.path.count=PATH;p.key_index=PATH-1;p.path.base=value(V_ARRAY,1);r.field_index=0;r.state_fields[0].value=p.path.base;strcpy(g.expr[0].name,"local");task.frames[frame].locals[7].value=p.path.base;r.array_entry_count=1;r.array_entries[0]=(Entry){1,0,value(V_ARRAY,1)};for(j=0;j<PATH;j++){p.path.keys[j]=0;p.ancestors[j]=ancestor(p.path.base);}if(i%7==0)p.ancestors[rnd()%(PATH-1)].kind=V_UNDEFINED;else if(i%11==0)r.array_count=0;else ++deep;}
 /* Missing locals/fields and absent children are undefined, not errors. */
 if(i%17==0){p.path.root=0;strcpy(g.expr[0].name,"absent");p.path.base=value(V_UNDEFINED,99);for(j=0;j<PATH;j++)p.ancestors[j]=ancestor(p.path.base);}
 before=r;tb=task;pb=p;calls=0;a=old_path(&r,&task,frame,&p);n=calls;memcpy(expected,trace,n*sizeof(*trace));assert(!memcmp(&r,&before,sizeof(r))&&!memcmp(&task,&tb,sizeof(task))&&!memcmp(&p,&pb,sizeof(p)));calls=0;b=new_path(&r,&task,frame,&p);assert(a==b&&calls==n&&!memcmp(expected,trace,n*sizeof(*trace)));assert(!memcmp(&r,&before,sizeof(r))&&!memcmp(&task,&tb,sizeof(task))&&!memcmp(&p,&pb,sizeof(p)));if(a)yes++;else no++;
 }assert(yes&&no&&deep);printf("R55C1 PASS: 200000 cases, accepted=%u refused=%u deep-generated=%u\n",yes,no,deep);return 0;}
