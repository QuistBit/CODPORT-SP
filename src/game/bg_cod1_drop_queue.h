#ifndef BG_COD1_DROP_QUEUE_H
#define BG_COD1_DROP_QUEUE_H
/* Original gamex86 2001B880: first unused/stale slot, otherwise the first
 * strictly farthest live item from entity0; defer its free until time+1.
 * A copied plan only: caller still owes real allocation/retirement fencing. */
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {uint32_t entity;unsigned char inuse;float origin[3];} c1dqEntry;
typedef struct {unsigned slot;uint32_t evictEntity;int clearStale,nextThink;} c1dqPlan;
static int C1DQ_Plan(const c1dqEntry rows[32],const float player[3],int now,c1dqPlan*out){
 c1dqPlan p;float farthest=0;unsigned i,j;memset(&p,0,sizeof(p));
 if(!rows||!player||!out||now<0||now==INT32_MAX)return 0;
 for(j=0;j<3;j++)if(!isfinite(player[j]))return 0;
 for(i=0;i<32;i++){
  float delta[3],sum,d;
  if(!rows[i].entity){p.slot=i;*out=p;return 1;}
  if(rows[i].entity>=2048)return 0;
  if(!rows[i].inuse){p.slot=i;p.clearStale=1;*out=p;return 1;}
  for(j=0;j<3;j++){if(!isfinite(rows[i].origin[j]))return 0;delta[j]=rows[i].origin[j]-player[j];if(!isfinite(delta[j]))return 0;}
  /* Original stores differences to float, evaluates z²+y²+x², rounds sqrt
   * to float, and updates only on a strict greater-than comparison. */
  sum=(float)((double)delta[2]*delta[2]+(double)delta[1]*delta[1]+(double)delta[0]*delta[0]);
  d=(float)sqrt((double)sum);if(!isfinite(d))return 0;
  if(d>farthest){farthest=d;p.slot=i;}
 }
 p.evictEntity=rows[p.slot].entity;p.nextThink=now+1;*out=p;return 1;
}
#endif
