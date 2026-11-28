#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define MAX_LOCALS 32
typedef struct {char name[64];unsigned value;} Local;
typedef struct {int local_count;Local locals[MAX_LOCALS];} Frame;
static uint64_t comparisons;
static int counted_strcmp(const char*a,const char*b){++comparisons;return strcmp(a,b);}
#define strcmp counted_strcmp
#include "gsc_vm_local_lookup55.inc"
static int reference(const Frame*f,const char*name,int reverse){
 int n;
 if(reverse){for(n=f->local_count-1;n>=0;n--)if(!strcmp(f->locals[n].name,name))return n;}
 else{for(n=0;n<f->local_count;n++)if(!strcmp(f->locals[n].name,name))return n;}
 return -1;
}
#undef strcmp
static uint64_t rng=UINT64_C(0xe38a61278179b62f);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static const char*names[]={"","a","ab","abc","AB","self","level","foo","foobar","food","fool","actor","actors","anim","animation","\xc0","\xc0\xe0","\xe0","field","Field","fieldName","fieldname","x","xy","xyz","z","_x","_y"};
int main(void){
 unsigned i,j,hit=0,miss=0;uint64_t old_calls=0,new_calls=0;
 for(i=0;i<200000;i++){
  Frame f={0},before;const char*name;int dir,a,b;f.local_count=(int)(rnd()%33);
  for(j=0;j<(unsigned)f.local_count;j++){
   if(i%11==0){memset(f.locals[j].name,'x',63);f.locals[j].name[62]=(char)('a'+rnd()%26);}
   else strcpy(f.locals[j].name,names[rnd()%(sizeof(names)/sizeof(names[0]))]);
   f.locals[j].value=rnd();
  }
  name=f.local_count&&i%3==0?f.locals[rnd()%(unsigned)f.local_count].name:names[rnd()%(sizeof(names)/sizeof(names[0]))];
  before=f;
  for(dir=0;dir<2;dir++){
   comparisons=0;a=reference(&f,name,dir);old_calls+=comparisons;
   comparisons=0;b=frame_local_find55(&f,name,dir);new_calls+=comparisons;
   assert(a==b&&!memcmp(&before,&f,sizeof(f)));if(a>=0)++hit;else ++miss;
  }
 }
 {Frame f={0};f.local_count=3;strcpy(f.locals[0].name,"same");strcpy(f.locals[1].name,"same");strcpy(f.locals[2].name,"same");assert(frame_local_find55(&f,"same",0)==0&&frame_local_find55(&f,"same",1)==2);}
 assert(hit&&miss&&new_calls<old_calls);printf("R55C4 PASS: 400000 lookups hits=%u misses=%u strcmp=%llu -> %llu\n",hit,miss,(unsigned long long)old_calls,(unsigned long long)new_calls);return 0;
}
