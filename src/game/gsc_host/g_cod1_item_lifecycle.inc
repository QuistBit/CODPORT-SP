/* R216 selected, genuinely linked port grenade items. Raw suspended ground0
 * stays0. ET_ITEM is an explicit port representation, not an R145 packet. */
#include "../g_cod1_item_lifecycle.h"
#include "../g_cod1_path_entity_host.h"

typedef struct {
    GscEntityRef entity;
    cod1ItemDescriptor_t descriptor;
    cod1ModelResourceReference_t resource;
    uintptr_t itemRow;
    uint64_t serial;
    int cap,wp,itemIndex,modelSlot,flags,count,ground,consumed;
} Cod1LiveItem;
static struct {
    uint64_t epoch,serial;
    int selected;
    cod1AmmoRegistryReceipt_t registry;
    cod1ItemLifecycleServices_t services;
    Cod1LiveItem rows[MAX_GENTITIES];
} cod1LiveItems;
static struct {
    int active,cancelled,checking,hasItem,hasPlayer;
    uint64_t serial,epoch;
    int time,count,maxclients;
    int spawnCount,health,playerHealth,pmFlags,team,noclip,autoActivate;
    gclient_t *playerClient;
    gentity_t *entities;
    gclient_t *clients;
    GscEntityRef item,player;
    const GscEntityRef *sourceItem,*sourcePlayer;
    const float *sourceVector;
    float sourceVectorCopy[3];
    const char *sourceClass;
    char sourceClassCopy[64];size_t sourceClassBytes;
    Cod1LiveItem row;
    gentity_t body;
    cod1ItemLifecycleServices_t services;
    cod1WeaponRegistrationReceipt_t registration;
} cod1LiveItemGuard;
static uint64_t cod1LiveItemSerial;
typedef struct {GscEntityRef ref;int attempted,installed,retired;Cod1LiveItem row;gentity_t before,body;} Cod1PreparedItem;
static struct {
 cod1ItemPreparedReceipt_t receipt;int active,closed,executing,aborting,cancelled,operation,freeActive,freeIndex;
 Cod1PreparedItem items[5];
} cod1PreparedItems;
static uint64_t cod1PreparedItemSerial;
static struct {int busy,cancelled;} cod1PreparedBegin;
static int ItemPreparedAttempt(const GscEntityRef*);
static int ItemPreparedInstall(const GscEntityRef*,const Cod1LiveItem*,const gentity_t*);
static int ItemPreparedInstalledCurrent(const GscEntityRef*);
static int ItemPreparedMutation(void){return !cod1PreparedItems.active||(!cod1PreparedItems.closed&&cod1PreparedItems.executing);}

void G_Cod1ItemLifecycleTouch(gentity_t *,gentity_t *,trace_t *);
void G_Cod1ItemLifecycleUse(gentity_t *,gentity_t *,gentity_t *);

