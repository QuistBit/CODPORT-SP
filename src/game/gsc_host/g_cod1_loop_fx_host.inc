/* R243 loop FX GAME owner. Original type9/broadcast is carried by the paired
 * c1l lane: RTCW type9 has a different meaning and is never copied into s.eType.
 * Actual G_Spawn/G_SetOrigin/link/free own the engine allocation. Private rows
 * bind the VM transaction to the exact Session/resource/map/allocation. Nothing
 * here enables a gameplay profile or substitutes for whole-module admission. */
#include "../cod1_gsc/gsc_loop_fx.h"
typedef struct {
 GscEntityRef entity;GscTestFxScene scene;GscLoopFxRequest request;
 gentity_t body;int present,committed,published,retiring,death;
} GscTestLoopFxRow;
static GscTestLoopFxRow gscTestLoops[MAX_GENTITIES];
static uint64_t gscTestLoopSequence;
static int gscTestLoopBusy,gscTestLoopCancelled,gscTestLoopFreeBusy;
static GscEntityRef gscTestLoopFreeAllowed;
static const char gscTestLoopClass[]="cod1_loop_fx";
static int GscTestLoopRef(const GscEntityRef*a,const GscEntityRef*b){
 return a&&b&&a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static GscTestLoopFxRow *GscTestLoopRow(const GscEntityRef*r){
 return r&&r->slot<MAX_GENTITIES&&gscTestLoops[r->slot].present&&
  GscTestLoopRef(r,&gscTestLoops[r->slot].entity)?gscTestLoops+r->slot:NULL;
}
static int GscTestLoopBody(const GscTestLoopFxRow*r){
 gentity_t expected;int i;
 if(!r||!r->present||!G_Cod1EntitySameAllocation(&r->entity))return 0;
 expected=r->body;
 /* Engine unlink is the only permitted body change after creation. */
 expected.r.linked=g_entities[r->entity.slot].r.linked;
 if(expected.r.linked!=qtrue&&expected.r.linked!=qfalse)return 0;
 if(memcmp(&expected,g_entities+r->entity.slot,sizeof(expected)))return 0;
 for(i=0;i<level.num_entities;i++)if(g_entities[i].inuse&&g_entities[i].tagParent==g_entities+r->entity.slot)return 0;
 return !expected.neverFree&&!expected.client&&!expected.r.bmodel&&
  expected.classname==gscTestLoopClass&&!expected.item&&!expected.aiCharacter;
}
static int GscTestLoopScene(const GscTestFxScene*s){
 return GscTestFxSceneSame(s)&&s->session==gscTestFx.session&&s->id==gscTestFx.id&&
  s->session==gscTestSession&&s->id==gscTestId;
}
static int GscTestLoopResource(const GscTestFxScene*s,int index,int now){
 char actual[MAX_STRING_CHARS],expected[MAX_STRING_CHARS];uint64_t revision=gscTestFx.revision;
 return GscTestLoopScene(s)&&index>0&&index<MAX_COD1_SCRIPT_FX&&
  revision>=s->revision&&s->names[index][0]&&!strcmp(gscTestFx.names[index],s->names[index])&&
  GscTestFxResourceText(expected,sizeof(expected),s->epoch,index,gscTestFx.names[index])&&
  GscTestFxRead(s,CS_COD1_SCRIPT_FX_TABLE+index,actual,sizeof(actual),now,1)&&
  !strcmp(actual,expected)&&GscTestFxMarker(s,now)&&GscTestLoopScene(s)&&gscTestFx.revision==revision&&!strcmp(gscTestFx.names[index],s->names[index]);
}
static int GscTestLoopCurrent(const GscTestFxScene*s,const GscLoopFxServices*api,
 const GscLoopFxServices*expected,int now,int time){
 return !gscTestLoopCancelled&&api&&api->current&&!memcmp(api,expected,sizeof(*api))&&
  GscTestLoopScene(s)&&level.time==time&&G_Cod1RngOwnerReady()&&GscTestFxTimeCurrent(s,now)&&
  api->current(api->context)==1&&!gscTestLoopCancelled&&
  !memcmp(api,expected,sizeof(*api))&&GscTestLoopScene(s)&&level.time==time&&G_Cod1RngOwnerReady()&&GscTestFxTimeCurrent(s,now);
}
static int GscTestLoopOverlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an&&bn&&(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<=y?y-x<an:x-y<bn));
}
static int GscTestLoopPrivate(const void*p,size_t n){
 return GscTestLoopOverlap(p,n,gscTestLoops,sizeof(gscTestLoops))||
  GscTestLoopOverlap(p,n,&gscTestFx,sizeof(gscTestFx))||
  GscTestLoopOverlap(p,n,g_entities,sizeof(g_entities))||
  GscTestLoopOverlap(p,n,&level,sizeof(level));
}
/* Reliable publication is port policy, not retail snapshot/renderer parity.
 * The original broadcast0x20 skips PVS; the client performs distance culling.
 * Retirement does not require a running VM or an unchanged resource revision. */
