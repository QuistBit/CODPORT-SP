#ifndef G_COD1_ACTOR_TRAVERSE_CORE_H
#define G_COD1_ACTOR_TRAVERSE_CORE_H
#include "g_cod1_actor_critical_core.h"
/* Original GAME4E060, requested state11. Admission row89FD0 excludes9
 * and0. Pruning row8A038 contains7,8,9,10,11. Queue reduction retains
 * its canonical native02A4 input; this operation does not enter the state. */
static int C1TraversePush(cod1ActorCriticalState_t*s){
 cod1ActorCriticalState_t next;int i,first=-1;
 if(!Cod1ActorCriticalStackValid(s))return -1;
 for(i=1;i<=s->top;i++)if(s->stack[i]==9||s->stack[i]==0)return 0;
 next=*s;
 for(i=1;i<=next.top;i++)if(next.stack[i]>=7&&next.stack[i]<=11){first=i;break;}
 if(first>=0)while(next.top>=first){if(!Cod1ActorCriticalAppend(&next,3,0))return -1;--next.top;}
 if(next.top>=COD1_CRITICAL_STACK_CAPACITY-1||!Cod1ActorCriticalAppend(&next,2,11))return -1;
 ++next.top;next.stack[next.top]=11;*s=next;return 1;
}
#endif
