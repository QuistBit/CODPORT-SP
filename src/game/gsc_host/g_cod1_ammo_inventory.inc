/* R213 read-only canonical SP inventory -> separate native-shaped CAP domains.
 * The dedicated synchronous guard is outside map-resettable storage. */
static struct { int active, cancelled; uint64_t serial; } cod1AmmoInventoryGuard;
static uint64_t cod1AmmoInventorySerial;

static int AmmoInventoryRange(const void *p,size_t n) {
    return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);
}
/* Malformed active-pool bounds cannot safely classify an error alias. */
static int AmmoInventoryPoolSafe(void) {
    return level.clients && level.maxclients>0 && level.maxclients<=MAX_CLIENTS &&
        AmmoInventoryRange(level.clients,sizeof(gclient_t)*(size_t)level.maxclients);
}
static int AmmoInventoryPrivate(const void *p,size_t n) {
    int i,count=CW_Count();
    if(!AmmoInventoryRange(p,n))return 1;
    if(CAR_StorageProtected(p,n) || G_Cod1EntityStorageProtected(p,n) ||
       CAR_Overlap(p,n,G_Cod1PlayerNativeHost(),sizeof(GscPlayerNativeHost)) ||
       CAR_Overlap(p,n,cod1ItemCompiled,sizeof(cod1ItemCompiled)) ||
       CAR_Overlap(p,n,&cod1AmmoInventoryGuard,sizeof(cod1AmmoInventoryGuard)) ||
       CAR_Overlap(p,n,&cod1AmmoInventorySerial,sizeof(cod1AmmoInventorySerial)) ||
       CAR_Overlap(p,n,cod1PlayerFreezeOwners,sizeof(cod1PlayerFreezeOwners)) ||
       CAR_Overlap(p,n,&cod1PlayerNativeBusy,sizeof(cod1PlayerNativeBusy)) ||
       CAR_Overlap(p,n,&cod1PlayerNativeCancelled,sizeof(cod1PlayerNativeCancelled)))return 1;
    if(count<0 || count>CW_MAX_DEFS)return 1;
    for(i=0;i<count;++i) {
        const cwDef_t *d=CW_Get(i);
        if(!d || CAR_Overlap(p,n,d,sizeof(*d)))return 1;
    }
    return 0;
}
/* A pool identifies exactly one physical cell and vice versa, independently
 * for ammo and clip. Coincident counter values do not resolve ambiguous cells. */
