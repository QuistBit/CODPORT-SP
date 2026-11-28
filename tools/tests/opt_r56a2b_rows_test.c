#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fenv.h>
static unsigned fail_alloc;
static void *fixture_malloc(size_t n){if(fail_alloc){--fail_alloc;if(!fail_alloc){errno=ENOMEM;return NULL;}}return malloc(n);}
#define malloc fixture_malloc
#include "bg_cod1_script_sound.h"
#undef malloc
#include "reference_parser.inc"
static uint64_t rng=UINT64_C(0xb19397a1f2c5bd6e);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static int compare(Cod1SSAbsentCache56 *cache,const char*data,size_t size,const char *alias,const char*map,int profile,int loop){
 Cod1SSPlan a,b;Cod1SSDiagnostic da,db;int x,y,ea,eb;
 memset(&a,0xa5,sizeof(a));memset(&b,0xa5,sizeof(b));errno=EDOM;x=ReferenceParse56(data,size,alias,map,&a,&da,profile,loop);ea=errno;errno=EDOM;y=Cod1SS_ParseCached56(cache,"a.csv",data,size,alias,map,&b,&db,profile,loop);eb=errno;
 assert(x==y&&da.reason==db.reason&&da.line==db.line&&ea==eb);assert(!memcmp(&a,&b,sizeof(a)));assert(cache->used<=COD1_SS56_BYTES&&cache->index_used<=COD1_SS56_INDEX_BYTES);return y;
}
int main(void){
 static Cod1SSAbsentCache56 cache;static char table[131072];char alias[64];size_t size=0;unsigned i,j,ready=0,none=0,held=0,used_index=0;uint64_t reference_rows=0,candidate_rows=0;const char *maps[]={"training","us_intro","maps/training.bsp","bad map"};
 for(i=0;i<1000;i++){
  size=(size_t)snprintf(table,sizeof(table),"%s%s",i%2?"\xef\xbb\xbf":"",i%7==0?"file,name,sequence,loadspec,vol_min\r\n":"# comment\r\nname,file,sequence,loadspec,vol_min\n");
  for(j=0;j<128;j++){
   unsigned name=rnd()%40;const char*map=j%3==0?"! training":j%3==1?"training":"";const char *vol=j%17==0?"bad":j%9==0?"0.123456789":"1";
   if(i%7==0)size+=(size_t)snprintf(table+size,sizeof(table)-size,"body/a.wav,a%u,%u,%s,%s\r\n",name,j,map,vol);
   else size+=(size_t)snprintf(table+size,sizeof(table)-size,"%sa%u%s,body/a.wav,%u,%s,%s%s",j%2?"\"":"",name,j%2?"\"":"",j,map,vol,j%3?"\n":"\r");
   if(j%19==0)size+=(size_t)snprintf(table+size,sizeof(table)-size,"# skipped,\"broken\n\n");
  }
  if(i%13==0)size+=(size_t)snprintf(table+size,sizeof(table)-size,"bad,\"unterminated\n");
  for(j=0;j<50;j++){
   int r;const char *map=maps[rnd()%4];snprintf(alias,sizeof(alias),"%s%u",j%2?"A":"a",rnd()%55);
   if(j==0){strcpy(alias,"nonexistent");map="training";}
   if(j%11==0)assert(!fesetround(j%22==0?FE_DOWNWARD:FE_UPWARD));else assert(!fesetround(FE_TONEAREST));
   if(cache.tables[0].bytes&&cache.tables[0].size==size&&!memcmp(cache.tables[0].bytes,table,size)&&cache.tables[0].index.ready==1)++used_index;
   r=compare(&cache,table,size,alias,map,(int)(rnd()%5),0);ready+=r==1;none+=r==0;held+=r==-1;
  }
  if(i%31==0)Cod1SS_AbsentReset56(&cache);
 }
 assert(!fesetround(FE_TONEAREST));Cod1SS_AbsentReset56(&cache);
 /* Same-size content changes, malformed structure, and allocation fallback. */
 {const char *a="name,file\na,body/a.wav\n",*b="name,file\nb,body/a.wav\n";size_t n=strlen(a);
 assert(compare(&cache,a,n,"a","training",0,0)==1);assert(cache.tables[0].index.ready==1);assert(compare(&cache,b,n,"a","training",0,0)==0);assert(compare(&cache,b,n,"b","training",0,0)==1);
 Cod1SS_AbsentReset56(&cache);fail_alloc=2;assert(compare(&cache,a,n,"a","training",0,0)==1);assert(cache.tables[0].index.ready==-1);assert(compare(&cache,a,n,"a","training",0,0)==1);Cod1SS_AbsentReset56(&cache);}
 /* Full 32-bit hash collision still compares the decoded alias. */
 {const char *collision="name,file,vol_min\npyotdougleq5,body/a.wav,1\nvgyikbeb5jms,body/b.wav,bad\n";
 assert(Cod1SS_RowHash56("pyotdougleq5")==Cod1SS_RowHash56("vgyikbeb5jms"));
 assert(compare(&cache,collision,strlen(collision),"pyotdougleq5","training",0,0)==1);
 assert(cache.tables[0].index.ready==1);
 assert(compare(&cache,collision,strlen(collision),"VGYIKBEB5JMS","training",0,0)==-1);
 assert(compare(&cache,collision,strlen(collision),"pyotdougleq5","training",0,0)==1);Cod1SS_AbsentReset56(&cache);}
 /* Exact per-table and global index limits: metadata refusal is not a resource refusal. */
 {char *large=malloc(1024*1024);Cod1SSPlan plan;Cod1SSDiagnostic diag;char path[32];size_t n;
 assert(large);n=(size_t)sprintf(large,"name,file\n");
 for(i=0;i<COD1_SS56_ROW_LIMIT;i++){memcpy(large+n,"a,a.wav\n",8);n+=8;}
 for(i=0;i<5;i++){snprintf(path,sizeof(path),"%u.csv",i);assert(Cod1SS_ParseCached56(&cache,path,large,n,"absent","training",&plan,&diag,0,0)==0);assert(cache.tables[i].index.ready==(i<4?1:-1));}
 assert(cache.index_used==COD1_SS56_INDEX_BYTES);Cod1SS_AbsentReset56(&cache);assert(!cache.index_used);
 memcpy(large+n,"a,a.wav\n",8);n+=8;assert(Cod1SS_ParseCached56(&cache,"large.csv",large,n,"absent","training",&plan,&diag,0,0)==0);assert(cache.tables[0].index.ready==-1);free(large);Cod1SS_AbsentReset56(&cache);}
 /* 4096 rows, 128 aliases; count indexed row candidates including collisions. */
 size=(size_t)snprintf(table,sizeof(table),"name,file,sequence\n");for(i=0;i<4096;i++)size+=(size_t)snprintf(table+size,sizeof(table)-size,"a%u,b.wav,%u\n",i%128,i/128);assert(size<sizeof(table));
 assert(compare(&cache,table,size,"a0","training",0,0)==1);assert(cache.tables[0].index.count==4096);
 for(i=0;i<128;i++){uint32_t slot,hash;snprintf(alias,sizeof(alias),"a%u",i);hash=Cod1SS_RowHash56(alias);for(slot=cache.tables[0].index.head[hash%COD1_SS56_BUCKETS];slot;slot=cache.tables[0].index.rows[slot-1].next)++candidate_rows;reference_rows+=4096;assert(compare(&cache,table,size,alias,"training",0,0)==1);}
 Cod1SS_AbsentReset56(&cache);assert(!cache.used&&!cache.index_used);assert(ready&&none&&held&&used_index&&candidate_rows<reference_rows);
 printf("R56A2B PASS: 50000 cases ready=%u none=%u held=%u warm-index opportunities=%u; candidate visits=%llu vs %llu reference rows\n",ready,none,held,used_index,(unsigned long long)candidate_rows,(unsigned long long)reference_rows);return 0;
}
