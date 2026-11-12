/* Owns only an unpublished stage, never a finalized native graph permit.
 * Current prepared refs come from the actual Session producer. No pointer
 * into a Session or registry arena survives acquisition. */
#include "g_cod1_dynamic_path_graph.h"
#include "g_cod1_path_entity_host.h"
static C1NPPrepared*cod1GraphPrepared;
static C1DPGraphCandidate*cod1GraphStaged;
static uint64_t cod1GraphSerial,cod1GraphReceipt,cod1GraphWorld,cod1GraphPending;
static int cod1GraphBusy,cod1GraphCancelled,cod1GraphPublished;
static cod1NavigationResource_t cod1GraphResource;
static struct {int active,built,checking;GscSession*session;const void*grant;uint64_t id,receipt,world;C1NPPrepared*prepared;C1DPGraphCandidate*graph;cod1NavigationResource_t resource;} cod1GraphLatePin;
int G_Cod1NodeGraphPostCohortPrivate(const void*p,size_t n){return Cod1NodeOverlap(p,n,&cod1GraphLatePin,sizeof(cod1GraphLatePin));}
static struct {int active;GscSession*session;uint64_t id,receipt;GscEntityRef row;
 C1NPPrepared*prepared;C1DPGraphCandidate*graph;cod1NavigationResource_t resource;
} cod1GraphActorPin;
static void Cod1GraphRelease(void){
 G_Cod1PathNodeWorldEnd(cod1GraphWorld);cod1GraphWorld=0;cod1GraphReceipt=0;cod1GraphPublished=0;
 memset(&cod1GraphResource,0,sizeof(cod1GraphResource));
 C1DP_GraphDestroy(cod1GraphStaged);cod1GraphStaged=NULL;
 G_Cod1NodePreparedFree(cod1GraphPrepared);cod1GraphPrepared=NULL;
}
void G_Cod1NodeGraphStageClear(void){
 if(cod1GraphBusy){cod1GraphCancelled=1;return;}
 Cod1GraphRelease();
}
void G_Cod1NodeGraphStageDiscard(uint64_t receipt){
 if(receipt&&receipt==cod1GraphReceipt)G_Cod1NodeGraphStageClear();
}
static void Cod1GraphSessionRetire(uint64_t receipt){
 if(receipt&&receipt==cod1GraphPending){cod1GraphCancelled=1;return;}
 if(receipt&&receipt==cod1GraphReceipt)G_Cod1NodeGraphStageClear();
}
typedef struct {GscSession*session;uint64_t id,world,receipt;C1NPPrepared*prepared;cod1NavigationResource_t resource;} C1GraphAcquire;
static int Cod1GraphResourceCurrent(const cod1NavigationResource_t*r){
 cod1NavigationRequest_t q;cod1NavigationResource_t value;
 if(!r->incarnation)return 1; /* Explicit DATA stage has no resource claim. */
 if(!Cod1NodeAvailable()||memcmp(r,&cod1NodeResource,sizeof(*r)))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_NAV_RESOURCE_VERSION;q.bytes=sizeof(q);
 q.incarnation=r->incarnation;q.length=r->length;q.operation=COD1_NAV_CURRENT;
 return trap_Cod1NavigationResource(&q,&value,NULL,0)&&!memcmp(&value,r,sizeof(value));
}
static int Cod1GraphAcquireCurrent(const C1GraphAcquire*a){
 if(cod1GraphLatePin.active){
  return a&&cod1GraphBusy&&!cod1GraphCancelled&&a->session==cod1GraphLatePin.session&&a->id==cod1GraphLatePin.id&&a->receipt==cod1GraphLatePin.receipt&&
   a->prepared==cod1GraphLatePin.prepared&&a->world==cod1GraphLatePin.world&&!memcmp(&a->resource,&cod1GraphLatePin.resource,sizeof(a->resource))&&
   gsc_session_postcohort_graph_current(a->session,a->id,a->receipt)&&Cod1GraphResourceCurrent(&a->resource)&&!cod1GraphCancelled&&
   Cod1NodePreparedPostCohortCurrent(a->prepared,a->session,a->id,a->receipt)&&Cod1GraphResourceCurrent(&a->resource)&&!cod1GraphCancelled&&G_Cod1LateDisconnectCurrent(a->world);
 }
 return !cod1GraphCancelled&&gsc_session_node_graph_current(a->session,a->id,a->receipt)&&Cod1GraphResourceCurrent(&a->resource)&&
  !cod1GraphCancelled&&G_Cod1PathNodeWorldCurrent(a->world)&&
  G_Cod1NodePreparedCurrent(a->prepared,a->session,a->id)&&
  Cod1GraphResourceCurrent(&a->resource)&&!cod1GraphCancelled&&G_Cod1PathNodeWorldCurrent(a->world);
}
static int Cod1NodePlaceTrace(const float*start,const float*mins,const float*maxs,const float*end,unsigned pass,unsigned mask,C1DPPlaceTrace*out){
 cod1SpatialTraceQuery_t q;trace_t tr;C1DPPlaceTrace value;
 if(pass!=0x7ff||mask!=0x00820011u)return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.contentsMask=mask;
 q.passEntity[0]=q.passEntity[1]=ENTITYNUM_NONE;
 memcpy(q.start,start,12);memcpy(q.end,end,12);memcpy(q.mins,mins,12);memcpy(q.maxs,maxs,12);
 if(!G_Cod1PathNodePlaceTrace(&q,&tr))return 0;
 memset(&value,0,sizeof(value));value.fraction=tr.fraction;memcpy(value.end,tr.endpos,12);
 value.allsolid=(uint8_t)tr.allsolid;value.startsolid=(uint8_t)tr.startsolid;*out=value;return 1;
}
static int Cod1GraphPlace(void*ctx,const float*start,const float*mins,const float*maxs,const float*end,unsigned pass,unsigned mask,C1DPPlaceTrace*out){
 C1GraphAcquire*a=(C1GraphAcquire*)ctx;C1DPPlaceTrace value;
 if(!Cod1GraphAcquireCurrent(a)||!Cod1NodePlaceTrace(start,mins,maxs,end,pass,mask,&value)||!Cod1GraphAcquireCurrent(a))return 0;
 *out=value;return 1;
}
typedef struct {C1GraphAcquire*acquire;const cod1PathNodeOwners_t*owners;} C1GraphInitial;
static int Cod1GraphInitialTrace(void*ctx,unsigned from,unsigned to,const float*start,const float*end,
 const float*mins,const float*maxs,unsigned entity,unsigned mask){
 C1GraphInitial*c=(C1GraphInitial*)ctx;GscEntityRef ref;int result;
 if(cod1GraphLatePin.active)return -1; /* Fixed late cohort has no candidate transitions. */
 if(entity>=c->owners->count||!c->owners->rows[entity].active||!Cod1GraphAcquireCurrent(c->acquire))return -1;
 ref=c->owners->rows[entity].ref;
 result=G_Cod1PathCandidateTrace(&ref,from,to,start,end,mins,maxs,entity,mask);
 return Cod1GraphAcquireCurrent(c->acquire)?result:-1;
}
/* Native6FAD0 is a later phase: after graph pool/neighbors, using actual
 * completed GAME owners. No caller-provided active/flags table is admitted.
 * A proved no3000 late scope selects no disconnect targets; execute the real body,
 * preserving its guards and callback, rather than issuing a ready shortcut.
 * General placement/trace world, main scheduling and dynamic3000 remain separate. */
