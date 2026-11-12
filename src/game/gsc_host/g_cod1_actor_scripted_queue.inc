#include "../g_cod1_actor_scripted_queue.h"
#include "../g_cod1_actor_scripted_queue_core.h"
/* No callbacks or asynchronous borrowed state: exact lease is checked before
 * reading the canonical life cell and committing the shared queue. The generic
 * critical guard also excludes a label callback from mutating this queue. */
static int ActorScriptedQueuePrivate(const void *p,size_t n) {
    return (n&&(!p||n>UINTPTR_MAX-(uintptr_t)p))||
        ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors))||
        ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients))||
        ActorOverlap(p,n,g_entities,sizeof(g_entities))||
        ActorOverlap(p,n,g_clients,sizeof(g_clients))||
        ActorOverlap(p,n,&level,sizeof(level))||G_Cod1EntityStorageProtected(p,n);
}
int G_Cod1ActorScriptedQueuePush(const cod1ActorLease_t *input,int *admitted,
    char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorRecord_t *a;cod1ActorCriticalState_t kernel;int status;
    if(ActorScriptedQueuePrivate(input,sizeof(*input))||
       ActorScriptedQueuePrivate(admitted,sizeof(*admitted))||
       ActorScriptedQueuePrivate(error,size)||
       !ActorArguments(input,sizeof(*input),admitted,sizeof(*admitted),error,size)||
       ActorOverlap(input,sizeof(*input),admitted,sizeof(*admitted)))return 0;
    if(!input||!admitted){EntityError(error,size,"scripted queue requires lease and admission output");return 0;}
    lease=*input;
    if(cod1ActorCriticalBusy||!ActorCriticalReady(&lease)){
        EntityError(error,size,"scripted queue requires current idle Actor");return 0;}
    a=&cod1Actors[lease.actorIndex];kernel=ActorStateQueueKernel(a);
    status=Cod1ActorScriptedQueuePush(&kernel);
    if(status==COD1_CRITICAL_HELD){EntityError(error,size,"scripted queue outside selected safe domain (HELD)");return 0;}
    ActorStateQueueCommit(a,&kernel);*admitted=status;return 1;
}
int G_Cod1ActorScriptedQueuePop(const cod1ActorLease_t *input,char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorRecord_t *a;cod1ActorCriticalState_t kernel;
    if(ActorScriptedQueuePrivate(input,sizeof(*input))||ActorScriptedQueuePrivate(error,size)||
       !ActorArguments(input,sizeof(*input),NULL,0,error,size))return 0;
    if(!input){EntityError(error,size,"scripted pop requires Actor lease");return 0;}
    lease=*input;
    if(cod1ActorCriticalBusy||!ActorCriticalReady(&lease)){
        EntityError(error,size,"scripted pop requires current idle Actor");return 0;}
    a=&cod1Actors[lease.actorIndex];kernel=ActorStateQueueKernel(a);
    if(kernel.top<1||kernel.top>=COD1_CRITICAL_STACK_CAPACITY||kernel.stack[kernel.top]!=9||
       Cod1ActorScriptedQueuePop(&kernel)!=1){
        EntityError(error,size,"scripted pop requires a selected state9 above the root (HELD)");return 0;}
    ActorStateQueueCommit(a,&kernel);return 1;
}
