#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define GSC_ARRAY_CONTAINER_CAPACITY 256
#define GSC_ARRAY_ENTRY_CAPACITY 4096
#define V_NUMBER 1
#define V_STRING 2
typedef struct {int kind,number;char string[16];} ArrayKey;
typedef struct {int owner;ArrayKey key;int value;} ArrayEntry;
typedef struct {int arrays,entries;ArrayEntry members[GSC_ARRAY_ENTRY_CAPACITY];int lookup_entries;uint16_t lookup_head[GSC_ARRAY_CONTAINER_CAPACITY],lookup_tail[GSC_ARRAY_CONTAINER_CAPACITY],lookup_next[GSC_ARRAY_ENTRY_CAPACITY];} R238ArrayStage;
static uint64_t comparisons;
static int array_key_equal(const ArrayKey*a,const ArrayKey*b){++comparisons;return a->kind==b->kind&&(a->kind==V_NUMBER?a->number==b->number:!strcmp(a->string,b->string));}
#include "gsc_vm_cow_lookup55.inc"
static uint64_t rng=UINT64_C(0xb72d8a3215fac9e3);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static ArrayKey key(void){ArrayKey k={0};k.kind=(rnd()&1)?V_NUMBER:V_STRING;k.number=(int)(rnd()%24);snprintf(k.string,sizeof(k.string),"key%u",rnd()%24);return k;}
static void append(R238ArrayStage*s){ArrayEntry*e=&s->members[s->entries++];e->owner=(int)(rnd()%258);e->key=key();e->value=(int)(rnd()%999);}
int main(void){
 static R238ArrayStage s;unsigned i,j;uint64_t reference_rows=0,index_rows=0,build_rows=0,queries=0,hits=0,misses=0;int a,b,prior;
 for(i=0;i<10000;i++){
  s.entries=0;s.lookup_entries=-1;s.arrays=256;
  for(j=0;j<128+rnd()%128;j++)append(&s);
  for(j=0;j<32;j++){
   uint32_t owner=rnd()%258;ArrayKey k=key();
   if(j%3==0&&s.entries){int idx=(int)(rnd()%(unsigned)s.entries);owner=(uint32_t)s.members[idx].owner;k=s.members[idx].key;}
   if(j%4==0&&s.entries<4096)append(&s);
   a=r238_stage_find_reference55(&s,owner,&k);prior=s.lookup_entries;b=r238_stage_find(&s,owner,&k);assert(a==b);
   reference_rows+=(uint64_t)(a<0?s.entries:a+1);
   if(!owner||owner>256||prior==-1)index_rows+=(uint64_t)(b<0?s.entries:b+1);
   else{uint16_t pos;build_rows+=(uint64_t)(s.entries-(prior<0||prior>s.entries?0:prior));for(pos=s.lookup_head[owner-1];pos;pos=s.lookup_next[pos-1]){++index_rows;if((int)pos-1==b)break;}}
   ++queries;hits+=b>=0;misses+=b<0;
  }
 }
 /* Full-capacity, duplicate order, unknown owners, and scratch reuse. */
 s.entries=4096;s.lookup_entries=-1;for(i=0;i<4096;i++){s.members[i].owner=1;s.members[i].key=(ArrayKey){V_NUMBER,(int)i,{0}};}
 {ArrayKey k={V_NUMBER,4095,{0}};assert(r238_stage_find(&s,1,&k)==4095);assert(r238_stage_find(&s,1,&k)==4095);k.number=0;assert(r238_stage_find(&s,1,&k)==0);}
 s.lookup_entries=-1;s.entries=2;s.members[0].owner=s.members[1].owner=7;s.members[0].key=s.members[1].key=(ArrayKey){V_STRING,0,"same"};
 {ArrayKey k={V_STRING,0,"same"};assert(r238_stage_find(&s,7,&k)==0);assert(r238_stage_find(&s,7,&k)==0);assert(r238_stage_find(&s,1,&k)==-1);}
 s.entries=1;{ArrayKey k={V_STRING,0,"same"};assert(r238_stage_find(&s,7,&k)==0);}
 assert(hits&&misses&&reference_rows>index_rows+build_rows);
 printf("R55C3A PASS: %llu queries hits=%llu misses=%llu reference_rows=%llu lookup_rows=%llu build_rows=%llu\n",(unsigned long long)queries,(unsigned long long)hits,(unsigned long long)misses,(unsigned long long)reference_rows,(unsigned long long)index_rows,(unsigned long long)build_rows);return 0;
}
