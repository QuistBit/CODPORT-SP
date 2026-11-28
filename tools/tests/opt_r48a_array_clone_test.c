#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum {GSC_ARRAY_CONTAINER_CAPACITY=256,GSC_ARRAY_ENTRY_CAPACITY=4096,V_NUMBER=1,V_ARRAY=2,V_STRING=3};
typedef struct {int kind; uint32_t array; uint64_t object_token; int data;} Value;
typedef struct {int count,numeric_count,keybinding;} ArrayContainer;
typedef struct {int owner; Value key,value;} ArrayEntry;
typedef struct {int array_count,array_entry_count; ArrayContainer arrays[GSC_ARRAY_CONTAINER_CAPACITY]; ArrayEntry array_entries[GSC_ARRAY_ENTRY_CAPACITY];char error[100];int errorline;} GscRuntime;
typedef struct {int arrays,entries;ArrayContainer containers[GSC_ARRAY_CONTAINER_CAPACITY];ArrayEntry members[GSC_ARRAY_ENTRY_CAPACITY];} R238ArrayStage;
static int rfail(GscRuntime*r,int line,const char*s){snprintf(r->error,sizeof(r->error),"%s",s);r->errorline=line;return 0;}
static int array_value_valid(GscRuntime*r,uint32_t id){return id && id<=(uint32_t)r->array_count;}
static int r238_stage_new(GscRuntime*r,R238ArrayStage*s,int line,uint32_t*out){
 if(s->arrays>=GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"R238 array COW container capacity");
 *out=(uint32_t)++s->arrays;memset(s->containers+*out-1,0,sizeof(s->containers[0]));return 1;
}
#define r238_stage_clone original_clone
#include "original.inc"
#undef r238_stage_clone
#include "candidate.inc"
static GscRuntime a,b;
static R238ArrayStage sa,sb;
static unsigned seed=0x44abcd;static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static int cases,success,fail;
static void check(uint32_t source,int arrays,int entries){
 int oa,ob;uint32_t u=0xfafa,v=u;
 b=a;memset(&sa,0xa5,sizeof(sa));sa.arrays=arrays;sa.entries=entries;sb=sa;
 oa=original_clone(&a,&sa,source,19,&u);ob=r238_stage_clone(&b,&sb,source,19,&v);
 if(oa!=ob||u!=v||memcmp(&sa,&sb,sizeof(sa))||memcmp(&a,&b,sizeof(a))){fprintf(stderr,"mismatch case=%d src=%u old=%d new=%d olderr=%s newerr=%s\n",cases,source,oa,ob,a.error,b.error);abort();}
 ++cases;if(oa)++success;else ++fail;
 a.error[0]=0;a.errorline=0;
}
static void add(int owner,int kind,uint32_t child,int key){ArrayEntry*e=a.array_entries+a.array_entry_count++;e->owner=owner;e->key.kind=key;e->key.data=a.array_entry_count;e->value.kind=kind;e->value.array=child;e->value.data=123;e->value.object_token=19;if(owner>0&&owner<=a.array_count){a.arrays[owner-1].count++;if(key==V_NUMBER)a.arrays[owner-1].numeric_count++;}}
int main(void){
 for(int k=0;k<8;k++){
  memset(&a,0,sizeof(a));a.array_count=200;
  for(int i=1;i<200;i++)add(i,V_ARRAY,i+1,V_NUMBER);
  for(int i=0;i<1000;i++)add(200,V_NUMBER,0,V_STRING);
  if(k==1)a.array_entries[198].value.array=1;
  if(k==2)a.array_entries[198].value.array=201;
  if(k==3)a.arrays[199].count++;
  if(k==4)add(-1,V_NUMBER,0,V_STRING);
  check(1,k==5?250:0,k==6?4090:0);check(200,0,0);check(0,0,0);
 }
 for(int k=0;k<1600;k++){
  memset(&a,0,sizeof(a));a.array_count=1+(int)(rnd()%48);int n=(int)(rnd()%400);
  for(int i=0;i<a.array_count;i++)a.arrays[i].keybinding=(int)(rnd()%7);
  for(int i=0;i<n;i++){
   int owner=1+(int)(rnd()%(unsigned)a.array_count),kind=V_NUMBER;uint32_t child=0;
   if(rnd()%5==0&&owner<a.array_count){kind=V_ARRAY;child=owner+1+rnd()%(unsigned)(a.array_count-owner);}
   if(k%11==0&&i==0){kind=V_ARRAY;child=owner;}
   if(k%13==0&&i==1){kind=V_ARRAY;child=a.array_count+1;}
   if(k%17==0&&i==2)owner=-1;
   add(owner,kind,child,rnd()%2?V_NUMBER:V_STRING);
  }
  if(k%7==0)a.arrays[rnd()%(unsigned)a.array_count].count++;
  check(1+rnd()%(unsigned)a.array_count,k%19==0?255:0,k%23==0?4095:0);
  check(1,0,0);
 }
 printf("%d exact output/error/partial-stage comparisons PASS (%d successful, %d refused)\n",cases,success,fail);
}
