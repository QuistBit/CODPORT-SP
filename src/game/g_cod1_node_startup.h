#ifndef G_COD1_NODE_STARTUP_H
#define G_COD1_NODE_STARTUP_H
#include <math.h>
#include <fenv.h>
#include <float.h>
#include "g_cod1_dynamic_path_core.h"
/* Original6FA70 and6FB20 final DATA startup. Pool index0 is the free-ring
 * sentinel. Source/target payload is deliberately untouched by ring reset.
 * Neighbors are physical indices, not authored identities or dynamic links.
 * Native diagnostics never grant permission to publish a port candidate. */
enum {C1NS_POOL_COUNT=2048};
typedef struct {unsigned duplicate,excess;} C1NSResult;
typedef struct {unsigned completed,index;} C1NSInitialResult;
/* Original6FAD0 later entity pass, separate from6FDE0 graph startup. Inputs
 * must be the native-slot-indexed active/owner DATA snapshot; this helper
 * does not certify that every live allocation is represented or current.
 * Both1000 and2000 are required. A failure may leave partial candidate
 * mutations: discard it; never publish it or return a fake native success. */
static int C1NS_InitialDisconnect(C1DPState*s,const unsigned char*active,unsigned count,
 int32_t time,C1DPTrace trace,void*ctx,C1NSInitialResult*out){
 unsigned i;int status;C1NSInitialResult result={0,0};
 if(!out||!active||!C1DP_Layout(s)||count!=s->owner_count)return C1DP_BAD_DATA;
 for(i=0;i<count;i++)if(s->owners[i].entity!=i)return C1DP_BAD_DATA;
 for(i=0;i<count;i++){
  if(!active[i]||(s->owners[i].flags&0x3000u)!=0x3000u)continue;
  result.index=i;status=C1DP_CandidateTransition(s,i,1,time,trace,ctx);
  if(status!=C1DP_OK){*out=result;return status;}result.completed++;
 }
 result.index=count;*out=result;return C1DP_OK;
}
static int C1NS_PoolInit(C1DPRecord*r,unsigned count){
 unsigned i;if(!r||count!=C1NS_POOL_COUNT)return 0;
 for(i=0;i<count;i++){r[i].previous=(uint16_t)(i?i-1:count-1);r[i].next=(uint16_t)(i+1<count?i+1:0);}return 1;
}
static void C1NS_Put16(unsigned char*p,unsigned v){uint16_t x=(uint16_t)v;memcpy(p,&x,2);}
static unsigned C1NS_Get16(const unsigned char*p){uint16_t x;memcpy(&x,p,2);return x;}
static void C1NS_AddNeighbor(unsigned char*r,unsigned index,C1NSResult*out){
 if(C1NS_Get16(r+0x4c)==0xffff)C1NS_Put16(r+0x4c,index);
 else if(C1NS_Get16(r+0x4e)==0xffff)C1NS_Put16(r+0x4e,index);
 else out->excess++;
}
/* Valid DATA transformation can report duplicate/excess diagnostics. Caller
 * must inspect both counters; full graph constructors reject either. All
 * precondition refusals preserve records and output, including alias cases.
 * Native stores each xyz difference to float before comparisons and computes
 * xy squared distance at PC53; bounded float inputs permit exact double math.
 * Nonfinite/out-of-profile coordinates and non-nearest rounding are refused. */
static int C1NS_NodeNeighbors(unsigned char*records,size_t bytes,unsigned count,C1NSResult*out){
 unsigned i,j,k;size_t need=(size_t)count*116;uintptr_t a=(uintptr_t)records,b=(uintptr_t)out;C1NSResult result={0,0};
 if(!records||!out||count>2048||bytes<need||fegetround()!=FE_TONEAREST||
    (need&&(a<=b?b-a<need:a-b<sizeof(*out))))return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 /* Match the original oracle profile without changing the process FP state. */
 if((_controlfp(0,0)&(_MCW_PC|_MCW_RC|_EM_AMBIGUOUS))!=(_PC_53|_RC_NEAR))return 0;
#endif
 for(i=0;i<count;i++){float p[3];memcpy(p,records+(size_t)i*116+0x38,12);
  for(k=0;k<3;k++)if(!isfinite(p[k])||fabsf(p[k])>1e7f)return 0;
 }
 for(i=0;i<count;i++){C1NS_Put16(records+(size_t)i*116+0x4c,0xffff);C1NS_Put16(records+(size_t)i*116+0x4e,0xffff);}
 for(i=0;i<count;i++)for(j=0;j<i;j++){
  unsigned char*x=records+(size_t)i*116,*y=records+(size_t)j*116;float p[3],q[3],dx,dy,dz;volatile double xx,yy,sum;
  memcpy(p,x+0x38,12);memcpy(q,y+0x38,12);
  dx=(float)((double)p[0]-q[0]);dy=(float)((double)p[1]-q[1]);dz=(float)((double)p[2]-q[2]);
  if(dx< -31||dx>31||dy< -31||dy>31||dz< -73||dz>73)continue;
  xx=(double)dx*dx;yy=(double)dy*dy;
  sum=xx+yy;if(sum<1.0&&!((x[0x2a]|y[0x2a])&1))result.duplicate++;
  C1NS_AddNeighbor(x,j,&result);C1NS_AddNeighbor(y,i,&result);
 }
 *out=result;return 1;
}
#endif
