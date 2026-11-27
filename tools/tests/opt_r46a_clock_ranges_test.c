#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1AGF14_MAX 4096u
/* Exact metadata member layout from cod1_actor_notetrack_metadata14.h. */
typedef struct {uint16_t token;float phase;} c1anc14Note_t;
typedef struct {uint32_t magic;unsigned count,authoredCount,loop;float frequency;
 c1anc14Note_t notes[256];char payload[256][128];} c1anm14Metadata_t;
/* Exact existing C1ANC14_Overlap from cod1_actor_named_clock14.h. */
static int C1ANC14_Overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;
}
#include "cod1_actor_clock_ranges46.h"
static const c1anm14Metadata_t *table[C1AGF14_MAX];
static c1agc46Ranges_t proof;
static unsigned long checks,hits,fallbacks;
static uint32_t random32(void){static uint32_t s=0x1c79e121u;s^=s<<13;s^=s>>17;s^=s<<5;return s;}
static const void*ptr(uintptr_t x){return (const void*)x;}
static int reference(unsigned count,const void*a,size_t an,const void*b,size_t bn,const void*c,size_t cn){
 unsigned i;
 for(i=0;i<count;i++)if(table[i]&&(C1ANC14_Overlap(a,an,table[i],sizeof(*table[i]))||C1ANC14_Overlap(b,bn,table[i],sizeof(*table[i]))||C1ANC14_Overlap(c,cn,table[i],sizeof(*table[i]))))return 0;
 return 1;
}
static void compare(unsigned count,const void*a,size_t an,const void*b,size_t bn,const void*c,size_t cn){
 int exact=reference(count,a,an,b,bn,c,cn),got=C1AGC46_Admit(&proof,table,count,a,an,b,bn,c,cn);
 if(got<0){fallbacks++;got=exact;}else hits++;
 assert(got==exact);checks++;
}
int main(void){
 unsigned i,j,k,count;uintptr_t x;size_t sz=sizeof(c1anm14Metadata_t);
 for(i=0;i<80;i++){
  count=(i%4==0)?2078:(i%4==1)?4096:(i%4==2)?1:1+random32()%199;
  memset(table,0,sizeof(table));
  for(j=0;j<count;j++)if(i!=0&&random32()%4==0){
   x=(uintptr_t)random32()|1u;
   if(i%5==0)x=UINTPTR_MAX-(random32()%100000u);
   if(i%7==0)x=1234567u; /* duplicate and overlapping metadata */
   table[j]=ptr(x);
  }
  assert(C1AGC46_Prepare(table,count,&proof));
  for(j=0;j<160;j++){
   uintptr_t q[3];size_t n[3];
   for(k=0;k<3;k++){
    x=(uintptr_t)random32();
    if(j%3==0){unsigned z=random32()%count;x=(uintptr_t)table[z];if(x)x+=(uintptr_t)(random32()%((unsigned)sz+2));}
    if(j%7==0)x=UINTPTR_MAX-(random32()%100000u);
    q[k]=x;n[k]=(size_t)(random32()%100000u)+1;
   }
   compare(count,ptr(q[0]),n[0],ptr(q[1]),n[1],ptr(q[2]),n[2]);
  }
  /* A stale pointer table must use fallback, not a cached decision. */
  table[0]=ptr((uintptr_t)table[0]^1u);assert(C1AGC46_Admit(&proof,table,count,ptr(17),8,ptr(23),4,ptr(40),8)==-1);
  compare(count,ptr(17),8,ptr(23),4,ptr(40),8);
 }
 memset(table,0,sizeof(table));table[0]=ptr(100000);table[1]=ptr(100000+sz+10);table[2]=table[0];
 assert(C1AGC46_Prepare(table,3,&proof));
 /* Boundary, zero-length, huge length and high-address arithmetic: the
  * reference predicate is subtractive, not a conventional interval API. */
 for(i=0;i<7;i++)for(j=0;j<7;j++){
  const uintptr_t a[]={0,99999,100000,100001,100000+sz-1,100000+sz,UINTPTR_MAX};
  const size_t n[]={0,1,2,sz-1,sz,sz+1,SIZE_MAX};
  compare(3,ptr(a[i]),n[j],ptr(1),1,ptr(2),1);
 }
 assert(C1AGC46_Admit(&proof,table,3,&proof,sizeof(proof),ptr(1),1,ptr(2),1)==-1);
 assert(!C1AGC46_Prepare(table,0,&proof)&&!C1AGC46_Prepare(table,C1AGF14_MAX+1,&proof));
 assert(C1AGC46_Admit(&proof,table,2,ptr(1),1,ptr(2),1,ptr(3),1)==-1);
 printf("R46A ranges: %lu comparisons, %lu indexed, %lu fallback: PASS\n",checks,hits,fallbacks);return 0;
}
