#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CAP 256
#define CASES 100000u
enum {V_LEVEL=5,V_ENTITY=4,V_STRUCT=9,V_ANIM=10,V_VEHICLE_NODE=16};
typedef struct {uint64_t epoch;uint32_t slot,generation;} GscEntityRef;
typedef struct {uint64_t epoch;uint32_t index,generation;} GscVehicleNodeRef;
typedef struct {int kind;GscEntityRef entity;GscVehicleNodeRef vehicle_node;uint32_t object;} Value;
typedef struct {int used,owner_kind;GscEntityRef owner;char name[128];Value value;uint32_t struct_owner;GscVehicleNodeRef vehicle_owner;} StateField;
typedef struct {StateField *state_fields;int state_field_capacity,state_field_scan_end_plus1,controls_enabled;} GscRuntime;
static int state_ref_equal(const GscEntityRef *a,const GscEntityRef *b){return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;}
static int vehicle_node_ref_equal(const GscVehicleNodeRef *a,const GscVehicleNodeRef *b){return a->epoch==b->epoch&&a->index==b->index&&a->generation==b->generation;}
static int control_field_equal(const char *a,const char *b) {
    unsigned char ca,cb;
    do {
        ca=(unsigned char)*a++;cb=(unsigned char)*b++;
        if(ca>='A'&&ca<='Z')ca=(unsigned char)(ca+('a'-'A'));
        if(cb>='A'&&cb<='Z')cb=(unsigned char)(cb+('a'-'A'));
        if(ca!=cb)return 0;
    } while(ca);
    return 1;
}
#include "bound.inc"
#define state_field_index old_index
#include "old.inc"
#undef state_field_index
#include "new.inc"
static uint64_t rng=UINT64_C(0xa859321c9d77f28b);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static const int kinds[]={V_LEVEL,V_ENTITY,V_STRUCT,V_ANIM,V_VEHICLE_NODE,3};
static const char*names[]={"field","Field","fIELD","target","x","", "\xc0", "\xe0"};
static Value owner(void){Value v={0};v.kind=kinds[rnd()%6];v.entity=(GscEntityRef){rnd()%3,rnd()%7,rnd()%4};v.vehicle_node=(GscVehicleNodeRef){rnd()%3,rnd()%7,rnd()%4};v.object=rnd()%7;return v;}
static StateField field(Value v,const char*name,int used){StateField f={0};f.used=used;f.owner_kind=v.kind;f.owner=v.entity;f.vehicle_owner=v.vehicle_node;f.struct_owner=v.object;strcpy(f.name,name);f.value=owner();return f;}
int main(void){
 StateField fields[CAP]={0};GscRuntime r={fields,32,1,0};unsigned i,hit=0,miss=0,unknown=0;uint64_t oldrows=0,newrows=0;
 for(i=0;i<CASES;i++){
  Value v=owner();const char*name=names[rnd()%8];int a,b,slot,j;StateField f;
  if(i%127==0){memset(fields,0,sizeof(fields));r.state_field_capacity=32;r.state_field_scan_end_plus1=1;}
  if(i%29==0&&r.state_field_capacity<CAP)r.state_field_capacity+=16;
  /* Publish both sparse and repeated slot writes, without requiring dense prefix. */
  slot=(int)(rnd()%(unsigned)(r.state_field_capacity/2+1));f=field(v,name,i%13?1:0);state_field_publish(&r,slot,&f);
  if(i%11==0){slot=(int)(rnd()%(unsigned)r.state_field_capacity);f=field(v,name,1);state_field_publish(&r,slot,&f);}
  if(i%5==0){v=owner();name=names[rnd()%8];}
  if(i%7==0){name=names[rnd()%8];}
  r.controls_enabled=i&1;
  for(j=state_fields_scan_end(&r);j<r.state_field_capacity;j++)assert(!fields[j].used);
  a=old_index(&r,&v,name);b=state_field_index(&r,&v,name);assert(a==b);
  hit+=a>=0;miss+=a<0;oldrows+=(unsigned)(a>=0?a+1:r.state_field_capacity);newrows+=(unsigned)(b>=0?b+1:state_fields_scan_end(&r));
  if(i%17==0){int saved=r.state_field_scan_end_plus1;r.state_field_scan_end_plus1=0;assert(old_index(&r,&v,name)==state_field_index(&r,&v,name));r.state_field_scan_end_plus1=r.state_field_capacity+2;assert(old_index(&r,&v,name)==state_field_index(&r,&v,name));r.state_field_scan_end_plus1=saved;++unknown;}
 }
 /* Prior matching record must win even if published after higher slot. */
 memset(fields,0,sizeof(fields));r.state_field_capacity=CAP;r.state_field_scan_end_plus1=1;r.controls_enabled=1;
 {Value v=owner();StateField f=field(v,"FIELD",1);state_field_publish(&r,200,&f);state_field_publish(&r,2,&f);assert(state_field_index(&r,&v,"field")==2);}
 /* Unknown storage must never infer an empty prefix from a write. */
 r.state_field_scan_end_plus1=0;{Value v=owner();StateField f=field(v,"target",1);state_field_publish(&r,255,&f);assert(!r.state_field_scan_end_plus1&&state_field_index(&r,&v,"target")==old_index(&r,&v,"target"));}
 assert(hit&&miss&&newrows<oldrows);printf("PASS: %u cases, hit=%u miss=%u fallback=%u rows=%llu -> %llu\n",CASES,hit,miss,unknown,(unsigned long long)oldrows,(unsigned long long)newrows);return 0;
}
