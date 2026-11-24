#include "../../qcommon/cod1_actor_check_prone14.h"
typedef struct {C1MayMoveCall owner;int failed;} C1CheckProneCall;
static int ActorCheckProneTraceInner(C1CheckProneCall*m,c1ap14Trace_t*out,const float*start,const float*mins,const float*maxs,const float*end,int pass,uint32_t mask,int capsule){
 cod1SpatialTraceQuery_t q,before;trace_t tr;int status;uint32_t f;
 if(m->failed||pass!=(int)m->owner.call->entity.slot||(mask!=0x810011u&&mask!=0x820011u)||
    !ActorMayMoveCurrent(&m->owner)||!C1MMP14_Vec(start)||!C1MMP14_Vec(end)||!C1MMP14_Vec(mins)||!C1MMP14_Vec(maxs))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);
 memcpy(q.start,start,12);memcpy(q.end,end,12);memcpy(q.mins,mins,12);memcpy(q.maxs,maxs,12);
 q.passEntity[0]=pass;q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;before=q;memset(&tr,0,sizeof(tr));
 status=capsule?trap_Cod1SpatialCapsuleTrace(&q,&tr):trap_Cod1SpatialBoxTrace(&q,&tr);m->owner.queryCount++;
 memcpy(&f,&tr.fraction,4);
 if(status!=COD1_SPATIAL_OK||memcmp(&q,&before,sizeof(q))||!ActorMayMoveCurrent(&m->owner)||
   f>0x3f800000u||!C1MMP14_Vec(tr.endpos)||!C1MMP14_Vec(tr.plane.normal)||
   (tr.allsolid!=0&&tr.allsolid!=1)||(tr.startsolid!=0&&tr.startsolid!=1))return 0;
 memset(out,0,sizeof(*out));out->fraction=tr.fraction;memcpy(out->end,tr.endpos,12);memcpy(out->normal,tr.plane.normal,12);
 out->surface_flags=tr.surfaceFlags;out->contents=tr.contents;out->allsolid=(unsigned char)tr.allsolid;out->startsolid=(unsigned char)tr.startsolid;return 1;
}
/* The original callback ABI is void and has no context. The outer operation
 * owns this private stage and excludes reentry. A failed query is latched;
 * its internal stop value can never be published as a native false result. */
static void ActorCheckProneTrace(c1ap14Trace_t*out,const float*start,const float*mins,const float*maxs,const float*end,int pass,uint32_t mask,int capsule){
 C1CheckProneCall*m=(C1CheckProneCall*)cod1ActorLookOperation.stage;
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;int ok;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=m&&cod1ActorLookOperation.bytes==sizeof(*m)&&ActorCheckProneTraceInner(m,out,start,mins,maxs,end,pass,mask,capsule);
 if(!ok){if(m)m->failed=1;memset(out,0,sizeof(*out));out->allsolid=1;}
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
}
static void __cdecl ActorCheckProneCapsule(c1ap14Trace_t*o,const float*s,const float*mi,const float*ma,const float*e,int pass,uint32_t mask){ActorCheckProneTrace(o,s,mi,ma,e,pass,mask,1);}
static void __cdecl ActorCheckProneBox(c1ap14Trace_t*o,const float*s,const float*mi,const float*ma,const float*e,int pass,uint32_t mask){ActorCheckProneTrace(o,s,mi,ma,e,pass,mask,0);}
static int ActorCheckProne(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*out,char*error,size_t cap){
 C1CheckProneCall*m;GscActorLookRequest canonical;float origin[3],yaw;int already,value,ok=0;
 memset(&canonical,0,sizeof(canonical));canonical.entity=q->entity;canonical.method=GSC_ACTOR_LOOK_CHECK_PRONE;
 canonical.runtime_identity=q->runtime_identity;canonical.catalog_identity=q->catalog_identity;canonical.now_ms=q->now_ms;canonical.precision_bits=53;
 memcpy(canonical.vector_bits,q->vector_bits,12);memcpy(canonical.scalar_bits,q->scalar_bits,8);
 if(memcmp(q,&canonical,sizeof(canonical)))return 0;
 m=calloc(1,sizeof(*m));if(!m)return 0;m->owner.call=c;m->owner.actor=cod1Actors[c->lease.actorIndex];m->owner.entity=g_entities[c->entity.slot];
 memcpy(m->owner.sentients,cod1Sentients,sizeof(m->owner.sentients));m->owner.graph=cod1GenericActors[c->lease.actorIndex];
 cod1ActorLookOperation.stage=m;cod1ActorLookOperation.bytes=sizeof(*m);
 if(!m->owner.graph.used||m->owner.graph.runtime!=q->runtime_identity||m->owner.graph.catalog!=q->catalog_identity||
   memcmp(&m->owner.graph.lease,&c->lease,sizeof(c->lease))||!ActorMayMoveCurrent(&m->owner))goto done;
 memcpy(origin,q->vector_bits,12);memcpy(&yaw,q->scalar_bits,4);memcpy(&already,q->scalar_bits+1,4);
 if(!C1CP14_Apply((int)c->entity.slot,origin,yaw,already,ActorCheckProneCapsule,ActorCheckProneBox,&value)||m->failed||!ActorMayMoveCurrent(&m->owner))goto done;
 out->kind=GSC_ACTOR_LOOK_RESULT_BOOL;out->boolean=value;ok=1;
 if(g_debugMove.integer>1)G_Printf("[413 checkProne] owner=%u tick=%d result=%d traces=%d already=%d\n",c->entity.slot,c->now,value,m->owner.queryCount,already);
done:if(!ok)EntityError(error,cap,"checkProne owner/query/finite profile unavailable");cod1ActorLookOperation.stage=NULL;cod1ActorLookOperation.bytes=0;free(m);return ok;
}