/* Original GAME20036390 owns a linked broadcast entity before a client
 * exists. A committed row is map state; published means a client command was
 * accepted. An absent/connecting real SP client is not a successful send. */
static int GscTestLoopPlayerPending(void){
 return level.clients&&!level.restarted&&
  (level.clients[0].pers.connected==CON_DISCONNECTED||level.clients[0].pers.connected==CON_CONNECTING)&&
  (!g_entities[0].client||g_entities[0].client==level.clients)&&
  !g_entities[0].aiCharacter&&!level.clients[0].ps.aiChar&&!(g_entities[0].r.svFlags&SVF_CASTAI);
}
static int GscTestLoopNow(const GscTestFxScene*s,int*out){
 GscSessionSnapshot v;
 if(!GscTestLoopScene(s)||!gsc_session_snapshot(s->session,&v)||
  v.session_id!=s->id||v.state!=GSC_SESSION_RUNNING||v.error[0]||v.logical_ms<0||
  !GscTestFxTimeCurrent(s,v.logical_ms))return 0;
 *out=v.logical_ms;return 1;
}
static int GscTestLoopSend(GscTestLoopFxRow*r,int live){
 cod1LoopFxSnapshot_t wire;GscEntityRef player;GscTestLoopFxRow before;
 char text[COD1_LOOP_FX_CAPACITY],marker[COD1_SCRIPT_FX_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
 int accepted,logical=0;
 if(!GscTestLoopBody(r)||!GscTestLoopScene(&r->scene)||gscTestLoopSequence==UINT64_MAX||
  !G_Cod1EntityCapture(g_entities,&player)||!GscTestQuakePlayerCurrent(&player)||
  !CF_EncodeMetadata(marker,sizeof(marker),r->scene.epoch))return 0;
 before=*r;memset(actual,0,sizeof(actual));
 trap_GetConfigstring(CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual));
 if(memcmp(&before,r,sizeof(before))||!memchr(actual,0,sizeof(actual))||strcmp(actual,marker)||
  !GscTestLoopScene(&r->scene)||!GscTestLoopBody(r)||!GscTestQuakePlayerCurrent(&player))return 0;
 if(live==COD1_LOOP_FX_VISIBLE&&(!GscTestLoopNow(&r->scene,&logical)||!GscTestLoopResource(&r->scene,r->request.value.index,logical)||
  memcmp(&before,r,sizeof(before))||!GscTestLoopBody(r)||!GscTestQuakePlayerCurrent(&player)))return 0;
 memset(&wire,0,sizeof(wire));wire.epoch=r->scene.epoch;wire.sequence=++gscTestLoopSequence;
 wire.entity_epoch=r->entity.epoch;wire.slot=r->entity.slot;wire.generation=r->entity.generation;wire.live=live;
 if(live==COD1_LOOP_FX_VISIBLE)wire.value=r->request.value;
 if(!CLF_Encode(text,sizeof(text),&wire))return 0;
 /* A callback may cancel after acceptance. Keep the published bit so rollback
  * still sends a tombstone. Definite queue refusal has no visible effect. */
 if(live==COD1_LOOP_FX_VISIBLE)r->published=1;
 accepted=trap_Cod1TrySendServerCommand(0,text);
 if(!GscTestLoopRef(&r->entity,&before.entity)||!r->present)return 0;
 if(!accepted){if(live==COD1_LOOP_FX_VISIBLE)r->published=before.published;return 0;}
 if(live==COD1_LOOP_FX_RETIRED)r->retiring=1;
 if(live==COD1_LOOP_FX_VISIBLE&&!GscTestLoopResource(&r->scene,r->request.value.index,logical))return 0;
 return GscTestLoopScene(&r->scene)&&GscTestLoopBody(r)&&GscTestQuakePlayerCurrent(&player);
}
int G_Cod1LoopFxBeforeFree(const GscEntityRef*ref){
 GscTestLoopFxRow*r=GscTestLoopRow(ref);int ok;
 if(!r)return 1;
 if(gscTestLoopFreeBusy||(gscTestLoopBusy&&!GscTestLoopRef(ref,&gscTestLoopFreeAllowed))){gscTestLoopCancelled=1;return 0;}
 if(!GscTestLoopBody(r))return 0;
 gscTestLoopFreeBusy=1;
 ok=!r->published||r->retiring||GscTestLoopSend(r,COD1_LOOP_FX_RETIRED);
 gscTestLoopFreeBusy=0;
 return ok&&GscTestLoopBody(r);
}
void G_Cod1LoopFxAfterFree(const GscEntityRef*ref){
 GscTestLoopFxRow*r=GscTestLoopRow(ref);
 if(r&&ref->epoch==G_Cod1EntityMapEpoch()&&!G_Cod1EntitySameAllocation(ref)&&
  !g_entities[ref->slot].inuse)memset(r,0,sizeof(*r));
}
static int GscTestLoopRemove(GscTestLoopFxRow*r){
 GscEntityRef ref;
 if(!r||!r->present)return 1;
 if(!GscTestLoopBody(r))return 0;
 ref=r->entity;gscTestLoopFreeAllowed=ref;G_FreeEntity(g_entities+ref.slot);
 memset(&gscTestLoopFreeAllowed,0,sizeof(gscTestLoopFreeAllowed));
 return !GscTestLoopRow(&ref)&&ref.epoch==G_Cod1EntityMapEpoch()&&!G_Cod1EntitySameAllocation(&ref);
}
static int GscTestLoopFxReset(void){
 int i,ok=1;
 if(gscTestLoopBusy||gscTestLoopFreeBusy){gscTestLoopCancelled=1;return 0;}
 gscTestLoopBusy=1;gscTestLoopCancelled=0;
 for(i=0;i<MAX_GENTITIES;i++)if(gscTestLoops[i].present&&!GscTestLoopRemove(gscTestLoops+i))ok=0;
 gscTestLoopBusy=0;return ok&&!gscTestLoopCancelled;
}
int G_Cod1LoopFxMapCleanup(void){return GscTestLoopFxReset();}
static int GscTestLoopFxPrepare(void*u,const GscLoopFxRequest*input,GscLoopFxTicket*out,
 const GscLoopFxServices*api,char*error,size_t cap){
 GscLoopFxRequest q;GscLoopFxServices expected;GscTestFxScene scene=gscTestFx;
 GscEntityRef ref;GscTestLoopFxRow*r=NULL;gentity_t*e;gentity_t before;int i,time=level.time,ok=0;
 (void)u;
 if(gscTestLoopBusy||gscTestLoopFreeBusy){gscTestLoopCancelled=1;return 0;}
 if(!input||!out||!api||!api->current||GscTestLoopPrivate(out,sizeof(*out))||GscTestLoopPrivate(error,cap)||
  GscTestLoopOverlap(input,sizeof(*input),out,sizeof(*out))||GscTestLoopOverlap(api,sizeof(*api),out,sizeof(*out))||
  GscTestLoopOverlap(input,sizeof(*input),error,cap)||GscTestLoopOverlap(api,sizeof(*api),error,cap)||
  GscTestLoopOverlap(out,sizeof(*out),error,cap))return 0;
 q=*input;expected=*api;
 if(!q.runtime||!q.operation||q.now_ms<0||!CLF_ValueValid(&q.value)||
  G_Cod1RngOwnerPolicy()!=COD1_RNG_POLICY_FRESH_SP)goto refuse;
 for(i=0;i<MAX_GENTITIES;i++)if(gscTestLoops[i].present&&gscTestLoops[i].request.runtime==q.runtime&&
  gscTestLoops[i].request.operation==q.operation)goto refuse;
 gscTestLoopBusy=1;gscTestLoopCancelled=0;
 if(!GscTestLoopCurrent(&scene,api,&expected,q.now_ms,time)||!GscTestLoopResource(&scene,q.value.index,q.now_ms)||
  memcmp(input,&q,sizeof(q))||!GscTestLoopCurrent(&scene,api,&expected,q.now_ms,time))goto done;
 /* G_Spawn's selected fresh-SP policy revalidates its exact birth across
  * Locate callbacks. Never accept an allocator pointer under legacy policy. */
 e=G_Spawn();
 if(!e||!G_Cod1EntityCapture(e,&ref)||ref.epoch!=scene.map_epoch||
  ref.slot<MAX_CLIENTS||ref.slot>=ENTITYNUM_MAX_NORMAL||gscTestLoops[ref.slot].present)goto done;
 r=gscTestLoops+ref.slot;memset(r,0,sizeof(*r));r->present=1;r->entity=ref;r->scene=scene;r->request=q;
 e->classname=(char*)gscTestLoopClass;e->s.eType=ET_GENERAL;e->s.eFlags=EF_NODRAW;e->flags|=FL_NODRAW;e->r.svFlags=SVF_NOCLIENT;
 /* Preserve the strict body across ordinary RTCW frame bookkeeping. No RTCW
  * script is assigned; G_RunThink copies this status before its no-script path. */
 e->scriptStatusCurrent=e->scriptStatus;
 G_SetOrigin(e,q.value.origin);r->body=*e;
 if(!GscTestLoopCurrent(&scene,api,&expected,q.now_ms,time)||memcmp(input,&q,sizeof(q)))goto done;
 before=*e;trap_LinkEntity(e);
 if(!G_Cod1EntitySameAllocation(&ref))goto done;
 /* Only the documented SV_LinkEntity output fields may change. */
 before.r.linked=e->r.linked;before.r.linkcount=e->r.linkcount;
 memcpy(before.r.absmin,e->r.absmin,sizeof(before.r.absmin));memcpy(before.r.absmax,e->r.absmax,sizeof(before.r.absmax));
 if(memcmp(&before,e,sizeof(before)))goto done;
 r->body=*e;
 if(!e->r.linked||!GscTestLoopBody(r)||!GscTestLoopResource(&scene,q.value.index,q.now_ms)||
  !GscTestLoopCurrent(&scene,api,&expected,q.now_ms,time)||memcmp(input,&q,sizeof(q)))goto done;
 out->operation=q.operation;out->entity=ref;ok=1;
done:
 /* The VM always invokes finish(cancel) on a refused prepare. Preserve the
  * exact row on failure so that cancellation never relies on a borrowed out. */
 gscTestLoopBusy=0;if(ok&&!gscTestLoopCancelled)return 1;
refuse:GscTestError(error,cap,"loop FX GAME prepare refused: resource, Session, fresh allocation or link changed");return 0;
}
static int GscTestLoopFxFinish(void*u,uint64_t runtime,uint64_t operation,int publish,
 const GscLoopFxServices*api,char*error,size_t cap){
 GscTestLoopFxRow*r=NULL;GscLoopFxServices expected;int i,ok=0,time=level.time;
 (void)u;
 if(gscTestLoopBusy||gscTestLoopFreeBusy){gscTestLoopCancelled=1;return 0;}
 if(!runtime||!operation||(publish!=0&&publish!=1)||!api||!api->current||GscTestLoopPrivate(error,cap))return 0;
 expected=*api;
 for(i=0;i<MAX_GENTITIES;i++)if(gscTestLoops[i].present&&gscTestLoops[i].request.runtime==runtime&&
  gscTestLoops[i].request.operation==operation){r=gscTestLoops+i;break;}
 if(!r)return !publish;
 gscTestLoopBusy=1;gscTestLoopCancelled=0;
 if(!publish)ok=GscTestLoopRemove(r);
 else if(!r->committed&&!r->published&&!r->retiring&&GscTestLoopBody(r)&&g_entities[r->entity.slot].r.linked&&
  GscTestLoopCurrent(&r->scene,api,&expected,r->request.now_ms,time)&&
  GscTestLoopResource(&r->scene,r->request.value.index,r->request.now_ms)&&
  GscTestLoopCurrent(&r->scene,api,&expected,r->request.now_ms,time)&&
  (GscTestLoopPlayerPending()||GscTestLoopSend(r,COD1_LOOP_FX_VISIBLE))&&
  GscTestLoopCurrent(&r->scene,api,&expected,r->request.now_ms,time)){r->committed=1;ok=1;}
 gscTestLoopBusy=0;if(ok&&!gscTestLoopCancelled)return 1;
 GscTestError(error,cap,"loop FX GAME finish refused; exact owned cancellation/retirement required");return 0;
}
/* Called after Session advancement/completion handling. Unsent rows remain
 * real owned entities and may die before any recipient exists. Never replay a
 * retired row or infer ownership from the Session being busy/running. */
