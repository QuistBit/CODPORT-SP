/* Separate generation-owned native structural topology. No RTCW tagParent,
 * raw native pointer/flags or Actor bootstrap certificate is manufactured. */
#include "../bg_cod1_entity_link_math.h"
typedef struct {GscEntityRef owner,parent;uint32_t next;float local[12];} C1EntityLink;
static C1EntityLink cod1EntityLinks[MAX_GENTITIES];
static GscEntityRef cod1EntityLinkParents[MAX_GENTITIES];
static uint32_t cod1EntityLinkHeads[MAX_GENTITIES];
static int cod1EntityLinkBusy,cod1EntityLinkCancelled;
static int EntityLinkPrivate(const void*p,size_t n){return
 ActorOverlap(p,n,cod1EntityLinks,sizeof(cod1EntityLinks))||ActorOverlap(p,n,cod1EntityLinkParents,sizeof(cod1EntityLinkParents))||ActorOverlap(p,n,cod1EntityLinkHeads,sizeof(cod1EntityLinkHeads))||
 ActorOverlap(p,n,&cod1EntityLinkBusy,sizeof(cod1EntityLinkBusy))||ActorOverlap(p,n,&cod1EntityLinkCancelled,sizeof(cod1EntityLinkCancelled))||ActorOverlap(p,n,G_Cod1EntityLinkHost(),sizeof(GscEntityLinkHost));}