static int AmmoInventoryMap(int pool,int cell,int poolCell[COD1_AMMO_PICKUP_CELLS],
    int cellPool[MAX_WEAPONS]) {
    if(pool<=0 || pool>=COD1_AMMO_PICKUP_CELLS || cell<=0 || cell>=MAX_WEAPONS ||
       (poolCell[pool]>=0 && poolCell[pool]!=cell) ||
       (cellPool[cell]>=0 && cellPool[cell]!=pool))return 0;
    poolCell[pool]=cell;cellPool[cell]=pool;return 1;
}
static int AmmoInventoryProject(const GscEntityRef *ref,
    const cod1AmmoRegistryReceipt_t *registry,cod1AmmoPickupInventorySnapshot_t *out) {
    const playerState_t *ps;
    int ammoCell[COD1_AMMO_PICKUP_CELLS],clipCell[COD1_AMMO_PICKUP_CELLS];
    int ammoPool[MAX_WEAPONS],clipPool[MAX_WEAPONS],weaponRoots[WP_NUM_WEAPONS];
    uint32_t allowed[2]={0,0};
    int i,w,chosen,ac,cc,spawn;
    if(!AmmoInventoryPoolSafe() || !PlayerNativePlayer(ref) || !registry ||
       WP_NUM_WEAPONS>64 || sizeof(out->portWeapons)!=sizeof(level.clients[0].ps.weapons) ||
       !CAR_Current(registry))return 0;
    ps=&g_entities[ref->slot].client->ps;spawn=ps->persistant[PERS_SPAWN_COUNT];
    if(spawn<0 || spawn==INT_MAX || registry->ammoCount<1 || registry->clipCount<1 ||
       strcmp(registry->ammo[0].name,"none") || strcmp(registry->clip[0].name,"none") ||
       registry->ammo[0].limit || registry->clip[0].limit)return 0;
    memset(out,0,sizeof(*out));
    out->version=COD1_AMMO_PICKUP_INVENTORY_VERSION;out->bytes=(uint32_t)sizeof(*out);
    out->player=*ref;out->spawnCount=spawn;
    out->clientOwner=(uintptr_t)g_entities[ref->slot].client;
    out->clientPool=(uintptr_t)level.clients;out->maxclients=level.maxclients;
    memcpy(&out->registry,registry,sizeof(*registry));
    memcpy(out->portWeapons,ps->weapons,sizeof(out->portWeapons));
    out->modeBits=ps->stats[STAT_COD1_MODE];out->inventory.weaponCount=registry->weaponCount;
    for(i=0;i<=CW_MAX_DEFS;++i)
        out->mapping[i].wp=out->mapping[i].ammoCell=out->mapping[i].clipCell=-1;
    for(i=0;i<COD1_AMMO_PICKUP_CELLS;++i)ammoCell[i]=clipCell[i]=-1;
    for(i=0;i<MAX_WEAPONS;++i)ammoPool[i]=clipPool[i]=-1;
    for(i=0;i<registry->ammoCount;++i)out->inventory.maxAmmo[i]=registry->ammo[i].limit;
    for(i=0;i<registry->clipCount;++i)out->inventory.maxClip[i]=registry->clip[i].limit;
    for(i=0;i<registry->sharedCapCount;++i)out->inventory.sharedCap[i]=registry->sharedCap[i].limit;
    /* Pure registry projection: resolve each physical weapon root once,
     * instead of repeating name lookups for every definition. Duplicate
     * family mapping rejection below remains unchanged. */
    for(w=1;w<WP_NUM_WEAPONS;++w)weaponRoots[w]=CW_WeaponRoot(w);
    for(i=1;i<=registry->weaponCount;++i) {
        const cod1AmmoRegistryWeapon_t *weapon=&registry->weapons[i];
        const cod1AmmoPickupWeapon_t *pickup=&weapon->pickup;
        chosen=-1;
        for(w=1;w<WP_NUM_WEAPONS;++w)if(weaponRoots[w]==weapon->familyRoot) {
            if(chosen>=0)return 0;
            chosen=w;
        }
        out->inventory.weapons[i]=*pickup;
        /* The mounted bank also owns turret/non-player and currently unmapped
         * player definitions. These resources have no inventory bit or counter
         * cell; their metadata remains present and their ownership stays zero.
         * Grant/pickup callers still require an explicit physical mapping. */
        if(chosen<0)continue;
        if(chosen<=WP_NONE || chosen>=MAX_WEAPONS || chosen>=64 ||
           pickup->ammoIndex<=0 || pickup->ammoIndex>=registry->ammoCount ||
           pickup->clipIndex<=0 || pickup->clipIndex>=registry->clipCount)return 0;
        ac=(int)BG_PlayerAmmoIndex(ps,(weapon_t)chosen);
        cc=(int)BG_FindClipForWeapon((weapon_t)chosen);
        if(!AmmoInventoryMap(pickup->ammoIndex,ac,ammoCell,ammoPool) ||
           !AmmoInventoryMap(pickup->clipIndex,cc,clipCell,clipPool) ||
           ps->ammo[ac]<0 || ps->ammoclip[cc]<0)return 0;
        out->mapping[i].wp=chosen;out->mapping[i].ammoCell=ac;out->mapping[i].clipCell=cc;
        out->inventory.ammo[pickup->ammoIndex]=ps->ammo[ac];
        out->inventory.clip[pickup->clipIndex]=ps->ammoclip[cc];
        allowed[chosen/32]|=UINT32_C(1)<<(chosen%32);
        if(out->portWeapons[chosen/32]&(UINT32_C(1)<<(chosen%32)))
            out->inventory.owned[i/32]|=UINT32_C(1)<<(i%32);
    }
    if((out->portWeapons[0]&~allowed[0]) || (out->portWeapons[1]&~allowed[1]))return 0;
    return 1;
}

/* Compare relevant fields, never incidental structure padding or unrelated
 * player trajectory/weapon animation state. All domains below are copied. */
