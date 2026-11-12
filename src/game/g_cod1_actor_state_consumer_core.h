#ifndef G_COD1_ACTOR_STATE_CONSUMER_CORE_H
#define G_COD1_ACTOR_STATE_CONSUMER_CORE_H
#include "g_cod1_actor_critical_core.h"
/* Portable control flow of native2004DC40/2004DB80. This component does NOT
 * supply any state callback, actor initialization, THINK or script completion.
 * Callbacks must execute the real selected state service; refusal keeps the
 * committed native prefix, never rolls back or announces completed state. */
enum { COD1_ACTOR_STATE_ENTER=1,COD1_ACTOR_STATE_EXIT=2,
       COD1_ACTOR_STATE_SUSPEND=3,COD1_ACTOR_STATE_RESUME=4 };
typedef struct {int32_t stack[COD1_CRITICAL_STACK_CAPACITY],top,base_entered_ms;} cod1ActorActualState_t;
typedef struct {void *context;int (*invoke)(void *,int,int,int,int *);} cod1ActorStateCallbacks_t;
static int Cod1ActorActualStateValid(const cod1ActorActualState_t *actual) {
    return actual&&actual->top>=0&&actual->top<COD1_CRITICAL_STACK_CAPACITY&&
        actual->stack[actual->top]>0&&actual->stack[actual->top]<12;
}
static int Cod1ActorStateCallback(cod1ActorActualState_t *actual,
    const cod1ActorStateCallbacks_t *host,int phase,int related,int *result) {
    int value=0;
    if(!Cod1ActorActualStateValid(actual)||!host||!host->invoke||
       !host->invoke(host->context,phase,actual->stack[actual->top],related,&value))return 0;
    if(!Cod1ActorActualStateValid(actual))return 0;
    *result=value;return 1;
}
static int Cod1ActorStateEnter(cod1ActorActualState_t *actual,int state,int32_t now,
    const cod1ActorStateCallbacks_t *host) {
    int n,result,previous;
    if(!actual||actual->top<0||actual->top>=COD1_CRITICAL_STACK_CAPACITY)return 0;
    if(!actual->top)actual->base_entered_ms=now;
    for(n=1;n<=12;n++){
        if(state<=0||state>=12)return 0;
        previous=actual->stack[actual->top];actual->stack[actual->top]=state;
        if(!Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_ENTER,previous,&result))return 0;
        if(!result)return 1;
        state=result;
    }
    /* Original DB80 stops after12 actual entry calls, even if the last
     * callback returned another state. No invented retry delay or callback. */
    return 1;
}
static int Cod1ActorStateConsume(cod1ActorActualState_t *actual,
    cod1ActorCriticalState_t *queue,int32_t now,const cod1ActorStateCallbacks_t *host) {
    int i=0,operation,next,result;
    if(!actual||!queue||!host||!host->invoke)return 0;
    while(i<queue->command_count){
        if(queue->command_count<0||queue->command_count>COD1_CRITICAL_COMMAND_CAPACITY||
           !Cod1ActorActualStateValid(actual))return 0;
        operation=queue->commands[i].operation;next=queue->commands[i].state;
        if(operation<0||operation>3)return 0;
        if(operation==1){
            if(!Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_EXIT,next,&result)||
               !Cod1ActorStateEnter(actual,next,now,host))return 0;
        }else if(operation==2){
            if(actual->top>=COD1_CRITICAL_STACK_CAPACITY-1||
               !Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_SUSPEND,next,&result))return 0;
            if(actual->top>=COD1_CRITICAL_STACK_CAPACITY-1)return 0;
            ++actual->top;
            if(!Cod1ActorStateEnter(actual,next,now,host))return 0;
        }else if(operation==3){
            if(actual->top<1||!Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_EXIT,
                actual->stack[actual->top-1],&result)||actual->top<1)return 0;
            --actual->top;
            if(!Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_RESUME,
                actual->stack[actual->top+1],&result))return 0;
            if(result){
                next=result;
                if(!Cod1ActorStateCallback(actual,host,COD1_ACTOR_STATE_EXIT,next,&result)||
                   !Cod1ActorStateEnter(actual,next,now,host))return 0;
            }
        }
        ++i;
    }
    if(queue->command_count<0||queue->command_count>COD1_CRITICAL_COMMAND_CAPACITY||
       !Cod1ActorActualStateValid(actual))return 0;
    queue->command_count=0;queue->top=actual->top;
    for(i=0;i<=actual->top;i++)queue->stack[i]=actual->stack[i];
    return 1;
}
#endif
