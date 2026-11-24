#include "g_cod1_navigation_probe.h"
#include "g_cod1_navigation_world.h"
#include "g_cod1_navigation_actor_input.h"
#include "g_cod1_turret.h"
#include "bg_cod1_actor_move_api.h"
#include "bg_cod1_path_delta.h"
#include "bg_cod1_path_navigation.h"
/* Graph owner, map/world and actor goal stay pinned through every SERVER call.
 * This diagnostic never changes path words, actor state or script events. */
typedef struct {
 GscSession*s;uint64_t id,receipt,world;int now;GscEntityRef actor;
 cod1ActorGoalSnapshot_t goal;cod1ActorLeaseSnapshot_t actorState;C1DPGraphCandidate*source,description;
 C1NTree tree,treeSource;unsigned char *records,*raw;C1DPNode*nodes;C1DPLink*links;C1PSNode*searchNodes;C1PSLink*searchLinks;unsigned routeTraces;
 int inputReady;cod1NavigationActorInput_t input;unsigned physicsTraces;
} C1NavigationProbe;
static int Cod1NavigationProbeCurrent(C1NavigationProbe*c){
 cod1ActorGoalSnapshot_t goal;cod1ActorLeaseSnapshot_t actor;C1DPGraphCandidate*g=c->source;cod1NavigationActorInput_t input;
 if(c->inputReady&&(!G_Cod1NavigationActorInput(&c->actor,c->now,&input)||memcmp(&input,&c->input,sizeof(input))))return 0;
 return c->now==level.time&&G_Cod1EntitySameAllocation(&c->actor)&&
  G_Cod1ActorReadGoal(&c->actor,&goal,NULL,0)&&!memcmp(&goal,&c->goal,sizeof(goal))&&
  G_Cod1ActorReadLease(&c->actorState.lease,&actor,NULL,0)&&!memcmp(&actor,&c->actorState,sizeof(actor))&&
  g==cod1GraphStaged&&!memcmp(g,&c->description,sizeof(*g))&&
  Cod1GraphPublishedOwnerCurrent(c->s,c->id,c->receipt)&&
  G_Cod1NavigationWorldCurrent(c->world)&&!memcmp(c->raw,g->records,g->count*116)&&
  !memcmp(c->nodes,g->nodes,g->count*sizeof(*g->nodes))&&
  (!g->linkCount||!memcmp(c->links,g->links,g->linkCount*sizeof(*g->links)))&&
  !memcmp(g->tree,&c->treeSource,sizeof(c->treeSource))&&
  !memcmp(c->tree.order,g->tree->order,g->count*sizeof(*g->tree->order))&&
  !memcmp(c->tree.nodes,g->tree->nodes,g->tree->used*sizeof(*g->tree->nodes));
}
static int Cod1NavigationProbeFence(void*ctx){return Cod1NavigationProbeCurrent(ctx);}
static int Cod1NavigationProbePhysics(void*ctx,const float*a,const float*b,const float*lo,const float*hi,int pass,unsigned mask,C1APTrace*out){
 C1NavigationProbe*c=ctx;cod1SpatialTraceQuery_t q;trace_t tr;C1APTrace v;
 if(pass!=(int)c->actor.slot||(mask!=0x2820011&&mask!=0x820011)||!Cod1NavigationProbeCurrent(c))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.passEntity[0]=pass;q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;
 memcpy(q.start,a,12);memcpy(q.end,b,12);memcpy(q.mins,lo,12);memcpy(q.maxs,hi,12);c->physicsTraces++;
 if(trap_Cod1SpatialCapsuleTrace(&q,&tr)!=COD1_SPATIAL_OK||!Cod1NavigationProbeCurrent(c))return 0;
 memset(&v,0,sizeof(v));v.fraction=tr.fraction;memcpy(v.end,tr.endpos,12);memcpy(v.normal,tr.plane.normal,12);v.allsolid=tr.allsolid;v.startsolid=tr.startsolid;v.surface=tr.surfaceFlags;v.contents=tr.contents;
 v.entity=tr.entityNum==ENTITYNUM_WORLD?2046:tr.entityNum==ENTITYNUM_NONE?2047:(unsigned short)tr.entityNum;*out=v;return 1;
}
static int Cod1NavigationProbeSight(void*ctx,const float*a,const float*lo,const float*hi,const float*b,unsigned p,unsigned p2,unsigned mask,int*out){
 C1NavigationProbe*c=ctx;cod1SpatialTraceQuery_t q;int value,status;
 if(p!=2047||p2!=2047||mask!=0x820011||!Cod1NavigationProbeCurrent(c))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.passEntity[0]=q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;
 memcpy(q.start,a,12);memcpy(q.end,b,12);memcpy(q.mins,lo,12);memcpy(q.maxs,hi,12);
 status=trap_Cod1PathWorldSight(&q,&value);
 if(status!=COD1_SPATIAL_OK||!Cod1NavigationProbeCurrent(c))return 0;
 *out=value!=0;return 1;
}
/* Read-only fresh-path51 lane. The native mask excludes sentients. Refuse
 * any unexpected sentient hit; no category or avoidance rule is invented. */