static int AmmoInventoryEqual(const cod1AmmoPickupInventorySnapshot_t *a,
    const cod1AmmoPickupInventorySnapshot_t *b) {
    return a->version==b->version && a->bytes==b->bytes &&
        PlayerNativeRefEqual(&a->player,&b->player) && a->spawnCount==b->spawnCount &&
        a->clientOwner==b->clientOwner && a->clientPool==b->clientPool &&
        a->maxclients==b->maxclients && a->modeBits==b->modeBits &&
        !memcmp(&a->registry,&b->registry,sizeof(a->registry)) &&
        !memcmp(&a->inventory,&b->inventory,sizeof(a->inventory)) &&
        !memcmp(a->mapping,b->mapping,sizeof(a->mapping)) &&
        !memcmp(a->portWeapons,b->portWeapons,sizeof(a->portWeapons));
}
int G_Cod1AmmoPickupInventoryCurrent(const cod1AmmoPickupInventorySnapshot_t *input) {
    cod1AmmoPickupInventorySnapshot_t fresh;
    if(!input || !AmmoInventoryPoolSafe() || AmmoInventoryPrivate(input,sizeof(*input)) ||
       input->version!=COD1_AMMO_PICKUP_INVENTORY_VERSION || input->bytes!=sizeof(*input))return 0;
    return AmmoInventoryProject(&input->player,&input->registry,&fresh) &&
        AmmoInventoryEqual(input,&fresh);
}
static int AmmoInventoryBuffers(const GscEntityRef *ref,const cod1AmmoRegistryReceipt_t *registry,
    cod1AmmoPickupInventorySnapshot_t *out,char *error,size_t size) {
    return AmmoInventoryPoolSafe() && !AmmoInventoryPrivate(ref,sizeof(*ref)) &&
        !AmmoInventoryPrivate(registry,sizeof(*registry)) && !AmmoInventoryPrivate(out,sizeof(*out)) &&
        !AmmoInventoryPrivate(error,size) &&
        !CAR_Overlap(ref,sizeof(*ref),registry,sizeof(*registry)) &&
        !CAR_Overlap(ref,sizeof(*ref),out,sizeof(*out)) &&
        !CAR_Overlap(registry,sizeof(*registry),out,sizeof(*out)) &&
        !CAR_Overlap(ref,sizeof(*ref),error,size) &&
        !CAR_Overlap(registry,sizeof(*registry),error,size) &&
        !CAR_Overlap(out,sizeof(*out),error,size);
}
static int AmmoInventoryFence(uint64_t serial,const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,const GscEntityRef *ref,
    const cod1AmmoRegistryReceipt_t *registry,const cod1AmmoPickupInventorySnapshot_t *staged,
    cod1AmmoPickupInventorySnapshot_t *out,char *error,size_t size) {
    cod1AmmoPickupInventorySnapshot_t fresh;
    return cod1AmmoInventoryGuard.active && !cod1AmmoInventoryGuard.cancelled &&
        cod1AmmoInventoryGuard.serial==serial &&
        AmmoInventoryBuffers(input,inputRegistry,out,error,size) &&
        !memcmp(input,ref,sizeof(*ref)) && !memcmp(inputRegistry,registry,sizeof(*registry)) &&
        AmmoInventoryProject(ref,registry,&fresh) && AmmoInventoryEqual(&fresh,staged);
}
int G_Cod1ReadAmmoPickupInventory(const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,cod1AmmoPickupInventorySnapshot_t *out,
    char *error,size_t size,int (*current)(void *),void *context) {
    GscEntityRef ref;
    cod1AmmoRegistryReceipt_t registry;
    cod1AmmoPickupInventorySnapshot_t staged;
    uint64_t serial;
    int ok=0,phase;
    if(!AmmoInventoryBuffers(input,inputRegistry,out,error,size))return 0;
    if(!input || !inputRegistry || !out || !current)return 0;
    if(cod1AmmoInventoryGuard.active) {
        cod1AmmoInventoryGuard.cancelled=1;
        EntityError(error,size,"ammo inventory read reentered");return 0;
    }
    if(cod1AmmoInventorySerial==UINT64_MAX) {
        EntityError(error,size,"ammo inventory operation serial exhausted");return 0;
    }
    memcpy(&ref,input,sizeof(ref));memcpy(&registry,inputRegistry,sizeof(registry));
    serial=++cod1AmmoInventorySerial;
    cod1AmmoInventoryGuard.active=1;cod1AmmoInventoryGuard.cancelled=0;
    cod1AmmoInventoryGuard.serial=serial;
    if(!AmmoInventoryProject(&ref,&registry,&staged))goto done;
    for(phase=0;phase<2;++phase) {
        if(!AmmoInventoryFence(serial,input,inputRegistry,&ref,&registry,&staged,out,error,size) ||
           current(context)!=1 ||
           !AmmoInventoryFence(serial,input,inputRegistry,&ref,&registry,&staged,out,error,size))goto done;
    }
    /* All borrowed code is over. Commit one staged copy, with no later call. */
    memcpy(out,&staged,sizeof(staged));ok=1;
done:
    cod1AmmoInventoryGuard.active=0;cod1AmmoInventoryGuard.cancelled=0;
    if(!ok && AmmoInventoryBuffers(input,inputRegistry,out,error,size))
        EntityError(error,size,"ammo inventory read lost current owner, registry or relevant state");
    return ok;
}
