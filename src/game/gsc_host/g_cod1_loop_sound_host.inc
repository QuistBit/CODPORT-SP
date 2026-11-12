/* R243 persistent loop state. This provider is deliberately NOT selected by a
 * gameplay profile until paired client transport/streaming playback exists.
 * Native flags live in the shared sound sidecar, never in RTCW wire fields.
 * Prepare has no registry/entity effects. Finish journals the exact commit so
 * VM/Session post-validation can cancel before any subsequent frame delivery. */
#include "../g_cod1_loop_sound_core.h"
typedef struct {
 int present;GscEntityRef owner;GscTestQuakeScene scene;uint64_t revision;
 Cod1LoopSoundState state;Cod1SSPlan *plan;
} GscLoopSoundRow;
static GscLoopSoundRow gscLoopSounds[MAX_GENTITIES];
static uint64_t gscLoopSoundRevision;
static int GscLoopDeliveryRetire(const GscEntityRef*ref);
static int GscLoopDeliveryRetireAll(void);
typedef struct {
 int present,prepared,committed,cancelled,busy,checking,time;uintptr_t token;
 GscLoopSoundRequest request;const GscLoopSoundRequest *borrowed;
 int (*current)(void*);void *context;GscTestQuakeScene scene;
 GscLoopSoundRow before,after;GscSoundOwner sideBefore,sideAfter;
 uint64_t registryEpoch;char registryBefore[256][128];char registryAfter[256][128];
 Cod1SSPlan *plan;Cod1SSDiagnostic diag;char map[128];
} GscLoopSoundStage;
static GscLoopSoundStage gscLoopSoundStage;
static uintptr_t gscLoopSoundToken;
static int GscLoopSoundRef(const GscEntityRef*a,const GscEntityRef*b){
 return a&&b&&a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int GscLoopSoundKey(const GscLoopSoundRequest*a,const GscLoopSoundRequest*b){
 return a&&b&&a->runtime&&a->operation&&a->runtime==b->runtime&&a->operation==b->operation&&
  !memcmp(a,b,sizeof(*a));
}
static int GscLoopSoundScope(void){
 GscLoopSoundStage*s=&gscLoopSoundStage;unsigned slot=s->request.owner.slot;
 return s->present&&!s->cancelled&&s->busy&&s->borrowed&&s->current&&slot<MAX_GENTITIES&&
  !memcmp(s->borrowed,&s->request,sizeof(s->request))&&level.time==s->time&&
  GscTestQuakeCurrent(&s->scene)&&G_Cod1EntitySameAllocation(&s->request.owner)&&
  gscSoundEpoch==(s->committed?s->request.owner.epoch:s->registryEpoch)&&
  !memcmp(gscSoundIndices,s->committed?s->registryAfter:s->registryBefore,sizeof(gscSoundIndices))&&
  !memcmp(&gscSoundOwners[slot],s->committed?&s->sideAfter:&s->sideBefore,sizeof(s->sideBefore))&&
  !memcmp(&gscLoopSounds[slot],s->committed?&s->after:&s->before,sizeof(s->before));
}
static int GscLoopSoundCurrent(void*token){
 GscLoopSoundStage*s=&gscLoopSoundStage;int ok;
 if(!s->present||(uintptr_t)token!=s->token)return 0;
 if(s->checking||!GscLoopSoundScope()){s->cancelled=1;return 0;}
 s->checking=1;ok=s->current(s->context)==1&&GscLoopSoundScope();s->checking=0;
 if(!ok)s->cancelled=1;return ok;
}
/* Drop the journal only after the VM has returned. The replaced resource is
 * retained until then; cancellation restores its exact pointer and state. */
static void GscLoopSoundForget(void){
 GscLoopSoundStage*s=&gscLoopSoundStage;
 if(s->committed){if(s->before.plan&&s->before.plan!=s->after.plan)free(s->before.plan);}
 else if(s->plan)free(s->plan);
 memset(s,0,sizeof(*s));
}
static int GscLoopSoundAlias(void*u,char*out,size_t n){
 if(!GscLoopSoundCurrent(u)||n<sizeof(gscLoopSoundStage.request.alias))return 0;
 memcpy(out,gscLoopSoundStage.request.alias,128);return 1;
}
static int GscLoopSoundResolve(void*u,const char*alias){
 GscLoopSoundStage*s=&gscLoopSoundStage;Cod1SSFiles f;int status;
 if(!GscLoopSoundCurrent(u))return -1;
 memset(&f,0,sizeof(f));f.user=u;f.current=GscLoopSoundCurrent;
 f.list=GscSoundList;f.read=GscSoundRead;f.release=GscSoundRelease;f.exists=GscSoundExists;
 status=Cod1LoopSoundResolve(&f,alias,s->map,s->plan,&s->diag);
 /* Fresh client admission currently implements only the original singleton
  * streamed spatial-auto profile. Unsupported plans remain explicit HOLD. */
 if(status==COD1_SS_RESOURCE_READY&&(s->plan->count!=1||!s->plan->variants[0].streamed||strcmp(s->plan->variants[0].channel,"auto"))){s->diag.reason=COD1_SS_REASON_UNSUPPORTED;return -1;}
 /* HELD resource admission is NOT a claim that the original alias is absent. */
 return status==COD1_SS_RESOURCE_READY?1:status==COD1_SS_RESOURCE_NONE?0:-1;
}
static int GscLoopSoundIndexGet(void*u,unsigned i,char*out,size_t n){
 if(!GscLoopSoundCurrent(u)||i<1||i>255||n<128)return 0;
 memcpy(out,gscLoopSoundStage.registryAfter[i],128);return 1;
}
static int GscLoopSoundIndexSet(void*u,unsigned i,const char*alias){
 if(!GscLoopSoundCurrent(u)||i<1||i>255||!Cod1ScriptSoundText(alias))return 0;
 strcpy(gscLoopSoundStage.registryAfter[i],alias);return 1;
}
static int GscLoopSoundPrepare(void*u,const GscLoopSoundRequest*q,char*error,size_t cap,
 int(*current)(void*),void*context){
 GscLoopSoundStage*s=&gscLoopSoundStage;Cod1ScriptSoundOps ops;int i,ok=0;unsigned slot;
 (void)u;
 if(s->busy){s->cancelled=1;return 0;}
 if(!q||!q->runtime||!q->operation||!current||q->owner.slot>=MAX_GENTITIES||
  !G_Cod1EntitySameAllocation(&q->owner)||!GscTestQuakeCurrent(&gscTestQuake)||
  (q->stop!=0&&q->stop!=1)||(!q->stop&&!Cod1SS_Token(q->alias,128))||gscLoopSoundToken==UINTPTR_MAX)return 0;
 /* An unconsumed prepare must be explicitly cancelled, never overwritten. */
 if(s->present&&!s->committed)return 0;
 GscLoopSoundForget();s->present=s->busy=1;s->request=*q;s->borrowed=q;s->current=current;s->context=context;
 s->token=++gscLoopSoundToken;s->scene=gscTestQuake;s->time=level.time;s->registryEpoch=gscSoundEpoch;
 slot=q->owner.slot;s->before=gscLoopSounds[slot];s->sideBefore=gscSoundOwners[slot];
 memcpy(s->registryBefore,gscSoundIndices,sizeof(gscSoundIndices));memcpy(s->registryAfter,gscSoundIndices,sizeof(gscSoundIndices));
 s->sideAfter=s->sideBefore;s->after=s->before;
 if(s->before.present&&(!GscLoopSoundRef(&s->before.owner,&q->owner)||
   !GscTestQuakeIdentity(&s->before.scene)))goto done;
 if(!GscLoopSoundRef(&s->sideAfter.owner,&q->owner)){
  if(s->sideAfter.pending.pending||s->sideAfter.voiceActive)goto done;
  memset(&s->sideAfter,0,sizeof(s->sideAfter));s->sideAfter.owner=q->owner;
 }
 if(gscSoundEpoch!=q->owner.epoch){
  for(i=0;i<MAX_GENTITIES;i++)if(gscLoopSounds[i].present)goto done;
  memset(s->registryAfter,0,sizeof(s->registryAfter));
 }
 s->after.owner=q->owner;s->after.scene=s->scene;s->after.present=1;
 s->after.state.flags=s->sideAfter.state.flags;
 if(!GscLoopSoundCurrent((void*)s->token))goto done;
 if(q->stop){
  if(Cod1LoopSoundStop(&s->after.state,1)!=COD1_SS_OK)goto done;
  s->after.plan=NULL;
 }else{
  s->plan=calloc(1,sizeof(*s->plan));if(!s->plan)goto done;
  trap_Cvar_VariableStringBuffer("mapname",s->map,sizeof(s->map));
  if(!memchr(s->map,0,sizeof(s->map))||!GscLoopSoundCurrent((void*)s->token))goto done;
  memset(&ops,0,sizeof(ops));ops.user=(void*)s->token;ops.read_alias=GscLoopSoundAlias;ops.alias_exists=GscLoopSoundResolve;
  ops.get_index=GscLoopSoundIndexGet;ops.set_index=GscLoopSoundIndexSet;
  if(Cod1LoopSoundPlay(&s->after.state,1,&ops)!=COD1_SS_OK)goto done;
  s->after.plan=s->plan;
 }
 s->sideAfter.state.flags=s->after.state.flags;
 if(!GscLoopSoundCurrent((void*)s->token))goto done;
 s->prepared=1;ok=1;
done:s->busy=0;s->borrowed=NULL;s->current=NULL;s->context=NULL;
 if(!ok)GscTestError(error,cap,"loop sound prepare refused: allocation/Session/registry/resource admission changed");
 return ok;
}
static int GscLoopSoundFinish(void*u,const GscLoopSoundRequest*q,int commit,char*error,size_t cap,
 int(*current)(void*),void*context){
 GscLoopSoundStage*s=&gscLoopSoundStage;unsigned slot;int ok=0;(void)u;
 if(s->busy){s->cancelled=1;return 0;}
 if(!q||!q->runtime||!q->operation||(commit!=0&&commit!=1))return 0;
 if(!s->present)return !commit;
 if(!GscLoopSoundKey(q,&s->request))return 0;
 slot=q->owner.slot;
 if(!commit){
  if(s->committed){
   /* Never undo an unrelated allocation, registry mutation or newer owner. */
   if(!G_Cod1EntitySameAllocation(&q->owner)||gscSoundEpoch!=q->owner.epoch||
    memcmp(&gscLoopSounds[slot],&s->after,sizeof(s->after))||
    memcmp(&gscSoundOwners[slot],&s->sideAfter,sizeof(s->sideAfter))||
    memcmp(gscSoundIndices,s->registryAfter,sizeof(gscSoundIndices)))return 0;
   gscLoopSounds[slot]=s->before;gscSoundOwners[slot]=s->sideBefore;
   memcpy(gscSoundIndices,s->registryBefore,sizeof(gscSoundIndices));gscSoundEpoch=s->registryEpoch;
   s->committed=0;
  }
  GscLoopSoundForget();return 1;
 }
 if(!s->prepared||s->committed||!current||gscLoopSoundRevision==UINT64_MAX)return 0;
 s->busy=1;s->borrowed=q;s->current=current;s->context=context;
 if(GscLoopSoundCurrent((void*)s->token)){
  s->after.revision=++gscLoopSoundRevision;
  /* No engine/user callbacks between final validation and the owned stores. */
  gscLoopSounds[slot]=s->after;gscSoundOwners[slot]=s->sideAfter;
  memcpy(gscSoundIndices,s->registryAfter,sizeof(gscSoundIndices));gscSoundEpoch=q->owner.epoch;
  s->committed=1;ok=1;
 }
 s->busy=0;s->borrowed=NULL;s->current=NULL;s->context=NULL;
 if(!ok)GscTestError(error,cap,"loop sound commit refused; exact cancellation required");return ok;
}
/* Retirement queues copied transport identity before releasing resource data.
 * No client callback or receipt is required to free the exact GAME owner. */
int G_Cod1LoopSoundBeforeFree(const GscEntityRef*ref){
 GscLoopSoundStage*s=&gscLoopSoundStage;GscLoopSoundRow*r;
 if(!ref||ref->slot>=MAX_GENTITIES)return 0;
 if(s->busy){s->cancelled=1;return 0;}
 if(s->present&&!s->committed&&GscLoopSoundRef(ref,&s->request.owner))return 0;
 r=&gscLoopSounds[ref->slot];if(!r->present)return 1;
 if(!GscLoopSoundRef(ref,&r->owner)||!G_Cod1EntitySameAllocation(ref))return 0;
 if(!GscLoopDeliveryRetire(ref))return 0;
 GscLoopSoundForget();if(r->plan)free(r->plan);memset(r,0,sizeof(*r));return 1;
}
int G_Cod1LoopSoundMapCleanup(void){
 int i;GscLoopSoundStage*s=&gscLoopSoundStage;
 if(s->busy){s->cancelled=1;return 0;}
 if(!GscLoopDeliveryRetireAll())return 0;
 /* Unpublished prepared state has no external stores; committed state belongs
  * to this map until here. Do not write old flags into a replacement entity. */
 GscLoopSoundForget();
 for(i=0;i<MAX_GENTITIES;i++){if(gscLoopSounds[i].plan)free(gscLoopSounds[i].plan);memset(gscLoopSounds+i,0,sizeof(gscLoopSounds[i]));}
 return 1;
}
static const GscLoopSoundHost*GscLoopSoundHostAPI(void){
 static const GscLoopSoundHost h={NULL,GscLoopSoundPrepare,GscLoopSoundFinish};return &h;
}
