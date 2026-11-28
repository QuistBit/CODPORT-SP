#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum {GSC_ARRAY_CONTAINER_CAPACITY=256,GSC_ARRAY_ENTRY_CAPACITY=4096,V_ARRAY=3,V_NUMBER=1};
typedef struct {int kind;uint32_t array;} Value;
typedef struct {int owner;Value value;} ArrayEntry;
typedef struct {int arrays,entries;ArrayEntry members[GSC_ARRAY_ENTRY_CAPACITY];} R238ArrayStage;
typedef struct {char error[100];int line;int failures;} GscRuntime;
static int rfail(GscRuntime*r,int line,const char*s){if(!r->error[0]){snprintf(r->error,sizeof(r->error),"%s",s);r->line=line;}r->failures++;return 0;}
#include "original.inc"
#include "candidate.inc"
static R238ArrayStage s;
static unsigned seed=0x44a1b2c3;static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static int cases;
static void check(uint32_t start,const uint32_t*targets,int count){
 GscRuntime a={0},b={0};int x=0,y,i;
 for(i=0;i<count;i++){x=r238_stage_reaches(&a,&s,start,targets[i],17);if(x)break;}
 y=r238_stage_reaches_any(&b,&s,start,targets,count,17);
 if(x!=y||memcmp(&a,&b,sizeof(a))){fprintf(stderr,"mismatch %d start=%u count=%d x=%d y=%d errors %s / %s\n",cases,start,count,x,y,a.error,b.error);abort();}
 ++cases;
}
static void add(int owner,int kind,uint32_t child){assert(s.entries<GSC_ARRAY_ENTRY_CAPACITY);s.members[s.entries++]=(ArrayEntry){owner,{kind,child}};}
int main(void){
 uint32_t t[48];
 /* Explicit ordering: LIFO visits 3 before 2; 3 has invalid child. */
 s.arrays=3;add(1,V_ARRAY,2);add(1,V_ARRAY,3);add(3,V_ARRAY,4);
 for(unsigned a=0;a<5;a++)for(unsigned b=0;b<5;b++){t[0]=a;t[1]=b;check(1,t,2);}
 /* Full graph / deep cycle / duplicate edges / maximum target count. */
 memset(&s,0,sizeof(s));s.arrays=256;for(int i=1;i<256;i++)add(i,V_ARRAY,i+1);add(256,V_ARRAY,1);
 for(int i=s.entries;i<4096;i++)add(256,V_NUMBER,0);
 for(int i=0;i<48;i++)t[i]=256-i;
 check(1,t,48);check(256,t,48);check(0,t,48);t[0]=999;check(1,t,48);check(1,t,0);
 for(int k=0;k<2000;k++){
  memset(&s,0,sizeof(s));s.arrays=1+(int)(rnd()%64);int n=(int)(rnd()%500);
  for(int i=0;i<n;i++){
   int owner=1+(int)(rnd()%(unsigned)s.arrays);int kind=rnd()%3?V_ARRAY:V_NUMBER;uint32_t child=1+rnd()%(unsigned)s.arrays;
   if(k%7==0&&i==0)child=0;
   if(k%11==0&&i==1)child=(uint32_t)s.arrays+1;
   if(k%13==0&&i==2)owner=-1;
   add(owner,kind,child);
  }
  for(int j=0;j<5;j++){
   int count=1+(int)(rnd()%48);for(int i=0;i<count;i++)t[i]=rnd()%(unsigned)(s.arrays+2);
   uint32_t start=1+rnd()%(unsigned)s.arrays;if(k%17==0&&j==0)start=0;
   check(start,t,count);
  }
 }
 printf("%d exact result/diagnostic comparisons PASS\n",cases);
}