int G_Cod1ItemLifecycleStorageProtected(const void *p,size_t n) {
    return !AmmoInventoryRange(p,n) || CAR_StorageProtected(p,n) ||
        CAR_Overlap(p,n,&cod1PreparedItems,sizeof(cod1PreparedItems)) ||
        CAR_Overlap(p,n,&cod1PreparedItemSerial,sizeof(cod1PreparedItemSerial)) ||
        CAR_Overlap(p,n,&cod1PreparedBegin,sizeof(cod1PreparedBegin)) ||
        CAR_Overlap(p,n,&cod1LiveItems,sizeof(cod1LiveItems)) ||
        CAR_Overlap(p,n,&cod1LiveItemGuard,sizeof(cod1LiveItemGuard)) ||
        CAR_Overlap(p,n,&cod1LiveItemSerial,sizeof(cod1LiveItemSerial));
}
static int LiveItemPrivate(const void *p,size_t n) {
    return G_Cod1ItemLifecycleStorageProtected(p,n) || AmmoInventoryPrivate(p,n);
}
static int LiveItemBuffers(const void *in,size_t inSize,const void *out,size_t outSize,
    char *error,size_t size) {
    if(!AmmoInventoryPoolSafe() || LiveItemPrivate(in,inSize) ||
       LiveItemPrivate(out,outSize) || LiveItemPrivate(error,size) ||
       CAR_Overlap(in,inSize,out,outSize) || CAR_Overlap(in,inSize,error,size) ||
       CAR_Overlap(out,outSize,error,size))return 0;
    return 1;
}
static void LiveItemError(char *error,size_t size,const char *why) {
    if(AmmoInventoryPoolSafe() && !LiveItemPrivate(error,size))EntityError(error,size,why);
}
static int LiveItemFinite(const float v[3]) {
    uint32_t bits;int i;if(!v)return 0;
    for(i=0;i<3;++i){memcpy(&bits,v+i,4);if((bits&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return 0;}
    return 1;
}
static int LiveItemMap(void) {
    return cod1EntityReady && ActorMapAvailable() && !level.spawning &&
        !level.restarted && cod1EntityEpoch!=UINT64_MAX && level.time>=0 &&
        level.gentities==g_entities && level.num_entities<=ENTITYNUM_MAX_NORMAL &&
        AmmoInventoryPoolSafe();
}
static int LiveItemRefEqual(const GscEntityRef *a,const GscEntityRef *b) {
    return a&&b&&a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int LiveItemOriginalOwner(const GscEntityRef *ref,const Cod1LiveItem *row,int consumed) {
    const gentity_t *e;
    if(!G_Cod1EntitySameAllocation(ref)||!ActorOwnerAllowed(ref,0))return 0;
    e=&g_entities[ref->slot];
    if(e->physicsObject||e->r.bmodel||e->neverFree||e->s.eType!=ET_ITEM||
       (uintptr_t)e->item!=row->itemRow||!e->classname||
       strcmp(e->classname,row->descriptor.classname))return 0;
    if(!consumed)return e->touch==G_Cod1ItemLifecycleTouch&&e->use==G_Cod1ItemLifecycleUse;
    return !e->touch&&!e->use&&!e->think&&!e->nextthink&&!e->r.contents&&!e->r.linked&&
        (e->r.svFlags&SVF_NOCLIENT)&&(e->flags&FL_NODRAW);
}
int G_Cod1ItemLifecycleOwns(const GscEntityRef *ref) {
    return ref && ref->slot<MAX_GENTITIES && ref->epoch && ref->generation &&
        LiveItemRefEqual(ref,&cod1LiveItems.rows[ref->slot].entity);
}
static int LiveItemBodyOwner(const GscEntityRef *ref) {
    return LiveItemMap() && G_Cod1ItemLifecycleOwns(ref) &&
        G_Cod1EntitySameAllocation(ref) && cod1LiveItems.epoch==ref->epoch &&
        cod1LiveItems.selected;
}
static int LiveItemBodyFields(const GscEntityRef *ref) {
    Cod1LiveItem *r;gentity_t *e;cod1ItemDescriptor_t d;
    r=&cod1LiveItems.rows[ref->slot];e=&g_entities[ref->slot];
    return !r->consumed && r->serial && r->itemIndex>0 && r->itemIndex<MAX_ITEMS && r->itemRow &&
        !e->client && !e->aiCharacter && !e->aiName && !e->aiAttributes &&
        !e->AIScript_AlertEntity && !e->aiInactive && !e->physicsObject &&
        !cod1ActorByEntity[ref->slot] && !cod1SentientByEntity[ref->slot] &&
        !AmmoCandidateOwned(ref) && !e->r.bmodel && !e->neverFree &&
        !(e->r.svFlags&(SVF_CASTAI|SVF_BOT|SVF_NOCLIENT)) &&
        e->r.linked && e->r.contents==(CONTENTS_TRIGGER|CONTENTS_ITEM) &&
        !(e->flags&(FL_NODRAW|FL_DROPPED_ITEM)) && !(e->s.eFlags&EF_NODRAW) &&
        e->s.eType==ET_ITEM && e->s.modelindex==r->itemIndex &&
        e->s.modelindex2==r->modelSlot && (uintptr_t)e->item==r->itemRow &&
        e->touch==G_Cod1ItemLifecycleTouch && e->use==G_Cod1ItemLifecycleUse && !e->think && !e->nextthink &&
        e->count==r->count && e->spawnflags==r->flags && e->classname &&
        !strcmp(e->classname,r->descriptor.classname) &&
        e->s.pos.trType==TR_STATIONARY && !e->s.pos.trTime && !e->s.pos.trDuration &&
        !memcmp(e->s.pos.trDelta,vec3_origin,sizeof(vec3_t)) &&
        e->s.apos.trType==TR_STATIONARY && !e->s.apos.trTime && !e->s.apos.trDuration &&
        !memcmp(e->s.apos.trDelta,vec3_origin,sizeof(vec3_t)) &&
        !memcmp(e->s.pos.trBase,e->r.currentOrigin,sizeof(vec3_t)) &&
        !memcmp(e->s.origin,e->r.currentOrigin,sizeof(vec3_t)) &&
        !memcmp(e->s.apos.trBase,e->r.currentAngles,sizeof(vec3_t)) &&
        !memcmp(e->s.angles,e->r.currentAngles,sizeof(vec3_t)) &&
        LiveItemFinite(e->r.currentOrigin) && LiveItemFinite(e->r.currentAngles) &&
        e->s.groundEntityNum==r->ground && r->ground!=-1 &&
        e->r.mins[0]==-1 && e->r.mins[1]==-1 &&
        e->r.mins[2]==(r->descriptor.type==1?-1:0) &&
        e->r.maxs[0]==1 && e->r.maxs[1]==1 &&
        e->r.maxs[2]==(r->descriptor.type==1?1:2) &&
        CI_Descriptor(r->descriptor.index,&d) && !memcmp(&d,&r->descriptor,sizeof(d));
}
static int LiveItemBody(const GscEntityRef *ref) {
    return LiveItemBodyOwner(ref) && CAR_Current(&cod1LiveItems.registry) && LiveItemBodyFields(ref);
}
/* Only the immediate callers that already checked this registry use this path.
 * They read fields and compare bytes between the check and this call. */
static int LiveItemBodyWithCurrentRegistry(const GscEntityRef *ref) {
    return LiveItemBodyOwner(ref) && LiveItemBodyFields(ref);
}
static int LiveItemScope(void) {
    return cod1LiveItemGuard.active && !cod1LiveItemGuard.cancelled &&
        LiveItemMap() && cod1LiveItems.selected &&
        cod1LiveItemGuard.epoch==cod1EntityEpoch && cod1LiveItems.epoch==cod1EntityEpoch &&
        cod1LiveItemGuard.serial==cod1LiveItems.serial &&
        level.time==cod1LiveItemGuard.time && level.num_entities==cod1LiveItemGuard.count &&
        level.gentities==cod1LiveItemGuard.entities && level.clients==cod1LiveItemGuard.clients &&
        level.maxclients==cod1LiveItemGuard.maxclients &&
        !memcmp(&cod1LiveItems.services,&cod1LiveItemGuard.services,sizeof(cod1LiveItems.services)) &&
        CAR_Current(&cod1LiveItems.registry) &&
        (!cod1LiveItemGuard.sourceItem || !memcmp(cod1LiveItemGuard.sourceItem,&cod1LiveItemGuard.item,sizeof(GscEntityRef))) &&
        (!cod1LiveItemGuard.sourcePlayer || !memcmp(cod1LiveItemGuard.sourcePlayer,&cod1LiveItemGuard.player,sizeof(GscEntityRef))) &&
        (!cod1LiveItemGuard.sourceVector || !memcmp(cod1LiveItemGuard.sourceVector,cod1LiveItemGuard.sourceVectorCopy,sizeof(vec3_t))) &&
        (!cod1LiveItemGuard.sourceClass || !memcmp(cod1LiveItemGuard.sourceClass,cod1LiveItemGuard.sourceClassCopy,cod1LiveItemGuard.sourceClassBytes)) &&
        (!cod1LiveItemGuard.hasItem || (LiveItemBodyWithCurrentRegistry(&cod1LiveItemGuard.item) &&
            !memcmp(&cod1LiveItemGuard.row,&cod1LiveItems.rows[cod1LiveItemGuard.item.slot],sizeof(Cod1LiveItem)) &&
            !memcmp(&cod1LiveItemGuard.body,&g_entities[cod1LiveItemGuard.item.slot],sizeof(gentity_t)))) &&
        (!cod1LiveItemGuard.hasPlayer || (PlayerNativePlayer(&cod1LiveItemGuard.player) &&
            g_entities[cod1LiveItemGuard.player.slot].client==cod1LiveItemGuard.playerClient &&
            g_entities[cod1LiveItemGuard.player.slot].health==cod1LiveItemGuard.health &&
            cod1LiveItemGuard.playerClient->ps.persistant[PERS_SPAWN_COUNT]==cod1LiveItemGuard.spawnCount &&
            cod1LiveItemGuard.playerClient->ps.stats[STAT_HEALTH]==cod1LiveItemGuard.playerHealth &&
            cod1LiveItemGuard.playerClient->ps.pm_flags==cod1LiveItemGuard.pmFlags &&
            cod1LiveItemGuard.playerClient->sess.sessionTeam==cod1LiveItemGuard.team &&
            cod1LiveItemGuard.playerClient->noclip==cod1LiveItemGuard.noclip &&
            cod1LiveItemGuard.playerClient->pers.autoActivate==cod1LiveItemGuard.autoActivate));
}
static int LiveItemCurrent(void *context) {
    int ok;
    if(context!=&cod1LiveItemGuard || !LiveItemScope())return 0;
    if(cod1LiveItemGuard.checking){cod1LiveItemGuard.cancelled=1;return 0;}
    cod1LiveItemGuard.checking=1;
    ok=cod1LiveItemGuard.services.current(cod1LiveItemGuard.services.user)==1;
    cod1LiveItemGuard.checking=0;
    return ok&&LiveItemScope();
}
static int LiveItemBegin(const GscEntityRef *item,const GscEntityRef *player) {
    if(cod1PreparedItems.active&&cod1PreparedItems.closed)return 0;
    if(cod1LiveItemGuard.active){cod1LiveItemGuard.cancelled=1;return 0;}
    if(!LiveItemMap() || !cod1LiveItems.selected || cod1LiveItems.epoch!=cod1EntityEpoch ||
       !CAR_Current(&cod1LiveItems.registry) || (item&&!LiveItemBodyWithCurrentRegistry(item)) ||
       (player&&!PlayerNativePlayer(player)))return 0;
    memset(&cod1LiveItemGuard,0,sizeof(cod1LiveItemGuard));
    cod1LiveItemGuard.active=1;cod1LiveItemGuard.epoch=cod1EntityEpoch;
    cod1LiveItemGuard.serial=cod1LiveItems.serial;cod1LiveItemGuard.time=level.time;
    cod1LiveItemGuard.count=level.num_entities;cod1LiveItemGuard.entities=level.gentities;
    cod1LiveItemGuard.clients=level.clients;cod1LiveItemGuard.maxclients=level.maxclients;
    cod1LiveItemGuard.services=cod1LiveItems.services;
    if(item){cod1LiveItemGuard.hasItem=1;cod1LiveItemGuard.item=*item;
        cod1LiveItemGuard.row=cod1LiveItems.rows[item->slot];cod1LiveItemGuard.body=g_entities[item->slot];}
    if(player){gclient_t *client=g_entities[player->slot].client;
        cod1LiveItemGuard.hasPlayer=1;cod1LiveItemGuard.player=*player;
        cod1LiveItemGuard.playerClient=client;cod1LiveItemGuard.health=g_entities[player->slot].health;
        cod1LiveItemGuard.spawnCount=client->ps.persistant[PERS_SPAWN_COUNT];
        cod1LiveItemGuard.playerHealth=client->ps.stats[STAT_HEALTH];
        cod1LiveItemGuard.pmFlags=client->ps.pm_flags;cod1LiveItemGuard.team=client->sess.sessionTeam;
        cod1LiveItemGuard.noclip=client->noclip;cod1LiveItemGuard.autoActivate=client->pers.autoActivate;}
    return 1;
}
static void LiveItemEnd(void){cod1LiveItemGuard.active=0;cod1LiveItemGuard.checking=0;}
void G_Cod1ItemLifecycleReset(void) {
    if(cod1PreparedBegin.busy)cod1PreparedBegin.cancelled=1;
    if(cod1PreparedItems.active)cod1PreparedItems.closed=1;
    if(cod1LiveItemGuard.active)cod1LiveItemGuard.cancelled=1;
    memset(&cod1LiveItems,0,sizeof(cod1LiveItems));
}
int G_Cod1ItemLifecycleSelect(const cod1AmmoRegistryReceipt_t *input,
    const cod1ItemLifecycleServices_t *services,char *error,size_t size) {
    cod1AmmoRegistryReceipt_t registry;cod1ItemLifecycleServices_t copy;
    uint64_t epoch;int now,count,maxclients,ok=0;gclient_t *clients;gentity_t *entities;
    if(!LiveItemBuffers(input,sizeof(*input),NULL,0,error,size) ||
       !LiveItemBuffers(services,sizeof(*services),NULL,0,error,size))return 0;
    if(cod1LiveItemGuard.active){cod1LiveItemGuard.cancelled=1;return 0;}
    if(cod1PreparedBegin.busy){cod1PreparedBegin.cancelled=1;goto done;}
    if(!input||!services||cod1PreparedItems.active||!LiveItemMap()||cod1LiveItems.selected||
       !services->current||!services->interaction_item||!services->floor_trace||!services->read_registration||!services->acquire||
       !services->pickup_notify||!CAR_Current(input)||cod1LiveItemSerial==UINT64_MAX)goto done;
    registry=*input;copy=*services;epoch=cod1EntityEpoch;now=level.time;
    clients=level.clients;entities=level.gentities;count=level.num_entities;maxclients=level.maxclients;
    cod1LiveItemGuard.active=1;cod1LiveItemGuard.cancelled=0;
    if(copy.current(copy.user)!=1||cod1LiveItemGuard.cancelled||!LiveItemMap()||
       epoch!=cod1EntityEpoch||now!=level.time||clients!=level.clients||entities!=level.gentities||
       count!=level.num_entities||maxclients!=level.maxclients||
       !LiveItemBuffers(input,sizeof(*input),NULL,0,error,size)||
       !LiveItemBuffers(services,sizeof(*services),NULL,0,error,size)||memcmp(input,&registry,sizeof(registry))||
       memcmp(services,&copy,sizeof(copy))||!CAR_Current(&registry))goto done;
    memset(&cod1LiveItems,0,sizeof(cod1LiveItems));
    cod1LiveItems.registry=registry;cod1LiveItems.services=copy;
    cod1LiveItems.epoch=epoch;cod1LiveItems.serial=++cod1LiveItemSerial;
    cod1LiveItems.selected=1;ok=1;
done:LiveItemEnd();if(!ok)LiveItemError(error,size,"R216 item selection requires current genuine registration services");return ok;
}
static int LiveItemClass(const char *name,Cod1LiveItem *out) {
    Cod1LiveItem r;cod1ItemDescriptor_t d;int cap=0,wp=0,i,index=0;
    if(!name||!out||!CAR_Current(&cod1LiveItems.registry))return 0;
    if(!strcmp(name,"weapon_fraggrenade"))wp=WP_GRENADE_PINEAPPLE;
    else if(!strcmp(name,"weapon_stielhandgranate"))wp=WP_GRENADE_LAUNCHER;
    else if(!strcmp(name,"weapon_mk1britishfrag"))wp=WP_MK1;
    else if(!strcmp(name,"weapon_rgd-33russianfrag"))wp=WP_RGD33;
    else if(!strcmp(name,"weapon_panzerfaust"))wp=WP_PANZERFAUST;
    else if(!strcmp(name,"item_ammo_stielhandgranate_open"))index=65;
    else if(!strcmp(name,"item_ammo_stielhandgranate_closed"))index=66;
    else return 0;
    if(index){for(i=0;i<COD1_AMMO_REGISTRY_BINDINGS;++i)
        if(cod1LiveItems.registry.bindings[i].itemIndex==index)cap=cod1LiveItems.registry.bindings[i].weaponIndex;
        wp=WP_GRENADE_LAUNCHER;
    }else{for(i=1;i<=cod1LiveItems.registry.weaponCount;++i)
        if(cod1LiveItems.registry.weapons[i].familyRoot==CW_WeaponRoot(wp))cap=i;index=cap;}
    if(cap<1||cap>cod1LiveItems.registry.weaponCount||!CI_Descriptor(index,&d)||
       strcmp(d.classname,name)||!d.model_present[0]||!d.models[0][0]||
       !cod1LiveItems.registry.weapons[cap].pickup.clipOnly||
       cod1LiveItems.registry.weapons[cap].familyRoot!=CW_WeaponRoot(wp))return 0;
    memset(&r,0,sizeof(r));r.cap=cap;r.wp=wp;r.descriptor=d;
    r.count=d.quantity>0?d.quantity:1;*out=r;return 1;
}
static int LiveItemResource(Cod1LiveItem *row) {
    cod1ModelPrecache_t precache;cod1ModelResourceReference_t query,result;
    char name[MAX_STRING_CHARS];int i,empty=0,slot;
    if(!LiveItemCurrent(&cod1LiveItemGuard)||
       cod1LiveItemGuard.services.interaction_item(cod1LiveItemGuard.services.user,row->wp,
            &row->itemIndex,&row->itemRow)!=1 || !LiveItemCurrent(&cod1LiveItemGuard)||
       row->itemIndex<1||row->itemIndex>=MAX_ITEMS||!row->itemRow)return 0;
    memset(&precache,0,sizeof(precache));precache.version=COD1_PRECACHE_VERSION;precache.bytes=sizeof(precache);
    Q_strncpyz(precache.name,row->descriptor.models[0],sizeof(precache.name));
    if(!LiveItemCurrent(&cod1LiveItemGuard))return 0;
    slot=trap_Cod1ModelPrecache(&precache);
    if(slot<=0||!LiveItemCurrent(&cod1LiveItemGuard)||
       precache.version!=COD1_PRECACHE_VERSION||precache.bytes!=sizeof(precache)||
       strcmp(precache.name,row->descriptor.models[0]))return 0;
    memset(&query,0,sizeof(query));query.version=COD1_MODEL_RESOURCE_REFERENCE_VERSION;
    query.bytes=sizeof(query);query.operation=COD1_MODEL_RESOURCE_QUERY;query.slot=(uint32_t)slot;
    Q_strncpyz(query.name,precache.name,sizeof(query.name));
    if(trap_Cod1ModelResourceReference(&query,&result)!=1||!LiveItemCurrent(&cod1LiveItemGuard)||
       query.version!=COD1_MODEL_RESOURCE_REFERENCE_VERSION||query.bytes!=sizeof(query)||
       query.operation!=COD1_MODEL_RESOURCE_QUERY||query.slot!=(uint32_t)slot||
       query.scene||query.serverId||query.reserved||strcmp(query.name,precache.name)||
       result.version!=COD1_MODEL_RESOURCE_REFERENCE_VERSION||result.bytes!=sizeof(result)||
       result.operation!=COD1_MODEL_RESOURCE_QUERY||!result.scene||result.reserved||
       result.slot!=(uint32_t)slot||strcmp(result.name,precache.name))return 0;
    row->resource=result;row->resource.operation=COD1_MODEL_RESOURCE_VALIDATE;
    for(i=1;i<MAX_MODELS;++i){trap_GetConfigstring(CS_MODELS+i,name,sizeof(name));
        if(!LiveItemCurrent(&cod1LiveItemGuard))return 0;
        if(!name[0]){if(!empty)empty=i;continue;}
        if(!strcmp(name,precache.name)){row->modelSlot=i;return 1;}}
    if(!empty)return 0;
    trap_SetConfigstring(CS_MODELS+empty,precache.name);
    if(!LiveItemCurrent(&cod1LiveItemGuard))return 0;
    trap_GetConfigstring(CS_MODELS+empty,name,sizeof(name));
    if(!LiveItemCurrent(&cod1LiveItemGuard)||strcmp(name,precache.name))return 0;
    row->modelSlot=empty;return 1;
}
static int LiveItemResourceCurrent(const Cod1LiveItem *row) {
    cod1ModelResourceReference_t query=row->resource,expected,result;
    char name[MAX_STRING_CHARS];
    query.operation=COD1_MODEL_RESOURCE_VALIDATE;expected=query;
    if(!LiveItemCurrent(&cod1LiveItemGuard)||trap_Cod1ModelResourceReference(&query,&result)!=1||
       !LiveItemCurrent(&cod1LiveItemGuard)||memcmp(&query,&expected,sizeof(query))||
       memcmp(&result,&expected,sizeof(result)))return 0;
    trap_GetConfigstring(CS_MODELS+row->modelSlot,name,sizeof(name));
    return LiveItemCurrent(&cod1LiveItemGuard)&&!strcmp(name,row->descriptor.models[0]);
}
int G_Cod1ItemLifecycleMaskCurrent(const GscEntityRef *input,int nativeItemIndex) {
    GscEntityRef ref;int ok=0;
    if(!input||!LiveItemBuffers(input,sizeof(*input),NULL,0,NULL,0))return 0;
    ref=*input;
    if(!LiveItemBegin(&ref,NULL))return 0;
    cod1LiveItemGuard.sourceItem=input;
    /* Only the original type1 frag constructor's fixed407C0108 mask has
     * been joined to this BSP lane. A retained consumed/stale row is not
     * current, and resource reads run under the existing full-body guard. */
    if(nativeItemIndex<1||cod1LiveItemGuard.row.descriptor.type!=1||
       cod1LiveItemGuard.row.descriptor.index!=nativeItemIndex||
       cod1LiveItemGuard.row.cap!=nativeItemIndex||
       cod1LiveItemGuard.row.wp!=WP_GRENADE_PINEAPPLE||
       strcmp(cod1LiveItemGuard.row.descriptor.classname,"weapon_fraggrenade"))goto done;
    ok=LiveItemResourceCurrent(&cod1LiveItemGuard.row)&&LiveItemCurrent(&cod1LiveItemGuard);
done:LiveItemEnd();return ok;
}
static void LiveItemPose(gentity_t *e,const float origin[3],const float angles[3]) {
    memset(&e->s.pos,0,sizeof(e->s.pos));memset(&e->s.apos,0,sizeof(e->s.apos));
    e->s.pos.trType=e->s.apos.trType=TR_STATIONARY;
    memcpy(e->s.origin,origin,sizeof(vec3_t));memcpy(e->s.pos.trBase,origin,sizeof(vec3_t));
    memcpy(e->r.currentOrigin,origin,sizeof(vec3_t));memcpy(e->s.angles,angles,sizeof(vec3_t));
    memcpy(e->s.apos.trBase,angles,sizeof(vec3_t));memcpy(e->r.currentAngles,angles,sizeof(vec3_t));
}
static int LiveItemInstall(const GscEntityRef *ref,Cod1LiveItem *row,int flags,
    const float origin[3],const float angles[3],int count) {
    gentity_t *e;gentity_t before;int ground=0;vec3_t placed,mins,maxs,initialAngles;
    if(cod1LiveItemSerial==UINT64_MAX||!G_Cod1EntitySameAllocation(ref)||!LiveItemCurrent(&cod1LiveItemGuard))return 0;
    e=&g_entities[ref->slot];before=*e;memcpy(placed,origin,sizeof(placed));
    VectorSet(mins,-1,-1,row->descriptor.type==1?-1:0);
    VectorSet(maxs,1,1,row->descriptor.type==1?1:2);
    memcpy(initialAngles,angles,sizeof(initialAngles));
    if(!(flags&1)&&row->descriptor.type==1)initialAngles[2]+=90.0f;
    if(!LiveItemFinite(initialAngles))return 0;
    if(!(flags&1)){
        if(cod1LiveItemGuard.services.floor_trace(cod1LiveItemGuard.services.user,
             (int)ref->slot,origin,mins,maxs,placed,&ground)!=1 ||
           !LiveItemCurrent(&cod1LiveItemGuard)||!G_Cod1EntitySameAllocation(ref)||
           memcmp(e,&before,sizeof(before))||ground<0||ground>=MAX_GENTITIES||
           !LiveItemFinite(placed))return 0;
    }
    if(!LiveItemResourceCurrent(row)||!G_Cod1EntitySameAllocation(ref)||
       memcmp(e,&before,sizeof(before)))return 0;
    row->entity=*ref;row->serial=++cod1LiveItemSerial;row->flags=flags;row->count=count;
    row->ground=(flags&1)?0:ground;
    e->spawnflags=flags;e->count=count;e->item=(gitem_t *)row->itemRow;
    e->s.eType=ET_ITEM;e->s.modelindex=row->itemIndex;e->s.modelindex2=row->modelSlot;
    e->s.otherEntityNum2=0;e->s.groundEntityNum=row->ground;
    e->s.eFlags&=~(EF_NODRAW|EF_SPINNING);e->flags&=~(FL_NODRAW|FL_DROPPED_ITEM);
    e->r.svFlags=SVF_USE_CURRENT_ORIGIN;e->r.contents=CONTENTS_TRIGGER|CONTENTS_ITEM;
    memcpy(e->r.mins,mins,sizeof(mins));memcpy(e->r.maxs,maxs,sizeof(maxs));
    e->touch=G_Cod1ItemLifecycleTouch;e->use=G_Cod1ItemLifecycleUse;e->think=NULL;e->nextthink=0;
    e->physicsObject=qfalse;e->active=qtrue;LiveItemPose(e,placed,initialAngles);
    cod1LiveItems.rows[ref->slot]=*row;
    if(!ItemPreparedInstall(ref,row,e))return 0;
    /* Link is the publication boundary; do not adopt a callback's replacement. */
    cod1LiveItemGuard.hasItem=0;before=*e;
    trap_LinkEntity(e);
    if(!LiveItemCurrent(&cod1LiveItemGuard)||!G_Cod1EntitySameAllocation(ref)||
       !LiveItemBody(ref)||!ItemPreparedInstalledCurrent(ref))return 0;
    /* The real linker owns linked/absbounds/area fields; all other body state
     * is checked by the canonical owned-row predicate and later snapshots. */
    return 1;
}
int G_Cod1ItemLifecycleAdopt(const GscEntityRef *input,char *error,size_t size) {
    GscEntityRef ref;Cod1LiveItem row;gentity_t before;vec3_t origin,angles;int flags,count,ok=0;
    if(!LiveItemBuffers(input,sizeof(*input),NULL,0,error,size))return 0;
    if(!input)goto refuse;ref=*input;
    if(!G_Cod1EntitySameAllocation(&ref)||G_Cod1ItemLifecycleOwns(&ref)||
       !ActorOwnerAllowed(&ref,0)||!LiveItemClass(g_entities[ref.slot].classname,&row))goto refuse;
    before=g_entities[ref.slot];flags=before.spawnflags;count=before.count?before.count:row.count;
    if((flags!=0&&flags!=1)||count<1||before.neverFree||before.r.bmodel||before.physicsObject||
       !LiveItemFinite(before.r.currentOrigin)||!LiveItemFinite(before.s.angles))goto refuse;
    memcpy(origin,before.r.currentOrigin,sizeof(origin));memcpy(angles,before.s.angles,sizeof(angles));
    if(!LiveItemBegin(NULL,NULL))goto refuse;
    cod1LiveItemGuard.item=ref;cod1LiveItemGuard.sourceItem=input;
    if(!ItemPreparedAttempt(&ref))goto done;
    G_Cod1PathHealthHandoff(&ref);
    if(!LiveItemResource(&row)||!G_Cod1EntitySameAllocation(&ref)||
       memcmp(&before,&g_entities[ref.slot],sizeof(before)))goto done;
    ok=LiveItemInstall(&ref,&row,flags,origin,angles,count);
done:if(cod1PreparedItems.active)cod1PreparedItems.operation=-1;LiveItemEnd();
refuse:if(!ok)LiveItemError(error,size,"R216 item adoption lost exact map, class, resource or allocation");return ok;
}
int G_Cod1ItemLifecycleSpawn(const char *input,const float position[3],int32_t flags,
    GscEntityRef *out,char *error,size_t size) {
    char classname[64];vec3_t origin,angles={0,0,0};Cod1LiveItem row;
    GscEntityRef ref;gentity_t *e=NULL;int ok=0;size_t length;
    if(!ItemPreparedMutation())return 0;
    if(!LiveItemBuffers(input,64,out,sizeof(*out),error,size)||
       !LiveItemBuffers(position,sizeof(vec3_t),out,sizeof(*out),error,size)||
       CAR_Overlap(input,64,position,sizeof(vec3_t)))return 0;
    if(!input||!position||!out||!LiveItemFinite(position)||(flags!=0&&flags!=1))goto refuse;
    for(length=0;length<sizeof(classname)&&input[length];++length){}
    if(!length||length==sizeof(classname))goto refuse;
    memcpy(classname,input,length+1);memcpy(origin,position,sizeof(origin));
    if(!LiveItemClass(classname,&row)||!LiveItemBegin(NULL,NULL))goto refuse;
    cod1LiveItemGuard.sourceClass=input;cod1LiveItemGuard.sourceClassBytes=length+1;
    memcpy(cod1LiveItemGuard.sourceClassCopy,classname,length+1);
    cod1LiveItemGuard.sourceVector=position;memcpy(cod1LiveItemGuard.sourceVectorCopy,origin,sizeof(origin));
    if(!LiveItemResource(&row)||!LiveItemCurrent(&cod1LiveItemGuard)||
       cod1LiveItemSerial==UINT64_MAX||level.num_entities>ENTITYNUM_MAX_NORMAL||
       (level.num_entities==ENTITYNUM_MAX_NORMAL&&!G_EntitiesFree()))goto done;
    e=G_Spawn();
    if(!G_Cod1EntityCapture(e,&ref))goto done;
    /* A real allocation may grow the entity table; pin its exact new extent. */
    cod1LiveItemGuard.count=level.num_entities;
    if(!LiveItemCurrent(&cod1LiveItemGuard)||!G_Cod1EntitySameAllocation(&ref))goto cleanup;
    e->classname=G_NewString(classname);
    if(!LiveItemCurrent(&cod1LiveItemGuard)||!G_Cod1EntitySameAllocation(&ref))goto cleanup;
    ok=LiveItemInstall(&ref,&row,flags,origin,angles,row.count);
    if(ok&&LiveItemBuffers(position,sizeof(vec3_t),out,sizeof(*out),error,size))*out=ref;
    else ok=0;
cleanup:if(!ok&&G_Cod1EntitySameAllocation(&ref)&&ActorOwnerAllowed(&ref,0)&&
    !g_entities[ref.slot].physicsObject&&!g_entities[ref.slot].r.bmodel&&!g_entities[ref.slot].neverFree)
        G_FreeEntity(&g_entities[ref.slot]);
done:LiveItemEnd();
refuse:if(!ok)LiveItemError(error,size,"R216 grenade spawn requires a complete current linked item");return ok;
}

/* SV_LinkEntity may change only its derived shared bounds/link bookkeeping.
 * Its body/trajectory/current pose and all GAME-owned fields stay exact. */
static int LiveItemLinkUnchanged(const gentity_t *before,const gentity_t *after) {
    gentity_t expected=*before;
    expected.r.linked=after->r.linked;expected.r.linkcount=after->r.linkcount;
    memcpy(expected.r.absmin,after->r.absmin,sizeof(vec3_t));
    memcpy(expected.r.absmax,after->r.absmax,sizeof(vec3_t));
    return !memcmp(&expected,after,sizeof(expected));
}
static int LiveItemSetPose(const GscEntityRef *input,const float vector[3],
    int angle,char *error,size_t size) {
    GscEntityRef ref;vec3_t copy,origin,angles;gentity_t before;int ok=0;
    if(!ItemPreparedMutation())return 0;
    if(!LiveItemBuffers(input,sizeof(*input),NULL,0,error,size)||
       !LiveItemBuffers(vector,sizeof(vec3_t),NULL,0,error,size)||
       CAR_Overlap(input,sizeof(*input),vector,sizeof(vec3_t)))return 0;
    if(!input||!LiveItemFinite(vector))goto refuse;
    ref=*input;memcpy(copy,vector,sizeof(copy));
    if(!LiveItemBegin(&ref,NULL))goto refuse;
    cod1LiveItemGuard.sourceItem=input;cod1LiveItemGuard.sourceVector=vector;
    memcpy(cod1LiveItemGuard.sourceVectorCopy,copy,sizeof(copy));
    if(!LiveItemResourceCurrent(&cod1LiveItemGuard.row)||
       memcmp(vector,copy,sizeof(copy))||memcmp(input,&ref,sizeof(ref))||
       !LiveItemBuffers(vector,sizeof(vec3_t),NULL,0,error,size)||
       !LiveItemCurrent(&cod1LiveItemGuard))goto done;
    memcpy(origin,g_entities[ref.slot].r.currentOrigin,sizeof(origin));
    memcpy(angles,g_entities[ref.slot].r.currentAngles,sizeof(angles));
    if(angle)memcpy(angles,copy,sizeof(copy));else memcpy(origin,copy,sizeof(copy));
    LiveItemPose(&g_entities[ref.slot],origin,angles);
    before=g_entities[ref.slot];cod1LiveItemGuard.hasItem=0;
    trap_LinkEntity(&g_entities[ref.slot]);
    ok=LiveItemCurrent(&cod1LiveItemGuard)&&G_Cod1EntitySameAllocation(&ref)&&
       LiveItemBody(&ref)&&LiveItemLinkUnchanged(&before,&g_entities[ref.slot]);
done:LiveItemEnd();
refuse:if(!ok)LiveItemError(error,size,"R216 item pose requires an exact current owned world item");return ok;
}
int G_Cod1ItemLifecycleSetOrigin(const GscEntityRef *r,const float v[3],char *e,size_t n){return LiveItemSetPose(r,v,0,e,n);}
int G_Cod1ItemLifecycleSetAngles(const GscEntityRef *r,const float v[3],char *e,size_t n){return LiveItemSetPose(r,v,1,e,n);}
int G_Cod1ItemLifecycleSetCount(const GscEntityRef *input,int32_t count,char *error,size_t size) {
    GscEntityRef ref;int ok=0;
    if(!ItemPreparedMutation())return 0;
    if(!LiveItemBuffers(input,sizeof(*input),NULL,0,error,size))return 0;
    if(!input||count<1)goto refuse;ref=*input;
    if(!LiveItemBegin(&ref,NULL))goto refuse;
    cod1LiveItemGuard.sourceItem=input;
    if(!LiveItemResourceCurrent(&cod1LiveItemGuard.row)||memcmp(input,&ref,sizeof(ref))||
       !LiveItemCurrent(&cod1LiveItemGuard))goto done;
    cod1LiveItems.rows[ref.slot].count=count;g_entities[ref.slot].count=count;ok=1;
done:LiveItemEnd();
refuse:if(!ok)LiveItemError(error,size,"R216 item count requires a positive exact int32 and current item");return ok;
}
int G_Cod1ItemLifecycleGetCount(const GscEntityRef *input,int32_t *out,char *error,size_t size) {
    GscEntityRef ref;int32_t value;int ok=0;
    if(!LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
    if(!input||!out)goto refuse;ref=*input;
    if(!LiveItemBegin(&ref,NULL))goto refuse;
    cod1LiveItemGuard.sourceItem=input;
    value=cod1LiveItemGuard.row.count;
    if(!LiveItemResourceCurrent(&cod1LiveItemGuard.row)||memcmp(input,&ref,sizeof(ref))||
       !LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||
       !LiveItemCurrent(&cod1LiveItemGuard))goto done;
    *out=value;ok=1;
done:LiveItemEnd();
refuse:if(!ok)LiveItemError(error,size,"R216 count read requires a current owned item");return ok;
}
int G_Cod1ItemLifecyclePickup(const GscEntityRef *input,const GscEntityRef *playerInput,
    int activated,int32_t *out,char *error,size_t size) {
    GscEntityRef item,player;Cod1LiveItem row;int32_t accepted=INT32_MIN;
    int ok=0,acquired=0,serviceResult;gentity_t *e;
    if(!ItemPreparedMutation())return 0;
    if(!LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||
       !LiveItemBuffers(playerInput,sizeof(*playerInput),out,sizeof(*out),error,size)||
       CAR_Overlap(input,sizeof(*input),playerInput,sizeof(*playerInput)))return 0;
    if(!input||!playerInput||!out||(activated!=0&&activated!=1))goto refuse;
    item=*input;player=*playerInput;
    if(!LiveItemBegin(&item,&player))goto refuse;
    cod1LiveItemGuard.sourceItem=input;cod1LiveItemGuard.sourcePlayer=playerInput;
    row=cod1LiveItemGuard.row;
    if(!activated && (row.descriptor.type==1 ||
       g_entities[player.slot].client->pers.autoActivate==PICKUP_ACTIVATE))goto done;
    if(g_entities[player.slot].health<=0 ||
       g_entities[player.slot].client->ps.stats[STAT_HEALTH]<=0 ||
       g_entities[player.slot].client->noclip ||
       (g_entities[player.slot].client->ps.pm_flags&PMF_LIMBO) ||
       g_entities[player.slot].client->sess.sessionTeam==TEAM_SPECTATOR)goto done;
    if(!LiveItemResourceCurrent(&row)||
       cod1LiveItemGuard.services.read_registration(cod1LiveItemGuard.services.user,row.cap,
            &cod1LiveItemGuard.registration,error,size,LiveItemCurrent,&cod1LiveItemGuard)!=1||
       !LiveItemCurrent(&cod1LiveItemGuard)||
       cod1LiveItemGuard.registration.selectedCAPIndex!=row.cap||
       memcmp(&cod1LiveItemGuard.registration.registry,&cod1LiveItems.registry,sizeof(cod1LiveItems.registry))||
       memcmp(input,&item,sizeof(item))||memcmp(playerInput,&player,sizeof(player))||
       !LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size))goto done;
    serviceResult=cod1LiveItemGuard.services.acquire(cod1LiveItemGuard.services.user,&player,
            &cod1LiveItemGuard.registration,row.count,&accepted,error,size,
            LiveItemCurrent,&cod1LiveItemGuard);
    if(accepted>0&&accepted<=row.count)acquired=1;
    if(!acquired && (serviceResult!=1||accepted!=0))goto done;
    /* The acquire core's final caller fence is immediately before its pure
     * player commit. Checking the world again must not reject that permitted
     * player mutation; the row/body remain fixed and player incarnation live. */
    if(!acquired && (!LiveItemScope()||memcmp(input,&item,sizeof(item))||
       memcmp(playerInput,&player,sizeof(player))||
       !LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size)))goto done;
    if(!accepted){*out=0;ok=1;goto done;}
    /* Positive acquisition is irrevocable. Even an adapter's later scope
     * retirement cannot leave the same owned allocation available to grant
     * twice. A replacement is never hidden, notified or freed. */
    ok=1;
    if(!G_Cod1EntitySameAllocation(&item)||!G_Cod1ItemLifecycleOwns(&item)||
       cod1LiveItems.rows[item.slot].serial!=row.serial)goto done;
    e=&g_entities[item.slot];cod1LiveItems.rows[item.slot].consumed=1;
    if(!memcmp(input,&item,sizeof(item))&&!memcmp(playerInput,&player,sizeof(player))&&
       LiveItemBuffers(input,sizeof(*input),out,sizeof(*out),error,size))*out=accepted;
    /* A trusted adapter may have attached a new owner after its grant. The
     * old row still cannot grant twice, but its foreign body is untouched. */
    if(!LiveItemOriginalOwner(&item,&row,0))goto done;
    /* Consumption precedes foreign notification and makes recursive touch
     * unable to grant twice. Never touch any replacement after a boundary. */
    e->r.contents=0;e->r.svFlags|=SVF_NOCLIENT;e->flags|=FL_NODRAW;
    e->touch=NULL;e->use=NULL;e->think=NULL;e->nextthink=0;
    cod1LiveItemGuard.hasItem=0;cod1LiveItemGuard.hasPlayer=0;
    trap_UnlinkEntity(e);
    if(G_Cod1EntitySameAllocation(&item)) {
        /* Service emits the genuine selected Session event/payload. The
         * inventory commit is final even if it retires the Session or map. */
        if(!e->r.linked && LiveItemScope())
            (void)cod1LiveItemGuard.services.pickup_notify(cod1LiveItemGuard.services.user,
                &item,&player,error,size);
        if(LiveItemOriginalOwner(&item,&row,1)&&
           G_Cod1ItemLifecycleOwns(&item)&&cod1LiveItems.rows[item.slot].serial==row.serial&&
           cod1LiveItems.rows[item.slot].consumed)
            G_FreeEntity(&g_entities[item.slot]);
    }
done:LiveItemEnd();
refuse:if(!ok&&!acquired)LiveItemError(error,size,"R216 item pickup lost current world, player or registration");return ok;
}
void G_Cod1ItemLifecycleTouch(gentity_t *item,gentity_t *player,trace_t *trace) {
    GscEntityRef i,p;int32_t accepted=0;char error[256]={0};int activated,spawn,ok;gclient_t *client;
    uint32_t started=0,elapsed=0;static uint64_t refusalEpoch;
    (void)trace;
    if(!item||!player||!player->client||!G_Cod1EntityCapture(item,&i)||
       !G_Cod1EntityCapture(player,&p))return;
    client=player->client;spawn=client->ps.persistant[PERS_SPAWN_COUNT];
    activated=client->pers.autoActivate==PICKUP_FORCE;
    /* A weapon touch without explicit activation cannot acquire anything.
     * Preserve reentrant guard cancellation; skip only an idle transaction. */
    if(!activated&&!cod1LiveItemGuard.active&&G_Cod1ItemLifecycleOwns(&i)&&cod1LiveItems.rows[i.slot].descriptor.type==1)return;
    if(activated)started=(uint32_t)trap_Milliseconds();
    ok=G_Cod1ItemLifecyclePickup(&i,&p,activated,&accepted,error,sizeof(error));
    if(activated)elapsed=(uint32_t)trap_Milliseconds()-started;
    if(PlayerNativePlayer(&p)&&player->client==client&&
       client->ps.persistant[PERS_SPAWN_COUNT]==spawn&&client->pers.autoActivate==PICKUP_FORCE)
        client->pers.autoActivate=PICKUP_ACTIVATE;
    /* At most one refusal per map; successful explicit pickups only. */
    if(activated&&accepted>0)G_Printf("[OPT_PICKUP] item=%u accepted=%d elapsed_ms=%u\n",(unsigned)i.slot,(int)accepted,(unsigned)elapsed);
    else if(activated&&!ok&&i.epoch&&refusalEpoch!=i.epoch){
        refusalEpoch=i.epoch;
        G_Printf("[R216] ITEM_PICKUP_REFUSED item=%u elapsed_ms=%u: %s\n",(unsigned)i.slot,(unsigned)elapsed,error[0]?error:"admission denied");
    }
}
void G_Cod1ItemLifecycleUse(gentity_t *item,gentity_t *other,gentity_t *activator) {
    GscEntityRef ref;(void)other;(void)activator;
    /* Native handler8 is reveal/respawn, not an inventory grant. A selected
     * live item is already visible; use leaves it live without granting. */
    if(!item||!G_Cod1EntityCapture(item,&ref)||!LiveItemBody(&ref))return;
}

#include "g_cod1_item_prepared.inc"
