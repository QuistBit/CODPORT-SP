/* R198 selected delete(): use the real generation-aware engine free, including
 * unlink and existing resource/actor/static sidecar cleanup. This admits only
 * established CoD script/trigger topology and ordinary health item bodies.
 * A script classname is never evidence of an AI/player lifecycle. */
static int cod1EntityDeleteBusy,cod1EntityDeleteCancelled;
static int EntityDeleteOverlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int EntityDeleteEligible(const GscEntityRef *ref) {
    const gentity_t *ent;int i;
    if(!ref||!cod1EntityReady||level.spawning||level.restarted||
       level.num_entities<MAX_CLIENTS||level.num_entities>MAX_GENTITIES||
       !G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||
       !EntityValidate(NULL,ref)||!ActorOwnerAllowed(ref,0))return 0;
    ent=&g_entities[ref->slot];
    if(ent->neverFree||ent->teamchain||ent->teammaster||ent->tagParent||ent->tagName||
       (ent->team&&ent->team[0]))return 0;
    /* Retained RTCW tag connections store raw pointers/configstrings rather
     * than the generation-owned DObj component attachments. Do not free their
     * parent before a separate tag-detachment contract exists. Compare borrowed
     * pointers only; a foreign tagParent is never dereferenced. */
    for(i=0;i<level.num_entities;++i)
        if(g_entities[i].inuse&&g_entities[i].tagParent==ent)return 0;
    if(EntityVisibilityEligible(ent)||EntityTriggerOriginEligible(ent))return 1;
    /* Item data is a static bg_itemlist descriptor, not an owned heap object.
     * Ordinary G_FreeEntity retires these same bodies on engine item cleanup.
     * Ammo/weapon/actor candidates have a separate ownership pipeline and are
     * excluded by ActorOwnerAllowed and by the exact health class/type pair. */
    if(!ent->classname||!ent->item||ent->s.eType!=ET_ITEM||
       (strcmp(ent->classname,"item_health")&&
        strcmp(ent->classname,"item_health_large")&&
        strcmp(ent->classname,"item_health_small")))return 0;
    for(i=1;i<bg_numItems;++i)if(ent->item==&bg_itemlist[i])
        return bg_itemlist[i].giType==IT_HEALTH&&bg_itemlist[i].classname&&
            !strcmp(bg_itemlist[i].classname,ent->classname);
    return 0;
}
static int EntityDeleteLease(uint64_t epoch,int hostTime,
    int (*current)(void *),void *lease) {
    return !cod1EntityDeleteCancelled&&current&&current(lease)&&
        !cod1EntityDeleteCancelled&&epoch&&G_Cod1EntityMapEpoch()==epoch&&
        cod1EntityReady&&!level.spawning&&!level.restarted&&level.time==hostTime&&
        G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER;
}
static int EntityDeleteInvoke(void *user,const GscEntityRef *input,int now,
    char *error,size_t size,int (*current)(void *),void *lease) {
    GscEntityRef ref;gentity_t before;int hostTime;
    (void)user;
    if(cod1EntityDeleteBusy) {
        cod1EntityDeleteCancelled=1;
        EntityError(error,size,"entity delete callback reentry refused");return 0;
    }
    if(!input||!current||EntityDeleteOverlap(input,sizeof(*input),error,size))return 0;
    ref=*input;
    if(now<0||!EntityDeleteEligible(&ref))goto refuse;
    before=g_entities[ref.slot];hostTime=level.time;
    cod1EntityDeleteBusy=1;cod1EntityDeleteCancelled=0;
    if(!EntityDeleteLease(ref.epoch,hostTime,current,lease)||
       !EntityDeleteEligible(&ref)||memcmp(&before,&g_entities[ref.slot],sizeof(before)))goto busyRefuse;
    /* No engine callback occurs between the final admission and G_FreeEntity.
     * That production function owns the unlink/reentry/resource cleanup order. */
    G_FreeEntity(&g_entities[ref.slot]);
    if(!EntityDeleteLease(ref.epoch,hostTime,current,lease)||
       cod1EntityGeneration[ref.slot]!=ref.generation||g_entities[ref.slot].inuse)goto busyRefuse;
    cod1EntityDeleteBusy=0;return 1;
busyRefuse:
    cod1EntityDeleteBusy=0;
refuse:
    EntityError(error,size,"delete requires a current unteamed CoD script/trigger or health item; protected, AI, player and replacement allocations are unsupported");return 0;
}
const GscEntityDeleteHost *G_Cod1EntityDeleteHost(void) {
    static const GscEntityDeleteHost host={NULL,EntityDeleteInvoke};return &host;
}