static int GscTestLoopFxFrame(void){
 int i,logical,time=level.time,ok=1;GscTestLoopFxRow*r;
 if(gscTestLoopBusy||gscTestLoopFreeBusy){gscTestLoopCancelled=1;return 0;}
 gscTestLoopBusy=1;gscTestLoopCancelled=0;
 for(i=0;i<MAX_GENTITIES;i++){
  r=gscTestLoops+i;if(!r->present||!r->committed||r->published)continue;
  if(r->retiring||r->death||!GscTestLoopBody(r)||!GscTestLoopNow(&r->scene,&logical)||
   !GscTestLoopResource(&r->scene,r->request.value.index,logical)||
   (!GscTestLoopPlayerPending()&&!GscTestLoopSend(r,COD1_LOOP_FX_VISIBLE))||
   gscTestLoopCancelled||level.time!=time||!GscTestLoopBody(r)||
   !GscTestFxTimeCurrent(&r->scene,logical)){ok=0;break;}
 }
 gscTestLoopBusy=0;return ok&&!gscTestLoopCancelled;
}
/* Separate dispatcher: the existing actor/ordinary admission is unchanged. */
static int GscTestLoopFxDelete(void*u,const GscDeleteTransactionRequest*input,
 const GscDeleteTransactionServices*api,char*error,size_t cap){
 GscTestLoopFxRow*r;GscTestFxScene scene;GscDeleteTransactionRequest q;GscDeleteTransactionServices expected;int ok=0,time=level.time;
 (void)u;if(!input||!api)return 0;r=GscTestLoopRow(&input->entity);
 if(!r)return G_Cod1EntityDeleteTransactionHost()->invoke(NULL,input,api,error,cap);
 if(gscTestLoopBusy||gscTestLoopFreeBusy){gscTestLoopCancelled=1;return 0;}
 if(!api->current||!api->notify_death||GscTestLoopPrivate(error,cap)||
  GscTestLoopOverlap(input,sizeof(*input),error,cap)||GscTestLoopOverlap(api,sizeof(*api),error,cap))return 0;
 q=*input;expected=*api;scene=r->scene;
 if(q.now_ms<0||!r->committed||r->retiring||r->death||!GscTestLoopScene(&r->scene)||!GscTestLoopBody(r))return 0;
 gscTestLoopBusy=1;gscTestLoopCancelled=0;
 if(!api->current(api->context)||memcmp(api,&expected,sizeof(expected))||memcmp(input,&q,sizeof(q))||
  gscTestLoopCancelled||level.time!=time||!GscTestLoopScene(&r->scene)||!GscTestLoopBody(r))goto done;
 r->death=1;
 if(api->notify_death(api->context,error,cap)!=1||memcmp(api,&expected,sizeof(expected))||
  memcmp(input,&q,sizeof(q))||gscTestLoopCancelled||level.time!=time||!GscTestLoopBody(r)||
  !GscTestLoopScene(&r->scene)||!api->current(api->context)||memcmp(api,&expected,sizeof(expected))||
  memcmp(input,&q,sizeof(q))||gscTestLoopCancelled||level.time!=time||!GscTestLoopBody(r))goto done;
 ok=GscTestLoopRemove(r)&&!gscTestLoopCancelled&&level.time==time&&GscTestLoopScene(&scene)&&
  !memcmp(api,&expected,sizeof(expected))&&!memcmp(input,&q,sizeof(q))&&api->current(api->context)&&
  !gscTestLoopCancelled&&level.time==time&&GscTestLoopScene(&scene)&&
  !memcmp(api,&expected,sizeof(expected))&&!memcmp(input,&q,sizeof(q));
done:gscTestLoopBusy=0;return ok;
}
static int GscTestLoopFxSelect(GscSession*s,char*error,size_t cap){
 static const GscLoopFxHost host={NULL,GscTestLoopFxPrepare,GscTestLoopFxFinish};
 return s==gscTestSession&&gsc_session_select_loop_fx(s,&host,error,cap);
}
static const GscDeleteTransactionHost *GscTestLoopFxDeleteHost(void){
 static const GscDeleteTransactionHost host={NULL,GscTestLoopFxDelete};return &host;
}
