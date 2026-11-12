/* Actual Actor allocation owns all three strings. Native699c0's write prefix
 * survives argument/repeat errors. This API grants no controller/AI readiness.
 * New-main clearing is deliberately NOT exposed until actual570d0 is owned. */
static int ActorScriptStateHostInvoke(void*,const GscEntityRef*,const GscActorScriptStateCall*,char*,size_t);
static int ActorScriptStateHostRead(void*,const GscEntityRef*,const char*,GscStringCastValue*,char*,size_t);
static const GscActorScriptStateHost cod1ScriptStateHost={NULL,ActorScriptStateHostInvoke,ActorScriptStateHostRead};
static cod1ActorRecord_t*cod1ScriptStateSnapshot;
static int cod1ScriptStateBusy,cod1ScriptStateCancelled;
static int ActorScriptStatePrivate(const void*p,size_t n){
 return ActorOverlap(p,n,&cod1ScriptStateHost,sizeof(cod1ScriptStateHost))||
  ActorOverlap(p,n,&cod1ScriptStateSnapshot,sizeof(cod1ScriptStateSnapshot))||
  ActorOverlap(p,n,&cod1ScriptStateBusy,sizeof(cod1ScriptStateBusy))||
  ActorOverlap(p,n,&cod1ScriptStateCancelled,sizeof(cod1ScriptStateCancelled))||
  ActorOverlap(p,n,cod1ScriptStateSnapshot,cod1ScriptStateSnapshot?sizeof(*cod1ScriptStateSnapshot):0);
}
static int ActorScriptStateHostInvoke(void*u,const GscEntityRef*input,
 const GscActorScriptStateCall*request,char*error,size_t size){
 cod1ActorLease_t lease;cod1ActorRecord_t*a,*snapshot;GscEntityRef ref;
 GscActorScriptStateCall call;char text[128];int i,read,ok=0;(void)u;
 if(cod1ScriptStateBusy){cod1ScriptStateCancelled=1;return 0;}
 if(!input||!request||(size&&!error)||
    !ActorGoalBuffers(input,sizeof(*input),NULL,0,error,size)||
    !ActorGoalBuffers(request,sizeof(*request),NULL,0,error,size)||
    ActorOverlap(input,sizeof(*input),request,sizeof(*request)))return 0;
 ref=*input;call=*request;
 if(!ActorCriticalCapture(&ref,&lease)||cod1Actors[lease.actorIndex].scriptStateBusy){
  EntityError(error,size,"trackScriptState requires a current idle Actor");return 0;}
 if(!call.current||!call.read){EntityError(error,size,"tracking typed getter contract absent");return 0;}
 /* Snapshot lives in a protected private heap for this synchronous operation. */
 snapshot=malloc(sizeof(*snapshot));if(!snapshot)return 0;
 cod1ScriptStateSnapshot=snapshot;cod1ScriptStateBusy=1;cod1ScriptStateCancelled=0;
 a=&cod1Actors[lease.actorIndex];a->scriptStateBusy=1;
 *snapshot=*a;
 if(call.current(call.context)!=1||cod1ScriptStateCancelled||!ActorLeaseValid(&lease)||
    memcmp(input,&ref,sizeof(ref))||memcmp(request,&call,sizeof(call))||memcmp(a,snapshot,sizeof(*a)))goto done;
 if(C1AST14_Begin(&a->scriptState,call.argc)!=1){
  EntityError(error,size,"trackScriptState newStateName, reasonForTransition");goto done;}
 for(i=0;i<2;i++){
  *snapshot=*a;memset(text,0,sizeof(text));
  read=call.read(call.context,(unsigned)i,text,sizeof(text),error,size);
  /* Failed native cast may already set VM error; do not ask current() to
   * turn that into a successful rollback. The committed prefix stays owned. */
  if(cod1ScriptStateCancelled||!ActorLeaseValid(&lease))goto done;
  a=&cod1Actors[lease.actorIndex];
  if(memcmp(a,snapshot,sizeof(*a))||memcmp(input,&ref,sizeof(ref))||memcmp(request,&call,sizeof(call)))goto done;
  if(read!=1)goto done;
  if(call.current(call.context)!=1||cod1ScriptStateCancelled||!ActorLeaseValid(&lease)||memcmp(a,snapshot,sizeof(*a))||
     memcmp(input,&ref,sizeof(ref))||memcmp(request,&call,sizeof(call)))goto done;
  if(!C1AST14_Assign(i?&a->scriptState.reason:&a->scriptState.current,text,sizeof(text))){
   EntityError(error,size,"tracking text outside selected bounded domain");goto done;}
 }
 if(C1AST14_Changed(&a->scriptState)!=1){
  char diagnostic[512];snprintf(diagnostic,sizeof(diagnostic),
   "trackScriptState should only be called on script state transitions.  Called for state %s from state %s.",
   a->scriptState.current.present?a->scriptState.current.text:"",
   a->scriptState.previous.present?a->scriptState.previous.text:"");
  EntityError(error,size,diagnostic);goto done;}
 ok=1;
done:
 if(ActorLeaseValid(&lease))cod1Actors[lease.actorIndex].scriptStateBusy=0;
 memset(snapshot,0,sizeof(*snapshot));cod1ScriptStateSnapshot=NULL;free(snapshot);cod1ScriptStateBusy=0;cod1ScriptStateCancelled=0;return ok;
}
const GscActorScriptStateHost*G_Cod1ActorScriptStateHost(void){
 return &cod1ScriptStateHost;
}
static int ActorScriptStateHostRead(void*u,const GscEntityRef*input,const char*name,
 GscStringCastValue*out,char*error,size_t size){
 cod1ActorLease_t lease;const c1ast14State_t*state;const c1ast14Text_t*text;
 GscStringCastValue value;(void)u;
 if(!input||!name||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||
    ActorGoalPrivate(name,128)||ActorOverlap(name,128,out,sizeof(*out))||ActorOverlap(name,128,error,size)||
    !memchr(name,0,128)||!ActorCriticalCapture(input,&lease))return 0;
 state=&cod1Actors[lease.actorIndex].scriptState;if(!C1AST14_State(state))return 0;
 text=!Q_stricmp(name,"scriptstate")?&state->current:!Q_stricmp(name,"lastscriptstate")?&state->previous:
      !Q_stricmp(name,"statechangereason")?&state->reason:NULL;
 if(!text)return 0;memset(&value,0,sizeof(value));
 if(text->present){value.type=1;memcpy(value.string,text->text,sizeof(value.string));}
 *out=value;return 1;
}