static int Cod1GraphInitialOwners(C1GraphAcquire*a,C1DPGraphCandidate*g){
 cod1PathNodeOwners_t*owners=NULL;unsigned char*active=NULL;C1DPState state;C1NSInitialResult result;
 C1GraphInitial context;unsigned i;int ok=0;
 if(g->owners||g->ownerCount)return 0;
 owners=malloc(sizeof(*owners));if(!owners)goto done;
 if(!Cod1GraphAcquireCurrent(a)||!(cod1GraphLatePin.active?G_Cod1LateDisconnectOwners(a->world,owners):G_Cod1PathNodeOwners(a->world,owners))||!owners->count)goto done;
 g->owners=calloc(owners->count,sizeof(*g->owners));active=calloc(owners->count,1);
 if(!g->owners||!active)goto done;g->ownerCount=owners->count;
 for(i=0;i<owners->count;i++){
  g->owners[i].entity=(uint16_t)i;g->owners[i].flags=owners->rows[i].flags;active[i]=(unsigned char)owners->rows[i].active;
 }
 state.nodes=g->nodes;state.links=g->links;state.records=g->blockRecords;state.owners=g->owners;
 state.node_count=g->count;state.link_count=g->linkCount;state.record_count=C1NS_POOL_COUNT;state.owner_count=g->ownerCount;
 context.acquire=a;context.owners=owners;
 if(!Cod1GraphAcquireCurrent(a)||C1NS_InitialDisconnect(&state,active,owners->count,owners->time,Cod1GraphInitialTrace,&context,&result)!=C1DP_OK||!Cod1GraphAcquireCurrent(a))goto done;
 ok=1;
done:free(active);free(owners);return ok;
}
static int Cod1GraphStageBuild(GscSession*s,uint64_t id,const void*lump,size_t bytes,const cod1NavigationResource_t*resource,uint64_t*out){
 C1GraphAcquire a;C1DPGraphCandidate*g=NULL;C1DPGraphFailure failure;unsigned char*copy=NULL;int ok=0;
 memset(&a,0,sizeof(a));a.session=s;a.id=id;a.receipt=cod1GraphPending;if(resource)a.resource=*resource;
 a.prepared=G_Cod1NodePreparedBuild(s,id);if(!a.prepared||cod1GraphCancelled)goto done;
 if(!G_Cod1PathNodeWorldBegin(&a.world)||!Cod1GraphAcquireCurrent(&a))goto done;
 copy=(unsigned char*)malloc(bytes);if(!copy)goto done;memcpy(copy,lump,bytes);
 g=C1DP_GraphLoadV7(a.prepared->records,(size_t)a.prepared->count*116,a.prepared->count,copy,bytes,Cod1GraphPlace,&a,&failure);
 if(!g||!Cod1GraphAcquireCurrent(&a)||!gsc_session_node_graph_end(s,id,cod1GraphPending,1))goto done;
 /* No callbacks between final validation and private ownership transfer. */
 cod1GraphPrepared=a.prepared;a.prepared=NULL;cod1GraphStaged=g;g=NULL;
 cod1GraphWorld=a.world;a.world=0;cod1GraphResource=a.resource;cod1GraphReceipt=cod1GraphPending;*out=cod1GraphReceipt;ok=1;
done:free(copy);C1DP_GraphDestroy(g);G_Cod1NodePreparedFree(a.prepared);G_Cod1PathNodeWorldEnd(a.world);return ok;
}
int G_Cod1NodeGraphStage(GscSession*s,uint64_t id,const void*lump,size_t bytes,uint64_t*out){
 int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!s||!id||!lump||!bytes||bytes>COD1_NAV_RESOURCE_LIMIT||!out||cod1GraphReceipt||cod1GraphSerial==UINT64_MAX)return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;cod1GraphPending=++cod1GraphSerial;
 ok=gsc_session_node_graph_begin(s,id,cod1GraphPending,Cod1GraphSessionRetire)&&!cod1GraphCancelled&&Cod1GraphStageBuild(s,id,lump,bytes,NULL,out);
 if(!ok)gsc_session_node_graph_end(s,id,cod1GraphPending,0);
 cod1GraphPending=0;cod1GraphBusy=0;return ok;
}
int G_Cod1NodeGraphStageMap(GscSession*s,uint64_t id,uint64_t*out){
 cod1NavigationRequest_t q;cod1NavigationResource_t resource,copied;
 unsigned char*buffer=NULL;uint64_t epoch;int ok=0;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!s||!id||!out||cod1GraphReceipt||cod1GraphSerial==UINT64_MAX)return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;cod1GraphPending=++cod1GraphSerial;epoch=G_Cod1EntityMapEpoch();
 if(!gsc_session_node_graph_begin(s,id,cod1GraphPending,Cod1GraphSessionRetire)||cod1GraphCancelled)goto done;
 memset(&q,0,sizeof(q));q.version=COD1_NAV_RESOURCE_VERSION;q.bytes=sizeof(q);q.operation=COD1_NAV_DESCRIBE;
 if(!epoch||!trap_Cod1NavigationResource(&q,&resource,NULL,0)||cod1GraphCancelled)goto done;
 if(memcmp(&resource,&cod1NodeResource,sizeof(resource)))goto done;
 if(resource.version!=COD1_NAV_RESOURCE_VERSION||resource.bytes!=sizeof(resource)||!resource.incarnation||resource.reserved||!resource.length||resource.length>COD1_NAV_RESOURCE_LIMIT)goto done;
 buffer=(unsigned char*)malloc(resource.length);if(!buffer)goto done;
 q.operation=COD1_NAV_COPY;q.incarnation=resource.incarnation;q.length=resource.length;
 if(!trap_Cod1NavigationResource(&q,&copied,buffer,resource.length)||memcmp(&resource,&copied,sizeof(resource))||
    cod1GraphCancelled||epoch!=G_Cod1EntityMapEpoch()||!Cod1GraphResourceCurrent(&resource)||cod1GraphCancelled)goto done;
 ok=Cod1GraphStageBuild(s,id,buffer,resource.length,&resource,out);