static int Cod1NavigationProbeTrace(void*ctx,int capsule,const float*a,const float*b,const float*lo,const float*hi,int pass,uint32_t mask,c1mmp14Trace_t*out){
 C1NavigationProbe*c=ctx;cod1SpatialTraceQuery_t q;trace_t tr;c1mmp14Trace_t v;
 if(capsule!=1||pass!=2047||mask!=0x820011||!Cod1NavigationProbeCurrent(c))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.passEntity[0]=q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;
 memcpy(q.start,a,12);memcpy(q.end,b,12);memcpy(q.mins,lo,12);memcpy(q.maxs,hi,12);c->routeTraces++;
 if(trap_Cod1SpatialCapsuleTrace(&q,&tr)!=COD1_SPATIAL_OK||!Cod1NavigationProbeCurrent(c))return 0;
 if((tr.allsolid||tr.fraction<1)&&tr.entityNum<ENTITYNUM_WORLD){
  GscEntityRef hit;cod1ActorPathView_t actor;
  if(tr.entityNum<MAX_CLIENTS||!G_Cod1EntityCapture(g_entities+tr.entityNum,&hit)||G_Cod1ActorPathRead(&hit,&actor))return 0;
 }
 memset(&v,0,sizeof(v));v.fraction=tr.fraction;memcpy(v.end,tr.endpos,12);memcpy(v.normal,tr.plane.normal,12);v.allsolid=tr.allsolid;v.startsolid=tr.startsolid;*out=v;return 1;
}
static int Cod1NavigationProbeInner(GscSession*s,uint64_t id,const GscEntityRef*actor,int now,cod1NavigationProbe_t*out){
 C1NavigationProbe*c=NULL;C1NRResult start,end;cod1NavigationProbe_t v;C1DPGraphCandidate*g;unsigned i;uint64_t receipt=cod1GraphReceipt;int ok=0,begun=0,captured=0;
 if(!s||!id||!actor||!out||now!=level.time||cod1NodeBusy||cod1GraphBusy||cod1GraphCancelled||
  Cod1GraphTargetPrivate(actor,sizeof(*actor))||Cod1GraphTargetPrivate(out,sizeof(*out))||
  Cod1NodeOverlap(out,sizeof(*out),actor,sizeof(*actor)))return 0;
 c=calloc(1,sizeof(*c));if(!c)return 0;c->actor=*actor;c->s=s;c->id=id;c->receipt=receipt;c->now=now;
 {cod1ActorPathView_t a;
 if(!G_Cod1EntitySameAllocation(actor)||!G_Cod1ActorReadGoal(actor,&c->goal,NULL,0)||!c->goal.assigned||
  !G_Cod1ActorPathRead(actor,&a)||!G_Cod1ActorReadLease(&a.lease,&c->actorState,NULL,0)||c->actorState.sentientDefaults.team>3)goto done;
 }
 if(!gsc_session_node_graph_check_begin(s,id,receipt))goto done;begun=1;cod1GraphBusy=1;
 if(!Cod1GraphPublishedOwnerCurrent(s,id,receipt)||!G_Cod1NavigationWorldBegin(0x2820011,&c->world))goto done;
 g=cod1GraphStaged;c->source=g;c->description=*g;
 if(!g->count||g->count>2048||!g->records||!g->nodes||!g->tree||g->tree->count!=g->count||!g->tree->used||g->tree->used>4095||!g->tree->order||!g->tree->nodes||g->linkCount>g->count*32767u||(g->linkCount&&!g->links))goto done;
 c->records=malloc(g->count*116);c->raw=malloc(g->count*116);c->nodes=malloc(g->count*sizeof(*g->nodes));
 c->links=malloc(g->linkCount*sizeof(*g->links));c->searchNodes=calloc(g->count,sizeof(*c->searchNodes));c->searchLinks=calloc(g->linkCount,sizeof(*c->searchLinks));
 c->treeSource=*g->tree;c->tree=*g->tree;c->tree.order=malloc(g->count*sizeof(*g->tree->order));c->tree.nodes=malloc(g->tree->used*sizeof(*g->tree->nodes));
 if(!c->records||!c->raw||!c->nodes||!c->tree.order||!c->tree.nodes||!c->searchNodes||(g->linkCount&&(!c->links||!c->searchLinks)))goto done;
 memcpy(c->records,g->records,g->count*116);memcpy(c->raw,g->records,g->count*116);memcpy(c->nodes,g->nodes,g->count*sizeof(*g->nodes));
 memcpy(c->tree.order,g->tree->order,g->count*sizeof(*g->tree->order));memcpy(c->tree.nodes,g->tree->nodes,g->tree->used*sizeof(*g->tree->nodes));
 if(g->linkCount)memcpy(c->links,g->links,g->linkCount*sizeof(*g->links));
 for(i=0;i<g->count;i++){
  if(c->nodes[i].active>c->nodes[i].count)goto done;memcpy(c->records+i*116+0x20,&c->nodes[i].active,2);
  c->searchNodes[i].first=c->nodes[i].first;c->searchNodes[i].active=c->nodes[i].active;memcpy(c->searchNodes[i].origin,c->nodes[i].origin,12);
 }
 for(i=0;i<g->linkCount;i++){c->searchLinks[i].target=c->links[i].target;memcpy(c->searchLinks[i].blocked,&c->links[i].payload[0],4);memcpy(&c->searchLinks[i].distance,&c->links[i].payload[1],4);}
 captured=1;
 c->inputReady=G_Cod1NavigationActorInput(&c->actor,now,&c->input);
 memset(&v,0,sizeof(v));memcpy(v.origin,g_entities[actor->slot].r.currentOrigin,12);memcpy(v.target,c->goal.position,12);v.assignment=c->goal.assignment;v.team=(int)c->actorState.sentientDefaults.team;
 if(!Cod1NavigationProbeCurrent(c)||!C1NRNearest(c->records,g->count*116,&c->tree,v.target,192,0xfffffffeu,64,0,0,0,Cod1NavigationProbeSight,c,&end)||
  !C1NRNearest(c->records,g->count*116,&c->tree,v.origin,192,0xfffffffeu,64,0,0,0,Cod1NavigationProbeSight,c,&start)||!Cod1NavigationProbeCurrent(c)||memcmp(actor,&c->actor,sizeof(*actor)))goto done;
 v.start=start.node;v.goal=end.node;v.candidates[0]=start.candidates;v.candidates[1]=end.candidates;v.traces[0]=start.traces;v.traces[1]=end.traces;
 if(start.node>=0)memcpy(v.startNode,c->records+start.node*116+0x38,12);if(end.node>=0)memcpy(v.goalNode,c->records+end.node*116+0x38,12);
 {C1PNGraph data;data.records=c->records;data.count=g->count;data.tree=&c->tree;data.nodes=c->searchNodes;data.links=c->searchLinks;data.linkCount=g->linkCount;
  /* Probe full5FE80 with traversal allowed. This is not52160 cache admission
   * and does not authorize publication into the Actor's retained path. */
  v.routeAvailable=C1PNRoute(v.path,&v.routeResult,(const unsigned char*)c->actorState.defaults.pathWords,&data,v.origin,v.target,v.team,1,now,Cod1NavigationProbeSight,Cod1NavigationProbeTrace,NULL,c);
  v.routeTraces=c->routeTraces;
  if(c->inputReady&&v.routeAvailable&&v.routeResult){
   c1ari14Result_t root;unsigned char physics[264],local[92]={0},path[984];C1APMovement moved;float length;
   v.category=c->input.category;v.rootAvailable=G_Cod1ActorGeneralRootInterval(&c->actor,now,0,&root,Cod1NavigationProbeFence,c);
   if(v.rootAvailable){
    memcpy(v.root,root.translation,12);length=(float)sqrt((double)root.translation[0]*root.translation[0]+(double)root.translation[1]*root.translation[1]);
    if(c->input.category==1&&!c->input.mode){
     v.deltaAvailable=C1PNDelta(path,v.delta,v.path,&data,v.origin,0,-1,now,length,Cod1NavigationProbeSight,Cod1NavigationProbeTrace,NULL,c);
     if(v.deltaAvailable){
      /* Read-only comparison using a fresh original route and the current
       * port velocity. Retained native path/physics publication is separate. */
      memcpy(physics,c->input.physics,264);memcpy(physics,v.origin,12);memcpy(physics+12,c->input.portVelocity,12);memcpy(physics+0x2c,v.delta,12);
      C1APSetU(physics,0x40,1);C1APSetF(physics,0x44,c->input.gravity);
      v.physicsAvailable=C1APMove(&moved,physics,local,0,Cod1NavigationProbePhysics,c);
      if(v.physicsAvailable){v.physicsResult=moved.result;memcpy(v.physicsEnd,moved.physics,12);memcpy(v.physicsVelocity,moved.physics+12,12);}
      v.physicsTraces=c->physicsTraces;
     }
    }
   }
  }
 }
 if(!Cod1NavigationProbeCurrent(c))goto done;ok=1;
done:
 if(begun){
  int current=captured?Cod1NavigationProbeCurrent(c):(Cod1GraphPublishedOwnerCurrent(s,id,receipt)&&(!c->world||G_Cod1NavigationWorldCurrent(c->world)));
  /* An unsupported query is not graph retirement. Only loss of ownership
   * consumes the published receipt; a failed diagnostic publishes nothing. */
  if(!gsc_session_node_graph_end(s,id,receipt,current)||!current)ok=0;cod1GraphBusy=0;
 }
 G_Cod1NavigationWorldEnd(c->world);free(c->records);free(c->raw);free(c->nodes);free(c->links);free(c->searchNodes);free(c->searchLinks);free(c->tree.order);free(c->tree.nodes);free(c);
 if(ok)*out=v;return ok;
}
int G_Cod1NavigationProbe(GscSession*s,uint64_t id,const GscEntityRef*actor,int now,cod1NavigationProbe_t*out){
 int ok,mode=trap_Cvar_VariableIntegerValue("g_cod1NavigationProbe"),started;
 /* Optional read-only experiment: ordinary play must not calculate a second
  * route/physics pipeline. Explicit modes1/2 enable it;2 also measures cost. */
 if(mode!=1&&mode!=2)return 0;
 started=mode==2?trap_Milliseconds():0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=Cod1NavigationProbeInner(s,id,actor,now,out);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=Cod1NavigationProbeInner(s,id,actor,now,out);
#endif
 if(mode==2)G_Printf("[568 probe cost] tick=%d elapsed=%d valid=%d\n",now,trap_Milliseconds()-started,ok);
 return ok;
}
/* G_Printf has a legacy1024-byte local buffer. Each encoded chunk plus
 * metadata stays below600 bytes; all984 source bytes remain recoverable. */
void G_Cod1NavigationDump(int now,uint64_t assignment,const unsigned char*path){
 unsigned offset,j,count;char hex[513];static const char digits[]="0123456789abcdef";
 if(!path)return;
 for(offset=0;offset<984;offset+=256){
  count=984-offset;if(count>256)count=256;
  for(j=0;j<count;j++){hex[j*2]=digits[path[offset+j]>>4];hex[j*2+1]=digits[path[offset+j]&15];}hex[count*2]=0;
  G_Printf("[566 path] tick=%d assignment=%llu offset=%u bytes=%u %s\n",now,(unsigned long long)assignment,offset,count,hex);
 }
}
