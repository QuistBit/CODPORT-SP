/* R228 selected eye-ray producer for the original trigger_lookat brushes.
 * Native GAME SP20044440 binds an invisible brush with look-at contents.
 * This port uses a separate CM trace against each admitted inline model;
 * touch/use and the player's collision hull never count as looking. */
static GscEntityRef cod1LookAtRefs[MAX_GENTITIES];
static unsigned char cod1LookAtOwned[MAX_GENTITIES];
static int Cod1LookAtSame(const GscEntityRef *ref){
 gentity_t *e;
 if(!ref||ref->slot>=MAX_GENTITIES||!cod1LookAtOwned[ref->slot]||
    memcmp(ref,&cod1LookAtRefs[ref->slot],sizeof(*ref))||!G_Cod1EntitySameAllocation(ref))return 0;
 e=&g_entities[ref->slot];return e->classname&&!strcmp(e->classname,"trigger_lookat")&&
  !e->spawnflags&&!e->touch&&!e->use&&e->r.linked&&e->r.bmodel&&e->r.contents==CONTENTS_TRIGGER&&
  e->s.modelindex>0&&e->s.eType==ET_INVISIBLE&&(e->r.svFlags&SVF_NOCLIENT);
}
void SP_trigger_lookat(gentity_t *e){
 GscEntityRef ref;int index,count;
 if(!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||!G_Cod1EntityCapture(e,&ref))return;
 cod1LookAtOwned[ref.slot]=0;
 if(G_Cod1GscFeature47(C1CP47_F_OPENING)!=1||!G_Cod1GscTestEntityEventsEnabled()){
  if(G_Cod1EntitySameAllocation(&ref))G_FreeEntity(e);return;
 }
 if(!G_Cod1EntitySameAllocation(&ref))return;
 G_Cod1TriggerInert(e);if(e->spawnflags)return;
 index=G_Cod1TriggerInlineIndex(e->model);if(index<=0)return;
 count=G_Cod1TriggerModelCount(&ref);if(!G_Cod1EntitySameAllocation(&ref)||count<=0||index>=count)return;
 InitTrigger(e);if(!G_Cod1EntitySameAllocation(&ref))return;
 e->s.eType=ET_INVISIBLE;e->r.svFlags|=SVF_NOCLIENT;e->r.contents=CONTENTS_TRIGGER;e->touch=NULL;e->use=NULL;
 trap_LinkEntity(e);if(!G_Cod1EntitySameAllocation(&ref)||!e->r.bmodel||!e->r.linked)return;
 cod1LookAtRefs[ref.slot]=ref;cod1LookAtOwned[ref.slot]=1;
 G_Cod1PathTriggerProjectedBorn(e,&ref,4);
}
void G_Cod1LookAtRunFrame(void){
 GscEntityRef player,hit,ref;gentity_t *p;vec3_t eye,end,forward,view;trace_t wall,trace;
 float nearest;int i,j,now=level.time,awakened;uint64_t epoch;
 if(!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||level.restarted||
    G_Cod1GscFeature47(C1CP47_F_OPENING)!=1||!G_Cod1GscTestEntityEventsEnabled())return;
 p=&g_entities[0];if(!p->client||p->client->pers.connected!=CON_CONNECTED||p->health<=0||
    p->client->ps.stats[STAT_HEALTH]<=0||p->client->noclip||p->client->sess.sessionTeam==TEAM_SPECTATOR||
    !G_Cod1EntityCapture(p,&player))return;
 epoch=player.epoch;VectorCopy(p->client->ps.origin,eye);eye[2]+=p->client->ps.viewheight;VectorCopy(p->client->ps.viewangles,view);
 for(j=0;j<3;j++)if(!isfinite(eye[j])||!isfinite(view[j]))return;
 AngleVectors(view,forward,NULL,NULL);VectorMA(eye,8192,forward,end);
 trap_Trace(&wall,eye,NULL,NULL,end,0,CONTENTS_SOLID|CONTENTS_BODY);
 if(now!=level.time||epoch!=G_Cod1EntityMapEpoch()||!G_Cod1EntitySameAllocation(&player)||
    wall.startsolid||wall.allsolid||!isfinite(wall.fraction)||wall.fraction<0||wall.fraction>1)return;
 nearest=wall.fraction;memset(&hit,0,sizeof(hit));
 for(i=MAX_CLIENTS;i<level.num_entities&&i<MAX_GENTITIES;i++){
  vec3_t position,angles;int model;
  ref=cod1LookAtRefs[i];if(!Cod1LookAtSame(&ref))continue;
  VectorCopy(g_entities[ref.slot].r.currentOrigin,position);VectorCopy(g_entities[ref.slot].r.currentAngles,angles);model=g_entities[ref.slot].s.modelindex;
  trap_TraceUseEntity(&trace,eye,end,(int)ref.slot);
  if(now!=level.time||epoch!=G_Cod1EntityMapEpoch()||!G_Cod1EntitySameAllocation(&player))return;
  if(!Cod1LookAtSame(&ref)||model!=g_entities[ref.slot].s.modelindex||
     !VectorCompare(position,g_entities[ref.slot].r.currentOrigin)||!VectorCompare(angles,g_entities[ref.slot].r.currentAngles)||
     !isfinite(trace.fraction)||trace.fraction<0||trace.fraction>nearest||
     (!trace.startsolid&&!trace.allsolid&&trace.fraction==1))continue;
  nearest=trace.fraction;hit=ref;
 }
 if(!hit.epoch||!Cod1LookAtSame(&hit)||!G_Cod1EntitySameAllocation(&player)||
    !VectorCompare(view,p->client->ps.viewangles)||now!=level.time||p->health<=0||
    p->client->ps.stats[STAT_HEALTH]<=0||p->client->noclip||p->client->pers.connected!=CON_CONNECTED)return;
 {vec3_t currentEye;VectorCopy(p->client->ps.origin,currentEye);currentEye[2]+=p->client->ps.viewheight;
  if(!VectorCompare(eye,currentEye))return;}
 {GscEventValue value;memset(&value,0,sizeof(value));value.kind=GSC_EVENT_ENTITY;value.entity=player;awakened=0;
  (void)G_Cod1GscTestEntityEventWithPayload(&g_entities[hit.slot],"trigger",&value,1,&awakened);}
}
