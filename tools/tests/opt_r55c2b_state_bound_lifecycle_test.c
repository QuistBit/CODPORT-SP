#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GSC_STATE_FIELD_CAPACITY 16
#define GSC_STATE_FIELD_OWNED_LIMIT 256
typedef struct {int used;unsigned payload[7];} StateField;
typedef struct {StateField *state_fields;int state_field_capacity,state_field_scan_end_plus1;uint32_t state_field_limit;int state_arena_busy,alloc_fail,free_fail;char error[100];} GscRuntime;
static int rfail(GscRuntime*r,int line,const char*s){(void)line;snprintf(r->error,sizeof(r->error),"%s",s);return 0;}
static void *state_storage_allocate(GscRuntime*r,size_t n,size_t width,int line){if(r->alloc_fail){rfail(r,line,"allocation failed");return NULL;}return calloc(n,width);}
static int state_storage_free(GscRuntime*r,void*p){if(!p)return 1;free(p);return r->free_fail?rfail(r,0,"free failed"):1;}
#include "bound.inc"
#include "reserve_helpers.inc"
#include "reserve.inc"
int main(void){unsigned i;for(i=0;i<4096;i++){
 GscRuntime r={0};StateField field={1,{42}};int mode=i%4; r.state_field_limit=256;
 assert(state_fields_can_fit(&r,16));r.alloc_fail=mode==1;
 assert(state_fields_reserve_free(&r,16,0)==(mode!=1));
 if(mode==1){assert(!r.state_fields&&!r.state_field_capacity&&!r.state_field_scan_end_plus1);continue;}
 assert(r.state_field_capacity==16&&r.state_field_scan_end_plus1==1&&state_fields_scan_end(&r)==0);
 state_field_publish(&r,3,&field);state_field_publish(&r,15,&field);assert(state_fields_scan_end(&r)==16);
 if(mode==2)r.state_field_scan_end_plus1=0;
 if(mode==3)r.free_fail=1;
 assert(state_fields_reserve_free(&r,16,0)==(mode!=3));
 assert(r.state_field_capacity==32&&r.state_fields[3].used&&r.state_fields[15].payload[0]==42);
 assert(state_fields_scan_end(&r)==(mode==2?32:16));
 if(mode!=3){state_field_publish(&r,30,&field);assert(state_fields_scan_end(&r)==(mode==2?32:31));}
 state_fields_release(&r);assert(!r.state_fields&&!r.state_field_capacity&&!r.state_field_scan_end_plus1);
 }puts("PASS: 4096 lifecycle cases, allocation, growth, failure, release and unknown bound");return 0;}
