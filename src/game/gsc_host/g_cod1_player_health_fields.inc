/* Retail entity field table2008A200: health +1D8 has setter20038AA0;
 * maxhealth +1DC is a plain signed integer field. Neither is client+7440.
 * This selected player sidecar is not a native birth or save/restore claim.
 * No fallback value is invented before an assignment establishes the cell. */
typedef struct {
    GscEntityRef player;
    int spawnCount,present;
    int32_t maximum;
} Cod1PlayerHealthFields;
static Cod1PlayerHealthFields cod1PlayerHealthFields[MAX_CLIENTS];
static int PlayerHealthFieldStoragePrivate(const void *p,size_t n) {
    return PlayerNativeOverlap(p,n,cod1PlayerHealthFields,sizeof(cod1PlayerHealthFields));
}

static int PlayerHealthFieldBorrowed(const void *p,size_t n) {
    return PlayerNormalHealthBorrowed(p,n)||
        PlayerNativeOverlap(p,n,cod1PlayerHealthFields,sizeof(cod1PlayerHealthFields));
}
static int PlayerHealthFieldCurrent(const GscEntityRef *ref,const Cod1PlayerHealthFields *cell) {
    return cell->present&&PlayerNativeRefEqual(ref,&cell->player)&&
        cell->spawnCount==level.clients[ref->slot].ps.persistant[PERS_SPAWN_COUNT];
}
int G_Cod1PlayerHealthFieldKnown(const GscEntityRef *ref) {
    GscEntityRef copy;
    if(!ref||PlayerHealthFieldBorrowed(ref,sizeof(*ref)))return 0;
    copy=*ref;
    return PlayerNativePlayer(&copy)&&PlayerHealthFieldCurrent(&copy,&cod1PlayerHealthFields[copy.slot]);
}
/* GetInt's already confirmed admitted numeric domain (R169): finite float
 * truncation toward zero or exact int32; no string parsing is selected. */
static int PlayerHealthFieldInteger(const GscStringCastValue *value,int32_t *out) {
    double n;
    if(value->type==5){*out=value->integer;return 1;}
    if(value->type!=4)return 0;n=value->number;
    if(!isfinite(n)||n < -2147483648.0||n >= 2147483648.0)return 0;
    *out=(int32_t)n;return 1;
}
static int PlayerHealthFieldAccess(const GscEntityRef *input,const char *name,
    const GscStringCastValue *value,GscStringCastValue *out,char *error,size_t size,
    int(*current)(void*),void *lease) {
    GscEntityRef ref;GscStringCastValue copied,result;
    Cod1PlayerHealthFields before,after;gentity_t *ent;gclient_t *client;
    int maximum,known,health,now,spawn,write=value!=NULL,noChange=0;int32_t desired=0;
    if(cod1PlayerNativeBusy)cod1PlayerNativeCancelled=1;
    if(!input||!name||!current||(!write&&!out)||
       PlayerHealthFieldBorrowed(input,sizeof(*input))||PlayerHealthFieldBorrowed(value,write?sizeof(*value):0)||
       PlayerHealthFieldBorrowed(out,out?sizeof(*out):0)||PlayerHealthFieldBorrowed(error,size)||
       PlayerNativeOverlap(input,sizeof(*input),out,out?sizeof(*out):0)||
       PlayerNativeOverlap(input,sizeof(*input),error,size)||
       PlayerNativeOverlap(value,write?sizeof(*value):0,error,size)||
       PlayerNativeOverlap(out,out?sizeof(*out):0,error,size))return 0;
    if(cod1PlayerNativeBusy){EntityError(error,size,"player field callback reentry refused");return 0;}
    /* Only literal native field names are selected; copied classification is
     * used after callbacks, never a mutable caller name. */
    maximum=!strcmp(name,"maxhealth");if(!maximum&&strcmp(name,"health"))return 0;
    ref=*input;if(!PlayerNativePlayer(&ref))goto refused;
    ent=&g_entities[ref.slot];client=ent->client;health=ent->health;now=level.time;
    spawn=client->ps.persistant[PERS_SPAWN_COUNT];before=cod1PlayerHealthFields[ref.slot];after=before;
    known=PlayerHealthFieldCurrent(&ref,&before);memset(&result,0,sizeof(result));
    if(write) {
        copied=*value;if(!PlayerHealthFieldInteger(&copied,&desired))goto refused;
        if(!maximum) {
            if(desired<=0)goto refused;
            if(health<=0) {
                if(!known)goto refused;
                /* Retail warns and performs no stores when already dead and
                 * entity maxhealth is nonzero; this is an observed no-op. */
                noChange=before.maximum!=0;
            }
        }
        if(!noChange){memset(&after,0,sizeof(after));after.player=ref;after.spawnCount=spawn;after.present=1;after.maximum=desired;}
    } else {
        if(maximum&&!known)goto refused;
        result.type=5;result.integer=maximum?before.maximum:health;
    }
    cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
    if(!current(lease)||cod1PlayerNativeCancelled||!PlayerNativePlayer(&ref)||ent->client!=client||
       level.time!=now||ent->health!=health||client->ps.persistant[PERS_SPAWN_COUNT]!=spawn||
       memcmp(&before,&cod1PlayerHealthFields[ref.slot],sizeof(before))||memcmp(input,&ref,sizeof(ref))||
       (write&&memcmp(value,&copied,sizeof(copied))))goto cancelled;
    if(write&&noChange) {
        G_Printf("^2Cannot set health on dead entities (health %i, max %i, ent %u)\n",health,before.maximum,(unsigned)ref.slot);
        if(!current(lease)||cod1PlayerNativeCancelled||!PlayerNativePlayer(&ref)||ent->client!=client||
           level.time!=now||ent->health!=health||client->ps.persistant[PERS_SPAWN_COUNT]!=spawn||
           memcmp(&before,&cod1PlayerHealthFields[ref.slot],sizeof(before)))goto cancelled;
    } else if(write) {
        /* Atomic bounded store, no callbacks. Field health writes the native
         * pair; normal-health native remains an independent health-only write. */
        cod1PlayerHealthFields[ref.slot]=after;
        if(!maximum)ent->health=desired;
    } else *out=result;
    cod1PlayerNativeBusy=0;return 1;
cancelled:
    cod1PlayerNativeBusy=0;
refused:
    EntityError(error,size,"player health field requires current allocation/spawn, admitted int32 and known maximum when needed");return 0;
}
int G_Cod1PlayerHealthFieldRead(const GscEntityRef *ref,const char *name,GscStringCastValue *out,
    char *error,size_t size,int(*current)(void*),void *lease) {
    return PlayerHealthFieldAccess(ref,name,NULL,out,error,size,current,lease);
}
int G_Cod1PlayerHealthFieldWrite(const GscEntityRef *ref,const char *name,const GscStringCastValue *value,
    char *error,size_t size,int(*current)(void*),void *lease) {
    if(!value)return 0;return PlayerHealthFieldAccess(ref,name,value,NULL,error,size,current,lease);
}
