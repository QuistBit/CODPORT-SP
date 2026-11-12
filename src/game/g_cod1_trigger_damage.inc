#include <math.h>
#include <limits.h>
/* R222 selected bullet-trigger lane. Original SP trigger_damage 20044050
 * binds an invisible inline brush; native segment producer 20044170 tests
 * exact brush contact before delivering trigger(attacker,damage). */
static unsigned char cod1DamageTriggerOwned[MAX_GENTITIES];
static GscEntityRef cod1DamageTriggerRefs[MAX_GENTITIES];
static int cod1DamageTriggerDeadline[MAX_GENTITIES];
static int Cod1DamageTriggerSame(const GscEntityRef *ref) {
    gentity_t *e;
    if(!ref||ref->slot>=MAX_GENTITIES||!cod1DamageTriggerOwned[ref->slot]||
       memcmp(ref,&cod1DamageTriggerRefs[ref->slot],sizeof(*ref))||!G_Cod1EntitySameAllocation(ref))return 0;
    e=&g_entities[ref->slot];return e->classname&&!strcmp(e->classname,"trigger_damage")&&
        e->r.linked&&e->r.bmodel&&e->r.contents==CONTENTS_TRIGGER&&e->s.eType==ET_INVISIBLE&&
        (e->r.svFlags&SVF_NOCLIENT)&&!e->touch&&!e->use&&
        e->takedamage==(e->spawnflags==3?qtrue:qfalse)&&
        (e->spawnflags==0||e->spawnflags==3);
}
/* A waiter's receiver is the brush allocation recorded by SP_trigger_damage.
 * Keep validating that same receiver through suspension and its timed return. */
int G_Cod1GrenadeTargetThreadCurrent(const GscEntityRef *ref, uint64_t birth) {
    const gentity_t *target;
    if (!ref || !birth || birth != ref->generation || level.spawning || level.restarted ||
        !G_Cod1MapEntities() || !Cod1DamageTriggerSame(ref)) return 0;
    target = &g_entities[ref->slot];
    return target->spawnflags == 3 && target->s.modelindex > 0 &&
           target->targetname && !strcmp(target->targetname, "grenade_target");
}
void SP_trigger_damage(gentity_t *e) {
    GscEntityRef ref;
    if(!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||!G_Cod1EntityCapture(e,&ref))return;
    cod1DamageTriggerOwned[ref.slot]=0;
    if(!G_Cod1GscTestEntityEventsEnabled()||
       (e->spawnflags!=0&&(e->spawnflags!=3||trap_Cvar_VariableIntegerValue("g_cod1GscTrainingGrenades")!=1))){G_Printf("R222 trigger_damage unsupported profile/flags; entity retired\n");G_FreeEntity(e);return;}
    G_SpawnFloat("wait","0.5",&e->wait);
    if(!isfinite(e->wait)||e->wait<0.f||e->wait>3600.f){G_FreeEntity(e);return;}
    InitTrigger(e);if(!G_Cod1EntitySameAllocation(&ref))return;
    e->s.eType=ET_INVISIBLE;e->r.svFlags|=SVF_NOCLIENT;e->r.contents=CONTENTS_TRIGGER;
    e->touch=NULL;e->use=NULL;e->takedamage=e->spawnflags==3?qtrue:qfalse;
    if(e->spawnflags==3)e->health=32000;
    trap_LinkEntity(e);
    if(!G_Cod1EntitySameAllocation(&ref)||!e->r.bmodel||!e->r.linked)return;
    cod1DamageTriggerRefs[ref.slot]=ref;cod1DamageTriggerDeadline[ref.slot]=INT_MIN;cod1DamageTriggerOwned[ref.slot]=1;
    G_Cod1PathTriggerProjectedBorn(e,&ref,5);
}
/* The native trigger has a damage receiver too (SP20044050, common handler
 * 20043EB0 and damage wrappers20043FF0/20044020). A grenade need not directly
 * cross its small authored brush: a genuine visible blast can damage it.
 * G_RadiusDamageEx retains its normal distance/falloff and CanDamage checks.
 * This bounded receiver does not turn the logical trigger into destructible
 * geometry; the original GSC waiter owns completion of each target. */