done:free(buffer);if(!ok)gsc_session_node_graph_end(s,id,cod1GraphPending,0);cod1GraphPending=0;cod1GraphBusy=0;return ok;
}
/* Native736A0+81..C5 scans 71CD0/71D10 with mask FFFFFFFF. Every
 * admitted type (including zero) participates; first physical match wins.
 * Local prepared tokens preserve equality, not the engine SL integer ABI.
 * Snapshot the input before any callback can retire its source storage. */
cod1NodeStageTarget_t G_Cod1NodeGraphStageTarget(GscSession*s,uint64_t id,uint64_t receipt,const char*target){
 cod1NodeStageTarget_t result={0},value={0};C1GraphAcquire a;
 char name[128];size_t length;unsigned i,token=0;uint16_t word;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return result;}
 if(!target||!Cod1NodeText(target,sizeof(name),&length)||!length)return result;
 memcpy(name,target,length+1);
 if(cod1GraphPublished||!s||!id||!receipt||receipt!=cod1GraphReceipt||!cod1GraphPrepared||!cod1GraphStaged||id!=cod1GraphPrepared->sessionId)return result;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return result;
 a.session=s;a.id=id;a.prepared=cod1GraphPrepared;a.world=cod1GraphWorld;a.resource=cod1GraphResource;a.receipt=receipt;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphAcquireCurrent(&a);
 if(ok){
  value.status=1;
  for(i=0;i<a.prepared->tokenCount;i++)if(!strcmp(name,a.prepared->tokens[i])){token=i+1;break;}
  if(token)for(i=0;i<cod1GraphStaged->count;i++){
   const unsigned char*r=cod1GraphStaged->records+116*i;
   memcpy(&word,r+0x2c,2);if(word!=token)continue;
   value.status=2;value.physicalIndex=i;memcpy(&word,r+0x28,2);value.authoredIndex=word;
   memcpy(&value.type,r+0x24,4);memcpy(value.origin,r+0x38,12);break;
  }
  ok=Cod1GraphAcquireCurrent(&a);
 }
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(ok)result=value;else Cod1GraphRelease();return result;
}
int G_Cod1NodeGraphStageCurrent(GscSession*s,uint64_t id,uint64_t receipt){
 C1GraphAcquire a;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(cod1GraphPublished||!s||!id||!receipt||receipt!=cod1GraphReceipt||!cod1GraphPrepared||!cod1GraphStaged||id!=cod1GraphPrepared->sessionId)return 0;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 a.session=s;a.id=id;a.prepared=cod1GraphPrepared;a.world=cod1GraphWorld;a.resource=cod1GraphResource;a.receipt=receipt;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphAcquireCurrent(&a);
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}

