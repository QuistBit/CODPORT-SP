#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
static int refuse_alloc;
static void *fixture_malloc(size_t n){if(refuse_alloc){errno=ENOMEM;return NULL;}return malloc(n);}
#define malloc fixture_malloc
#include "bg_cod1_script_sound.h"
#undef malloc
#include "reference.inc"

typedef struct {
 const char *names[3],*data[3];int count,fail_read,missing,expire;
 unsigned current_calls,trace_count;unsigned trace[128];
} Fixture;
static void record(Fixture *f,unsigned kind){assert(f->trace_count<128);f->trace[f->trace_count++]=kind;}
static int current(void *u){Fixture*f=u;record(f,1);++f->current_calls;return !f->expire||f->current_calls<(unsigned)f->expire;}
static int list(void *u,char*out,size_t cap){Fixture*f=u;int i;size_t n=0;record(f,2);for(i=0;i<f->count;i++){size_t len=strlen(f->names[i])+1;assert(n+len<cap);memcpy(out+n,f->names[i],len);n+=len;}return f->count;}
static int read_table(void *u,const char*path,char**out,size_t*size){Fixture*f=u;int i;record(f,3);for(i=0;i<f->count;i++)if(!strcmp(path+13,f->names[i]))break;assert(i<f->count);*size=strlen(f->data[i]);*out=malloc(*size?*size:1);assert(*out);memcpy(*out,f->data[i],*size);return f->fail_read!=i+1;}
static void release_table(void*u,char*p){record(u,4);free(p);}
static int exists(void*u,const char*path){Fixture*f=u;record(f,5);assert(!strncmp(path,"sound/",6));return !f->missing;}
static int compare(Cod1SSAbsentCache56*c,const Fixture *input,const char *alias,const char *map,int profile,int loop){
 Fixture a=*input,b=*input;Cod1SSFiles fa={&a,current,list,read_table,release_table,exists},fb={&b,current,list,read_table,release_table,exists};Cod1SSPlan x,y;Cod1SSDiagnostic dx,dy;int ra,rb,ea,eb;
 memset(&x,0xa5,sizeof(x));memset(&y,0xa5,sizeof(y));errno=EDOM;ra=ReferenceResolve56(&fa,alias,map,&x,&dx,profile,loop);ea=errno;errno=EDOM;rb=Cod1ScriptSoundResolveModeCached56(&fb,alias,map,&y,&dy,profile,loop,c);eb=errno;
 assert(ra==rb&&dx.reason==dy.reason&&dx.line==dy.line&&ea==eb);
 assert(!memcmp(&x,&y,sizeof(x)));assert(a.trace_count==b.trace_count&&!memcmp(a.trace,b.trace,a.trace_count*sizeof(a.trace[0])));assert(c->used<=COD1_SS56_BYTES);return rb;
}
static uint64_t rng=UINT64_C(0xe38a61278179b62f);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
int main(void){
 static Cod1SSAbsentCache56 cache;unsigned i,ready=0,none=0,held=0;
 static const char *tables[]={"name,file,loadspec\nstep,body/step.wav,training\n", "name,file,loadspec\nstop,body/step.wav,training\n", "file,name\nbody/step.wav,step\n", "name,file\nother,body/other.wav\n", "name,file\nother,\"broken\n", "name,file,vol_min\nstep,body/step.wav,broken\n", "name,file,loop\nstep,body/step.wav,looping\n", "name,file,loadspec\nstep,body/step.wav,! training\n", "name,file,sequence\nstep,body/a.wav,1\nstep,body/b.wav,1\n", "name,file,type,channel\nstep,body/step.wav,streamed,voice\n"};
 static const char *aliases[]={"step","stop","other","STEP","missing","bad alias"};
 static const char *maps[]={"training","us_intro","maps/training.bsp","bad map"};
 Fixture f={0};f.names[0]="a.csv";f.names[1]="b.csv";f.names[2]="c.csv";f.count=3;f.data[0]=tables[0];f.data[1]=tables[3];f.data[2]=tables[3];
 assert(compare(&cache,&f,"step","training",0,0)==1);assert(cache.tables[0].bytes&&cache.tables[1].bytes);assert(compare(&cache,&f,"step","training",0,0)==1);
 f.data[1]=tables[0];assert(compare(&cache,&f,"step","training",0,0)==-1); /* ambiguous despite cached miss */
 f.data[1]=tables[3];f.missing=1;assert(compare(&cache,&f,"step","training",0,0)==-1);f.missing=0;
 f.count=1;f.data[0]=tables[1];assert(compare(&cache,&f,"step","training",0,0)==0);f.data[0]=tables[0];assert(strlen(tables[0])==strlen(tables[1]));assert(compare(&cache,&f,"step","training",0,0)==1);
 for(i=0;i<50000;i++){
  const char *alias=aliases[rnd()%6],*map=maps[rnd()%4];int profile=(int)(rnd()%5),loop=(int)(rnd()%2),r,j;
  if(i%11==0)for(j=0;j<3;j++)f.data[j]=tables[rnd()%10];
  f.count=1+(int)(rnd()%3);f.names[1]=i%67==0?"a.csv":"b.csv";f.fail_read=i%23==0?1:0;f.missing=i%13==0;f.expire=i%17==0?1+(int)(rnd()%14):0;
  if(i%101==0)Cod1SS_AbsentReset56(&cache);
  r=compare(&cache,&f,alias,map,profile,loop);ready+=r==1;none+=r==0;held+=r==-1;
 }
 Cod1SS_AbsentReset56(&cache);assert(!cache.used);
 /* Repeated valid table absence fills ring without caching errors or READY. */
 {Cod1SSPlan p;Cod1SSDiagnostic d;char alias[64];const char *table=tables[0];size_t size=strlen(table);
  for(i=0;i<100;i++){snprintf(alias,sizeof(alias),"absent%u",i);assert(Cod1SS_ParseCached56(&cache,"a.csv",table,size,alias,"training",&p,&d,0,0)==0);}
  assert(cache.used==size&&cache.tables[0].next==100%COD1_SS56_KEYS);assert(Cod1SS_ParseCached56(&cache,"a.csv",table,size,"absent99","training",&p,&d,0,0)==0);assert(cache.tables[0].next==100%COD1_SS56_KEYS);
  assert(Cod1SS_ParseCached56(&cache,"a.csv",table,size,"step","training",&p,&d,0,0)==1);assert(cache.tables[0].next==100%COD1_SS56_KEYS);
 }
 Cod1SS_AbsentReset56(&cache);assert(!cache.used);
 {Cod1SSPlan p;Cod1SSDiagnostic d;char path[32];const char *table=tables[0];size_t size=strlen(table);
  refuse_alloc=1;errno=EDOM;assert(Cod1SS_ParseCached56(&cache,"alloc.csv",table,size,"absent","training",&p,&d,0,0)==0);assert(!cache.used&&errno==EDOM);refuse_alloc=0;
  for(i=0;i<COD1_SS56_TABLES+1;i++){snprintf(path,sizeof(path),"test%u.csv",i);assert(Cod1SS_ParseCached56(&cache,path,table,size,"absent","training",&p,&d,0,0)==0);}
  assert(cache.used==COD1_SS56_TABLES*size);Cod1SS_AbsentReset56(&cache);assert(!cache.used);
 }
 {Cod1SSPlan p;Cod1SSDiagnostic d;char *large=malloc(COD1_SS_TABLE_BYTES);const char *header="name,file\n#";assert(large);
  memcpy(large,header,strlen(header));memset(large+strlen(header),'a',COD1_SS_TABLE_BYTES-strlen(header));
  /* Keep each comment first cell within the CSV limit. */
  for(i=(unsigned)strlen(header)+1;i<COD1_SS_TABLE_BYTES;i+=2){large[i-1]='\n';large[i]='#';}
  assert(Cod1SS_ParseCached56(&cache,"large1.csv",large,COD1_SS_TABLE_BYTES,"absent","training",&p,&d,0,0)==0);
  assert(Cod1SS_ParseCached56(&cache,"large2.csv",large,COD1_SS_TABLE_BYTES,"absent","training",&p,&d,0,0)==0);
  assert(Cod1SS_ParseCached56(&cache,"large3.csv",large,COD1_SS_TABLE_BYTES,"absent","training",&p,&d,0,0)==0);
  assert(cache.used==COD1_SS56_BYTES);free(large);Cod1SS_AbsentReset56(&cache);assert(!cache.used);
 }
 printf("R56A2 PASS: 50000 resolver cases ready=%u none=%u held=%u metadata=%zu bytes\n",ready,none,held,sizeof(cache));return 0;
}
