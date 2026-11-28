#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GSC_STATE_FIELD_CAPACITY 16
#define GSC_STATE_FIELD_OWNED_LIMIT 256
#define CASES 100000u
typedef struct {int used;unsigned payload[7];} StateField;
typedef struct {StateField *state_fields;int state_field_capacity;uint32_t state_field_limit;int state_arena_busy,alloc_fail,free_fail,allocs,frees;char error[100];} GscRuntime;
static int rfail(GscRuntime*r,int line,const char*s){(void)line;snprintf(r->error,sizeof(r->error),"%s",s);return 0;}
static void *state_storage_allocate(GscRuntime*r,size_t n,size_t width,int line){r->allocs++;if(r->alloc_fail){rfail(r,line,"allocation failed");return NULL;}return calloc(n,width);}
static int state_storage_free(GscRuntime*r,void*p){if(!p)return 1;r->frees++;free(p);return r->free_fail?rfail(r,0,"free failed"):1;}
#define state_fields_limit old_limit
#define state_fields_used old_used
#define state_fields_can_fit old_fit
#define state_fields_reserve_free old_reserve
#include "old.inc"
#include "old_reserve.inc"
#undef state_fields_limit
#undef state_fields_used
#undef state_fields_can_fit
#undef state_fields_reserve_free
#include "new.inc"
#include "new_reserve.inc"
static uint64_t rng=UINT64_C(0xb19397a1f2c5bd6e);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
int main(void){unsigned i,yes=0,no=0,grow=0,fail=0;for(i=0;i<CASES;i++){
 GscRuntime a={0},b;size_t empty;int n,ra,rb,cap=(int)(rnd()%129);
 a.state_field_capacity=cap;a.state_field_limit=(i%7==0)?0:16+rnd()%241;a.state_arena_busy=i%79==0;a.alloc_fail=i%31==0;a.free_fail=i%47==0;
 if(cap){a.state_fields=calloc((size_t)cap,sizeof(StateField));assert(a.state_fields);for(n=0;n<cap;n++){a.state_fields[n].used=i%3==0?n<cap/2:(int)(rnd()%3);a.state_fields[n].payload[0]=rnd();}}
 b=a;if(cap){b.state_fields=malloc((size_t)cap*sizeof(StateField));assert(b.state_fields);memcpy(b.state_fields,a.state_fields,(size_t)cap*sizeof(StateField));}
 empty=i%83==0?SIZE_MAX:i%5==0?0:i%5==1?1:rnd()%257;
 assert(old_fit(&a,empty)==state_fields_can_fit(&b,empty));
 ra=old_reserve(&a,empty,1);rb=state_fields_reserve_free(&b,empty,1);
 assert(ra==rb&&a.state_field_capacity==b.state_field_capacity&&a.allocs==b.allocs&&a.frees==b.frees&&!strcmp(a.error,b.error));
 if(a.state_field_capacity)assert(!memcmp(a.state_fields,b.state_fields,(size_t)a.state_field_capacity*sizeof(StateField)));
 yes+=ra!=0;no+=ra==0;grow+=a.state_field_capacity>cap;fail+=a.allocs&&a.error[0];free(a.state_fields);free(b.state_fields);
 }assert(yes&&no&&grow&&fail);printf("PASS %u cases accepted=%u refused=%u grown=%u alloc/free failure=%u\n",CASES,yes,no,grow,fail);
 /* Inspect algorithmic work independently: free tail or no reserve needed. */
 {GscRuntime r={0};StateField fields[128]={0};r.state_fields=fields;r.state_field_capacity=128;r.state_field_limit=256;for(i=0;i<96;i++)fields[i].used=1;assert(state_fields_unused_up_to(&r,1)==1&&state_fields_unused_up_to(&r,32)==32&&state_fields_unused_up_to(&r,33)==32);assert(state_fields_can_fit(&r,160)&&!state_fields_can_fit(&r,161));}return 0;}