/* Explicit later phase, never folded into StageMap/6FDE0 preparation. The
 * original caller must establish its post-entity-init scheduling separately.
 * Failure consumes the entire private stage; no partial owner pool survives. */
int G_Cod1NodeGraphStageInitialOwners(GscSession*s,uint64_t id,uint64_t receipt){
 C1GraphAcquire a;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(cod1GraphPublished||!s||!id||!receipt||receipt!=cod1GraphReceipt||!cod1GraphPrepared||!cod1GraphStaged||
    id!=cod1GraphPrepared->sessionId||cod1GraphStaged->owners||cod1GraphStaged->ownerCount)return 0;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 a.session=s;a.id=id;a.prepared=cod1GraphPrepared;a.world=cod1GraphWorld;a.resource=cod1GraphResource;a.receipt=receipt;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphInitialOwners(&a,cod1GraphStaged);
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}

/* Publication transfers completed construction DATA out of its synchronous
 * world fence. It does not grant VM traversal/dynamic-path permission.
 * Map resource/registry/program ownership survives ordinary Actor/player
 * mutations; later native owner initialization needs a new world snapshot. */
static int Cod1GraphPublishedOwnerCurrent(GscSession*s,uint64_t id,uint64_t receipt){
 return !cod1GraphCancelled&&cod1GraphPublished&&receipt==cod1GraphReceipt&&
  cod1GraphPrepared&&cod1GraphStaged&&!cod1GraphWorld&&
  gsc_session_node_graph_current(s,id,receipt)&&Cod1GraphResourceCurrent(&cod1GraphResource)&&
  !cod1GraphCancelled&&G_Cod1NodePreparedCurrent(cod1GraphPrepared,s,id)&&
  !cod1GraphCancelled&&Cod1GraphResourceCurrent(&cod1GraphResource)&&!cod1GraphCancelled;
}
int G_Cod1NodeGraphPublish(GscSession*s,uint64_t id,uint64_t receipt){
 C1GraphAcquire a;uint64_t world;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(cod1GraphPublished||!s||!id||!receipt||receipt!=cod1GraphReceipt||!cod1GraphResource.incarnation||
    !cod1GraphPrepared||!cod1GraphStaged||!cod1GraphWorld)return 0;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 a.session=s;a.id=id;a.receipt=receipt;a.prepared=cod1GraphPrepared;a.world=cod1GraphWorld;a.resource=cod1GraphResource;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphAcquireCurrent(&a);
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;
 if(ok){
  /* End is pure here. Consume, never refresh or recapture the old world. */
  world=cod1GraphWorld;cod1GraphWorld=0;G_Cod1PathNodeWorldEnd(world);cod1GraphPublished=1;
 }
 cod1GraphBusy=0;if(!ok)Cod1GraphRelease();return ok;
}
int G_Cod1NodeGraphPublishedCurrent(GscSession*s,uint64_t id,uint64_t receipt){int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt)return 0;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}
cod1NodeStageTarget_t G_Cod1NodeGraphPublishedTarget(GscSession*s,uint64_t id,uint64_t receipt,const char*target){
 cod1NodeStageTarget_t result={0},value={0};char name[128];size_t length;unsigned i,token=0;uint16_t word;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return result;}
 if(!target||!Cod1NodeText(target,sizeof(name),&length)||!length)return result;memcpy(name,target,length+1);
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt)return result;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return result;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 if(ok){value.status=1;
  for(i=0;i<cod1GraphPrepared->tokenCount;i++)if(!strcmp(name,cod1GraphPrepared->tokens[i])){token=i+1;break;}
  if(token)for(i=0;i<cod1GraphStaged->count;i++){const unsigned char*r=cod1GraphStaged->records+116*i;
   memcpy(&word,r+0x2c,2);if(word!=token)continue;value.status=2;value.physicalIndex=i;
   memcpy(&word,r+0x28,2);value.authoredIndex=word;memcpy(&value.type,r+0x24,4);memcpy(value.origin,r+0x38,12);break;}
  ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 }
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(ok)result=value;else Cod1GraphRelease();return result;
}
/* Read-only DATA query under the separately pinned exact returned Actor row.
 * Never changes Session busy or gives native traversal/claim permission. */
