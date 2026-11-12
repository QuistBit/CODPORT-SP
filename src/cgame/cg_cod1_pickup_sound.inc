/* R219: weapon pickup events retain their legacy bg_itemlist index, but CoD
 * WeaponDef owns the sound alias. Resolve during loading, never in an event.
 * Missing resources are reported once at load; no RTCW sound substitution. */
#include "../game/bg_cod1_soundalias.h"
#include "../game/bg_cod1_inventory.h"
#include "../game/bg_cod1_item_precache.h"
typedef struct {
    int count;
    char alias[MAX_QPATH];
    cwSoundAliasVariant variants[4];
    sfxHandle_t handles[4];
} cod1PickupSound_t;
static cod1PickupSound_t cgCod1PickupSounds[MAX_WEAPONS][2];
/* Compiled retail item rows 67..69 share the local item-event transport. */
enum { CG_HEALTH_FIRST = 67, CG_HEALTH_COUNT = 3 };
static cod1PickupSound_t cgHealthSounds[CG_HEALTH_COUNT];
static int CG_HealthPickupSlot(int index) {
    int slot;
    if (!cgs.cod1MapEntities || index<=0 || index>=bg_numItems ||
        bg_itemlist[index].giType!=IT_HEALTH || !bg_itemlist[index].classname) return -1;
    for (slot=0; slot<CG_HEALTH_COUNT; ++slot)
        if (!strcmp(bg_itemlist[index].classname,cod1ItemCompiled[CG_HEALTH_FIRST+slot].classname)) return slot;
    return -1;
}
int CG_Cod1AliasPickupItem(int index) {
    return CG_Cod1WeaponPickupItem(index) || CG_HealthPickupSlot(index)>=0;
}
static unsigned cgCod1PickupRandom=0x219u;
static float CG_Cod1PickupUnit(void) {
    cgCod1PickupRandom=cgCod1PickupRandom*1664525u+1013904223u;
    return (cgCod1PickupRandom>>8)*(1.0f/16777216.0f);
}
int CG_Cod1WeaponPickupItem(int index) {
    return cgs.cod1MapEntities && index>0 && index<bg_numItems &&
        bg_itemlist[index].giType==IT_WEAPON &&
        CW_WeaponDef(bg_itemlist[index].giTag)!=NULL;
}
void CG_Cod1PickupSoundsReset(void) {
    memset(cgCod1PickupSounds,0,sizeof(cgCod1PickupSounds));
    memset(cgHealthSounds,0,sizeof(cgHealthSounds));
    cgCod1PickupRandom=0x219u;
}
static void CG_RegisterPickupAlias(cod1PickupSound_t *set,const char *alias,char *const data[2],const int size[2]) {
    int i,t;
    Q_strncpyz(set->alias,alias,sizeof(set->alias));
    for(t=0;t<2;t++) {
        cwSoundAliasVariant variants[4];int n;
        if(!data[t])continue;
        n=CW_SoundAliasVariants(data[t],size[t],alias,variants,4);
        for(i=0;i<n;i++) {
            if(!CW_SoundAliasLoadSpecMatches(variants[i].loadspec,cgs.mapname))continue;
            set->handles[set->count]=trap_S_RegisterSound(variants[i].path);
            if(set->handles[set->count]<=0)
                CG_Printf("CoD1 R219: pickup sound '%s' unavailable (alias %s)\n",variants[i].path,alias);
            set->variants[set->count++]=variants[i];
        }
        if(set->count)break;
    }
    if(!set->count)CG_Printf("CoD1 R219: pickup alias '%s' unavailable on %s\n",alias,cgs.mapname);
    else CG_Printf("CoD1 R219: precached pickup alias %s (%d variants)\n",alias,set->count);
}
void CG_Cod1PickupSoundsInit(void) {
    static const char *tables[]={"soundaliases/iw_sound.csv","soundaliases/iw_sound2.csv"};
    char *data[2]={NULL,NULL};int size[2]={0,0};
    fileHandle_t file;int i,j,k,t,w,kind,found;const cwDef_t *def;const char *alias;
    CG_Cod1PickupSoundsReset();
    if(!cgs.cod1MapEntities)return;
    for(t=0;t<2;t++) {
        file=0;size[t]=trap_FS_FOpenFile(tables[t],&file,FS_READ);
        if(file && size[t]>0 && size[t]<=1048576) {
            data[t]=(char *)malloc(size[t]);
            if(data[t])trap_FS_Read(data[t],size[t],file);
        }
        if(file)trap_FS_FCloseFile(file);
    }
    for(w=1;w<MAX_WEAPONS;w++)for(kind=0;kind<2;kind++) {
        cod1PickupSound_t *set=&cgCod1PickupSounds[w][kind];
        def=CW_WeaponDef(w);if(!def)continue;
        alias=CW_String(def,kind?"ammoPickupSound":"pickupSound");
        /* Empty retail ammoPickupSound fields use the dedicated retail ammo
         * alias in this adapter; normal pickupSound may intentionally be empty. */
        if(kind && !*alias)alias="weap_ammo_pickup";
        if(!*alias)continue;
        /* Shared aliases need only one registration and one CSV lookup. */
        found=0;
        for(j=1;j<=w && !found;j++)for(k=0;k<2;k++) {
            if(j==w && k>=kind)break;
            if(!strcmp(alias,cgCod1PickupSounds[j][k].alias)) {
                *set=cgCod1PickupSounds[j][k];found=1;break;
            }
        }
        if(found)continue;
        CG_RegisterPickupAlias(set,alias,data,size);
    }
    for(i=0;i<CG_HEALTH_COUNT;++i) {
        CG_RegisterPickupAlias(&cgHealthSounds[i],cod1ItemCompiled[CG_HEALTH_FIRST+i].pickup_sound,data,size);
    }
    for(t=0;t<2;t++)free(data[t]);
}
static void CG_PlayPickupAlias(const cod1PickupSound_t *set,int entity) {
    const cwSoundAliasVariant *v;
    float total=0,choice,gain,pitch;int i,last=-1,channel;
    for(i=0;i<set->count;i++)if(set->handles[i]>0 && set->variants[i].probability>0) {
        total+=set->variants[i].probability;last=i;
    }
    if(last<0 || total<=0)return;
    choice=CG_Cod1PickupUnit()*total;
    for(i=0;i<set->count;i++)if(set->handles[i]>0 && set->variants[i].probability>0) {
        last=i;if(choice<set->variants[i].probability)break;
        choice-=set->variants[i].probability;
    }
    v=&set->variants[last];
    gain=0.8f*(v->volumeMin+(v->volumeMax-v->volumeMin)*CG_Cod1PickupUnit());
    pitch=v->pitchMin+(v->pitchMax-v->pitchMin)*CG_Cod1PickupUnit();
    channel=CW_AliasEqual(v->channel,"item")?CHAN_ITEM:CHAN_AUTO;
    trap_Cod1StartAliasSound(NULL,entity,channel,set->handles[last],gain,pitch,v->distMin,v->distMax);
}
static void CG_Cod1PlayWeaponPickupKind(int index,int entity,int ammo) {
    if(CG_Cod1WeaponPickupItem(index))CG_PlayPickupAlias(&cgCod1PickupSounds[bg_itemlist[index].giTag][ammo],entity);
}
static void CG_PlayItemPickup(int index,int entity) {
    int slot=CG_HealthPickupSlot(index);
    if(slot>=0)CG_PlayPickupAlias(&cgHealthSounds[slot],entity);
    else CG_Cod1PlayWeaponPickupKind(index,entity,0);
}
void CG_Cod1PlayWeaponPickup(int index,int entity) {
    CG_Cod1PlayWeaponPickupKind(index,entity,0);
}
void CG_Cod1PlayWeaponAmmoPickup(int index,int entity) {
    CG_Cod1PlayWeaponPickupKind(index,entity,1);
}
