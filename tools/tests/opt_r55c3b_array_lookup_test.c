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
typedef struct {int array_entry_count,array_lookup_entries;ArrayEntry array_entries[GSC_ARRAY_ENTRY_CAPACITY];uint16_t array_lookup_head[GSC_ARRAY_CONTAINER_CAPACITY],array_lookup_tail[GSC_ARRAY_CONTAINER_CAPACITY],array_lookup_next[GSC_ARRAY_ENTRY_CAPACITY];} GscRuntime;
static int array_key_equal(const ArrayKey*a,const ArrayKey*b){return a->kind==b->kind&&(a->kind==V_NUMBER?a->number==b->number:!strcmp(a->string,b->string));}
#include "gsc_vm_array_lookup55.inc"
static uint64_t rng=UINT64_C(0xb72d8a3215fac9e3);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static ArrayKey key(void){ArrayKey k={0};k.kind=(rnd()&1)?V_NUMBER:V_STRING;k.number=(int)(rnd()%24);snprintf(k.string,sizeof(k.string),"key%u",rnd()%24);return k;}
static ArrayEntry entry(void){ArrayEntry e={0};e.owner=(int)(rnd()%258);e.key=key();e.value=(int)rnd();return e;}
static void check(const GscRuntime*r){
 unsigned char seen[GSC_ARRAY_ENTRY_CAPACITY]={0};int n;unsigned id;
 assert(r->array_lookup_entries==r->array_entry_count);
 for(id=1;id<=GSC_ARRAY_CONTAINER_CAPACITY;id++){
  uint16_t slot,previous=0;
  for(slot=r->array_lookup_head[id-1];slot;slot=r->array_lookup_next[slot-1]){
   assert(slot>previous&&slot<=r->array_entry_count&&!seen[slot-1]);assert(r->array_entries[slot-1].owner==(int)id);seen[slot-1]=1;previous=slot;
  }
  assert(previous==r->array_lookup_tail[id-1]);
 }
 for(n=0;n<r->array_entry_count;n++)assert(seen[n]==(r->array_entries[n].owner>=1&&r->array_entries[n].owner<=GSC_ARRAY_CONTAINER_CAPACITY));
}
int main(void){
 static GscRuntime r;uint64_t reference_rows=0,index_rows=0,maintenance_rows=0,hits=0,misses=0,appends=0,bulk=0,rebuilds=0,queries=0;unsigned i,j;
 for(i=0;i<10000;i++){
  memset(&r,0,sizeof(r));
  for(j=0;j<128;j++){ArrayEntry e=entry();array_entry_append55(&r,&e);++appends;++maintenance_rows;}
  for(j=0;j<32;j++){
   unsigned op=rnd()%12;int count=r.array_entry_count;
   if(op<=2&&count<4096){ArrayEntry e=entry();array_entry_append55(&r,&e);++appends;++maintenance_rows;}
   else if(op==3&&count<4092){int k;for(k=0;k<4;k++)r.array_entries[r.array_entry_count++]=entry();array_index_appended55(&r,count);++bulk;maintenance_rows+=4;}
   else if(op==4&&count){int k=(int)(rnd()%(unsigned)count);memmove(r.array_entries+k,r.array_entries+k+1,(size_t)(count-k-1)*sizeof(ArrayEntry));--r.array_entry_count;array_index_rebuild55(&r);++rebuilds;maintenance_rows+=r.array_entry_count;}
   else if(op==5&&count){int k,m=0;for(k=0;k<count;k++)if(r.array_entries[k].owner%3){ArrayEntry e=r.array_entries[k];e.owner=(e.owner+11)%258;r.array_entries[m++]=e;}r.array_entry_count=m;array_index_rebuild55(&r);++rebuilds;maintenance_rows+=m;}
   else if(op==6&&count){r.array_entry_count=count-1;array_index_rebuild55(&r);++rebuilds;maintenance_rows+=r.array_entry_count;}
   else if(op==7&&count){r.array_entries[rnd()%(unsigned)count].value=42;}
   else if(op==8&&count<4096){/* Missed append must fall back by count, then recover on next publication. */
    ArrayKey k;int id;r.array_entries[r.array_entry_count++]=entry();k=r.array_entries[count].key;id=r.array_entries[count].owner;
    assert(array_entry_find(&r,(uint32_t)id,&k)==array_entry_find_reference55(&r,(uint32_t)id,&k));array_index_rebuild55(&r);++rebuilds;maintenance_rows+=r.array_entry_count;
   }
   check(&r);
   {uint32_t owner=rnd()%258;ArrayKey k=key();int a,b;uint16_t pos;
    if(j%3==0&&r.array_entry_count){int n=(int)(rnd()%(unsigned)r.array_entry_count);owner=(uint32_t)r.array_entries[n].owner;k=r.array_entries[n].key;}
    a=array_entry_find_reference55(&r,owner,&k);b=array_entry_find(&r,owner,&k);assert(a==b);
    reference_rows+=(uint64_t)(a<0?r.array_entry_count:a+1);
    if(!owner||owner>256)index_rows+=(uint64_t)(a<0?r.array_entry_count:a+1);
    else for(pos=r.array_lookup_head[owner-1];pos;pos=r.array_lookup_next[pos-1]){++index_rows;if((int)pos-1==b)break;}
    ++queries;hits+=b>=0;misses+=b<0;
   }
  }
 }
 memset(&r,0,sizeof(r));for(i=0;i<4096;i++){ArrayEntry e={1,{V_NUMBER,(int)i,{0}},0};array_entry_append55(&r,&e);}check(&r);
 {ArrayKey k={V_NUMBER,4095,{0}};assert(array_entry_find(&r,1,&k)==4095);k.number=0;assert(array_entry_find(&r,1,&k)==0);k.number=5000;assert(array_entry_find(&r,1,&k)==-1);r.array_lookup_next[0]=1;assert(array_entry_find(&r,1,&k)==-1);array_index_rebuild55(&r);}
 memset(&r,0,sizeof(r));{ArrayEntry e={7,{V_STRING,0,"same"},1};array_entry_append55(&r,&e);e.value=2;array_entry_append55(&r,&e);assert(array_entry_find(&r,7,&e.key)==0);r.array_lookup_head[6]=0;assert(array_entry_find(&r,7,&e.key)==0);}
 printf("R55C3B PASS: %llu queries hits=%llu misses=%llu append=%llu bulk=%llu rebuild=%llu reference_rows=%llu lookup_rows=%llu maintenance_rows=%llu\n",(unsigned long long)queries,(unsigned long long)hits,(unsigned long long)misses,(unsigned long long)appends,(unsigned long long)bulk,(unsigned long long)rebuilds,(unsigned long long)reference_rows,(unsigned long long)index_rows,(unsigned long long)maintenance_rows);return 0;
}