static int Cod1GraphActorOwnerCurrent(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row){
 return !cod1GraphCancelled&&cod1GraphPublished&&receipt==cod1GraphReceipt&&cod1GraphPrepared&&cod1GraphStaged&&!cod1GraphWorld&&
  gsc_session_actor_graph_current(s,id,receipt,row)&&Cod1GraphResourceCurrent(&cod1GraphResource)&&!cod1GraphCancelled&&
  Cod1NodePreparedActorCurrent(cod1GraphPrepared,s,id,receipt,row)&&!cod1GraphCancelled&&
  Cod1GraphResourceCurrent(&cod1GraphResource)&&!cod1GraphCancelled&&gsc_session_actor_graph_current(s,id,receipt,row)&&
  !cod1GraphCancelled&&cod1GraphPublished&&receipt==cod1GraphReceipt&&cod1GraphPrepared&&cod1GraphStaged&&!cod1GraphWorld;
}
static int Cod1GraphTargetPrivate(const void*p,size_t n){
 C1NPPrepared*a=cod1GraphPrepared;C1DPGraphCandidate*g=cod1GraphStaged;
 return G_Cod1NodeGraphPostCohortPrivate(p,n)||Cod1NodeOverlap(p,n,&cod1GraphActorPin,sizeof(cod1GraphActorPin))||Cod1NodePrivate(p,n)||Cod1NodeOverlap(p,n,&cod1GraphPrepared,sizeof(cod1GraphPrepared))||
  Cod1NodeOverlap(p,n,&cod1GraphStaged,sizeof(cod1GraphStaged))||Cod1NodeOverlap(p,n,&cod1GraphBusy,sizeof(cod1GraphBusy))||
  Cod1NodeOverlap(p,n,&cod1GraphCancelled,sizeof(cod1GraphCancelled))||Cod1NodeOverlap(p,n,&cod1GraphReceipt,sizeof(cod1GraphReceipt))||
  Cod1NodeOverlap(p,n,&cod1GraphResource,sizeof(cod1GraphResource))||
  (a&&(Cod1NodeOverlap(p,n,a,sizeof(*a))||Cod1NodeOverlap(p,n,a->records,(size_t)a->count*116)||
   Cod1NodeOverlap(p,n,a->scripts,(size_t)a->count*sizeof(*a->scripts))||Cod1NodeOverlap(p,n,a->tokens,(size_t)C1NP_TOKEN_CAPACITY*sizeof(*a->tokens))))||
  (g&&(Cod1NodeOverlap(p,n,g,sizeof(*g))||Cod1NodeOverlap(p,n,g->records,(size_t)g->count*116)||
   Cod1NodeOverlap(p,n,g->nodes,(size_t)g->count*sizeof(*g->nodes))||Cod1NodeOverlap(p,n,g->links,(size_t)g->linkCount*sizeof(*g->links))||
   Cod1NodeOverlap(p,n,g->physical,(size_t)g->count*sizeof(*g->physical))||Cod1NodeOverlap(p,n,g->authoredToPhysical,(size_t)g->count*2)||
   Cod1NodeOverlap(p,n,g->blockRecords,(size_t)C1NS_POOL_COUNT*sizeof(*g->blockRecords))||Cod1NodeOverlap(p,n,g->owners,(size_t)g->ownerCount*sizeof(*g->owners))));
}
int G_Cod1NodeGraphActorTarget(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,const char*target,GscActorBootstrapNodeTarget*out){
 GscActorBootstrapNodeTarget value={0};char name[128];size_t length;unsigned i,token=0;uint16_t word;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!out||!target||!row||Cod1GraphTargetPrivate(out,sizeof(*out))||Cod1GraphTargetPrivate(row,sizeof(*row))||
    Cod1NodeOverlap(out,sizeof(*out),row,sizeof(*row))||Cod1NodeOverlap(out,sizeof(*out),target,sizeof(name))||
    !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
    !Cod1NodeText(target,sizeof(name),&length)||!length)return 0;memcpy(name,target,length+1);
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt||!gsc_session_actor_graph_current(s,id,receipt,row))return 0;
 cod1GraphBusy=1;memset(&cod1GraphActorPin,0,sizeof(cod1GraphActorPin));
 cod1GraphActorPin.active=1;cod1GraphActorPin.session=s;cod1GraphActorPin.id=id;cod1GraphActorPin.receipt=receipt;cod1GraphActorPin.row=*row;
 cod1GraphActorPin.prepared=cod1GraphPrepared;cod1GraphActorPin.graph=cod1GraphStaged;cod1GraphActorPin.resource=cod1GraphResource;
 ok=Cod1GraphActorOwnerCurrent(s,id,receipt,row);
 if(ok){value.status=1;
  for(i=0;i<cod1GraphPrepared->tokenCount;i++)if(!strcmp(name,cod1GraphPrepared->tokens[i])){token=i+1;break;}
  if(token)for(i=0;i<cod1GraphStaged->count;i++){const unsigned char*r=cod1GraphStaged->records+116*i;
   memcpy(&word,r+0x2c,2);if(word!=token)continue;value.status=2;value.physicalIndex=i;
   memcpy(&word,r+0x28,2);value.authoredIndex=word;memcpy(&value.type,r+0x24,4);memcpy(value.origin,r+0x38,12);break;}
  ok=Cod1GraphActorOwnerCurrent(s,id,receipt,row);
 }
 if(ok){*out=value;return 1;} /* Matching end retains GAME operation. */
 memset(&cod1GraphActorPin,0,sizeof(cod1GraphActorPin));cod1GraphBusy=0;Cod1GraphRelease();return 0;
}
int G_Cod1NodeGraphActorTargetEnd(GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,int keep){
 int ok;
 if(!cod1GraphBusy||!cod1GraphActorPin.active)return 0;
 if(!row||s!=cod1GraphActorPin.session||id!=cod1GraphActorPin.id||receipt!=cod1GraphActorPin.receipt||
    memcmp(row,&cod1GraphActorPin.row,sizeof(*row))){cod1GraphCancelled=1;return 0;}
 /* Pure private identity/resource/registry fence. Resource retirement is
  * delivered by its existing immediate observer; no trap/provider runs here. */
 ok=keep==1&&!cod1GraphCancelled&&cod1GraphPublished&&receipt==cod1GraphReceipt&&!cod1GraphWorld&&
  cod1GraphPrepared==cod1GraphActorPin.prepared&&cod1GraphStaged==cod1GraphActorPin.graph&&
  !memcmp(&cod1GraphResource,&cod1GraphActorPin.resource,sizeof(cod1GraphResource))&&
  cod1GraphResource.incarnation&&cod1GraphResource.incarnation>cod1NodeRetiredResource&&
  !memcmp(&cod1GraphResource,&cod1NodeResource,sizeof(cod1GraphResource))&&Cod1NodeAvailable()&&
  cod1GraphPrepared&&cod1GraphPrepared->registryEpoch==cod1NodeEpoch&&cod1GraphPrepared->sessionId==id;
 memset(&cod1GraphActorPin,0,sizeof(cod1GraphActorPin));cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}
