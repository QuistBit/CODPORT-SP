/* Actual live actor composition and current tree snapshot publication.
 * Config records are immutable for this map epoch; no hardcoded Avery model. */
#include "../bg_cod1_turret_actor_wire.h"
typedef struct {int used;GscEntityRef owner;entityState_t previous;int oldFlags,oldSvFlags;char hudInfo[256],placedHudName[128];int hudIndex,placedHudRead;} C1TActorPublished;
static C1TActorPublished cod1TurretPublished[C1T_CAPACITY];
static char cod1TurretActorConfigs[MAX_COD1_TURRET_ACTORS][1024];
static uint64_t cod1TurretActorConfigEpoch,cod1TurretActorConfigBusy;
static int TurretActorPublisherPrivate(const void*p,size_t n){return ActorUnmountedPublisherPrivate(p,n)||ActorOverlap(p,n,cod1TurretPublished,sizeof(cod1TurretPublished))||ActorOverlap(p,n,cod1TurretActorConfigs,sizeof(cod1TurretActorConfigs))||ActorOverlap(p,n,&cod1TurretActorConfigEpoch,sizeof(cod1TurretActorConfigEpoch))||ActorOverlap(p,n,&cod1TurretActorConfigBusy,sizeof(cod1TurretActorConfigBusy));}
static void TurretActorPublisherReset(void){ActorUnmountedPublisherReset();memset(cod1TurretPublished,0,sizeof(cod1TurretPublished));memset(cod1TurretActorConfigs,0,sizeof(cod1TurretActorConfigs));cod1TurretActorConfigEpoch=cod1TurretActorConfigBusy=0;}
static int TurretActorPublisherBegin(void){uint64_t epoch=cod1EntityEpoch;int i;
 if(epoch&&cod1TurretActorConfigEpoch==epoch)return 1;
 if(!epoch||!cod1EntityActive||!level.spawning||cod1TurretActorConfigBusy)return 0;
 cod1TurretActorConfigBusy=epoch;
 for(i=0;i<MAX_COD1_TURRET_ACTORS;i++){trap_SetConfigstring(CS_COD1_TURRET_ACTORS+i,"");if(epoch!=cod1EntityEpoch||!cod1EntityActive||!level.spawning)goto fail;}
 trap_SetConfigstring(CS_COD1_JUMPER,"");
 if(epoch!=cod1EntityEpoch||!cod1EntityActive||!level.spawning)goto fail;
 memset(cod1TurretActorConfigs,0,sizeof(cod1TurretActorConfigs));cod1TurretActorConfigEpoch=epoch;
 if(cod1TurretActorConfigBusy==epoch)cod1TurretActorConfigBusy=0;return 1;
 fail:if(cod1TurretActorConfigBusy==epoch)cod1TurretActorConfigBusy=0;return 0;
}
/* Share immutable display metadata between actors with the same original
 * name and weapon. Allocation and visibility still come from each snapshot. */
