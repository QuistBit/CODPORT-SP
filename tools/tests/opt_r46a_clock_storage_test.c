/* Actual old/new storage entry, with a stub of the unchanged clock kernel.
 * Tests only preflight, fallback, output publication and disposable storage. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1AGF14_MAX 4096u
typedef struct {uint16_t token;float phase;} c1anc14Note_t;
typedef struct {uint32_t magic;unsigned count,authoredCount,loop;float frequency;
 c1anc14Note_t notes[256];char payload[256][128];} c1anm14Metadata_t;
typedef struct {unsigned parent,children,flags;} c1agt14Definition_t;
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
typedef struct {unsigned count,first[C1AGF14_MAX],next[C1AGF14_MAX],order[C1AGF14_MAX];} c1agf14Topology_t;
typedef struct {unsigned node,leaf;uint16_t notify,payload;} c1agc14Event_t;
typedef struct {unsigned count,resume;float usedDt;c1agc14Event_t event[1024];} c1agc14Step_t;
typedef struct {const c1agt14Definition_t*d;const c1agf14Topology_t*t;const c1anm14Metadata_t*const*m;const float*frequency;c1atl14Node_t*n;c1afl14State_t*f;c1agc14Step_t*out;int owner,ok;} c1agc14Context_t;
static int C1ANC14_Overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
#include "cod1_actor_clock_ranges46.h"
static unsigned called;static int innerOK;
static int C1AGC14_Inner(c1agc14Context_t*c,float dt,int notify){++called;c->n[0].phase=dt;c->f[0].cursor=(int16_t)notify;memset(c->out,0,sizeof(*c->out));c->out->usedDt=dt;return innerOK;}
#define C1AGC14_AdvanceStorage RefStorage
#define C1AGC14_Advance RefAdvance
#define C1AGC14_AdvanceDisposable RefDisposable
#include "old.inc"
#undef C1AGC14_AdvanceStorage
#undef C1AGC14_Advance
#undef C1AGC14_AdvanceDisposable
#include "new.inc"
#define COUNT 2078u
static c1agt14Definition_t d[COUNT];static c1agf14Topology_t t;
static const c1anm14Metadata_t *m[COUNT];static c1anm14Metadata_t resource;
static float frequency[COUNT];static c1atl14Node_t n[COUNT],refn[COUNT];static c1afl14State_t f[COUNT],reff[COUNT];
static c1agc14Step_t out,refout;static c1agc46Ranges_t ranges;
static void reset(void){memset(n,0,sizeof(n));memset(f,0,sizeof(f));memset(&out,0xab,sizeof(out));memcpy(refn,n,sizeof(n));memcpy(reff,f,sizeof(f));refout=out;called=0;}
int main(void){unsigned i,checks=0;int a,b;t.count=COUNT;m[5]=&resource;assert(C1AGC46_Prepare(m,COUNT,&ranges));
 for(i=0;i<4;i++){
  innerOK=(int)(i&1);reset();a=RefDisposable(d,refn,reff,&t,m,frequency,.05f,1,1,&refout);b=C1AGC14_AdvanceDisposablePrepared(d,n,f,&t,m,frequency,.05f,1,1,&out,&ranges);
  assert(a==b&&called==2&&!memcmp(n,refn,sizeof(n))&&!memcmp(f,reff,sizeof(f))&&!memcmp(&out,&refout,sizeof(out)));checks++;
  reset();a=RefAdvance(d,refn,reff,&t,m,frequency,.05f,1,1,&refout);b=C1AGC14_Advance(d,n,f,&t,m,frequency,.05f,1,1,&out);
  assert(a==b&&called==2&&!memcmp(n,refn,sizeof(n))&&!memcmp(f,reff,sizeof(f))&&!memcmp(&out,&refout,sizeof(out)));checks++;
 }
 /* Legacy public disposable API remains usable with no prepared index. */
 reset();innerOK=1;assert(C1AGC14_AdvanceDisposable(d,n,f,&t,m,frequency,.05f,1,1,&out)&&called==1);checks++;
 /* Exact alias refusal for any writable range, with and without stale proof. */
 for(i=0;i<3;i++){
  const void *addr=i==0?(const void*)n:i==1?(const void*)f:(const void*)&out;
  m[9]=(const c1anm14Metadata_t*)addr;assert(C1AGC46_Prepare(m,COUNT,&ranges));reset();
  assert(!RefDisposable(d,n,f,&t,m,frequency,.05f,1,1,&out));
  assert(!C1AGC14_AdvanceDisposablePrepared(d,n,f,&t,m,frequency,.05f,1,1,&out,&ranges)&&!called);checks++;
  m[10]=&resource;assert(!C1AGC14_AdvanceDisposablePrepared(d,n,f,&t,m,frequency,.05f,1,1,&out,&ranges)&&!called);checks++;
  m[9]=m[10]=NULL;
 }
 reset();assert(C1AGC14_AdvanceDisposablePrepared(d,n,f,&t,m,frequency,.05f,1,1,&out,&ranges)&&called==1);checks++;
 assert(!C1AGC14_AdvanceDisposablePrepared(d,n,(c1afl14State_t*)n,&t,m,frequency,.05f,1,1,&out,&ranges));checks++;
 printf("R46A storage: %u preflight/publication cases PASS\n",checks);return 0;
}