/* Explicit later pass: capture this later world, then consume it on every
 * path. Publication never carries its original acquisition world forward. */
int G_Cod1NodeGraphPublishedInitialOwners(GscSession*s,uint64_t id,uint64_t receipt){
 C1GraphAcquire a;int ok=0;uint64_t world=0;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt||!cod1GraphStaged||
    cod1GraphStaged->owners||cod1GraphStaged->ownerCount)return 0;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;
 if(!Cod1GraphPublishedOwnerCurrent(s,id,receipt)||!G_Cod1PathNodeWorldBegin(&world))goto done;
 a.session=s;a.id=id;a.receipt=receipt;a.prepared=cod1GraphPrepared;a.world=world;a.resource=cod1GraphResource;
 ok=Cod1GraphInitialOwners(&a,cod1GraphStaged)&&Cod1GraphPublishedOwnerCurrent(s,id,receipt);
done:
 G_Cod1PathNodeWorldEnd(world);if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}

cod1NodeGraphPublication_t G_Cod1NodeGraphPublishedInfo(GscSession*s,uint64_t id,uint64_t receipt){
 cod1NodeGraphPublication_t out={0},value={0};unsigned i;int ok;
 if(cod1GraphBusy){cod1GraphCancelled=1;return out;}
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt||!gsc_session_node_graph_check_begin(s,id,receipt))return out;
 cod1GraphBusy=1;cod1GraphCancelled=0;ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 if(ok){value.receipt=receipt;value.nodes=cod1GraphStaged->count;value.links=cod1GraphStaged->linkCount;value.owners=cod1GraphStaged->ownerCount;
  for(i=0;i<cod1GraphPrepared->count;i++)if(cod1GraphPrepared->scripts[i].cache)value.beginRefs++;
  ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 }
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(ok)out=value;else Cod1GraphRelease();return out;
}