static int EntityLinkSame(const GscEntityRef*a,const GscEntityRef*b){return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;}
static int EntityLinkOwner(const GscEntityRef*r){return r&&r->slot<MAX_GENTITIES&&EntityLinkSame(r,&cod1EntityLinks[r->slot].owner);}
static int EntityLinkMap(void){return cod1EntityReady&&!level.spawning&&!level.restarted&&level.time>=0&&ActorMapAvailable();}
static int EntityLinkCapable(const GscEntityRef*r){const gentity_t*e;
 if(!EntityLinkMap()||!EntityLinkOwner(r)||!G_Cod1EntitySameAllocation(r))return 0;e=g_entities+r->slot;
 return e->classname&&!strcmp(e->classname,"script_origin")&&!e->client&&!e->r.contents&&!e->r.bmodel&&e->s.eType==ET_GENERAL&&!e->tagParent&&!e->tagName&&!e->neverFree&&!e->teamchain&&!e->teammaster;
}
static void EntityLinkPose(gentity_t*e,const float*o,const float*a,int interpolated){
 vec3_t origin,angles;memcpy(origin,o,12);memcpy(angles,a,12);G_SetOrigin(e,origin);G_SetAngle(e,angles);memcpy(e->s.origin,o,12);memcpy(e->s.angles,a,12);
 if(interpolated){e->s.pos.trType=TR_INTERPOLATE;e->s.apos.trType=TR_INTERPOLATE;}
}
static int EntityLinkDetach(const GscEntityRef*r,int preserve){
 C1EntityLink*l;uint32_t*cursor,n;int limit=0;gentity_t*e;
 if(!EntityLinkOwner(r))return 1;l=cod1EntityLinks+r->slot;if(!l->parent.epoch)return 1;
 if(l->parent.slot>=MAX_GENTITIES||!EntityLinkSame(&l->parent,cod1EntityLinkParents+l->parent.slot))return 0;
 cursor=cod1EntityLinkHeads+l->parent.slot;
 while((n=*cursor)!=r->slot+1){if(!n||n>MAX_GENTITIES||++limit>MAX_GENTITIES)return 0;cursor=&cod1EntityLinks[n-1].next;}
 *cursor=l->next;
 if(preserve&&G_Cod1EntitySameAllocation(r)){e=g_entities+r->slot;EntityLinkPose(e,e->r.currentOrigin,e->r.currentAngles,0);}
 memset(&l->parent,0,sizeof(l->parent));l->next=0;memset(l->local,0,sizeof(l->local));return 1;
}
static int EntityLinksRetire(const GscEntityRef*r,int born){
 uint32_t n;int limit=0;GscEntityRef child;
 if(!r||r->slot>=MAX_GENTITIES)return 0;
 if(EntityLinkOwner(r)&&!EntityLinkDetach(r,!born))return 0;
 if(EntityLinkSame(r,cod1EntityLinkParents+r->slot)){
  while((n=cod1EntityLinkHeads[r->slot])!=0){if(n>MAX_GENTITIES||++limit>MAX_GENTITIES)return 0;child=cod1EntityLinks[n-1].owner;if(!child.epoch||!EntityLinkDetach(&child,1))return 0;}
  memset(cod1EntityLinkParents+r->slot,0,sizeof(*cod1EntityLinkParents));
 }
 if(EntityLinkOwner(r))memset(cod1EntityLinks+r->slot,0,sizeof(*cod1EntityLinks));return 1;
}
static void EntityLinksReset(void){
 memset(cod1EntityLinks,0,sizeof(cod1EntityLinks));memset(cod1EntityLinkParents,0,sizeof(cod1EntityLinkParents));memset(cod1EntityLinkHeads,0,sizeof(cod1EntityLinkHeads));
 if(cod1EntityLinkBusy)cod1EntityLinkCancelled=1;else cod1EntityLinkCancelled=0;
}
void G_Cod1EntityLinkOriginBorn(const GscEntityRef*r){
 /* Called only by the actual script_origin initializer after its real link
  * and allocation check. This is constructor provenance, not classname inference. */
 if(r&&G_Cod1EntitySameAllocation(r)&&r->slot<MAX_GENTITIES&&!EntityLinkOwner(r)){memset(cod1EntityLinks+r->slot,0,sizeof(*cod1EntityLinks));cod1EntityLinks[r->slot].owner=*r;}
}
int G_Cod1EntityLinksBeforeFree(const GscEntityRef*r){
 if(cod1EntityLinkBusy&&(EntityLinkOwner(r)||(r&&r->slot<MAX_GENTITIES&&EntityLinkSame(r,cod1EntityLinkParents+r->slot))))cod1EntityLinkCancelled=1;
 return EntityLinksRetire(r,0);
}
static int EntityLinkContext(const GscEntityLinkRequest*q,int(*current)(void*),void*ctx){return !cod1EntityLinkCancelled&&EntityLinkMap()&&q->now_ms==level.time&&current(ctx)==1&&!cod1EntityLinkCancelled&&EntityLinkMap()&&q->now_ms==level.time;}
static int EntityLinkInvoke(void*u,const GscEntityLinkRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscEntityLinkRequest q;C1EntityLink*l;GscEntityRef parent;gentity_t childBefore,parentBefore;float child[12],frame[12],inverse[12],local[12];int ok=0,limit=0;(void)u;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;
 if(cod1EntityLinkBusy){cod1EntityLinkCancelled=1;return 0;}q=*input;cod1EntityLinkBusy=1;cod1EntityLinkCancelled=0;
 if(!EntityLinkContext(&q,current,ctx)||memcmp(input,&q,sizeof(q))||!EntityLinkCapable(&q.child))goto done;
 if(q.operation==GSC_ENTITY_LINK_CHECK){ok=1;goto done;}
 if(q.operation==GSC_ENTITY_UNLINK){ok=EntityLinkDetach(&q.child,1);goto done;}
 if(q.operation!=GSC_ENTITY_LINK_ATTACH||!G_Cod1EntitySameAllocation(&q.parent)||q.parent.epoch!=q.child.epoch||!memchr(q.tag,0,sizeof(q.tag))||(q.explicit_offsets!=0&&q.explicit_offsets!=1))goto done;
 /*44B90 commits unlink before tag/model/cycle errors. Unimplemented tag owner
  * refuses explicitly after that prefix; it never creates a fabricated tag. */
 if(!EntityLinkDetach(&q.child,1))goto done;
 if(q.tag[0]){EntityError(error,cap,"linkto named parent tag owner remains unsupported");goto done;}
 parent=q.parent;
 while(parent.epoch){if(EntityLinkSame(&parent,&q.child)||++limit>MAX_GENTITIES){EntityError(error,cap,"failed to link entity due to link cycle");goto done;}if(!EntityLinkOwner(&parent))break;parent=cod1EntityLinks[parent.slot].parent;}
 childBefore=g_entities[q.child.slot];parentBefore=g_entities[q.parent.slot];
 if(!C1EL_Frame(parentBefore.r.currentOrigin,parentBefore.r.currentAngles,frame))goto done;
 if(q.explicit_offsets){if(!C1EL_Frame(q.origin,q.angles,local))goto done;}
 else if(!C1EL_Frame(g_entities[q.child.slot].r.currentOrigin,g_entities[q.child.slot].r.currentAngles,child)||!C1EL_Inverse(frame,inverse)||!C1EL_Multiply(child,inverse,local))goto done;
 if(!EntityLinkContext(&q,current,ctx)||memcmp(input,&q,sizeof(q))||!EntityLinkCapable(&q.child)||!G_Cod1EntitySameAllocation(&q.parent)||memcmp(&childBefore,g_entities+q.child.slot,sizeof(childBefore))||memcmp(&parentBefore,g_entities+q.parent.slot,sizeof(parentBefore)))goto done;
 if(cod1EntityLinkHeads[q.parent.slot]&&!EntityLinkSame(&q.parent,cod1EntityLinkParents+q.parent.slot))goto done;
 l=cod1EntityLinks+q.child.slot;cod1EntityLinkParents[q.parent.slot]=q.parent;l->parent=q.parent;l->next=cod1EntityLinkHeads[q.parent.slot];memcpy(l->local,local,sizeof(local));cod1EntityLinkHeads[q.parent.slot]=q.child.slot+1;ok=1;
done:cod1EntityLinkBusy=0;cod1EntityLinkCancelled=0;if(!ok&&error&&cap&&!error[0])EntityError(error,cap,"linkto requires current own script_origin constructor/parent/clock and supported empty tag transform");return ok;
}
const GscEntityLinkHost*G_Cod1EntityLinkHost(void){static const GscEntityLinkHost h={NULL,EntityLinkInvoke};return &h;}
int G_Cod1EntityLinkRunEntity(gentity_t*e){
 GscEntityRef child,parent;C1EntityLink before;float frame[12],world[12],angles[3],parentOrigin[3],parentAngles[3];trajectory_t pos,apos;uint64_t epoch;int now,ok=0;
 if(!e||!G_Cod1EntityCapture(e,&child)||!EntityLinkOwner(&child)||!cod1EntityLinks[child.slot].parent.epoch)return 1;
 if(cod1EntityLinkBusy){cod1EntityLinkCancelled=1;return 0;}cod1EntityLinkBusy=1;cod1EntityLinkCancelled=0;before=cod1EntityLinks[child.slot];parent=before.parent;epoch=child.epoch;now=level.time;
 if(!EntityLinkCapable(&child)||!G_Cod1EntitySameAllocation(&parent)||!C1EL_Frame(g_entities[parent.slot].r.currentOrigin,g_entities[parent.slot].r.currentAngles,frame)||!C1EL_Multiply(before.local,frame,world)||!C1EL_Angles(world,angles))goto done;
 memcpy(parentOrigin,g_entities[parent.slot].r.currentOrigin,12);memcpy(parentAngles,g_entities[parent.slot].r.currentAngles,12);EntityLinkPose(e,world+9,angles,1);pos=e->s.pos;apos=e->s.apos;trap_LinkEntity(e);
 ok=!cod1EntityLinkCancelled&&EntityLinkMap()&&level.time==now&&G_Cod1EntityMapEpoch()==epoch&&EntityLinkCapable(&child)&&G_Cod1EntitySameAllocation(&parent)&&!memcmp(&before,cod1EntityLinks+child.slot,sizeof(before))&&!memcmp(parentOrigin,g_entities[parent.slot].r.currentOrigin,12)&&!memcmp(parentAngles,g_entities[parent.slot].r.currentAngles,12)&&!memcmp(world+9,e->r.currentOrigin,12)&&!memcmp(angles,e->r.currentAngles,12)&&!memcmp(&pos,&e->s.pos,sizeof(pos))&&!memcmp(&apos,&e->s.apos,sizeof(apos))&&!memcmp(world+9,e->s.origin,12)&&!memcmp(angles,e->s.angles,12);
done:cod1EntityLinkBusy=0;cod1EntityLinkCancelled=0;return ok;
}