static void ActorHudPublish(entityState_t *wire,C1TActorPublished *owner,const cod1ActorLease_t *lease){
 const cod1ActorRecord_t *a=&cod1Actors[lease->actorIndex];
 const char *name=a->scriptStrings.namePresent?a->scriptStrings.name:"";
 const char *weapon=a->scriptStrings.weaponPresent?a->scriptStrings.weapon:"";
 char info[256],actual[256];int i,n;
 wire->otherEntityNum2=0;
 if(cod1Sentients[lease->sentientIndex].defaults.team!=COD1_ACTOR_TEAM_ALLIES)return;
 wire->otherEntityNum2=COD1_ALLY_PRESENT | (a->nativeData.actor.drawoncompass?COD1_ALLY_COMPASS:0);
 if(!*name && !owner->placedHudRead){
    cod1PlacedActorSnapshot_t placed;
    if(G_Cod1PlacedActorRead(&lease->entity,&placed,NULL,0)){
        unsigned field;
        for(field=0;field<placed.rawFieldCount;field++){
            char key[MAX_SPAWN_VARS_CHARS],value[MAX_SPAWN_VARS_CHARS];
            if(!G_Cod1PlacedActorRawField(&lease->entity,field,key,sizeof(key),value,sizeof(value),NULL,0))return;
            if(!Q_stricmp(key,"script_friendname")){
                if(strlen(value)<sizeof(owner->placedHudName))strcpy(owner->placedHudName,value);
                break;
            }
        }
        owner->placedHudRead=1;
    }
 }
 if(!*name)name=owner->placedHudName;
 if(!name[0] || strchr(name,'|') || strchr(weapon,'|'))return;
 n=snprintf(info,sizeof(info),"%s|%s",name,weapon);if(n<0||n>=sizeof(info))return;
 if(strcmp(info,owner->hudInfo)){
    for(i=0;i<COD1_ALLY_INFO_COUNT;i++){
        trap_GetConfigstring(CS_COD1_ALLY_INFO+i,actual,sizeof(actual));
        if(!ActorLeaseValid(lease))return;
        if(actual[0]&&strcmp(actual,info))continue;
        if(!actual[0]){
            trap_SetConfigstring(CS_COD1_ALLY_INFO+i,info);
            if(!ActorLeaseValid(lease))return;
            trap_GetConfigstring(CS_COD1_ALLY_INFO+i,actual,sizeof(actual));
            if(!ActorLeaseValid(lease)||strcmp(actual,info))return;
        }
        strcpy(owner->hudInfo,info);owner->hudIndex=i+1;break;
    }
 }
 if(!strcmp(info,owner->hudInfo))wire->otherEntityNum2|=owner->hudIndex;
}
static void TurretActorPublisherRestore(entityState_t*s,const entityState_t*p){
 s->otherEntityNum2=p->otherEntityNum2;
 s->eType=p->eType;s->eFlags=(s->eFlags&~EF_NODRAW)|(p->eFlags&EF_NODRAW);
 s->cod1TurretVersion=p->cod1TurretVersion;s->cod1TurretEpochLo=p->cod1TurretEpochLo;s->cod1TurretEpochHi=p->cod1TurretEpochHi;
 s->pos.trType=p->pos.trType;s->apos.trType=p->apos.trType;
 s->cod1TurretActorVersion=p->cod1TurretActorVersion;s->cod1TurretActorGeneration=p->cod1TurretActorGeneration;
 s->cod1TurretActorGun=p->cod1TurretActorGun;s->cod1TurretActorGunGeneration=p->cod1TurretActorGunGeneration;
 s->cod1TurretActorTick=p->cod1TurretActorTick;s->cod1TurretActorConfig=p->cod1TurretActorConfig;s->cod1TurretActorFlags=p->cod1TurretActorFlags;
 memcpy(s->cod1TurretActorCurrent,p->cod1TurretActorCurrent,sizeof(s->cod1TurretActorCurrent));
 memcpy(s->cod1TurretActorPhases,p->cod1TurretActorPhases,sizeof(s->cod1TurretActorPhases));
 memcpy(s->cod1TurretActorGenericCurrent,p->cod1TurretActorGenericCurrent,sizeof(s->cod1TurretActorGenericCurrent));
}
static void TurretActorPublisherRetire(const GscEntityRef*actor){int i;gentity_t*e;C1TActorPublished old;
 if(!actor)return;ActorUnmountedPublisherRetire(actor);for(i=0;i<C1T_CAPACITY;i++)if(cod1TurretPublished[i].used&&ActorRefEqual(actor,&cod1TurretPublished[i].owner)){
  old=cod1TurretPublished[i];memset(&cod1TurretPublished[i],0,sizeof(old));if(!G_Cod1EntitySameAllocation(actor))continue;e=&g_entities[actor->slot];TurretActorPublisherRestore(&e->s,&old.previous);e->flags=(e->flags&~FL_NODRAW)|(old.oldFlags&FL_NODRAW);e->r.svFlags=(e->r.svFlags&~SVF_NOCLIENT)|(old.oldSvFlags&SVF_NOCLIENT);
  /* Pool retirement is pure: pose/bounds/contents are unchanged, and snapshot
   * visibility reads live svFlags. Never introduce a callback before its wipe. */
 }
}
static int TurretActorCompositionText(const cod1ActorModelComposition_t*c,char out[1024]){unsigned i;size_t at=6;cod1TurretActorResourceState_t check;
 if(!c||c->descriptorCount<1||c->descriptorCount>8)return 0;memset(out,0,1024);memcpy(out,"c1ta1|",6);
 for(i=0;i<c->descriptorCount;i++){const char*model=c->descriptors[i].model,*tag=c->descriptors[i].tag;size_t n,t;
  if(!memchr(model,0,128)||!memchr(tag,0,128)||strncmp(model,"xmodel/",7))return 0;model+=7;n=strlen(model);t=strlen(tag);
  if(!n||n>=64||t>=64||at+n+1+t+(i+1<c->descriptorCount?1:0)>=1024)return 0;
  memcpy(out+at,model,n);at+=n;out[at++]='@';memcpy(out+at,tag,t);at+=t;if(i+1<c->descriptorCount)out[at++]='|';
 }return C1TAW_Composition(out,&check);
}
typedef struct {C1TRecord*record;C1TRecord gun;GscEntityRef actor;cod1ActorLease_t lease;C1TARecord animation;cod1ActorModelComposition_t composition;entityState_t entity;int flags,svFlags,now;int(*current)(void*);void*context;} C1TActorPublishCall;
static int TurretActorPublishCurrent(C1TActorPublishCall*c){GscEntityRef actor;gentity_t*e;cod1ActorRecord_t*a;
 if(!c||!ActorLeaseValid(&c->lease)||!cod1EntityReady||level.spawning||level.restarted||c->now!=level.time||cod1TurretActorConfigEpoch!=c->actor.epoch||
    !TurretMountOwnerCurrent(c->record,c->now,&actor)||!ActorRefEqual(&actor,&c->actor)||memcmp(c->record,&c->gun,sizeof(c->gun)))return 0;
 a=&cod1Actors[c->lease.actorIndex];e=&g_entities[c->actor.slot];
 return !memcmp(&cod1TurretAnimations[c->record-cod1Turrets],&c->animation,sizeof(c->animation))&&!memcmp(&a->modelComposition,&c->composition,sizeof(c->composition))&&
  a->dobj.present&&!a->dobj.busy&&a->dobj.summary.skippedCount==0&&a->dobj.summary.modelCount==c->composition.descriptorCount&&
  !memcmp(&e->s,&c->entity,sizeof(c->entity))&&e->flags==c->flags&&e->r.svFlags==c->svFlags;
}
static int TurretActorPublishFence(C1TActorPublishCall*c){return TurretActorPublishCurrent(c)&&c->current(c->context)==1&&TurretActorPublishCurrent(c);}
static int TurretActorPublish(const GscEntityRef*gun,int now,int(*current)(void*),void*context){
 C1TActorPublishCall c;C1TRecord*r;C1TActorPublished saved,next;entityState_t wire;gentity_t*e;char text[1024],actual[1024];int index,slot=-1,i,g;
 if(!gun||!current||now!=level.time||(r=TurretFind(gun))==NULL||!r->animated||r->poseTick!=now||cod1TurretActorConfigBusy)return 0;
 memset(&c,0,sizeof(c));c.record=r;c.gun=*r;c.now=now;c.current=current;c.context=context;
 if(!TurretMountOwnerCurrent(r,now,&c.actor)||!TurretMountLease(&c.actor,&c.lease))return 0;
 ActorUnmountedPublisherRetire(&c.actor);
 index=(int)(r-cod1Turrets);c.animation=cod1TurretAnimations[index];c.composition=cod1Actors[c.lease.actorIndex].modelComposition;e=&g_entities[c.actor.slot];c.entity=e->s;c.flags=e->flags;c.svFlags=e->r.svFlags;
 if(!TurretAnimationLive(&c.animation,r,now)||!TurretActorPublishFence(&c)||!TurretActorCompositionText(&c.composition,text))return 0;
 for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!strcmp(cod1TurretActorConfigs[i],text)){slot=i;break;}
 if(slot<0){for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!cod1TurretActorConfigs[i][0]){slot=i;break;}}
 if(slot<0)return 0;
 trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!TurretActorPublishFence(&c))return 0;
 if(cod1TurretActorConfigs[slot][0]){if(strcmp(actual,text))return 0;}
 else{
  /* A previous SetConfigstring may have committed before its fence refused.
   * Reconcile only identical immutable content; never overwrite foreign data. */
  if(actual[0]&&strcmp(actual,text))return 0;cod1TurretActorConfigBusy=c.actor.epoch;if(!actual[0])trap_SetConfigstring(CS_COD1_TURRET_ACTORS+slot,text);
  if(!TurretActorPublishFence(&c))goto failedConfig;
  trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!TurretActorPublishFence(&c)||strcmp(actual,text))goto failedConfig;
  strcpy(cod1TurretActorConfigs[slot],text);if(cod1TurretActorConfigBusy==c.actor.epoch)cod1TurretActorConfigBusy=0;
 }
 saved=next=cod1TurretPublished[index];if(next.used&&!ActorRefEqual(&next.owner,&c.actor))return 0;
 if(!next.used){next.used=1;next.owner=c.actor;next.previous=c.entity;next.oldFlags=c.flags;next.oldSvFlags=c.svFlags;}
 wire=c.entity;wire.eType=ET_GENERAL;wire.cod1TurretVersion=0;wire.cod1TurretEpochLo=(int)(uint32_t)c.actor.epoch;wire.cod1TurretEpochHi=(int)(uint32_t)(c.actor.epoch>>32);
 wire.cod1TurretActorVersion=COD1_TURRET_ACTOR_WIRE_VERSION;wire.cod1TurretActorGeneration=c.actor.generation;wire.cod1TurretActorGun=gun->slot;wire.cod1TurretActorGunGeneration=gun->generation;
 wire.cod1TurretActorTick=now;wire.cod1TurretActorConfig=CS_COD1_TURRET_ACTORS+slot;wire.cod1TurretActorFlags=1;wire.pos.trType=wire.apos.trType=TR_STATIONARY;
 memset(wire.cod1TurretActorGenericCurrent,0,sizeof(wire.cod1TurretActorGenericCurrent));
 for(g=0;g<2;g++){for(i=0;i<25;i++)wire.cod1TurretActorCurrent[g][i]=c.animation.actorTree[g].node[i].currentWeight;for(i=0;i<21;i++)wire.cod1TurretActorPhases[g][i]=c.animation.actorTree[g].node[4+i].phase;}
 {
  int generic=G_Cod1ActorGenericPresent(&c.actor);
  if(generic<0)return 0;
  if(generic==1){
   if(!G_Cod1ActorGenericSample(&c.actor,now,wire.cod1TurretActorGenericCurrent,current,context)||!TurretActorPublishFence(&c))return 0;
   wire.cod1TurretActorVersion=COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION;
   memset(wire.cod1TurretActorCurrent,0,sizeof(wire.cod1TurretActorCurrent));
  }
 }
 ActorHudPublish(&wire,&next,&c.lease);
 if(!C1TAW_Valid(&wire,now)||!TurretActorPublishFence(&c)||memcmp(&cod1TurretPublished[index],&saved,sizeof(saved)))return 0;
 wire.eFlags&=~EF_NODRAW;e->s=wire;e->flags&=~FL_NODRAW;if(!cod1Actors[c.lease.actorIndex].motion.active || cod1Actors[c.lease.actorIndex].motion.model)
   e->r.svFlags&=~SVF_NOCLIENT;
 cod1TurretPublished[index]=next;
 c.entity=wire;c.flags=e->flags;c.svFlags=e->r.svFlags;
 if(!(e->r.svFlags&SVF_NOCLIENT))trap_LinkEntity(e);
 return TurretActorPublishFence(&c)&&!memcmp(&cod1TurretPublished[index],&next,sizeof(next));
 failedConfig:if(cod1TurretActorConfigBusy==c.actor.epoch)cod1TurretActorConfigBusy=0;return 0;
}