/* The generic public entry above stays nonbusy. This distinct fixed bridge
 * retains the fresh later world and busy graph until all VM/Session foreign
 * checks finish. The final End is pure and consumes only this exact pin. */
static int Cod1GraphLateIdentity(GscSession*s,uint64_t id,uint64_t receipt){
 return cod1GraphLatePin.active&&cod1GraphBusy&&!cod1GraphCancelled&&s==cod1GraphLatePin.session&&id==cod1GraphLatePin.id&&receipt==cod1GraphLatePin.receipt&&
  cod1GraphPublished&&receipt==cod1GraphReceipt&&!cod1GraphWorld&&cod1GraphPrepared==cod1GraphLatePin.prepared&&cod1GraphStaged==cod1GraphLatePin.graph&&
  !memcmp(&cod1GraphResource,&cod1GraphLatePin.resource,sizeof(cod1GraphResource));
}
static int Cod1GraphLateOwnersCurrent(GscSession*s,uint64_t id,uint64_t receipt){
 return Cod1GraphLateIdentity(s,id,receipt)&&gsc_session_postcohort_graph_current(s,id,receipt)&&
  Cod1GraphResourceCurrent(&cod1GraphResource)&&Cod1GraphLateIdentity(s,id,receipt)&&
  Cod1NodePreparedPostCohortCurrent(cod1GraphPrepared,s,id,receipt)&&Cod1GraphResourceCurrent(&cod1GraphResource)&&Cod1GraphLateIdentity(s,id,receipt);
}
int G_Cod1LateDisconnectGraphPrivate(const void*p,size_t n){
 return Cod1GraphTargetPrivate(p,n)||Cod1NodeOverlap(p,n,&cod1GraphSerial,sizeof(cod1GraphSerial))||
  Cod1NodeOverlap(p,n,&cod1GraphWorld,sizeof(cod1GraphWorld))||Cod1NodeOverlap(p,n,&cod1GraphPending,sizeof(cod1GraphPending))||
  Cod1NodeOverlap(p,n,&cod1GraphPublished,sizeof(cod1GraphPublished));
}
int G_Cod1NodeGraphPostCohortInitialOwners(GscSession*s,uint64_t id,uint64_t receipt,const void*grant){C1GraphAcquire a;int ok;uint64_t world=0;
 if(!gsc_session_postcohort_native_scope(s,id,receipt,grant,1))return 0;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 if(!s||!id||!receipt||!cod1GraphPublished||receipt!=cod1GraphReceipt||!cod1GraphPrepared||!cod1GraphStaged||cod1GraphWorld||
  cod1GraphLatePin.active||cod1GraphStaged->owners||cod1GraphStaged->ownerCount)return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;memset(&cod1GraphLatePin,0,sizeof(cod1GraphLatePin));cod1GraphLatePin.active=1;
 cod1GraphLatePin.session=s;cod1GraphLatePin.grant=grant;cod1GraphLatePin.id=id;cod1GraphLatePin.receipt=receipt;cod1GraphLatePin.prepared=cod1GraphPrepared;
 cod1GraphLatePin.graph=cod1GraphStaged;cod1GraphLatePin.resource=cod1GraphResource;
 if(!Cod1GraphLateOwnersCurrent(s,id,receipt)||!G_Cod1LateDisconnectBegin(&world))return 0;
 cod1GraphLatePin.world=world;
 memset(&a,0,sizeof(a));a.session=s;a.id=id;a.receipt=receipt;a.prepared=cod1GraphPrepared;a.resource=cod1GraphResource;a.world=cod1GraphLatePin.world;
 ok=Cod1GraphInitialOwners(&a,cod1GraphStaged)&&Cod1GraphLateOwnersCurrent(s,id,receipt)&&G_Cod1LateDisconnectCurrent(a.world);
 cod1GraphLatePin.built=ok;return ok;
}
int G_Cod1NodeGraphPostCohortCurrent(GscSession*s,uint64_t id,uint64_t receipt,const void*grant){int ok;
 if(grant!=cod1GraphLatePin.grant||!gsc_session_postcohort_native_scope(s,id,receipt,grant,2))return 0;
 if(cod1GraphLatePin.checking){cod1GraphCancelled=1;return 0;}cod1GraphLatePin.checking=1;
 ok=cod1GraphLatePin.built&&Cod1GraphLateOwnersCurrent(s,id,receipt)&&G_Cod1LateDisconnectCurrent(cod1GraphLatePin.world)&&!cod1GraphCancelled;
 cod1GraphLatePin.checking=0;return ok;
}
int G_Cod1NodeGraphPostCohortEnd(GscSession*s,uint64_t id,uint64_t receipt,const void*grant,int keep){int ok;
 if(!cod1GraphLatePin.active||s!=cod1GraphLatePin.session||id!=cod1GraphLatePin.id||receipt!=cod1GraphLatePin.receipt||grant!=cod1GraphLatePin.grant||!gsc_session_postcohort_native_scope(s,id,receipt,grant,3))return 0;
 /* No provider/certificate call: the previous check ended with WorldCurrent. */
 ok=keep==1&&!cod1GraphLatePin.checking&&cod1GraphLatePin.built&&Cod1GraphLateIdentity(s,id,receipt)&&cod1GraphPrepared&&
  cod1GraphPrepared->registryEpoch==cod1NodeEpoch&&cod1GraphPrepared->sessionId==id&&Cod1NodeAvailable()&&
  cod1GraphResource.incarnation>cod1NodeRetiredResource&&!memcmp(&cod1GraphResource,&cod1NodeResource,sizeof(cod1GraphResource))&&
  cod1GraphStaged->owners&&cod1GraphStaged->ownerCount&&G_Cod1LateDisconnectHeld(cod1GraphLatePin.world);
 G_Cod1LateDisconnectEnd(cod1GraphLatePin.world);memset(&cod1GraphLatePin,0,sizeof(cod1GraphLatePin));cod1GraphBusy=0;
 if(!ok)Cod1GraphRelease();return ok;
}