int G_Cod1DamageTriggerReceive(gentity_t *target,gentity_t *attacker,int damage,int flags,int mod){
    GscEntityRef ref,player;GscEventValue values[2];int wait,awakened=0,now=level.time;
    if(!G_Cod1EntityCapture(target,&ref)||!Cod1DamageTriggerSame(&ref)||target->spawnflags!=3)return 0;
    if(!G_Cod1GscTestGrenadeDamageActive()||damage<=0||!(flags&DAMAGE_RADIUS)||mod!=MOD_GRENADE_SPLASH||
       now<0||!level.clients||!attacker||!attacker->client||attacker->health<=0||!G_Cod1EntityCapture(attacker,&player)||
       player.slot>=(uint32_t)level.maxclients||attacker->client!=&level.clients[player.slot]||
       attacker->client->pers.connected!=CON_CONNECTED||now<cod1DamageTriggerDeadline[ref.slot])return 1;
    if(!isfinite(target->wait)||target->wait<0||target->wait>3600)return 1;
    wait=(int)ceil((double)target->wait*1000);if(now>INT_MAX-wait)return 1;
    if(now!=level.time||!G_Cod1EntitySameAllocation(&player)||!Cod1DamageTriggerSame(&ref)||!G_Cod1GscTestGrenadeDamageActive())return 1;
    cod1DamageTriggerDeadline[ref.slot]=now+wait;
    memset(values,0,sizeof(values));values[0].kind=GSC_EVENT_ENTITY;values[0].entity=player;
    values[1].kind=GSC_EVENT_NUMBER;values[1].number=(float)damage;
    G_Cod1GscTestEntityEventWithPayload(target,"trigger",values,2,&awakened);
    G_Printf("[R226] GRENADE_TARGET source=blast entity=%u generation=%u damage=%d awakened=%d\n",ref.slot,ref.generation,damage,awakened);
    return 1;
}
void G_Cod1BulletDamageTriggers(const vec3_t start,const vec3_t end,gentity_t *attacker,int damage) {
    GscEntityRef player,refs[MAX_GENTITIES];int i,j,count=0,awakened,now=level.time;uint64_t epoch;GscEventValue values[2];
    if(!G_Cod1GscTestCarbineActive()||damage<=0||!attacker||!attacker->client||attacker->health<=0||
       !G_Cod1EntityCapture(attacker,&player))return;epoch=player.epoch;
    for(j=0;j<3;j++)if(!isfinite(start[j])||!isfinite(end[j]))return;
    for(i=MAX_CLIENTS;i<level.num_entities&&i<MAX_GENTITIES;i++)if(Cod1DamageTriggerSame(&cod1DamageTriggerRefs[i]))refs[count++]=cod1DamageTriggerRefs[i];
    for(i=0;i<count;i++) {
        trace_t trace;gentity_t *e;int wait,model;vec3_t oldOrigin,oldAngles;
        if(epoch!=G_Cod1EntityMapEpoch()||now!=level.time||!G_Cod1GscTestCarbineActive()||!G_Cod1EntitySameAllocation(&player))return;
        if(!Cod1DamageTriggerSame(&refs[i])||g_entities[refs[i].slot].spawnflags||now<cod1DamageTriggerDeadline[refs[i].slot])continue;
        e=&g_entities[refs[i].slot];model=e->s.modelindex;VectorCopy(e->r.currentOrigin,oldOrigin);VectorCopy(e->r.currentAngles,oldAngles);
        trap_TraceUseEntity(&trace,start,end,(int)refs[i].slot);
        if(epoch!=G_Cod1EntityMapEpoch()||now!=level.time||!G_Cod1GscTestCarbineActive()||!G_Cod1EntitySameAllocation(&player))return;
        if(!Cod1DamageTriggerSame(&refs[i])||model!=g_entities[refs[i].slot].s.modelindex||
           !VectorCompare(oldOrigin,g_entities[refs[i].slot].r.currentOrigin)||!VectorCompare(oldAngles,g_entities[refs[i].slot].r.currentAngles)||
           !isfinite(trace.fraction)||trace.fraction<0.f||trace.fraction>1.f)return;
        if(!trace.startsolid&&!trace.allsolid&&trace.fraction==1.f)continue;
        e=&g_entities[refs[i].slot];if(!isfinite(e->wait)||e->wait<0.f||e->wait>3600.f)return;
        wait=(int)ceil((double)e->wait*1000.0);if(now>INT_MAX-wait)return;
        cod1DamageTriggerDeadline[refs[i].slot]=now+wait;
        memset(values,0,sizeof(values));values[0].kind=GSC_EVENT_ENTITY;values[0].entity=player;values[1].kind=GSC_EVENT_NUMBER;values[1].number=(float)damage;
        G_Cod1GscTestEntityEventWithPayload(e,"trigger",values,2,&awakened);
    }
}

