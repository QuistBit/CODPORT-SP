/* Sole owner g_client.c. Retail GAME13EC0 -> think5=13D00, dispatched by
 * 1E8F0 at time+200. No GSC/controller or path-graph readiness is implied.
 * A failed provider is terminal for this allocation, never a clear trace. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_path_entity_host.h"
#include <limits.h>
#include <math.h>
typedef struct {
 GscEntityRef ref;
 entityShared_t shared;
 entityState_t state;
 int phase,due;
 char *classname;
} C1SpawnPoint;
static C1SpawnPoint cod1SpawnPoints[MAX_GENTITIES];
void G_Cod1SpawnPointThink(gentity_t *ent);
int G_Cod1SpawnPointCurrent(const GscEntityRef *ref){
 C1SpawnPoint *r;gentity_t *e;
 if(!ref||!G_Cod1EntitySameAllocation(ref))return 0;
 r=cod1SpawnPoints+ref->slot;e=g_entities+ref->slot;
 if(!r->phase||r->ref.epoch!=ref->epoch||r->ref.generation!=ref->generation||
    r->ref.slot!=ref->slot||e->client||e->r.linked||e->r.bmodel||e->r.contents||
    e->s.modelindex||e->touch||e->use||e->think!=G_Cod1SpawnPointThink||
    e->classname!=r->classname||memcmp(&e->r,&r->shared,sizeof(e->r))||
    memcmp(&e->s,&r->state,sizeof(e->s)))return 0;
 return e->nextthink==0||(r->phase==1&&e->nextthink==r->due);
}
static int Cod1SpawnPointTrace(C1SpawnPoint *r,const float *start,const float *end,trace_t *out){
 cod1SpatialTraceQuery_t q,before;trace_t tr;int i,status;C1SpawnPoint saved;
 if(!G_Cod1SpawnPointCurrent(&r->ref)||!G_Cod1PathSpawnWorldReady())return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);
 VectorCopy(start,q.start);VectorCopy(end,q.end);
 VectorSet(q.mins,-15,-15,0);VectorSet(q.maxs,15,15,72);
 q.contentsMask=0x0281c011u;q.passEntity[0]=r->ref.slot;q.passEntity[1]=ENTITYNUM_NONE;
 before=q;saved=*r;memset(&tr,0,sizeof(tr));status=G_Cod1PathSpawnTrace(&q,&tr);
 if(!status||memcmp(&before,&q,sizeof(q))||
    memcmp(&saved,r,sizeof(saved))||!G_Cod1SpawnPointCurrent(&saved.ref)||!G_Cod1PathSpawnWorldReady()||
    !isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1||
    tr.entityNum<0||tr.entityNum>=MAX_GENTITIES)return 0;
 for(i=0;i<3;i++)if(!isfinite(tr.endpos[i]))return 0;
 *out=tr;return 1;
}
void G_Cod1SpawnPointThink(gentity_t *ent){
 GscEntityRef ref;C1SpawnPoint *r;vec3_t start,end;trace_t tr;int ground;
 if(!G_Cod1EntityCapture(ent,&ref))return;r=cod1SpawnPoints+ref.slot;
 if(!G_Cod1SpawnPointCurrent(&ref)||r->phase!=1||ent->nextthink||level.time<r->due)return;
 r->phase=2;VectorCopy(ent->r.currentOrigin,start);VectorCopy(start,end);end[2]+=128.0f;
 if(!Cod1SpawnPointTrace(r,start,end,&tr))goto refused;
 VectorCopy(tr.endpos,start);VectorCopy(start,end);end[2]-=256.0f;
 if(!Cod1SpawnPointTrace(r,start,end,&tr))goto refused;
 ground=tr.entityNum;VectorCopy(tr.endpos,end);VectorCopy(end,start);
 /* Third call is a position test at the SECOND trace endpoint. Native
  * records the SECOND hit entity, even if the third returns another. */
 if(!Cod1SpawnPointTrace(r,start,end,&tr))goto refused;
 if(tr.startsolid)G_Printf("CoD spawn point %u starts in solid\n",ref.slot);
 if(!G_Cod1SpawnPointCurrent(&ref))goto refused;
 G_SetOrigin(ent,tr.endpos);VectorCopy(tr.endpos,ent->s.origin);
 ent->s.groundEntityNum=ground;r->shared=ent->r;r->state=ent->s;r->phase=3;return;
refused:
 /* Keep the real initial point and no completion receipt. A new allocation
  * or map owns a new record; no sticky failure is reset on this allocation. */
 if(r->ref.epoch==ref.epoch&&r->ref.generation==ref.generation){
  r->phase=-1;G_Printf("CoD spawn point %u native placement unavailable\n",ref.slot);
 }
}
static int Cod1SpawnPointStart(gentity_t *ent){
 GscEntityRef ref;C1SpawnPoint *r;int i;
 if(!G_Cod1MapEntities())return 0;
 if(!level.spawning||!G_Cod1EntityCapture(ent,&ref)||ent->client||ent->r.linked||
    ent->r.bmodel||ent->r.contents||ent->s.modelindex||ent->touch||ent->use||ent->think||
    ent->nextthink||level.time<0||level.time>INT_MAX-200)return 1;
 for(i=0;i<3;i++)if(!isfinite(ent->r.currentOrigin[i]))return 1;
 r=cod1SpawnPoints+ref.slot;
 if(r->phase&&r->ref.epoch==ref.epoch&&r->ref.generation==ref.generation)return 1;
 memset(r,0,sizeof(*r));r->ref=ref;r->phase=1;r->due=level.time+200;
 ent->classname="info_player_deathmatch";ent->nextthink=r->due;ent->think=G_Cod1SpawnPointThink;
 r->classname=ent->classname;r->shared=ent->r;r->state=ent->s;
 if(!G_Cod1PathSpawnPointBorn(ent,&ref)){r->phase=-1;ent->nextthink=0;}
 return 1;
}
#endif
