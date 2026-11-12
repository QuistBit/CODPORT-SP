/* Sole owner g_spawn.c. Original static health rows67..69 enter1C5A0;
 * normal and delayed constructors set native C4=407C0108/flags|8000.
 * Shared stage owner also serves kind9 authored weapons, whose distinct
 * CW binding is checked below. Only masked participation, not geometry. */
#include "g_cod1_weapon_mask.inc"
#include "g_cod1_item_lifecycle.h"
void Use_Item(gentity_t*,gentity_t*,gentity_t*);
typedef struct {
 gentity_t snapshot;gitem_t item;int phase,modelIndex,itemIndex,due;
} C1HealthMask;
static C1HealthMask cod1HealthMasks[MAX_GENTITIES];
static int cod1HealthReadBusy;
static int Cod1HealthSnapshotMatches(const gentity_t*e,const C1HealthMask*h){
 gentity_t expected=h->snapshot;
 /* G_RunThink copies this exact inactive RTCW script status before every
  * callback. It does not change native health state. Accept only the
  * initial shadow or this exact copy; all other bytes remain fenced. */
 if(!memcmp(&e->scriptStatusCurrent,&expected.scriptStatus,sizeof(expected.scriptStatus)))
  expected.scriptStatusCurrent=expected.scriptStatus;
 return !memcmp(e,&expected,sizeof(expected));
}
static const char*Cod1HealthModel(const char*name){
 if(!name)return NULL;
 if(!strcmp(name,"weapon_m1carbine"))return "xmodel/weapon_m1carbine";
 if(!strcmp(name,"weapon_thompson"))return "xmodel/weapon_thompson";
 if(!strcmp(name,"weapon_springfield"))return "xmodel/weapon_springfield";
 if(!strcmp(name,"weapon_fraggrenade"))return "xmodel/weapon_mk2fraggrenade";
 if(!strcmp(name,"item_health_small"))return "xmodel/health_small";
 if(!strcmp(name,"item_health"))return "xmodel/health_medium";
 if(!strcmp(name,"item_health_large"))return "xmodel/health_large";
 return NULL;
}
static int Cod1HealthItemIndex(const gitem_t*item){
 uintptr_t p=(uintptr_t)item,b=(uintptr_t)bg_itemlist;
 if(!item||bg_numItems<=0||bg_numItems>MAX_ITEMS||p<b||
    p-b>=(size_t)bg_numItems*sizeof(*item)||(p-b)%sizeof(*item))return -1;
 return (int)((p-b)/sizeof(*item));
}
static int Cod1HealthResource(const GscEntityRef*ref){
 gentity_t before;char path[MAX_STRING_CHARS];const char*expected;
 C1HealthMask*h;C1PathEntityRecord*r;int phase;
 if(cod1HealthReadBusy||!G_Cod1EntitySameAllocation(ref))return 0;
 h=cod1HealthMasks+ref->slot;r=cod1PathEntities+ref->slot;before=g_entities[ref->slot];phase=h->phase;
 expected=Cod1HealthModel(before.classname);
 if((r->kind==9&&!Cod1WeaponMaskCurrent(ref->slot))||!expected||!before.model||strcmp(before.model,expected)||h->modelIndex<=0||h->modelIndex>=MAX_MODELS)return 0;
 cod1HealthReadBusy=1;trap_GetConfigstring(CS_MODELS+h->modelIndex,path,sizeof(path));cod1HealthReadBusy=0;
 return G_Cod1EntitySameAllocation(ref)&&r->entity.epoch==ref->epoch&&
  r->entity.generation==ref->generation&&(r->kind==8||r->kind==9)&&h->phase==phase&&(r->kind!=9||Cod1WeaponMaskCurrent(ref->slot))&&
  !memcmp(&before,g_entities+ref->slot,sizeof(before))&&!strcmp(path,expected);
}
int G_Cod1PathHealthStart(gentity_t*e,gitem_t*item,GscEntityRef*out){
 GscEntityRef ref;C1PathEntityRecord*r;C1HealthMask*h;const char*model;int index;
 if(!g_cod1MapEntities||!level.spawning||!out||!G_Cod1EntityCapture(e,&ref))return 0;
 r=cod1PathEntities+ref.slot;
 if(r->entity.epoch!=ref.epoch||r->entity.generation!=ref.generation||(r->kind!=8&&r->kind!=9))return 0;
 h=cod1HealthMasks+ref.slot;
 if(r->birth!=2||r->present){r->present=0;r->birth=4;h->phase=-1;return 0;}
 model=Cod1HealthModel(e->classname);index=Cod1HealthItemIndex(item);
 if(!model||index<=0||!item->classname||
    (r->kind==8?(item->giType!=IT_HEALTH||strcmp(item->classname,e->classname)):!Cod1WeaponMaskCapture(&ref,e,item))||
    !e->model||strcmp(e->model,model)||e->client||e->r.linked||e->r.contents||e->flags||
    (e->spawnflags!=0&&e->spawnflags!=1)||e->targetname||e->team||e->think||e->nextthink||
    e->touch||e->use||e->die||e->takedamage||e->scriptName||e->scriptEvents||e->numScriptEvents||
    e->scriptStatus.scriptEventIndex!=-1||e->scriptStatus.scriptFlags||level.time>INT_MAX-FRAMETIME*2)return 0;
 memset(h,0,sizeof(*h));h->phase=1;h->item=*item;h->itemIndex=index;h->due=level.time+FRAMETIME*2;
 r->nativeContents=0x407c0108u;r->flags=0;*out=ref;return 1;
}
void G_Cod1PathHealthPending(gentity_t*e,const GscEntityRef*ref){
 C1HealthMask*h;C1PathEntityRecord*r;
 if(!G_Cod1EntitySameAllocation(ref)||e!=g_entities+ref->slot)return;
 h=cod1HealthMasks+ref->slot;r=cod1PathEntities+ref->slot;
 if(h->phase!=1||r->birth!=2||(r->kind!=8&&r->kind!=9)||e->item!=bg_itemlist+h->itemIndex||
    memcmp(e->item,&h->item,sizeof(h->item))||e->r.linked||e->r.contents||
    e->s.eType!=ET_GENERAL||e->think!=FinishSpawningItem||e->nextthink!=h->due)return;
 h->modelIndex=e->s.modelindex2;
 if(!Cod1HealthResource(ref))return;
 h->snapshot=*e;h->phase=2;r->present=1;r->birth=3;
}
int G_Cod1PathHealthFinishStart(gentity_t*e,GscEntityRef*out){
 GscEntityRef ref;C1HealthMask*h;C1PathEntityRecord*r;gentity_t expected;
 if(!out||!G_Cod1EntityCapture(e,&ref))return 0;r=cod1PathEntities+ref.slot;h=cod1HealthMasks+ref.slot;
 if((r->kind!=8&&r->kind!=9)||r->entity.epoch!=ref.epoch||r->entity.generation!=ref.generation||
    r->birth!=3||!r->present||h->phase!=2)return 0;
 expected=h->snapshot;expected.nextthink=0;expected.scriptStatusCurrent=expected.scriptStatus;
 if(memcmp(e,&expected,sizeof(expected))||memcmp(e->item,&h->item,sizeof(h->item))){
  r->present=0;h->phase=-1;return -1;
 }
 /* Consume the pending receipt before any external namespace read. A
  * replacement allocated by that read must never receive failure stores. */
 r->present=0;h->phase=3;
 if(!Cod1HealthResource(&ref))return -1;
 *out=ref;return 1;
}
void G_Cod1PathHealthFinished(gentity_t*e,const GscEntityRef*ref){
 C1HealthMask*h;C1PathEntityRecord*r;
 if(!G_Cod1EntitySameAllocation(ref)||e!=g_entities+ref->slot)return;
 h=cod1HealthMasks+ref->slot;r=cod1PathEntities+ref->slot;
 if(h->phase!=3||(r->kind!=8&&r->kind!=9)||r->entity.epoch!=ref->epoch||r->entity.generation!=ref->generation||
    e->item!=bg_itemlist+h->itemIndex||memcmp(e->item,&h->item,sizeof(h->item))||
    !e->r.linked||e->r.contents!=(CONTENTS_TRIGGER|CONTENTS_ITEM)||e->s.eType!=ET_ITEM||
    e->s.modelindex!=h->itemIndex||e->s.modelindex2!=h->modelIndex||e->client||
    e->think!=FinishSpawningItem||e->nextthink||e->touch!=Touch_Item_Auto||e->use!=Use_Item)return;
 if(!Cod1HealthResource(ref))return;
 h->snapshot=*e;h->phase=4;r->present=1;
}
void G_Cod1PathHealthHandoff(const GscEntityRef*ref){
 C1PathEntityRecord*r;C1HealthMask*h;gentity_t*e;
 if(!G_Cod1EntitySameAllocation(ref))return;
 r=cod1PathEntities+ref->slot;h=cod1HealthMasks+ref->slot;e=g_entities+ref->slot;
 if(r->kind!=9||r->entity.epoch!=ref->epoch||r->entity.generation!=ref->generation)return;
 if(r->birth!=3||!r->present||(h->phase!=2&&h->phase!=4)||
    !e->classname||strcmp(e->classname,"weapon_fraggrenade")||
    !Cod1HealthSnapshotMatches(e,h)||e->item!=bg_itemlist+h->itemIndex||
    memcmp(e->item,&h->item,sizeof(h->item))||!Cod1WeaponMaskCurrent(ref->slot)){
  r->present=0;h->phase=-1;return;
 }
 h->phase=5; /* consumed initial snapshot; selected owner must stay current */
}
static int Cod1HealthCurrent(const GscEntityRef*ref){
 C1PathEntityRecord*r;C1HealthMask*h;gentity_t*e;uint64_t actor;char error[128];
 if(!G_Cod1EntitySameAllocation(ref))return 0;r=cod1PathEntities+ref->slot;h=cod1HealthMasks+ref->slot;e=g_entities+ref->slot;
 if((r->kind!=8&&r->kind!=9)||r->birth!=3||!r->present||r->entity.epoch!=ref->epoch||r->entity.generation!=ref->generation||
    (h->phase!=2&&h->phase!=4&&h->phase!=5)||
    e->item!=bg_itemlist+h->itemIndex||memcmp(e->item,&h->item,sizeof(h->item))||
    r->nativeContents!=0x407c0108u)return 0;
 if(r->kind==9&&h->phase==5){
  C1PathEntityRecord pathBefore=*r;C1HealthMask initialBefore=*h;gentity_t bodyBefore=*e;
  /* The genuine selected lifecycle owns pose/callback/resource changes.
   * Keep the original BSP/registry binding; never refresh its old snapshot
   * or fall back to it when the selected lifecycle refuses. */
  return G_Cod1ItemLifecycleOwns(ref)&&Cod1WeaponMaskCurrent(ref->slot)&&
   G_Cod1ItemLifecycleMaskCurrent(ref,cod1WeaponMaskBindings[ref->slot].nativeItemIndex)&&
   G_Cod1EntitySameAllocation(ref)&&!memcmp(r,&pathBefore,sizeof(*r))&&
   !memcmp(h,&initialBefore,sizeof(*h))&&!memcmp(e,&bodyBefore,sizeof(*e))&&
   !memcmp(e->item,&h->item,sizeof(h->item))&&Cod1WeaponMaskCurrent(ref->slot);
 }
 if(r->kind==9&&G_Cod1ItemLifecycleOwns(ref))return 0;
 if(!Cod1HealthSnapshotMatches(e,h))return 0;
 if(!G_Cod1ActorPresenceRead(ref,&actor,error,sizeof(error))||actor)return 0;
 return Cod1HealthResource(ref)&&Cod1HealthSnapshotMatches(e,h);
}

/* Read existing exact initial ownership without consuming/refreshing it. */
int G_Cod1PathHealthPreparedItem(const GscEntityRef*input){
 GscEntityRef ref;gentity_t before;int phase;
 if(!input)return 0;ref=*input;
 if(!G_Cod1EntitySameAllocation(&ref))return 0;
 phase=cod1HealthMasks[ref.slot].phase;before=g_entities[ref.slot];
 return (phase==2||phase==4)&&cod1PathEntities[ref.slot].kind==9&&
  Cod1HealthCurrent(&ref)&&before.classname&&!strcmp(before.classname,"weapon_fraggrenade")&&
  G_Cod1EntitySameAllocation(&ref)&&cod1HealthMasks[ref.slot].phase==phase&&
  !memcmp(&before,g_entities+ref.slot,sizeof(before));
}