/* Original missile segment producer 20024D3C -> 200442D0 checks real brush
 * contact. The selected training grenade targets have authored spawnflags3;
 * they are excluded from the ordinary bullet lane above. Other flag groups
 * and other projectile kinds remain outside this bounded port profile. */
int G_Cod1GrenadeDamageTriggers(const vec3_t start,const vec3_t end,gentity_t *missile){
 GscEntityRef shot,player,refs[MAX_GENTITIES];GscEventValue values[2];
 int i,j,count=0,now=level.time,awakened,owner;uint64_t epoch;
 if(!missile||!G_Cod1EntityCapture(missile,&shot))return 0;
 if(!G_Cod1GscTestGrenadeDamageActive()||missile->s.eType!=ET_MISSILE||
    missile->s.weapon!=WP_GRENADE_PINEAPPLE)return 1;
 if(missile->r.ownerNum<0||missile->r.ownerNum>=level.maxclients||
    !g_entities[missile->r.ownerNum].client||
    !G_Cod1EntityCapture(&g_entities[missile->r.ownerNum],&player))return 1;
 for(j=0;j<3;j++)if(!isfinite(start[j])||!isfinite(end[j]))return 1;
 epoch=shot.epoch;owner=missile->r.ownerNum;
 for(i=MAX_CLIENTS;i<level.num_entities&&i<MAX_GENTITIES;i++)
  if(Cod1DamageTriggerSame(&cod1DamageTriggerRefs[i])&&g_entities[i].spawnflags==3)refs[count++]=cod1DamageTriggerRefs[i];
 for(i=0;i<count;i++){
  trace_t trace;gentity_t *e;int wait,model;vec3_t origin,angles;
  if(epoch!=G_Cod1EntityMapEpoch()||now!=level.time||!G_Cod1EntitySameAllocation(&shot)||!G_Cod1EntitySameAllocation(&player)||!G_Cod1GscTestGrenadeDamageActive()||missile->r.ownerNum!=owner||missile->s.eType!=ET_MISSILE||missile->s.weapon!=WP_GRENADE_PINEAPPLE)return 0;
  if(!Cod1DamageTriggerSame(&refs[i])||g_entities[refs[i].slot].spawnflags!=3||now<cod1DamageTriggerDeadline[refs[i].slot])continue;
  e=&g_entities[refs[i].slot];model=e->s.modelindex;VectorCopy(e->r.currentOrigin,origin);VectorCopy(e->r.currentAngles,angles);
  trap_TraceUseEntity(&trace,start,end,(int)refs[i].slot);
  if(epoch!=G_Cod1EntityMapEpoch()||now!=level.time||!G_Cod1EntitySameAllocation(&shot)||!G_Cod1EntitySameAllocation(&player)||!G_Cod1GscTestGrenadeDamageActive()||missile->r.ownerNum!=owner||missile->s.eType!=ET_MISSILE||missile->s.weapon!=WP_GRENADE_PINEAPPLE)return 0;
  if(!Cod1DamageTriggerSame(&refs[i])||model!=g_entities[refs[i].slot].s.modelindex||!VectorCompare(origin,g_entities[refs[i].slot].r.currentOrigin)||!VectorCompare(angles,g_entities[refs[i].slot].r.currentAngles)||!isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1)continue;
  if(!trace.startsolid&&!trace.allsolid&&trace.fraction==1)continue;
  e=&g_entities[refs[i].slot];if(!isfinite(e->wait)||e->wait<0||e->wait>3600)return 1;
  wait=(int)ceil((double)e->wait*1000);if(now>INT_MAX-wait)return 1;
  cod1DamageTriggerDeadline[refs[i].slot]=now+wait;
  memset(values,0,sizeof(values));values[0].kind=GSC_EVENT_ENTITY;values[0].entity=player;
  values[1].kind=GSC_EVENT_NUMBER;values[1].number=1;
  G_Cod1GscTestEntityEventWithPayload(e,"trigger",values,2,&awakened);
 }
 return G_Cod1EntitySameAllocation(&shot);
}
