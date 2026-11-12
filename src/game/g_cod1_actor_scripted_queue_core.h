#ifndef G_COD1_ACTOR_SCRIPTED_QUEUE_CORE_H
#define G_COD1_ACTOR_SCRIPTED_QUEUE_CORE_H
/* R243 original2004E060/DF20 prospective state9 operations, using the exact
 * retained R189 reducer. Actual stack consumption, state callbacks, VM init,
 * THINK, root playback and phase6 are separate owners. This is not readiness.
 * Authority: gamex86.dll SHA27930aa3f663d0af39a90e6956c980be1e599e0f0c98191a9fb7eaba61481577;
 * queue-native-first-ASAN and state9-prototype-first-ASAN receipts. */
#include "g_cod1_actor_critical_core.h"
static int Cod1ActorScriptedQueuePush(cod1ActorCriticalState_t *s) {
    cod1ActorCriticalState_t staged;int i,first=-1;
    if(!Cod1ActorCriticalStackValid(s))return COD1_CRITICAL_HELD;
    /* State9 admission row20089FA8 is five zero cells; root slot0 excluded. */
    for(i=1;i<=s->top;i++)if(!s->stack[i])return 0;
    staged=*s;
    /* State9 prune row2008A010:7,8,9,10,11. Repeat9 replaces/restarts. */
    for(i=1;i<=staged.top;i++)if(staged.stack[i]>=7&&staged.stack[i]<=11){first=i;break;}
    if(first>=0)while(staged.top>=first){
        if(!Cod1ActorCriticalAppend(&staged,3,0))return COD1_CRITICAL_HELD;
        --staged.top;
    }
    if(staged.top>=COD1_CRITICAL_STACK_CAPACITY-1||
       !Cod1ActorCriticalAppend(&staged,2,9))return COD1_CRITICAL_HELD;
    staged.stack[++staged.top]=9;*s=staged;return 1;
}
static int Cod1ActorScriptedQueuePop(cod1ActorCriticalState_t *s) {
    cod1ActorCriticalState_t staged;
    if(!Cod1ActorCriticalStackValid(s)||s->top<0)return COD1_CRITICAL_HELD;
    staged=*s;
    if(!Cod1ActorCriticalAppend(&staged,3,0))return COD1_CRITICAL_HELD;
    --staged.top;*s=staged;return 1;
}
#endif
